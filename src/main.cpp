#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <Wifi.h>
#include <WebServer.h>
#include <Wire.h>
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

#define OLED_DC     18
#define OLED_RESET 17
#define OLED_CS    14
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RESET, OLED_CS);

//Temp bmp280
float temperature;

//Potentiometer
#define POT_PIN 32

//SD
#define chipSelect 15

//misc
#define RED_PIN 26
#define GREEN_PIN 25
#define Button_PIN 14
#define FAN_PIN 9

//RFID CHECKIN/OUT
const int numAuthorized = 2; //admin card and student card for access..
String authorizedUIDs[numAuthorized];
int tappedCard = 0;
bool occupied = false;



//temp readings var
const int NUM_SAMPLES = 10;
int samples[NUM_SAMPLES];

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
  tappedCard++;
  return uid;
}

//card tapped gets authorisation and books the room ( if unoccupied)
void addUID(){
  if (tappedCard == 1){
    authorizedUIDs[1] += getUID();
  }
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

void checkIN()
{
    ledOff();
    Serial.println("Checked in");
    digitalWrite(GREEN_PIN, HIGH);
    addUID();
    occupied = true;
}

void checkOUT(){
    ledOff();
    Serial.println("Checked OUT");
    digitalWrite(GREEN_PIN, LOW);
    occupied = false;
}

void accessDenied(){
    digitalWrite(RED_PIN, HIGH);
}

void cardCheck(String uid){
  if (isAuthorized(uid) == false){
    accessDenied();
  }
  else{
    checkOUT();
  }
}

void accessCheck(){
    if(occupied == false){
        checkIN();
    }
    else{
        cardCheck(getUID());
    }
}


//bmp
Adafruit_BMP280 bmp;

void grabTemp(){
    int sampleTime = millis();
    if (millis() - sampleTime >= 2000){
        for (int i = 0 ; i < NUM_SAMPLES; i++){
            
            samples[i] = bmp.readTemperature();
            sampleTime = millis();
        }
    }

}

float getAvgTemp(){
    long sum = 0;
    for (int i = 0; i < NUM_SAMPLES; i++){
        sum = sum + samples[i];
    }

    return (float)sum/NUM_SAMPLES;
}










void setup(){
    Serial.begin(115200);
    SPI.begin();
    //Rfid
    rfid.PCD_Init();
    rfid.PCD_DumpVersionToSerial();
    pinMode(RED_PIN, OUTPUT);
    pinMode(GREEN_PIN, OUTPUT);

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

}

void loop(){
  // Card detection
  if (rfid.PICC_IsNewCardPresent() == true && rfid.PICC_ReadCardSerial() == true)
  {
    accessCheck();
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }
}