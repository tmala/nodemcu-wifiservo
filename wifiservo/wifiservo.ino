
/*
 *  wifiservo
 *  
 *  A simple REST-like HTTP-Server that will control a servo (MG90S) attached to PIN4 on a NodeMCU v2 Board.
 *  
 *  API:
 *  GET /servo1/attach                   => Enables servo1 (turns it "on")
 *  GET /servo1/detach                   => Disables servo1 (turns it "off")
 *  GET /servo1/moveto/[0-180]/[1-10000] => Moves servo1 to degree with delayInMsBetweenDegrees
 *  GET /servo1/set/[0-180]              => moves servo to desired angle [0˚-180˚]
 *  GET /servo1/steplength/[1-90]        => sets the steplength to be used by moveto
 *  GET /servo1                          => returns current degree and status of servo1 {name: servo1, status: ['attached','detached'], degree: [0-180]}
 *  
 */

#include <ESP8266WiFi.h>
#include <Servo.h>
#include "wifisettings.h"

// global values of Servo1
Servo servo1;
int PIN_SERVO_1 = 2; // D4
String strServo1_Status = "/servo1";
String strServo1_Set = "/servo1/set/";
String strServo1_StepLength = "/servo1/steplength/";
String strServo1_Attach = "/servo1/attach";
String strServo1_Detach = "/servo1/detach";
String strServo1_MoveTo = "/servo1/moveto/";

int servo1_degree = 0;
bool servo1_attached = false;
int stepLength = 15;

int DELAY_PER_DEGREEE = 8;

// global return value
String msg = "";
String HTTP_CODE = "200 OK";

// Create an instance of the server, specify the port to listen on as an argument
WiFiServer server(80);

void setup() {
  Serial.begin(115200);
  delay(10);

  // prepare LED_BUILTIN
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  
  // Connect to WiFi network
  Serial.println();
  Serial.print("Connecting to Wifi ");
  Serial.println(wifi_ssid);
  
  WiFi.hostname(wifi_hostname);
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_password);
  
  while ( WiFi.status() != WL_CONNECTED ) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("");
  
  // Start the server
  server.begin();
  Serial.println("Server started");
  Serial.print("IP: "); Serial.println( WiFi.localIP() );
  Serial.print("MAC: "); Serial.println( WiFi.macAddress() );
  Serial.print("Hostname: "); Serial.println( WiFi.hostname() );
  Serial.println("");

  // Attach the servo
  servo1.attach(PIN_SERVO_1);
  Serial.println("Servo1 attached");
  servo1.write(0);
  delay(800);
  servo1.detach();
  Serial.println("Servo1 set to 0 degrees and detached");
  
  delay(2000);
  Serial.println("Blink LED_BUILTIN to indicate boot");
  blinkLed3times();
  Serial.println("                   *                         ");
  Serial.println("             *************                   ");
  Serial.println("     *****************************           ");
  Serial.println("   ***********   HTTP    ***********         ");
  Serial.println("  ********      R E S T       ********       ");
  Serial.println("   ***********   READY   ************        ");
  Serial.println("    ********************************         ");
  Serial.println("             *************                   ");
  Serial.println("                   *                         ");

}

void loop() {
  // Check if a client has connected
  WiFiClient client = server.available();
  if  ( ! client ) {
    return;
  }
  
  // Wait until the client sends some data
  Serial.println("new HTTP client");
  while( ! client.available() ){
    delay(1);
  }
  
  // Read the first line of the request
  String req = client.readStringUntil('\r');
  Serial.println(req);
  client.flush();

  boolean validRequest = false;
  msg = "";

  HTTP_CODE = "200 OK";
  
  if ( req.indexOf("/antifjomp") > 0 ) {
    validRequest = true;
    releaseAntifjomp();
  } else if ( req.indexOf(strServo1_Attach) > 0 ) {
    validRequest = true;
    cmdAttach_Servo1();
  } else if ( req.indexOf(strServo1_Detach) > 0 ) {
    validRequest = true;
    cmdDetach_Servo1();
  } else if ( req.indexOf(strServo1_MoveTo) > 0 ) {
    validRequest = true;
    cmdMoveTo_Servo1(req);    
  } else if ( req.indexOf(strServo1_Set) > 0) {
    validRequest = true;
    cmdSet_Servo1(req);
  } else if ( req.indexOf(strServo1_StepLength) > 0 ) {
    validRequest = true;
    cmdSet_setStepLength(req);
  } else if ( req.indexOf(strServo1_Status) > 0 ) {
    validRequest = true;
    msg = "info";
  }
  
  if ( validRequest ) {
    // Send the response to the client
    String s = "HTTP/1.1 ";
    s += HTTP_CODE;
    s += "\r\nContent-Type: text/html\r\n\r\n";
    s += jsonResult_Servo1();
    s += "\n";
    if ( client.available() ){
      client.print(s);
      delay(1);
      Serial.println("HTTP Client disconnected");
      // The client will actually be disconnected 
      // when the function returns and 'client' object is destroyed
    } else {
      Serial.println("HTTP Client is gone :(");
      client.stop();
    }
  } else {
    String s = "HTTP/1.1 404 NOT FOUND";
    s += "\r\nContent-Type: text/html\r\n\r\n";
    s += "no such content here";
    s += "\n";
    client.print(s);
    delay(1);
    Serial.println("HTTP Client disconnected due to invalid request");
    Serial.println(req);
    client.stop();
    return;
  }
}

