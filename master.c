/* 
 * ======================================================================
 *  hardware:
 *  1x ESP8266
 *  1x PC817 AC opto coupler
 *  1x 1K Ohm resistor
 * ----------------------------------------------------------------------
 *  pin mapping:
 *  PC817: AC with 1K resistor, GND --> GND, out --> D4
 * ----------------------------------------------------------------------
 * credits:
 * initial source: https://randomnerdtutorials.com/telegram-control-
 * esp32-esp8266-nodemcu-outputs/
 * ======================================================================
 *  about this file:
 *  - digital doorbell: in case someone presses the doorbell (~12VAC
 *    on the PC817), an UDP message is sent to a receiver, a chat
 *    message is sent via Telegram and a MQTT message is sent to a MQTT
 *    broker (ioBroker here)
 * ======================================================================
 */

#include <ESP8266WiFi.h>
#include <WiFiManager.h>         // more elegant than hardcoded WiFi
#include <WiFiUdp.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <PubSubClient.h>        // for MQTT only
#include <time.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

// ======================================================================
// OTA CONFIGURATION
// ======================================================================
const char* baseUrl  = "YOUR_OTA_BASE_URL_GOES_HERE";
#define CURRENT_VERSION "1.0"

// ======================================================================
// DOORBELL CONFIGURATION
// ======================================================================

// you need a Telegram bot and the API key and your own Telegram ChatID
#define BOTtoken "YOUR_TELEGRAM_BOT_API_TOKEN_GOES_HERE"
#define CHAT_ID1 "YOUR_CHATID_GOES_HERE"

const int inputPin = D4;
const unsigned int remotePort = 4210;
const unsigned long debounceDelay = 5000;

IPAddress bellIP1(192, 168, 123, 123);    // your receiver's IP

// MQTT broker
IPAddress mqttServer(192, 168, 123, 124); // your MQTT broker's IP
const char* mqttUsername = "YOUR_MQTT_USER";
const char* mqttPassword = "YOUR_MQTT_PASSWORD";

// ======================================================================
// OBJECTS & VARIABLES
// ======================================================================

WiFiManager wifiManager;

WiFiUDP Udp;

WiFiClientSecure secureClient;
WiFiClient mqttClient;
PubSubClient client(mqttClient);

X509List cert(TELEGRAM_CERTIFICATE_ROOT);
UniversalTelegramBot bot(BOTtoken, secureClient);

unsigned long lastRingMillis = 0;

// ======================================================================
// FUNCTIONS
// ======================================================================

void checkForOTA() {
  String chipID = String(ESP.getChipId(), HEX);
  chipID.toUpperCase();

  String versionUrl  = String(baseUrl) + "fw_" + chipID + ".txt";
  String firmwareUrl = String(baseUrl) + "fw_" + chipID + ".bin";

  Serial.println("Version-URL: " + versionUrl);
  Serial.println("Firmware-URL: " + firmwareUrl);

  HTTPClient http;
  WiFiClient httpClient;
  http.begin(httpClient, versionUrl.c_str());
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String newVersion = http.getString();
    newVersion.trim();
    Serial.printf("ESP version: %s / server version: %s\n",
                  CURRENT_VERSION, newVersion.c_str());

    if (!newVersion.equals(CURRENT_VERSION)) {
      Serial.println("found new version – starting OTA update ...");
      WiFiClient client;
      t_httpUpdate_return ret = 
        ESPhttpUpdate.update(client, firmwareUrl, CURRENT_VERSION);

      switch (ret) {
        case HTTP_UPDATE_FAILED:
          Serial.printf("update failed. error (%d): %s\n",
                        ESPhttpUpdate.getLastError(),
                        ESPhttpUpdate.getLastErrorString().c_str());
          break;

        case HTTP_UPDATE_NO_UPDATES:
          Serial.println("no new version found.");
          break;

        case HTTP_UPDATE_OK:
          Serial.println("Update successful! restarting ...");
          break;
      }
    } else {
      Serial.println("firmware is up to date.");
    }
  } else {
    Serial.printf("error while searching new firmware version: HTTP code %d\n",
      httpCode);
  }
  http.end();
}

void connectToMQTT() {
  while (!client.connected()) {
    if (client.connect("doorbell", mqttUsername, mqttPassword)) {
    } else {
      delay(5000);
    }
  }
}

void mqttPublishWithUpdate(const char* topic, const char* message) {
  if (client.connected()) {
    // to make sure that the MQTT topic is really changing even in case
    // of two identical calls in a row (e.g. ringing twice, rebooting
    // twice), we send two different texts in a row:
    client.publish(topic, "update"); 
    delay(2000); // wait 2 secs
    client.publish(topic, message);
    Serial.printf("MQTT: %s -> %s\n", topic, message);
  }
}

void handleDoorbellRing() {
  Serial.println("doorbell pressed – sending UDP, Telegram & MQTT ...");

  // UDP
  Udp.beginPacket(bellIP1, remotePort);
  Udp.write("R", 1);
  Udp.endPacket();

  // send Telegram
  bot.sendMessage(CHAT_ID1, "YOUR_NAME, the door is ringing!", "");

  // get and format time
  time_t now = time(nullptr);
  struct tm *tm_struct = localtime(&now);
  char timeBuffer[30];
  strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%dT%H:%M:%S%z", tm_struct);

  // create JSON payload
  String payload = "{";
  payload += "\"event\":\"ring\",";
  payload += "\"timestamp\":\"" + String(timeBuffer) + "\"";
  payload += "}";

  // you may have to adopt this to your MQTT broker
  mqttPublishWithUpdate("doorbell/main/json", payload.c_str());
  mqttPublishWithUpdate("doorbell/main/lastevent", "ring");
}



// ======================================================================
// SETUP
// ======================================================================

void setup() {
  Serial.begin(115200);
  pinMode(inputPin, INPUT);

  // connect WiFi with WiFiManager
  String ssid = "doorbell_" + String(ESP.getChipId());
  wifiManager.autoConnect(ssid.c_str());
  Serial.println("WiFi connected.");

  // check for new firmware via OTA
  checkForOTA();

  // timezone: Germany with DST
  configTime(3600, 3600, "pool.ntp.org");
  setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1);
  tzset();

  // prepare Telegram
  secureClient.setTrustAnchors(&cert);
  secureClient.setTimeout(2000); // Telegram Timeout

  // start UDP
  Udp.begin(4210);

  // start MQTT
  client.setServer(mqttServer, 1883);
  connectToMQTT();

  delay(2000);

  // document reboot via MQTT
  // you may have to adopt this to your MQTT broker
  mqttPublishWithUpdate("doorbell/main/json", "{\"event\":\"start\"}");
  mqttPublishWithUpdate("doorbell/main/lastevent", "start");
}

// ======================================================================
// LOOP
// ======================================================================
void loop() {
  if (!client.connected()) {
    connectToMQTT();
  }
  client.loop();

  // get time
  time_t now = time(nullptr);
  struct tm *tm_struct = localtime(&now);

  // daily reboot at 03:00h
  if (tm_struct->tm_hour == 3 && tm_struct->tm_min == 0 && tm_struct->tm_sec < 10) {
    Serial.println("daily reboot at 03:00h.");
    delay(1000);
    ESP.restart();
  }

  // check doorbell
  int inputValue = digitalRead(inputPin);
  if (inputValue == 0 && millis() - lastRingMillis > debounceDelay) {
    lastRingMillis = millis();
    handleDoorbellRing();
  }

  delay(100);
}
