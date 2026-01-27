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

void printToLCD(String msg);
String readRfidCard();

void setup()
{
    Serial.begin(115200);

    SPI.begin();        // Init SPI bus
    mfrc522.PCD_Init(); // Init MFRC522 card

    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    // Volume level (0-21)
    audio.setVolume(12);
    // Connect to an online MP3 stream or file
    audio.connecttohost("http://stream.radioparadise.com/mp3-128");

    lcd.init();
    lcd.backlight();

    printToLCD("Connecting...");

    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    WiFi.begin("android-phone", "20002000");

    while (WiFi.status() != WL_CONNECTED)
    {
    }

    printToLCD("Tap your ID Card");
}

void loop()
{
    // Attempt to read a card.
    String cardUID = readRfidCard();

    if (cardUID != "")
    {
        Serial.println(cardUID);
        printToLCD(cardUID);
        audio.loop();

        delay(1000);
        printToLCD("Tap your ID Card");
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