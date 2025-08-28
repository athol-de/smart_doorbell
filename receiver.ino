/*
   ==========================================================================
    hardware:
    1x ESP8266
    1x LED strip with WS2812 (here: with 8 LEDs)
   --------------------------------------------------------------------------
    pin mapping:
    D1 --> data pin of LED strip
   ==========================================================================
    about this file:
    listens to a doorbell sensor / actor who sends a UDP message each time
    the doorbell is pressed. Then it blinks a small Neopixel stripe for 20
    seconds to create an optical door alarm wherever you are (connected to
    the same network).
   ==========================================================================
*/

#include <ESP8266WiFi.h>          // for general Wifi connection
#include <WiFiManager.h>          // remembers known WiFi, enables
                                  // easy WiFi configuration
#include <WiFiUdp.h>              // for UDP trigger reception
#include <Adafruit_NeoPixel.h>    // for LED strip

#define LEDPIN D1                 // PIN attached to strip

#define PI 3.1415926535897932384626433832795

int n, bright = 0;
unsigned int localUdpPort = 4210;  // local port to listen on
float i = 0;
long lastRingMillis = 0;

WiFiManager       wifiManager;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(8, LEDPIN, NEO_GRB + NEO_KHZ800);
WiFiUDP           Udp;



void fadeOn(int Pixel, char Color) {
  for (float i = 0; i < PI/2; i += PI/32) {
    bright = int(255 * abs(sin(i)));
    switch (Color) {
      case 0:
        strip.setPixelColor(Pixel, bright, 0, 0);
        break;
      case 1:
        strip.setPixelColor(Pixel, 0, bright, 0);
        break;
      default:
        strip.setPixelColor(Pixel, 0, 0, bright);
    }
    strip.show();
    delay(6);
  }
}

void fadeOff(int Pixel, int Color) {
  for (float i = PI/2; i > 0.01; i -= PI/32) {
    bright = int(255 * abs(sin(i)));
    switch (Color) {
      case 0:
        strip.setPixelColor(Pixel, bright, 0, 0);
        break;
      case 1:
        strip.setPixelColor(Pixel, 0, bright, 0);
        break;
      default:
        strip.setPixelColor(Pixel, 0, 0, bright);
    }
    strip.show();
    delay(6);
  }
  strip.setPixelColor(Pixel, 0, 0, 0);
  strip.show();
}


// --------------------------------------------------------------------------
// ------------------------------------------------------------------ SETUP -
// --------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  strip.begin();                   // start LED strip
  strip.setBrightness(255);        // choose brightness of LED (0 .. 255)

  String ssid = "doorbell_receiver_" + String(ESP.getChipId());
  Serial.println("WiFi initialization with " + ssid + 
                 " in case no WiFi credentials stored.");
  wifiManager.autoConnect(ssid.c_str());
  Serial.println("WiFi connected.");

// ------------------------------------------------------- start webserver
  Udp.begin(localUdpPort);
  Serial.println(F("Listening for UDP messages."));

// initial animation on Neopixel stripe
  for (n = 0; n < 8; n++) {
    int r = random(0, 256);
    strip.setPixelColor(n, r, 255 - r, (3 * r) % 256);
    strip.show();
    delay(1000);
    strip.setPixelColor(n, 0, 0, 0);
    strip.show();
  }
}



// --------------------------------------------------------------------------
// ------------------------------------------------------------------- LOOP -
// --------------------------------------------------------------------------

void loop() {
  int packetSize;
  packetSize = Udp.parsePacket();
  if (packetSize) {
    Serial.print("Ring by IP "); Serial.println(Udp.remoteIP());
    // pulse the Neopixel stripe for 20 secs
    lastRingMillis = millis();
    while (millis() - lastRingMillis < 20 * 1000) { // blink for 20 secs
      for (n = 0; n < 8; n++) {
        fadeOn(n, 1);      
      }
      delay (200);  
      for (n = 0; n < 8; n++) {
        fadeOff(n, 1);      
      }
      delay (200);  
      for (n = 0; n < 8; n++) {
        fadeOn(n, 2);      
      }
      delay (200);  
      for (n = 0; n < 8; n++) {
        fadeOff(n, 2);      
      }
      delay (200);  
      for (n = 0; n < 8; n++) {
        fadeOn(n, 0);      
      }
      delay (200);  
      for (n = 0; n < 8; n++) {
        fadeOff(n, 0);      
      }
      delay (200);  
    }    
  }
  delay(250);
}
