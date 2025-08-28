This project solves a simple common problem: you are sitting at home anywhere in your house / flat, wearing headphones because you are on a Teams / Zoom / Meet call, on the phone or just listening to music or a Youtube video, and when your Amazon parcel is delivered you are missing the doorbell. At least in Germany that means starting to track your parcel, wait at least a day and then travel through your city to fetch the parcel at some store with inconvenient opening hours. You get the problem? Let's solve it.

What does it do? When your doorbell is pressed:
- a separate device ("receiver", code included) on your desk, connected to the same network, is blinking for 20sec,
- you get a Telegram message that your door is ringing,
- a MQTT message is sent (whatever that is good for).

Doorbells usually consists of a simple 12V AC power source (transformator), the doorbell switch at the door that powers some device that makes noise and finally that ringing device itself. That's sometimes a simple solenoid pinging a piece of metal, sometimes a more sophisticated device. And not relevant for our project here - your classic doorbell stays almost untouched and will still work of course.

In this project, we use a PC817 AC opto coupler to get the doorbell signal and make it digital. The ESP8266 then makes three different things with that:
- sending an UDP package to a receiver with an LED stripe, giving an optical signal (also part of this project)
- sending a Telegram notification (Telegram, because there is a lightweight and capable API for that)
- sending a MQTT message (in my case to an ioBroker instance, for documentation only so far)

Additionally the code restarts the device every night (because an ESP8266 seems to get fragile when running for days, weeks and months), and it is OTA capable and looks for an updated firmware on every restart (so, every night).

Of course you can strip away the parts you don't need (e.g. MQTT, OTA) to make it leaner still.

Remark: I'm not an overly talented software engineer. The project works, and I'm proud of it, but probably there are more elegant and/or more correct ways of solving the problem. Feel free to improve.


-----------------------------------------
in detail
-----------------------------------------


HOW TO GET IT UP AND RUNNING

__hardware you will need__

