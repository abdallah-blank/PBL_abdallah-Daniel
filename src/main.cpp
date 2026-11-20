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
String authorizedUIDs[2];
int numAuthorized = 2;
int tappedCard = 0;



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

//First card tapped gets authorisation and books the room
String addUID(){
  if (tappedCard = 1){
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

void CardCheck(String uid){
  if (isAuthorized(uid) == false){
    digitalWrite(RED_PIN, HIGH);
  }
  else{
    digitalWrite(GREEN_PIN, !digitalRead(GREEN_PIN));
    digitalWrite(RED_PIN, LOW);
  }
}


void setup(){


}

void loop(){

}