void cmdAttach_Servo1() {
    if ( servo1_attached ) {
      servo1.attach(PIN_SERVO_1);  
      msg.concat("Servo1 already attached");
    } else {
      servo1.attach(PIN_SERVO_1);  
      msg = "Servo1 attached";
      servo1_attached = true;
    }
}

void cmdDetach_Servo1() {
    if(!servo1_attached) {
      servo1.detach();
      msg.concat("Servo1 already detached");
    } else {
      servo1.detach();
      msg = "Servo1 detached";
      servo1_attached = false;
    }
}

/*
 * cmdMoveTo
 *   /servo1/moveto/<newPos>/<delayInMs>
 */
void cmdMoveTo_Servo1(String req) {
  bool detachAfter = false;
  int x = 0;
  int posStartParams = req.indexOf(strServo1_MoveTo) + strServo1_MoveTo.length();
  int posEndParams = req.substring(posStartParams).indexOf(" ") + posStartParams;
  String strParams = req.substring(posStartParams, posEndParams);
  
  int posParam2 = strParams.indexOf("/");

  if ( posParam2 < 0 ) {
    HTTP_CODE = "400 Bad Request";
    msg.concat("ERROR: Missing second parameter (delayInMsBetweenDegrees");
    return; 
  }

  String strParam1 = strParams.substring(0, posParam2); // not including the post"/"
  String strParam2 = strParams.substring(posParam2 + 1); // not including the pre "/"
  int degree = strParam1.toInt();
  int delayInMs = strParam2.toInt();

  Serial.print("StartPos: "); Serial.print(servo1_degree); Serial.println(" degrees");
  Serial.print("MoveTo: "); Serial.print(degree); Serial.println(" degrees");
  Serial.print("Delay between each degree: "); Serial.print(delayInMs); Serial.println(" ms");
  
  
  if ( degree < 0 || degree > 180 ) {
    HTTP_CODE = "400 Bad Request";
    msg.concat("ERROR: Degree must be in the range [0-180]");
    return;
  }
  if ( delayInMs < 1 || delayInMs > 10000 ) {
    HTTP_CODE = "400 Bad Request";
    msg.concat("ERROR: delayInMsBetweenDegrees must be in the range [1-10000]");
    return;
  }

  if ( degree == servo1_degree) {
    msg.concat("Servo already in desired position");
    return;
  }

  digitalWrite(LED_BUILTIN, LOW);

  if ( !servo1_attached ) {
    detachAfter = true;
    servo1_attached = true;
    servo1.attach(PIN_SERVO_1);
    delay(100);
    msg.concat("Servo1 attached. ");
  }
  
  // Move servo1 in steps of stepLength degrees with delay
  Serial.print(" moving sensor from "); Serial.print(servo1_degree); Serial.print(" degrees to "); Serial.print(degree); Serial.print(" degrees with delay "); Serial.print(delayInMs); Serial.println(" ms");
  if ( stepLength * DELAY_PER_DEGREEE > delayInMs) {
    Serial.print(" Increasing delayInMs from "); Serial.print(delayInMs); Serial.print(" ms to ");
    delayInMs = stepLength * DELAY_PER_DEGREEE;
    Serial.print(delayInMs); Serial.print(" ms since steplength is "); Serial.println(stepLength);
    Serial.print(" (at least "); Serial.print(DELAY_PER_DEGREEE); Serial.println(" ms for each degree is needed)");
  }
  if ( degree > servo1_degree ) {
    for (x = servo1_degree; x <= degree; x = x + stepLength ) {
      Serial.print(" moveTo: "); Serial.print(x); Serial.print(" delay: "); Serial.print(delayInMs); Serial.println(" ms");
      if ( x > degree ) {
        x = degree;
      }
      servo1.write(x);
      delay(delayInMs);
      servo1_degree = x;
    }
  } else {
    for ( int x = servo1_degree; x >= degree; x = x - stepLength ) {
      Serial.print(" moveTo: "); Serial.print(x); Serial.print(" delay: "); Serial.print(delayInMs); Serial.println(" ms");
      if ( x < degree ) {
        x = degree;
      }
      servo1.write(x);
      delay(delayInMs);
      servo1_degree = x;
    }
  }

  msg.concat("Servo1 moved to new position "); msg.concat(degree); msg.concat(" ("); msg.concat(servo1_degree); msg.concat(")");

  if ( detachAfter ) {
    delay(100);
    servo1.detach();
    delay(100);
    servo1_attached = false;
    msg.concat(". Servo1 detached");
  }
  
  digitalWrite(LED_BUILTIN, HIGH);  
}