- 2x ESP8266 (ESP32 will also work, but then you have to figure out the WiFi on your own because it's slightly different)
- 1x opto coupler PC817 (that's basically a LED and a photo resistor in a small case to uncouple the 12V AC power of the doorbell from the 5V / 3V3 DC of the ESP)
- 1x 1kOhm resistor
- 1 ... n WS2812 LED strip, at least 5 recommended (8 is default; you can also modify the code to use a single LED, but then you loose part of the effect)

__how to build__

One ESP becomes the "master", one the "receiver".

On the master, connect the opto coupler on the photo resistor side to GND and D4 of the ESP (or any other GPIO you find more convenient). Connect the AC side with the 1k resistor in a row to the doorbell switch. The resistor makes the 12V AC survivable for the LED built into the opto coupler.

On the receiver, connect the LED stripe to GND, 5V and the data pin to D1 (or any other GPIO you find more convenient).

That's it for the hardware!

__getting the receiver ready__

To determine the receiver's IP it's necessary we start with this one. That's easy: open the receiver.c code in your favorite IDE (e.g. Arduino IDE), connect the receiver ESP to your PC, and flash the firmware. The code plans for 8 LEDs; if you have a longer strip, make sure you adjust the parameter in the code.

Once it has rebooted, it will look for known WiFi networks (there are none), than it will switch to AP mode. You will see a new WiFi named "doorbell_receiver_ABCDEF" (with ABCDEF the hex serial number of your ESP). Connect to it, open http://192.168.4.1 with a browser, there choose the WiFi the ESP shall connect to in production, provide the password and confirm. The ESP will reboot and connect to that given network. (This process is provided by the WiFiManager.h library, not my own work.) You will see a short initial animation on the connected LED stripe (in case you managed to connect the LED stripe correctly).

Now your receiver is already running and listining. To identify its IP address you will have to look in your router. There will be a new device called "ESP-ABCDEF" (with ABCDEF again your device's serial number). Identify the IP address your router assigned to it, and better make sure it always gets the same one. E.g. using a Fritzbox, you can choose the option "always assign the same IP to this device" to ensure that. Remember the assigned IP, we will need it in a minute.

__getting the master ready__

The master works similar, also using WiFiManager for an easy configuration of the WiFi without hardcoding the credentials. But first, we have to change the code according to your personal preferences and accounts. For that open master.c in your favorite IDE and start changing it.

If there are features you don't want to use, e.g. OTA or MQTT, make sure to delete everything belonging to that feature (or make it a comment).

__UPD__

I chose UDP for the communication between master and receiver for timing reasons - UDP is quite fast, because it does not have the overhead including DNS resolving etc. But therefore you have to enter the receiver's IP in the code ("bellIP1"). That will be the IP the UDP package will be sent to in case the doorbell rings and the receiver should listen to - see above.

__Telegram__

You will need a Telegram bot API and therefore use a bot you already have or get a new one.

To get one, open Telegram (web or mobile device) and start talking to "BotFather". He will guide you through the process of creating a bot (and changing the name, adding a photo etc.). You will get the API key from him.

Then talk to "userinfobot" by typing "/start". This bot will tell you your own chatID. Finally, start talking to your own, newly created bot by typing "/start". It won't answer, but it is necessary that the conversation is initiated by you - it can't just start talking to you.

If you want to add additional recipients, do the same for their accounts and duplicate the Telegram message sending-related lines in the code. (It's called CHAT_ID1 in the code by purpose, because my personal instance is sending out messages to two different chatIDs.)

Adjust the Telegram message as you like.

__OTA__

Find a file location where you can store the firmware. In my case that's my ioBroker instance, but it can be any URL accessable by the ESP, e.g. a simple file server. It can even be on the internet, but I do not recommend that, because anybody can access it if he knows the URL, and even it's an compiled BIN file, it contains at least your Telegram bot API key. And as the ESP8266 is not capable of SSL, it must be an unprotected HTTP URL. So handle with care! Something locally on your LAN is recommended.

Once you have determined the base URL, add it to the code ("baseUrl"). For my ioBroker that would be "http://iobroker:8081/files/0_userdata.0/ota/" (with the name "iobroker" assigned to ioBroker's IP in my Fritzbox router, port 8081 being default for ioBroker, and "/ota" a subfolder I created in the ioBroker files section).

The master will look for a file named "fw_ABCDEF.txt" first, with "ABCDEF" again the serial number of your ESP8266 (the master's one this time!). To determine that, either open the serial monitor when flashing the ESP (see chapter below) - it will print the filenames it looks for on the serial console. You can also look at the logs of your fileserver, it will probably show a warning like "file ESP_ABCDEF.txt asked for, but not found" or similar. ioBroker will do so.

To use OTA, store two files at your base URL:
- fw_ABCDEF.txt contains the version number, starting with 1.0 (current version). Only in case the number in the file is greater than the one in the installed firmware, the ESP will download the new one.
- fw_ABCDEF.bin new firmware. To create it, open the code in your IDE and save the compiled binary file.

For future updates make sure that the new firmware contains the new version number ("CURRENT_VERSION") and the value in fw_ABCDEF.txt is identically to that, otherwise the ESP might end up with endless tries of reinstalling the same firmware over and over.

OTA comes extremely handy once your master device is installed near the doorbell, probably somewhere down in your home's electric appliances where it is hard to disconnect and flash with a USB cable for any minor updates. That's the reason I went for OTA for the master only.

__MQTT__

MQTT is for documentation purposes only so far - I run a separate script on my ioBroker acting as a heartbeat and looking for the last signs of life of different systems, including the doorbell. Therefore the doorbell updates two different MQTT topics with "ring" each time the door rings and "start" each time the device reboots (lastevent topic), and a lean JSON in the topic "json" with pretty much the same information. To ensure the "event" topic is updated even in case two start events or two ring events happen in a row, it's always set to "update" first, followed two seconds later with the real event.

To make that work you have to add your MQTT broker's IP ("mqttServer"), the username ("mqttUsername") and password ("mqttPassword"). You may also want to adjust the topics - by default it's using my structure ("doorbell/main/json" and "doorbell/main/lastevent").  

__flashing__

Finally you can compile the code and flash it to your ESP. Concerning the WiFi connection it works exactly the same way like the receiver: connect to the new WiFi "doorbell_ABCDEF" this time, open http://192.168.4.1, choose your WiFi und provide the password, confirm.

You are done! Whenever you apply approx. 12V AC to the two AC pins of the PC817 (with the 1k resistor in a row of course!) now, the master will be triggered and send the UDP package to the receiver, a Telegram message and the MQTT updates. So when finally installing the device, you apply the same 12V AC that goes to your ringing device also to the ESP, so you put the ESP input (the two PC817 inputs, one resistor-protected) parallel to your existing ringing device. Whenever the doorbell switch is pressed, both the PC817 and your existing ringing device get power. 

(For testing you can also use e.g. 8V DC to trigger the PC817, but be aware it will only work in one direction as the internal trigger device is a LED that will only light on when connected to DC the right way. So try both directions if one does not work.) 

Looking forward to your feedback.
