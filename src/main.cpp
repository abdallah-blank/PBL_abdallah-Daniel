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

//Global Variables
volatile bool helpButtonPressed = false; 
byte statusByte = 0; // STATUS BYtE

//4.1
unsigned long lastButtonPress = 0; //Stores time of last button press L10
const unsigned long debounceDelay = 200; // Makes it need at least 200 milisec L10

bool occupied = false; //In Abdullah's code too

void IRAM_ATTR handleHelpButton() 
{
helpButtonPressed = true;
}

void setup ()
{
pinMode(Button_PIN, INPUT_PULLUP); // BUTTON INPUT PULLUP SET
attachInterrupt(digitalPinToInterrupt(Button_PIN), handleHelpButton, FALLING); 
}

void loop ()
{
if (helpButtonPressed) //4.1
{
  helpButtonPressed = false;
  //Debounce button
  if (millis() - lastButtonPress >= debounceDelay) //4.1
  {
    lastButtonPress = millis(); //4.1
  statusByte |= 0b00000100; // SET HELP REQ
  } 
}

if (occupied)
{
  statusByte |= 0b00000001; // SET OCCUPIED
}
else
{
  statusByte &= 0b00000001; // CLEAR OCCUPIED
}
}
