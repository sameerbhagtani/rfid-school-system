#include "wifi_helper.h"
#include "display.h"
#include "buzzer.h"

#include <ESP8266WiFi.h>
#include <WiFiManager.h>

static void APCallback(WiFiManager *manager)
{
    printToLCD("AP Mode Started");
    failureBeep();
}

void connectToWifi(const char *apName)
{
    WiFiManager wifiManager;

    printToLCD("Connecting WiFi...");
    wifiManager.setAPCallback(APCallback);

    if (!wifiManager.autoConnect(apName))
    {
        printToLCD("WiFi Failed");
        failureBeep();
        return;
    }

    successBeep();
    printToLCD("Connected!");
    delay(2000);
}

bool isWifiConnected()
{
    return WiFi.status() == WL_CONNECTED;
}
