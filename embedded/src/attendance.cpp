#include <Arduino.h>           // base arduino library
#include <Wire.h>              // for I2C communication protocol
#include <LiquidCrystal_I2C.h> // for controlling the 16x2 I2C LCD
#include <SPI.h>               // SPI protocol for the RC522
#include <MFRC522.h>           // RC522 library

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

// global variables & objects
LiquidCrystal_I2C lcd(0x27, LCD_COLS, LCD_ROWS);
MFRC522 mfrc522(SDA_PIN, RST_PIN);

const char *AP_NAME = "attendance";
const String API_URL = "http://arch-laptop.local:3000/api/mark-attendance";
// const String API_URL = "http://10.80.185.89:3000/api/mark-attendance";
bool showReconnectedMsg = false;

const String ADMIN_CARDS[] = {"A3517729", "5D497406"};
const int ADMIN_COUNT = sizeof(ADMIN_CARDS) / sizeof(ADMIN_CARDS[0]);
const String RESET_CARD_UID = "4340F113";
const String RESET_URL = "http://arch-laptop.local:3000/api/reset-attendance";

const int MAX_STUDENTS = 50;
String cardUID = "";
String recordedUIDs[MAX_STUDENTS];
int UIDCount = 0;
bool inAttendanceQueue = false;

// function prototypes
void APCallback(WiFiManager *myWiFiManager);
void connectToWifi();
String readRfidCard();
int manageAttendanceQueue();
void addUID();
String buildJSONPayload(String teacherUID);
bool isAdmin(String uid);
void printToLCD(const String &msg);
void printTapMsg();
void successBeep();
void failureBeep();

void setup()
{
    Serial.begin(115200);

    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);

    SPI.begin();        // init SPI bus
    mfrc522.PCD_Init(); // init MFRC522 card

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

    printTapMsg();
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

        printTapMsg();
    }

    // attempt to read a card
    cardUID = readRfidCard();

    if (cardUID != "")
    {
        successBeep();

        if (!inAttendanceQueue && cardUID == RESET_CARD_UID)
        {
            printToLCD("Resetting\nAttendance...");

            WiFiClient client;
            HTTPClient http;
            http.begin(client, RESET_URL);

            int httpCode = http.POST("{}");

            if (httpCode >= 200 && httpCode < 300)
            {
                printToLCD("Reset\nSuccessful");
                successBeep();
            }
            else
            {
                printToLCD("Reset Faile");
                failureBeep();
            }

            http.end();
            delay(2000);
            printTapMsg();
            return;
        }

        int shouldReturn = manageAttendanceQueue();
        if (shouldReturn)
        {
            printTapMsg();
            return;
        }

        addUID();
        delay(2000);
        printTapMsg();
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

int manageAttendanceQueue()
{
    if (!inAttendanceQueue)
    {
        if (isAdmin(cardUID))
        {
            printToLCD("Beginning Attendance Queue...");
            delay(2000);

            inAttendanceQueue = true;
            return 1;
        }
        else
        {
            printToLCD("Invalid Teacher's Card");
            failureBeep();
            delay(2000);

            return 1;
        }
    }
    else
    {
        if (isAdmin(cardUID))
        {
            if (UIDCount == 0)
            {
                printToLCD("No IDs recorded.");
                failureBeep();
                delay(2000);

                inAttendanceQueue = false;

                printTapMsg();
                return 1;
            }

            printToLCD("Marking Attendnace...");

            String JSONPayload = buildJSONPayload(cardUID);

            WiFiClient client;
            HTTPClient http;

            http.begin(client, API_URL);
            http.addHeader("Content-Type", "application/json");

            int httpCode = http.POST(JSONPayload);

            if (httpCode > 0)
            {
                String payload = http.getString();

                if (httpCode >= 200 && httpCode < 300)
                {
                    printToLCD(String("Attendance Marked. Students : ") + UIDCount);
                    successBeep();

                    UIDCount = 0;
                }
                else
                {
                    printToLCD("Unable to mark attendance!");
                    failureBeep();
                }
            }
            else
            {
                printToLCD("Unable to mark attendance!");
                failureBeep();
            }

            http.end();
            delay(2000);

            inAttendanceQueue = false;

            printTapMsg();
            return 1;
        }
        else
        {
            // in attendance queue and student's card, hence return 0 to continue scanning
            return 0;
        }
    }
}

void addUID()
{
    // check for duplicates first
    for (int i = 0; i < UIDCount; i++)
    {
        if (recordedUIDs[i] == cardUID)
        {
            printToLCD("Already Added.\nTotal: " + String(UIDCount));
            successBeep();
            return;
        }
    }

    if (UIDCount < MAX_STUDENTS)
    {
        recordedUIDs[UIDCount] = cardUID;
        UIDCount++;

        printToLCD("ID Added.\nTotal: " + String(UIDCount));
        successBeep();
    }
    else
    {
        printToLCD("UID array is full.");
        failureBeep();
    }
}

bool isAdmin(String uid)
{
    for (int i = 0; i < ADMIN_COUNT; i++)
    {
        if (uid == ADMIN_CARDS[i])
            return true;
    }
    return false;
}

String buildJSONPayload(String teacherUID)
{
    JsonDocument doc;

    doc["markedBy"] = teacherUID;

    // creating the root JSON object and nested array
    JsonArray UIDsArray = doc["studentIds"].to<JsonArray>();

    // populating the array
    for (int i = 0; i < UIDCount; i++)
    {
        UIDsArray.add(recordedUIDs[i]);
    }

    // serialize JSON to string
    String JSONPayload;
    serializeJson(doc, JSONPayload);

    return JSONPayload;
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

void printTapMsg()
{
    if (!inAttendanceQueue)
    {
        printToLCD("Tap Teacher's ID Card");
    }
    else
    {
        printToLCD("Tap your ID Card");
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