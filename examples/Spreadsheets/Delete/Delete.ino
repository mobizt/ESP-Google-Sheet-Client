
/**
 * Created by K. Suwatchai (Mobizt)
 * 
 * Email: suwatchai@outlook.com
 * 
 * Github: https://github.com/mobizt
 * 
 * Copyright (c) 2021 mobizt
 *
*/

//This example shows how to delete spreadsheet from Google Drive.

#include <Arduino.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#endif
#include <ESP_Google_Sheet_Client.h>

#define WIFI_SSID "WIFI_AP"
#define WIFI_PASSWORD "WIFI_PASSWORD"

//For how to create Service Account and how to use the library, go to https://github.com/mobizt/ESP-Google-Sheet-Client

#define PROJECT_ID "PROJECT_ID"

//Service Account's client email
#define CLIENT_EMAIL "CLIENT_EMAIL"

//Service Account's private key
const char PRIVATE_KEY[] PROGMEM = "-----BEGIN PRIVATE KEY-----XXXXXXXXXXXX-----END PRIVATE KEY-----\n";

bool taskComplete = false;

void tokenStatusCallback(TokenInfo info);

void setup()
{

    Serial.begin(115200);
    Serial.println();
    Serial.println();

    Serial.printf("ESP Google Sheet Client v%s\n\n", ESP_GOOGLE_SHEET_CLIENT_VERSION);

    WiFi.setAutoReconnect(true);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(300);
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();

    //Set the callback for Google API access token generation status (for debug only)
    GSheet.setTokenCallback(tokenStatusCallback);

    //Begin the access token generation for Google API authentication
    GSheet.begin(CLIENT_EMAIL, PROJECT_ID, PRIVATE_KEY);
}

void loop()
{
    //Call ready() repeatedly in loop for authentication checking and processing
    bool ready = GSheet.ready();

    if (ready && !taskComplete)
    {
        //For basic FirebaseJson usage example, see examples/FirebaseJson/Create_Edit_Parse/Create_Edit_Parse.ino

        FirebaseJson response;

        Serial.println("\nDelete spreadsheet from Google Drive...");
        Serial.println("------------------------------------------------");

        bool success = GSheet.deleteFile(&response /* returned response */, "<spreadsheetId>" /* spreadsheet Id to delete */);
        response.toString(Serial, true);
        Serial.println();

        Serial.println("\nDelete last 5 spreadsheets from Google Drive...");
        Serial.println("------------------------------------------------");

        success = GSheet.deleteFiles(&response /* returned response */);

        if (success)
            Serial.println("ok");

        taskComplete = true;
    }
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