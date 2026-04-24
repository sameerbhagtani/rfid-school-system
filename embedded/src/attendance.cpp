#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>

#include <display.h>
#include <buzzer.h>
#include <wifi_helper.h>
#include <rfid_helper.h>
#include <mdns_helper.h>

const char *AP_NAME = "attendance";

const String ATTENDANCE_URL = String(API_BASE_URL) + "/api/attendances";
const String RESET_URL = String(API_BASE_URL) + "/api/attendances/today";
const String TEACHERS_URL = String(API_BASE_URL) + "/api/users/teachers/ids";
const String RESET_ID_URL = String(API_BASE_URL) + "/api/users/reset/id";

const int MAX_TEACHERS = 25;
String teacherIds[MAX_TEACHERS];
int teacherCount = 0;

String resetCardUID = "";

const int MAX_STUDENTS = 50;
String recordedUIDs[MAX_STUDENTS];
int UIDCount = 0;

bool inQueue = false;
bool showReconnect = false;

bool isTeacher(const String &uid)
{
    for (int i = 0; i < teacherCount; i++)
    {
        if (teacherIds[i] == uid)
            return true;
    }

    return false;
}

void printTapMsg()
{
    if (inQueue)
        printToLCD("Tap your ID Card");
    else
        printToLCD("Tap Teacher Card");
}

void addUID(const String &uid)
{
    for (int i = 0; i < UIDCount; i++)
    {
        if (recordedUIDs[i] == uid)
        {
            printToLCD("Already Added");
            successBeep();
            return;
        }
    }

    if (UIDCount < MAX_STUDENTS)
    {
        recordedUIDs[UIDCount++] = uid;
        printToLCD("Added: " + String(UIDCount));
        successBeep();
    }
    else
    {
        printToLCD("List Full");
        failureBeep();
    }
}

String buildPayload(const String &teacherUID)
{
    JsonDocument doc;

    doc["markedBy"] = teacherUID;

    JsonArray arr = doc["studentIds"].to<JsonArray>();

    for (int i = 0; i < UIDCount; i++)
        arr.add(recordedUIDs[i]);

    String payload;
    serializeJson(doc, payload);

    return payload;
}

bool fetchTeacherIds()
{
    WiFiClient client;
    HTTPClient http;

    http.begin(client, TEACHERS_URL);

    int code = http.GET();

    if (code != HTTP_CODE_OK)
    {
        http.end();
        return false;
    }

    String payload = http.getString();
    http.end();

    JsonDocument doc;
    auto err = deserializeJson(doc, payload);

    if (err || !doc["success"])
        return false;

    JsonArray arr = doc["data"].as<JsonArray>();

    teacherCount = 0;

    for (JsonVariant v : arr)
    {
        if (teacherCount >= MAX_TEACHERS)
            break;

        teacherIds[teacherCount++] = v.as<String>();
    }

    return teacherCount > 0;
}

bool fetchResetId()
{
    WiFiClient client;
    HTTPClient http;

    http.begin(client, RESET_ID_URL);

    int code = http.GET();

    if (code != HTTP_CODE_OK)
    {
        http.end();
        return false;
    }

    String payload = http.getString();
    http.end();

    JsonDocument doc;
    auto err = deserializeJson(doc, payload);

    if (err || !doc["success"])
        return false;

    resetCardUID = doc["data"]["id"].as<String>();

    return !resetCardUID.isEmpty();
}

void loadBootData()
{
    printToLCD("Loading Config");

    bool teachersOk = fetchTeacherIds();
    bool resetOk = fetchResetId();

    if (teachersOk && resetOk)
    {
        printToLCD("Config Loaded");
        successBeep();
    }
    else
    {
        printToLCD("Load Failed");
        failureBeep();
    }

    delay(2000);
}

void setup()
{
    Serial.begin(115200);

    initLCD();
    initBuzzer();
    initRFID();

    connectToWifi(AP_NAME);

    initMDNS("esp8266-attendance");

    loadBootData();

    printTapMsg();
}

void loop()
{
    updateMDNS();

    if (!isWifiConnected())
    {
        showReconnect = true;
        printToLCD("WiFi Lost");
        failureBeep();
        delay(3000);
        return;
    }

    if (showReconnect)
    {
        showReconnect = false;

        printToLCD("Reconnected");
        successBeep();
        delay(1500);

        loadBootData();
        printTapMsg();
    }

    String uid = readRfidCard();

    if (uid.isEmpty())
        return;

    successBeep();

    // reset mode
    if (!inQueue && uid == resetCardUID)
    {
        WiFiClient client;
        HTTPClient http;

        printToLCD("Resetting...");

        http.begin(client, RESET_URL);
        http.addHeader("x-api-key", DEVICE_API_KEY);

        int code = http.sendRequest("DELETE");

        if (code >= 200 && code < 300)
        {
            printToLCD("Reset Done");
            successBeep();
        }
        else
        {
            printToLCD("Reset Failed");
            failureBeep();
        }

        http.end();

        delay(2000);
        printTapMsg();
        return;
    }

    // start queue
    if (!inQueue)
    {
        if (isTeacher(uid))
        {
            inQueue = true;
            printToLCD("Queue Started");
        }
        else
        {
            printToLCD("Invalid Teacher");
            failureBeep();
        }

        delay(2000);
        printTapMsg();
        return;
    }

    // end queue + submit
    if (isTeacher(uid))
    {
        if (UIDCount == 0)
        {
            inQueue = false;

            printToLCD("No Students");
            failureBeep();

            delay(2000);
            printTapMsg();
            return;
        }

        WiFiClient client;
        HTTPClient http;

        http.begin(client, ATTENDANCE_URL);
        http.addHeader("Content-Type", "application/json");
        http.addHeader("x-api-key", DEVICE_API_KEY);

        printToLCD("Submitting...");

        int code = http.POST(buildPayload(uid));

        if (code >= 200 && code < 300)
        {
            UIDCount = 0;

            printToLCD("Attendance Done");
            successBeep();
        }
        else
        {
            printToLCD("Submit Failed");
            failureBeep();
        }

        http.end();

        inQueue = false;

        delay(2000);
        printTapMsg();
        return;
    }

    // student tap
    addUID(uid);

    delay(1500);
    printTapMsg();
}
