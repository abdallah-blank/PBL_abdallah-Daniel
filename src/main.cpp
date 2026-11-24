// Libraries
#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SD.h>

//Pin Definitions
#define Button_PIN 14
#define chipSelect 15

//Global Variables
volatile bool helpButtonPressed = false; 
byte statusByte = 0; // STATUS BYtE
byte previousStatusByte = 0; // PREVIOUS STATUS BYTE
bool sdReady = false; // SD CARD READY FLAG
const char* ssid = "STUDYROOMESP32"; // SSID - L7
const char* password = "password167!"; // PASSWORD -----------------------
WebServer server(80); // Create a web server object that listens for HTTP request on port 80  

//4.1
unsigned long lastButtonPress = 0; //Stores time of last button press L10
const unsigned long debounceDelay = 200; // Makes it need at least 200 milisec L10

bool occupied = false; //In Abdullah's code too

void IRAM_ATTR handleHelpButton() 
{
helpButtonPressed = true;
}

void logEvent(const char* eventType) {  //LAB 5
if (!sdReady) {
    Serial.println("SD card not read");
    return; 
    }
File logFile = SD.open("/events.csv", FILE_APPEND);
if (logFile) {
    logFile.print(millis());
    logFile.print(",");
    logFile.println(eventType);
    logFile.close();
    } 
else {
    Serial.println("Error opening log file");
    }
  }

  void handleData()
{
JsonDocument doc;

doc["roomStatus"] = occupied ? "Occupied" : "Free";
doc["averageTemperature"] = getAvgTemp();
doc["fanLevel"] = 0;
doc["statusByte"] = statusByte;
String json;
serializeJson(doc, json);
server.send(200, "application/json", json);
}

//SETUP
void setup ()
{
Serial.begin(115200);
SPI.begin();
pinMode(Button_PIN, INPUT_PULLUP); // BUTTON INPUT PULLUP SET
attachInterrupt(digitalPinToInterrupt(Button_PIN), handleHelpButton, FALLING); 

sdReady = SD.begin(chipSelect);
if (sdReady)
{
 Serial.println("SD card initialized.");

if (!SD.exists("/events.csv"))
{
  File logFile = SD.open("/events.csv", FILE_WRITE);

  if (logFile)
  {
    logFile.println("Timestamp,EventType");
    logFile.close();
    Serial.println("Log file created.");
  }
  else
  {
  Serial.println("Error creating log file.");
  }
}
}
else
{
    Serial.println("SD card initialization failed!");
}
WiFi.mode(WIFI_AP);
WiFi.softAP(ssid, password);
Serial.print("IP Adress: "); // not sure if needed... but
Serial.println(WiFi.softAPIP());
server.on("/data", HTTP_GET,handleData);
server.begin();
Serial.println("Server started");
} 


//LOOP
void loop ()
{
server.handleClient(); 
if (helpButtonPressed) //4.1
{
  helpButtonPressed = false;
  //Debounce button
  if (millis() - lastButtonPress >= debounceDelay) //4.1
  {
  lastButtonPress = millis(); //4.1
  statusByte |= 0b00000100; // SET HELP REQ
  logEvent("HELP");
  } 
}

if (occupied) //L2
{
  statusByte |= 0b00000001; // ROOm OCCUPIED
}
else
{
  statusByte &= ~0b00000001; // ROOM FREe
}

if (getAvgTemp() > potRead())
{
  statusByte |= 0b00000010; // SET TEMP HIGH
}
else
{
  statusByte &= ~0b00000010; // SET TEMP LOW
}

if (statusByte != previousStatusByte)
{
  Serial.print("Status Byte: ");
  Serial.println(statusByte, BIN);
  previousStatusByte = statusByte; //L2
}

}
