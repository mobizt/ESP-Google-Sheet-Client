
/**
 * Created by K. Suwatchai (Mobizt)
 * 
 * Email: k_suwatchai@hotmail.com
 * 
 * Github: https://github.com/mobizt
 * 
 * Copyright (c) 2021 mobizt
 *
*/

//This example shows how to connect to Google API via ethernet.

//This example is for ESP8266 and ENC28J60 Ethernet module.

/**
 * 
 * The ENC28J60 Ethernet module and ESP8266 board, SPI port wiring connection.
 * 
 * ESP8266 (Wemos D1 Mini or NodeMCU)        ENC28J60         
 * 
 * GPIO12 (D6) - MISO                        SO
 * GPIO13 (D7) - MOSI                        SI
 * GPIO14 (D5) - SCK                         SCK
 * GPIO16 (D0) - CS                          CS
 * GND                                       GND
 * 3V3                                       VCC
 * 
*/

#include <Arduino.h>
#include <ESP_Google_Sheet_Client.h>

//For how to create Service Account and how to use the library, go to https://github.com/mobizt/ESP-Google-Sheet-Client

#define PROJECT_ID "PROJECT_ID"

//Service Account's client email
#define CLIENT_EMAIL "CLIENT_EMAIL"

//Service Account's private key
const char PRIVATE_KEY[] PROGMEM = "-----BEGIN PRIVATE KEY-----XXXXXXXXXXXX-----END PRIVATE KEY-----\n";

bool gsheetSetupReady = false;

bool taskComplete = false;

void setupGsheet();

void tokenStatusCallback(TokenInfo info);

#define ETH_CS_PIN 16 //D0

ENC28J60lwIP eth(ETH_CS_PIN);
//Wiznet5100lwIP eth(ETH_CS_PIN);
//Wiznet5500lwIP eth(ETH_CS_PIN);


void setup()
{

    Serial.begin(115200);
    Serial.println();
    Serial.println();

    Serial.printf("ESP Google Sheet Client v%s\n\n", ESP_GOOGLE_SHEET_CLIENT_VERSION);

#if defined(ESP8266)
    SPI.begin();
    SPI.setClockDivider(SPI_CLOCK_DIV4); // 4 MHz?
    SPI.setBitOrder(MSBFIRST);
    SPI.setDataMode(SPI_MODE0);
    eth.setDefault(); // use ethernet for default route
    if (!eth.begin())
    {
        Serial.println("ethernet hardware not found ... sleeping");
        while (1)
        {
            delay(1000);
        }
    }
    else
    {
        Serial.print("connecting ethernet");
        while (!eth.connected())
        {
            Serial.print(".");
            delay(1000);
        }
    }
    Serial.println();
    Serial.print("ethernet IP address: ");
    Serial.println(eth.localIP());
#endif
}

void loop()
{
#if defined(ESP8266)

    if (!gsheetSetupReady)
        setupGsheet();

    bool ready = GSheet.ready();

    if (ready && !taskComplete)
    {

        //Google sheet code here

        taskComplete = true;
    }
#endif
}

void setupGsheet()
{
    //Set the callback for Google API access token generation status (for debug only)
    GSheet.setTokenCallback(tokenStatusCallback);

    //Begin the access token generation for Google API authentication
    GSheet.begin(CLIENT_EMAIL, PROJECT_ID, PRIVATE_KEY);

    gsheetSetupReady = true;
}

void tokenStatusCallback(TokenInfo info)
{
    if (info.status == esp_signer_token_status_error)
    {
        Serial.printf("Token info: type = %s, status = %s\n", GSheet.getTokenType(info).c_str(), GSheet.getTokenStatus(info).c_str());
        Serial.printf("Token error: %s\n", GSheet.getTokenError(info).c_str());
    }
    else
    {
        Serial.printf("Token info: type = %s, status = %s\n", GSheet.getTokenType(info).c_str(), GSheet.getTokenStatus(info).c_str());
    }
}
