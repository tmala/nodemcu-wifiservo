# nodemcu-wifiservo
A simple REST-like HTTP-Server that will control a servo (MG90S) attached to PIN4 on a NodeMCU v2 Board.

The network is connected using a file named "wifisetting.h" (placed under directory "wifiservo" alongside "wifiservo.ino") containing the following global parameters
  const char* wifi_ssid = "<wifi-ssid>";
  const char* wifi_password = "<wifi-pwd>";
  const char* wifi_hostname = "<wifi-hostname>";
This file is not in the GIT repo

Upon startup the IP address (and some more info) of the ESP8266 module will be printed to Serial when the module is connected.

To be programmed using Arduino IDE with the "ESP8266 Modules" board "NodeMCU v1.0 (ESP-12E Module)"
