# HttpButton
ESP8266 based wireless push-button sending http requests

Features
========
The http button is able to store configuration such as WiFi SSID and password as well as an URL
The button can run in Config mode and Run mode.

Config Mode
===========
If you hold down the button when powering on the ESP8266, it will enter config mode.
In config mode, the button starts as an AP (SSID = ESP8266 Thing ****, password=sparkfun).
You can connect the AP and access the following configuration request: http://192.168.4.1/config?ssid=yourwifissid&password=yourwifipassword&url=http://yourserverurl/some/path
The configuration will be saved by the ESP and loaded at startup.

Run Mode
========
Powering on the ESP will enter the Run mode.
In run mode, the ESP waits for the button to be pushed. Whenever the button gets pushed, an http request will be initiated for the URL.

Sketch
======
Connect one pin of the button to +
Connect the other pin of the button to pin 12 of the ESP as well as to - through a resistor

Enjoy!
