
/*
 *  wifiservo
 *  
 *  A simple REST-like HTTP-Server that will control a servo (MG90S) attached to PIN4 on a NodeMCU v2 Board.
 *  
 *  The network is connected using a file named wifisetting.h containing the following parameters
 *     const char* wifi_ssid = "<wifi-ssid>";
 *     const char* wifi_password = "<wifi-pwd>";
 * .   const char* wifi_hostname = "<wifi-hostname>";
 * 
 *    http://server_ip/gpio/0 will set the LED_BUILTIN low,
 *    http://server_ip/gpio/1 will set the LED_BUILTIN high
 *  
 *  Upon startup the IP address of the ESP8266 module, will be 
 *  printed to Serial when the module is connected.
 *  
 *  To be programmed using Arduino IDE with the "ESP8266 Modules" board "NodeMCU v1.0 (ESP-12E Module)"
 *  
 */

#include <ESP8266WiFi.h>
#include <Servo.h>
#include "wifisettings.h"

Servo servo1;
String strServo1 = "/servo1/";
String strServo1Attach = "/servo1/attach";
String strServo1Detach = "/servo1/detach";

// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(80);

void setup() {
  Serial.begin(115200);
  delay(10);

  // prepare LED_BUILTIN
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  
  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);
  
  WiFi.hostname(wifi_hostname);
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  
  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.print("IP: ");
  Serial.println( WiFi.localIP() );
  // Print the Mac Address
  Serial.print("MAC: ");
  Serial.println( WiFi.macAddress() );
  // Print the Hostname
  Serial.print("Hostname: ");
  Serial.println( WiFi.hostname() );

  // Attach the servo
  servo1.attach(2); // Connect D4
  Serial.println("Servo1 attached");
  servo1.write(0);
  delay(800);
  servo1.detach();
  Serial.println("Servo1 set to 0 degrees and detached");
  
  delay(2000);
  Serial.println("Blink LED_BUILTIN to indicate boot");
  blinkLed3times();

}

void loop() {
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
  
  // Wait until the client sends some data
  Serial.println("new client");
  while(!client.available()){
    delay(1);
  }
  
  // Read the first line of the request
  String req = client.readStringUntil('\r');
  Serial.println(req);
  client.flush();

  boolean validRequest = false;
  String msg = "";

  String HTTP_CODE = "200 OK";
  
  if (req.indexOf("/antifjomp") != -1) {
    validRequest = true;
    releaseAntifjomp();
    msg = "Antifjomp Released !!!";
  } else if (req.indexOf(strServo1Attach) != -1) {
    validRequest = true;
    servo1.attach(2); // Connect to D4
    msg = "Servo1 attached";
  } else if (req.indexOf(strServo1Detach) != -1) {
    validRequest = true;
    servo1.detach();
    msg = "Servo1 detached";
  } else if (req.indexOf(strServo1) != -1) {
    validRequest = true;
    // find out what position to set set for the servo
    int startServo1Degree = req.indexOf(strServo1) + strServo1.length(); 
    int stopServo1Degree = req.substring(startServo1Degree).indexOf(" ") + startServo1Degree;
    String strDegree = req.substring(startServo1Degree, stopServo1Degree);
    int degree = strDegree.toInt();
    if (degree >= 0 && degree <= 180) {
      setServo1(degree);
      msg = "Servo1 set to ";
      msg += strDegree;
      msg += " degrees";
    } else {
      // invalid parameters
      HTTP_CODE = "400 Bad Request";
      msg = "invalid degree parameter in request: ";
      msg += strDegree;
    }   
  } else {
    Serial.println("Invalid request");
    Serial.println(req);
  }
  
  if (validRequest) {
    // Send the response to the client
    String s = "HTTP/1.1 ";
    s += HTTP_CODE;
    s += "\r\nContent-Type: text/html\r\n\r\n";
    s += msg;
    s += "\n";
    client.print(s);
    delay(1);
    Serial.println("Client disonnected");
    // The client will actually be disconnected 
    // when the function returns and 'client' object is detroyed
  } else {
    Serial.println("invalid request");
    client.stop();
    return;
  }
}

void releaseAntifjomp() {
  blinkLed3times();
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);
  servo1.write(90);
  delay(1000);
  servo1.write(1);
  delay(1000);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  blinkLed3times();
}

void setServo1(int degree) {
  digitalWrite(LED_BUILTIN, LOW);
  servo1.write(degree);
  // Servospeed is 60˚/0.1s @5V, slower when operating @3.3V - testing shows the MG90S needs close to 800ms to go from 0˚ to 180˚
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);
}

void blinkLed3times() {
  // 1
  digitalWrite(LED_BUILTIN, LOW);
  delay(300);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(300);
  // 2
  digitalWrite(LED_BUILTIN, LOW);
  delay(300);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(300);
  // 3
  digitalWrite(LED_BUILTIN, LOW);
  delay(300);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(300);
}

