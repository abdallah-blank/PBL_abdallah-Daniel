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
byte statusByte = 0; // SET HELP REQ BIT

void setup ()
{
pinMode(Button_PIN, INPUT_PULLUP); // BUTTON INPUT PULLUP SET
attachInterrupt(digitalPinToInterrupt(Button_PIN), handleHelpButton, FALLING); 
}

void loop ()
{
if (helpButtonPressed) 
{
  helpButtonPressed = false;
  statusByte |= 0b00000100; // HELP REQUEST BIT
}
}

void IRAM_ATTR handleHelpButton() 
{
helpButtonPressed = true;
}

