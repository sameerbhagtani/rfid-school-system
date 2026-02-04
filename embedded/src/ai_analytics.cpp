#include <Arduino.h>
#include <Wire.h>              // for I2C communication protocol
#include <LiquidCrystal_I2C.h> // for controlling the 16x2 I2C LCD
#include <SPI.h>               // SPI protocol for the RC522
#include <MFRC522.h>           // RC522 library
#include <WiFi.h>
#include "Audio.h"

#define LCD_COLS 16
#define LCD_ROWS 2

// pin definitions
const int SDA_PIN = 5;
const int RST_PIN = 4;

const int I2S_LRC = 25;
const int I2S_BCLK = 26;
const int I2S_DOUT = 27;

// global variables & objects
LiquidCrystal_I2C lcd(0x27, LCD_COLS, LCD_ROWS);
MFRC522 mfrc522(SDA_PIN, RST_PIN);
Audio audio;

bool isAudioPlaying = false;

void printToLCD(const String &msg);
String readRfidCard();
void saySomething(String text);
void audio_eof_speech(const char *info);

void setup()
{
    Serial.begin(115200);

    // 1. Initialize Hardware First
    lcd.init();
    lcd.backlight();

    SPI.begin();
    mfrc522.PCD_Init();

    // 2. Start WiFi
    WiFi.mode(WIFI_STA);
    WiFi.begin("android-phone", "20002000");

    printToLCD("Connecting WiFi");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    // 3. ONLY start Audio after WiFi is 100% stable
    Serial.println("\nWiFi Connected!");

    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setVolume(21);

    printToLCD("Tap your ID Card");
    saySomething("Tap your ID Card");
}

void loop()
{
    audio.loop();

    if (!isAudioPlaying)
    {
        String cardUID = readRfidCard();

        if (cardUID != "")
        {
            printToLCD("ID: " + cardUID);

            saySomething("Your attendance is 80% in this month. You need to come 5 more days to take your attendance upto 85%.");
        }
    }
}

String readRfidCard()
{
    // Look for new cards
    if (!mfrc522.PICC_IsNewCardPresent())
    {
        return "";
    }

    // Select one of the cards
    if (!mfrc522.PICC_ReadCardSerial())
    {
        return "";
    }

    // Convert UID to String
    String uid_str = "";
    for (byte i = 0; i < mfrc522.uid.size; i++)
    {
        if (mfrc522.uid.uidByte[i] < 0x10)
        {
            uid_str += "0";
        }
        uid_str += String(mfrc522.uid.uidByte[i], HEX);
    }

    mfrc522.PICC_HaltA();      // Halt PICC to go back to idle mode
    mfrc522.PCD_StopCrypto1(); // Stop encryption

    uid_str.toUpperCase();

    return uid_str;
}

void printToLCD(const String &msg)
{
    int row = 0;
    int col = 0;
    unsigned int len = msg.length();

    lcd.clear();

    for (unsigned int i = 0; i < len; i++)
    {
        // Handle manual newline characters
        if (msg[i] == '\n')
        {
            row++;
            col = 0;
            continue;
        }

        // Handle automatic word wrap when reaching end of column
        if (col == LCD_COLS)
        {
            row++;
            col = 0;
        }

        // Stop printing if we run out of LCD vertical space
        if (row >= LCD_ROWS)
            break;

        lcd.setCursor(col, row);
        lcd.print(msg[i]);

        col++;
    }
}

void saySomething(String text)
{
    isAudioPlaying = true;
    audio.connecttospeech(text.c_str(), "en");
}

void audio_eof_speech(const char *info)
{
    isAudioPlaying = false;
}