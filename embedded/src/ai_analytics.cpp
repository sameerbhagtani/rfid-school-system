#include <Arduino.h>
#include <Wire.h>              // for I2C communication protocol
#include <LiquidCrystal_I2C.h> // for controlling the 16x2 I2C LCD
#include <SPI.h>               // SPI protocol for the RC522
#include <MFRC522.h>           // RC522 library

#include <WiFi.h>
#include "Audio.h"

// pin definitions
const int SDA_PIN = 5;
const int RST_PIN = 4;

const int I2S_LRC = 25;
const int I2S_BCLK = 26;
const int I2S_DOUT = 27;

// global variables & objects
LiquidCrystal_I2C lcd(0x27, 16, 2);
MFRC522 mfrc522(SDA_PIN, RST_PIN);
Audio audio;

bool isAudioPlaying = false;

void printToLCD(String msg);
String readRfidCard();
void saySomething(String text);
void audio_eof_speech(const char *info);

void setup()
{
    Serial.begin(115200);

    // 1. Initialize Hardware First
    lcd.init();
    lcd.backlight();
    printToLCD("Booting...");

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
    printToLCD("WiFi Ready!");

    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setVolume(21);

    printToLCD("Ready!");
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
            Serial.println("Card Detected: " + cardUID);
            printToLCD("ID: " + cardUID);

            // Play new audio based on input
            saySomething("Access Granted");

            // The isAudioPlaying flag is now true,
            // so this block won't run again until speech ends.
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

void printToLCD(String msg)
{
    int row = 0;
    int col = 0;

    lcd.clear();

    for (unsigned int i = 0; i < msg.length(); i++)
    {
        if (col == 16 || msg[i] == '\n')
        {
            row++;
            col = 0;

            if (msg[i] == '\n')
                continue;
        }

        lcd.setCursor(col++, row);
        lcd.print(msg[i]);
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