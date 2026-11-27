// Libraries
#include <Arduino.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SD.h>

//Define pins
//rfid
#define RFID_SS 2
#define RFID_RST 27
MFRC522 rfid(RFID_SS, RFID_RST);
//OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_DC     16
#define OLED_RESET 17
#define OLED_CS    5
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RESET, OLED_CS);

//Temp bmp280
float temperature;
Adafruit_BMP280 bmp;

//Potentiometer
#define POT_PIN 32

//SD
#define chipSelect 15

//misc
#define RED_PIN 26
#define GREEN_PIN 25
#define Button_PIN 14
#define FAN_PIN 13
//Misc Global Vars
String lastUID = "";
unsigned long lastScanTime = 0;
unsigned long indicatorStart = 0;
unsigned long indicatorTime = 0;
bool indicatorOn = false;
unsigned long sampleTime = 0;

//RFID CHECKIN/OUT Variables
const int numAuthorized = 1; //admin card and student card for access..
String authorizedUIDs[numAuthorized];
bool occupied = false;

//Potentiometer/ADC Variables
int ADC_BITS = 12;

//Fan PWM variables
#define LED_CH 0
const int PWM_FREQ = 25000;
const int PWM_RES = 8;

//temp readings var
const int NUM_SAMPLES = 10;
float samples[NUM_SAMPLES];


//Global Variables (Bitwise, SD, wifi, webserver)
volatile bool helpButtonPressed = false; //LAB 10
byte statusByte = 0; // LAB 2
byte previousStatusByte = 0; // Ref
bool sdReady = false; // LAB 5
const char* ssid = "STUDYROOMESP32"; // LAB 7
const char* password = "password167!"; // PASSWORD ---------------------- LAB 7
WebServer server(80); // LAB 7 


//4.1 
volatile unsigned long lastButtonPress = 0; //LAB 10 PART 4 Stores time of last button press 



String getUID()
{
  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++)
  {
    if (rfid.uid.uidByte[i] < 0x10)
    {
      uid = uid + "0";
    }
    uid = uid + String(rfid.uid.uidByte[i], HEX);
    if (i < rfid.uid.size - 1)
    {
      uid = uid + " ";
    }
  }
  uid.toUpperCase();
  return uid;
}



bool isAuthorized(String uid)
{
  for (int i = 0; i < numAuthorized; i++)
  {
    if (uid == authorizedUIDs[i])
    {
      return true;
    }
  }
  return false;
}



void ledOff(){
    digitalWrite(RED_PIN, LOW);
}
//time functions for red led turning off after a bit
void startIndicatorTimer(unsigned long duration)
{
  indicatorStart = millis();
  indicatorTime = duration;
  indicatorOn = true;
} 







void onTimerExpired(){
  ledOff();
  indicatorOn = false;
  Serial.println("Ready");
}



void checkIN(String uid)
{
  Serial.println("Checked in");
  authorizedUIDs[0] = uid;
  ledOff();
  digitalWrite(GREEN_PIN, HIGH);
  occupied = true;
}

void checkOUT(){
  Serial.println("Checked OUT");
  authorizedUIDs[0] = "";
  ledOff();
  digitalWrite(GREEN_PIN, LOW);
  occupied = false;
}

void accessDenied(){
  digitalWrite(RED_PIN, HIGH);
  Serial.println("ACCESS DENIED");
  startIndicatorTimer(1000);
}

void cardCheck(String uid){
  if (isAuthorized(uid) == false){
    accessDenied();
  }
  else{
    checkOUT();
  }
}

void accessCheck(String uid){
  
  if (uid == lastUID && millis() - lastScanTime < 2000)
  {
    Serial.println("Repeat ignored");
    return;
  }
  lastUID = uid;
  lastScanTime = millis();

  Serial.print("Card UID: ");
  Serial.println(uid);

  if(occupied == false){
      checkIN(uid);
  }
  else{
      cardCheck(uid);
  }
}


//bmp

void grabTemp(){
  if (millis() - sampleTime >= 2000){
    for (int i = 0 ; i < NUM_SAMPLES; i++){ 
      samples[i] = bmp.readTemperature();
      sampleTime = millis();
    }
  }

}

float getAvgTemp(){
  float sum = 0;
  for (int i = 0; i < NUM_SAMPLES; i++){
    sum = sum + samples[i];
  }

  return (float)sum/NUM_SAMPLES;
}

