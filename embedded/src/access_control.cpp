#include <Arduino.h>
#include <Servo.h>

#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>

#include <display.h>
#include <buzzer.h>
#include <wifi_helper.h>
#include <rfid_helper.h>
#include <mdns_helper.h>
#include <config.h>

const char *AP_NAME = "access_control";
const String BASE_URL = API_BASE_URL;

Servo servo;
bool showReconnect = false;

void setup()
{
    Serial.begin(115200);

    initLCD();
    initBuzzer();
    initRFID();

    connectToWifi(AP_NAME);

    initMDNS("esp8266-access-control");

    printToLCD("Tap your ID Card");
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
        delay(2000);
        printToLCD("Tap your ID Card");
    }

    String uid = readRfidCard();
    if (uid == "")
        return;

    printToLCD("Checking...");

    WiFiClient client;
    HTTPClient http;

    String url = BASE_URL + "/api/users/" + uid + "/role";

    http.begin(client, url);

    int code = http.GET();

    if (code == HTTP_CODE_OK)
    {
        String payload = http.getString();

        StaticJsonDocument<300> doc;
        auto err = deserializeJson(doc, payload);

        if (!err && doc["success"])
        {
            String name = doc["data"]["name"];
            String role = doc["data"]["role"];

            if (role == "teacher")
            {
                printToLCD("Granted\n" + name);
                successBeep();

                servo.attach(SERVO_PIN);
                delay(10);

                servo.write(180);
                delay(3000);
                servo.write(0);
                delay(1000);

                servo.detach();
            }
            else
            {
                printToLCD("Denied\n" + name);
                failureBeep();
            }
        }
        else
        {
            printToLCD("User Missing");
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
