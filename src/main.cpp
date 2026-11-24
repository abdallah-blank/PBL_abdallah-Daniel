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

const char webpage[] PROGMEM = R"rawliteral(

//MS SAJU'S WEBPAGE FROM LAB 7 
<!DOCTYPE html>
<html>

<head>

<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">

<meta http-equiv="refresh" content="5">

<title>Study Room</title>

<style>

body{
    font-family:Arial,sans-serif;
    background:linear-gradient(
        135deg,
        #1E3C72,
        #2A5298
    );
    color:white;
    text-align:center;
    margin:0;
    padding:20px;
}

.container{
    max-width:800px;
    margin:auto;
}

.card{
    background:rgba(
        255,
        255,
        255,
        0.15
    );

    padding:25px;
    border-radius:15px;

    box-shadow:
    0px 4px 12px rgba(
        0,
        0,
        0,
        0.3
    );
}

.value{
    font-size:42px;
    font-weight:bold;
    color:#FFD54F;
}

button{

    width:180px;
    height:60px;

    margin:10px;

    font-size:20px;

    border:none;

    border-radius:10px;

    cursor:pointer;
}

.on{
    background:#4CAF50;
    color:white;
}

.off{
    background:#F44336;
    color:white;
}

.info{
    margin-top:20px;
}

</style>

</head>

<body>

<div class="container">

<h1>🌎 Study Room</h1>

<div class="card">

<h2>Temperature</h2>

<p class="value">
TEMP_PLACEHOLDER
</p>

</div>

<br>

<div class="card">
<h2>Room Status</h2>
<p class="value">
ROOM_STATUS_PLACEHOLDER
</p>
</div>

<br>

<div class="card">
<h2>Fan Level</h2>
<p class="value">
FAN_LEVEL_PLACEHOLDER
</p>
</div>

<br>

<div class="card">
<h2>Status Byte</h2>
<p class="value">
STATUS_BYTE_PLACEHOLDER
</p>

</div>

<div class="info">

<p>
SSID: STUDYROOMESP32
</p>

<p>
IP Address: 192.168.4.1
</p>

</div>

</div>

</body>

</html>

)rawliteral";


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

void handleRoot()
{
  String page = webpage;
  page.replace(
  "TEMP_PLACEHOLDER", 
  String(getAvgTemp(), 1) + " °C"
);
  page.replace(
  "ROOM_STATUS_PLACEHOLDER", 
  occupied ? "Occupied" : "Free"
);
  page.replace(
  "FAN_LEVEL_PLACEHOLDER", 
  String(0)
);
  page.replace(
  "STATUS_BYTE_PLACEHOLDER", 
  String(statusByte, BIN)
);
  server.send(200, "text/html", page);
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
server.on("/", HTTP_GET, handleRoot);
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
