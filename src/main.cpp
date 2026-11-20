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
#define RFID_SS 5
#define RFID_RST 27
MFRC522 rfid(RFID_SS, RFID_RST);
//OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_DC     18
#define OLED_RESET 17
#define OLED_CS    6
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RESET, OLED_CS);

//Temp bmp280
float temperature;

//Potentiometer
#define POT_PIN 32

//SD
#define chipSelect 7

//misc
#define RED_PIN 26
#define GREEN_PIN 25
#define Button_PIN 14
#define FAN_PIN 9