/*
 * cmdSet_Servo1
 *   /servo1/<newPos>
 */
void cmdSet_Servo1(String req) {
  bool detachAfter = false;
  digitalWrite(LED_BUILTIN, LOW);

  // find out what position to set set for the servo
  int posStartParams = req.indexOf(strServo1_Set) + strServo1_Set.length();
  int posEndParams = req.substring(posStartParams).indexOf(" ") + posStartParams;
  String strDegree = req.substring(posStartParams, posEndParams);
  int degree = strDegree.toInt();

  if ( degree < 0 || degree > 180 ) {
    // invalid parameters
    HTTP_CODE = "400 Bad Request";
    msg = "invalid degree parameter in request: ";
    msg += strDegree;
    return;
  }

  if ( !servo1_attached ) {
    detachAfter = true;
    servo1_attached = true;
    servo1.attach(PIN_SERVO_1);
    delay(100);
    msg.concat("Servo1 attached. ");
  }

  // Servospeed is 60˚/0.1s @5V, slower when operating @3.3V - testing shows the MG90S needs close to 800ms to go from 0˚ to 180˚ (5 ms per 1˚)
  int delayInMs = (abs(degree - servo1_degree) * DELAY_PER_DEGREEE) + 50; // adding extra 50ms

  Serial.print("StartPos: "); Serial.print(servo1_degree); Serial.println(" degrees");
  Serial.print("MoveTo: "); Serial.print(degree); Serial.println(" degrees");
  Serial.print("Delay: "); Serial.print(delayInMs); Serial.println(" ms");

  servo1.write(degree);
  delay(delayInMs);
  servo1_degree = degree;
  msg.concat("Servo1 set to ");
  msg.concat(strDegree);
  msg.concat(" degrees");

  if ( detachAfter ) {
    servo1.detach();
    delay(100);
    servo1_attached = false;
    msg.concat(". Servo1 detached");
  }
  
  digitalWrite(LED_BUILTIN, HIGH);
}

/*
 * cmdSet_StepLength
 *   /servo1/<newPos>
 */
void cmdSet_setStepLength(String req) {
  // find out what position to set set for the servo
  int posStartParams = req.indexOf(strServo1_StepLength) + strServo1_StepLength.length();
  int posEndParams = req.substring(posStartParams).indexOf(" ") + posStartParams;
  String strStepLength = req.substring(posStartParams, posEndParams);
  int newStepLength = strStepLength.toInt();
  if ( newStepLength > 0 && newStepLength < 90 ) {
    stepLength = newStepLength;
    msg.concat("New Steplength set");
  } else {
    msg.concat("New stepLength is not valid => "); msg.concat(strStepLength);
    HTTP_CODE = "400 Bad Request";    
  }
  
}

String jsonResult_Servo1() {
  String json = "{";
  json.concat("\"id\": \"Servo1\",");
  json.concat("\"status\": \""); json.concat(servo1_attached ? "attached" : "detached"); json.concat("\",");
  json.concat("\"degree\": \""); json.concat(servo1_degree); json.concat("\",");
  json.concat("\"step\": \""); json.concat(stepLength); json.concat("\",");
  json.concat("\"msg\": \""); json.concat(msg); json.concat("\",");
  json.concat("}");
  return json;
}

void releaseAntifjomp() {
  blinkLed3times();
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);
  servo1.write(90);
  servo1_degree = 90;
  delay(1000);
  servo1.write(0);
  servo1_degree = 0;  
  delay(1000);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  blinkLed3times();
  msg.concat("Antifjomp Released !!!");
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

