/*--------------------------------------------------
Wifi RC controller.

Parts used:
- Nodemcu 1.0 ESP-12F module
- TB6612 shield (clone of Sparkfun)
- 74HC00 NAND chip
- MCP1700 LDO 3.3 V regulator
- misc

Author: Heindal
License: http://creativecommons.org/licenses/by-nc/3.0/
licensed under the Creative Commons - Attribution - Non-Commercial license

Inspired by the Lego controller by Eisbaeer

Pin Out for Nodemcu with ESP-12F module
#define D0 16
#define D1 5 // I2C Bus SCL (clock)
#define D2 4 // I2C Bus SDA (data)
#define D3 0
#define D4 2 // Same as "LED_BUILTIN", but inverted logic
#define D5 14 // SPI Bus SCK (clock)
#define D6 12 // SPI Bus MISO
#define D7 13 // SPI Bus MOSI
#define D8 15 // SPI Bus SS (CS)
#define D9 3 // RX0 (Serial console)
#define D10 1 // TX0 (Serial console)
--------------------------------------------------*/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Servo.h>
#define DEBUG

/******************************************************************************
TB6612FNG H-Bridge Motor Driver with Logic Gates
Heindal
Based on https://github.com/sparkfun/SparkFun_TB6612FNG_Arduino_Library

Uses a 74HC00 NAND chip to reduce pin count
******************************************************************************/
class Motor
{
  public:
    // Constructor
    Motor(const int Direction_Pin, const int PWM_Pin, const int Standby_Pin)
    {
      DirectionPin = Direction_Pin;
      PWMPin = PWM_Pin;
      StandbyPin = Standby_Pin;
      motorSpeed = 0;
  
      pinMode(DirectionPin, OUTPUT);
      pinMode(PWMPin, OUTPUT);
      pinMode(StandbyPin, OUTPUT);
  };

  void run(int mSpeed=0) {
    motorSpeed = ((mSpeed > -1024) && (mSpeed < 1024)) ? mSpeed : 0;
    digitalWrite(StandbyPin,HIGH);
    if (motorSpeed > 0) { 
      digitalWrite(DirectionPin,HIGH);
      analogWrite(PWMPin,motorSpeed); }
    else if (motorSpeed < 0) {
      digitalWrite(DirectionPin,LOW);
      analogWrite(PWMPin,-motorSpeed); }
    else { analogWrite(PWMPin,0); }
  };

  void stop(void) {
    motorSpeed = 0;
    analogWrite(PWMPin,0);
  };

  void standby(void) {
    analogWrite(PWMPin,0);
    digitalWrite(StandbyPin,LOW);    
  };

  int getSpeed(void) {
    return motorSpeed;
  };
  
  private:
    //variables for the pins
    int DirectionPin, PWMPin, StandbyPin;
    int motorSpeed;
};

const char* ssid = "yourSSID";
const char* password = "yourPassphrase";  // set to "" for open access point w/o password

// Create an instance of the server on Port 80
ESP8266WebServer server(80);   // the server listen on: 192.168.4.1

static int deviceSpeed[4];

// Servos
Servo servo1;
Servo servo2;

// Motor Driver
const int motor1DirPin = D3;    
const int motor1PWMPin = D4;    
const int motor2DirPin = D5;    
const int motor2PWMPin = D6;  
const int standbyPin = D7;

// Motor 
Motor motor1(motor1DirPin, motor1PWMPin, standbyPin);
Motor motor2(motor2DirPin, motor2PWMPin, standbyPin);


void setup() 
{
  // Setup Servos
  servo1.attach(D1);
  servo2.attach(D2);
  deviceSpeed[2] = 90;
  servo1.write(deviceSpeed[2]);
  deviceSpeed[3] = 90;
  servo2.write(deviceSpeed[3]);
    
#ifdef DEBUG
  Serial.begin(115200);
  delay(1);
  Serial.println("Wifi RC controller ver.1.0");
#endif
  
  // Start wifi in the default mode
  WiFi.begin(ssid, password);

  // Server Setup
  server.on("/control.html", mainPage);
  server.onNotFound(handleNotFound);
  server.begin();
}

void loop() 
{ 
  server.handleClient();
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for ( uint8_t i = 0; i < server.args(); i++ ) {
    message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }

  server.send ( 404, "text/plain", message );
}

void mainPage() { 
  if (server.args() == 4) {
    deviceSpeed[0] = server.arg("motor1").toInt();
    if ((deviceSpeed[0] >= -100) && (deviceSpeed[0] <= 100)) motor1.run(deviceSpeed[0]*10);
    deviceSpeed[1] = server.arg("motor2").toInt();
    if ((deviceSpeed[1] >= -100) && (deviceSpeed[1] <= 100)) motor2.run(deviceSpeed[1]*10);
    deviceSpeed[2] = server.arg("servo1").toInt();
    if ((deviceSpeed[2] >= 0) && (deviceSpeed[2] <= 180)) servo1.write(deviceSpeed[2]);
    deviceSpeed[3] = server.arg("servo2").toInt();
    if ((deviceSpeed[3] >= 0) && (deviceSpeed[3] <= 180)) servo2.write(deviceSpeed[3]);
  }
  // Chunk the data 
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200,"text/html");
  String sResponse  = "<!DOCTYPE html><html><head><title>ESP8266 Wireless Controller</title>"
                "</head><body>"
               "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=yes\">"
               "<meta charset=\"utf-8\">"
               "<script>function showSliderVal(idtext, val) {if (document.getElementById(idtext)) { document.getElementById(idtext).innerHTML = val; } }</script>";
  server.sendContent(sResponse);
  server.sendContent("<t1>WiFi CONTROLLER</t1>");
  String sForm  = "<form action=\"/control.html\" >";
  sForm = sForm + "<div>Motor 1 Speed:<input name=\"motor1\" type=\"range\" min=\"-100\" max=\"100\" value=\"" + deviceSpeed[0] + "\" oninput=\"showSliderVal('motor1val',this.value)\" ><span id=\"motor1val\">" + deviceSpeed[0] + "</span>&#37;</div>";
  sForm = sForm + "<div>Motor 2 Speed:<input name=\"motor2\" type=\"range\" min=\"-100\" max=\"100\" value=\"" + deviceSpeed[1] + "\" oninput=\"showSliderVal('motor2val',this.value)\" ><span id=\"motor2val\">" + deviceSpeed[1] + "</span>&#37;</div>";
  sForm = sForm + "<div>Servo 1 Position:<input name=\"servo1\" type=\"range\" min=\"0\" max=\"180\" value=\"" + deviceSpeed[2] + "\" oninput=\"showSliderVal('servo1val',this.value)\" ><span id=\"servo1val\">" + deviceSpeed[2] + "</span> deg</div>";
  sForm = sForm + "<div>Servo 2 Postion:<input name=\"servo2\" type=\"range\" min=\"0\" max=\"180\" value=\"" + deviceSpeed[3] + "\" oninput=\"showSliderVal('servo2val',this.value)\" ><span id=\"servo2val\">" + deviceSpeed[3] + "</span> deg</div>";
  sForm += "<input type=\"submit\" value=\"Update\">";
  sForm += "</form>";
  server.sendContent(sForm);
  server.sendContent("</body></html>");
  server.sendContent("");
}
 
