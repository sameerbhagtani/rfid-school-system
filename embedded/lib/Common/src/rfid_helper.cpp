#include "rfid_helper.h"
#include "config.h"

#include <SPI.h>
#include <MFRC522.h>

static MFRC522 mfrc522(SDA_PIN, RST_PIN);

void initRFID()
{
    SPI.begin();
    mfrc522.PCD_Init();
}

String readRfidCard()
{
    if (!mfrc522.PICC_IsNewCardPresent())
        return "";
    if (!mfrc522.PICC_ReadCardSerial())
        return "";

    String uid = "";

    for (byte i = 0; i < mfrc522.uid.size; i++)
    {
        if (mfrc522.uid.uidByte[i] < 0x10)
            uid += "0";
        uid += String(mfrc522.uid.uidByte[i], HEX);
    }

    uid.toUpperCase();

    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();

    return uid;
}
