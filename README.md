This project solves a simple common problem: you are sitting at home anywhere in your house / flat, wearing headphones because you are on a Teams / Zoom / Meet call, on the phone or just listening to msuic or a Youtube video, and when your Amazon parcel is delivered you are missing the doorbell. At least in Germany that means starting to track your parcel, wait at least a day and then travel through your city to fetch the parcel at some store with inconvenient opening hours. You get the problem? Let's solve it.

What does do? When your doorbell is pressed:
- a separate device ("receiver", code included) on your desk, connected to the same network, is blinking for 20sec,
- you get a Telegram message that your door is ringing,
- a MQTT message is sent (whatever that is good for).

Doorbells are usually a simple 12V AC power source (transformator), the doorbell itself as a switch that powers some device that makes noise and that device. That's sometimes a simple solenoid pinging a piece of metal, sometimes a more sophisticated device.

In this project, we use a PC817 AC opto coupler to get the doorbell signal and make it digital. The ESP8266 then makes three different things with that:
- sending an UDP package to a receiver with an LED stripe, giving an optical signal (also part of this project)
- sending a Telegram notification (Telegram, because there is an lightweight and capable API for that)
- sending a MQTT message (in my case to an ioBroker instance, for documentation only so far)

Additionally the code restarts the device every night (because an ESP8266 seems to get fragile when running for days, weeks and months), and it is OTA capable and looks for an updated firmware on every restart (so, every night).

Of course you can strip away the parts you don't need (e.g. MQTT, OTA) to make it leaner still.

Remark: I'm not an overly talented software engineer. The project works, and I'm proud of it, but probably there are more elegant and/or more correct ways of solving the problem. Feel free to improve.