//FAN CONTROLS

float potRead(){
  int potValue = analogRead(POT_PIN);
  int maxValue = pow(2,ADC_BITS) -1;
  float setPoint = potValue * 40.0 / maxValue;
  return setPoint;
}

int fanSpeed(){
  float setPoint = potRead();
  float temp = getAvgTemp();
  float difference = temp - setPoint;

  if(setPoint < temp && occupied == true){
    int duty = map(difference, 0, 10, 150, 255);
    ledcWrite(FAN_PIN, duty);
    return duty;
  }
  else{
    ledcWrite(FAN_PIN, 0);
    return 0;
  }


}

//OLED display function
void showOnOLED(int Ycursor, String message){
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);

  display.setCursor(0,Ycursor);
  display.println(message);
  display.display();
}

void IRAM_ATTR handleHelpButton()
{ if (millis() - lastButtonPress > 200)
  {
    lastButtonPress = millis();
    helpButtonPressed = true;
  }
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

void handleData() //LAB 11 msotly bit from 7
{
JsonDocument doc;
int fan = fanSpeed();
doc["roomStatus"] = occupied ? "Occupied" : "Free";
doc["averageTemperature"] = getAvgTemp();
doc["fanLevel"] = fan;
doc["statusByte"] = statusByte;
String json;
serializeJson(doc, json);
server.send(200, "application/json", json);
}



// MS SAJU WEBPAGE START
const char webpage[] PROGMEM = R"rawliteral( 

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
// MS SAJU WEBPAGE END

void handleRoot() // LAB 7
{
  int fan = fanSpeed();
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
  String(fan)
);
  page.replace(
  "STATUS_BYTE_PLACEHOLDER", 
  String(statusByte, BIN)
);
  server.send(200, "text/html", page);
}


void setup(){
  Serial.begin(115200);
  pinMode(chipSelect, OUTPUT); digitalWrite(chipSelect, HIGH);
  pinMode(RFID_SS, OUTPUT); digitalWrite(RFID_SS, HIGH);
  pinMode(OLED_CS, OUTPUT); digitalWrite(OLED_CS, HIGH);
  SPI.begin();

  //Rfid
  rfid.PCD_Init();
  rfid.PCD_DumpVersionToSerial();
  
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  
  //OLED setup
  display.begin(SSD1306_SWITCHCAPVCC);
  display.clearDisplay();
  
  //sd start
  sdReady = SD.begin(chipSelect);
  
  //bmp280
  Wire.begin();
  // BMP280 code
  if(bmp.begin(0x76))
  {
    Serial.println("BMP280 Found");
  }
  else
  {
     Serial.println("BMP280 Not Found");
  }

  //ADC settings
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  //PWM settings
  ledcAttach(FAN_PIN, PWM_FREQ, PWM_RES);
  //button setups
  pinMode(Button_PIN, INPUT_PULLUP); // BUTTON INPUT PULLUP SET
  attachInterrupt(digitalPinToInterrupt(Button_PIN), handleHelpButton, FALLING); 





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
  Serial.print("IP Address: "); // not sure if needed... but
  Serial.println(WiFi.softAPIP());
  server.on("/", HTTP_GET, handleRoot);
  server.on("/data", HTTP_GET,handleData);
  server.begin();
  Serial.println("Server started"); 
}




void loop(){
  //Card detection
  if (rfid.PICC_IsNewCardPresent() == true && rfid.PICC_ReadCardSerial() == true)
  {
    String uid = getUID();
    accessCheck(uid);
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }

  //Display Room information on OLED
  grabTemp();
  float temp = getAvgTemp();
  String temperature = String(temp);
  showOnOLED(0, temperature);
  if(occupied){
    showOnOLED(16, "Occupied");
    display.setCursor(0,0);
    display.println("Temperature: ");
    display.print(temperature);
    display.display();
  }
  else{
    showOnOLED(16, "Free");
    display.setCursor(0,0);
    display.println("Temperature: ");
    display.print(temperature);
    display.display();
  }
  fanSpeed();

  //timer
  if (indicatorOn == true && millis() - indicatorStart >= indicatorTime){
    onTimerExpired();
  }
  server.handleClient(); 
  if (helpButtonPressed == true) // LAB 10 PArt 4
  {
    helpButtonPressed = false;
    statusByte |= 0b00000100; // LAB 2 BIT MASKING 
    logEvent("HELP"); // LAB 5 .. 
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






