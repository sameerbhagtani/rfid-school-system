#include <Arduino.h>           // base arduino library
#include <Wire.h>              // for I2C communication protocol
#include <LiquidCrystal_I2C.h> // for controlling the 16x2 I2C LCD
#include <SPI.h>               // SPI protocol for the RC522
#include <MFRC522.h>           // RC522 library
#include <Servo.h>

#define LCD_COLS 16
#define LCD_ROWS 2

// pin definitions
const int SDA_PIN = D4;
const int RST_PIN = D3;
const int BUZZER_PIN = D8;
const int SERVO_PIN = 16;

// global variables & objects
LiquidCrystal_I2C lcd(0x27, LCD_COLS, LCD_ROWS);
MFRC522 mfrc522(SDA_PIN, RST_PIN);
Servo servo;

// function prototypes
String readRfidCard();
void printToLCD(const String &msg);
void successBeep();
void failureBeep();

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);

    Serial.begin(115200);

    SPI.begin();        // Init SPI bus
    mfrc522.PCD_Init(); // Init MFRC522 card
    servo.attach(SERVO_PIN);

    lcd.init();
    lcd.backlight();

    printToLCD("Tap your ID Card");
}

void loop()
{
    // Attempt to read a card.
    String cardUID = readRfidCard();

    if (cardUID != "")
    {
        if (cardUID == "53CC0829")
        {
            printToLCD("Access Granted");
            successBeep();

            servo.write(180);
            delay(3000);

            servo.write(0);
        }
        else
        {
            printToLCD("Access Denied");
            failureBeep();
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

void successBeep()
{
    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);

    digitalWrite(BUZZER_PIN, LOW);
}

void failureBeep()
{
    digitalWrite(BUZZER_PIN, HIGH);
    delay(1000);

    digitalWrite(BUZZER_PIN, LOW);
}