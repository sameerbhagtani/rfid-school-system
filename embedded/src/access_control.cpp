#include <Arduino.h>           // base arduino library
#include <Wire.h>              // for I2C communication protocol
#include <LiquidCrystal_I2C.h> // for controlling the 16x2 I2C LCD
#include <SPI.h>               // SPI protocol for the RC522
#include <MFRC522.h>           // RC522 library
#include <Servo.h>

#include <ESP8266WiFi.h>       // connect to wifi network
#include <ESP8266HTTPClient.h> // send http requests and receive resopnses
#include <ESP8266mDNS.h>       // mDNS resolution
#include <WiFiManager.h>       // for storing network credentials
#include <ArduinoJson.h>       // for building JSON payload

#define LCD_COLS 16
#define LCD_ROWS 2

#define SDA_PIN D4
#define RST_PIN D3
#define BUZZER_PIN D8
#define SERVO_PIN 16

// global variables & objects
LiquidCrystal_I2C lcd(0x27, LCD_COLS, LCD_ROWS);
MFRC522 mfrc522(SDA_PIN, RST_PIN);
Servo servo;

const char *AP_NAME = "access_control";
const String API_URL = "http://arch-laptop.local:3000/api/get-role";
// const String API_URL = "http://10.80.185.89:3000/api/get-role";
bool showReconnectedMsg = false;

// function prototypes
void APCallback(WiFiManager *myWiFiManager);
void connectToWifi();
String readRfidCard();
void printToLCD(const String &msg);
void successBeep();
void failureBeep();

void setup()
{
    Serial.begin(115200);

    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);

    SPI.begin();        // Init SPI bus
    mfrc522.PCD_Init(); // Init MFRC522 card

    Wire.begin();
    lcd.init();
    lcd.backlight();

    connectToWifi();

    if (!MDNS.begin("esp8266-attendance"))
    {
        Serial.println("Error setting up MDNS responder!");
    }
    else
    {
        Serial.println("mDNS responder started");
    }

    printToLCD("Tap your ID Card");
}

void loop()
{
    MDNS.update();

    if (WiFi.status() != WL_CONNECTED)
    {
        showReconnectedMsg = true;

        printToLCD("WiFi Lost\nReconnecting...");
        failureBeep();

        delay(5000);
        return;
    }

    if (WiFi.status() == WL_CONNECTED && showReconnectedMsg)
    {
        showReconnectedMsg = false;

        printToLCD("Reconnected!");
        successBeep();

        delay(2000);

        printToLCD("Tap your ID Card");
    }

    // attempt to read a card
    String cardUID = readRfidCard();

    if (cardUID != "")
    {
        printToLCD("Checking ID..."); // Give user feedback
        WiFiClient client;
        HTTPClient http;

        http.begin(client, API_URL + "/" + cardUID);

        int httpCode = http.GET();

        if (httpCode == HTTP_CODE_OK)
        {
            String payload = http.getString();

            StaticJsonDocument<300> doc;
            DeserializationError error = deserializeJson(doc, payload);

            if (!error)
            {
                bool success = doc["success"];

                if (success)
                {
                    String name = doc["data"]["name"];
                    String role = doc["data"]["role"];

                    if (role == "teacher")
                    {

                        printToLCD("Access Granted\n" + name);
                        successBeep();

                        // 1. Wake up the servo
                        servo.attach(SERVO_PIN);
                        delay(10); // Tiny delay to stabilize signal

                        servo.write(180);
                        delay(3000);
                        servo.write(0);

                        delay(1000);

                        servo.detach();
                    }
                    else
                    {
                        printToLCD("Access Denied\n" + name);
                        failureBeep();
                    }
                }
                else
                {
                    printToLCD("User Not Found");
                    failureBeep();
                }
            }
            else
            {
                printToLCD("Internal Error");
                failureBeep();
            }
        }
        else
        {
            printToLCD("Server Error");
            failureBeep();
        }

        http.end();

        delay(2000);
        printToLCD("Tap your ID Card");
    }
}

void APCallback(WiFiManager *myWiFiManager)
{
    printToLCD(String("Started AP Mode : ") + AP_NAME);
    failureBeep();
}

void connectToWifi()
{
    WiFiManager wifiManager;

    // the following will reset saved credentials
    // wifiManager.resetSettings();

    printToLCD("Connecting to saved WiFi...");

    wifiManager.setAPCallback(APCallback);

    // the following will try saved Wi-Fi, if not found, it starts AP mode
    if (!wifiManager.autoConnect(AP_NAME))
    {
        printToLCD("Can't Connect to WiFi");
        failureBeep();
        return;
    }

    successBeep();
    printToLCD("Connected!");

    delay(2000);
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
        if (msg[i] == '\n')
        {
            row++;
            col = 0;
            continue;
        }

        if (col == LCD_COLS)
        {
            row++;
            col = 0;
        }

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