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
const int numAuthorized = 2; //admin card and student card for access..
String authorizedUIDs[numAuthorized];
bool occupied = false;

//Potentiometer Variables
int setPoint = 0;
int ADC_BITS = 12;

//Fan PWM variables
#define LED_CH 0
const int PWM_FREQ = 25000;
const int PWM_RES = 8;

//temp readings var
const int NUM_SAMPLES = 10;
float samples[NUM_SAMPLES];

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

void fanSpeed(){
  int potValue = analogRead(POT_PIN);

  if(potRead() < getAvgTemp() && occupied == true){
    int duty = map(potValue, 0, 4095, 100, 255);
    ledcWrite(FAN_PIN, duty);
  }
  else{
    ledcWrite(FAN_PIN, 0);
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





void setup(){
  Serial.begin(115200);
  SPI.begin();
  //Rfid
  rfid.PCD_Init();
  rfid.PCD_DumpVersionToSerial();
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  
  //OLED setp
  display.begin(SSD1306_SWITCHCAPVCC);
  display.clearDisplay();


  
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
  ledcWrite(FAN_PIN, 255);
  // fanSpeed();
  Serial.println(potRead());

  //timer
  if (indicatorOn == true && millis() - indicatorStart >= indicatorTime){
    onTimerExpired();
  }

}