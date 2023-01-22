
/**
 * Created by K. Suwatchai (Mobizt)
 *
 * Email: suwatchai@outlook.com
 *
 * Github: https://github.com/mobizt
 *
 * Copyright (c) 2023 mobizt
 *
 */

// This example shows how to create the spreadsheet.

#include <Arduino.h>
#if defined(ESP32) || defined(PICO_RP2040)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#include <ESP_Google_Sheet_Client.h>

#define WIFI_SSID "WIFI_AP"
#define WIFI_PASSWORD "WIFI_PASSWORD"

// For how to create Service Account and how to use the library, go to https://github.com/mobizt/ESP-Google-Sheet-Client

#define PROJECT_ID "PROJECT_ID"

// Service Account's client email
#define CLIENT_EMAIL "CLIENT_EMAIL"

// Your email to share access to spreadsheet
#define USER_EMAIL "USER_EMAIL"

// Service Account's private key
const char PRIVATE_KEY[] PROGMEM = "-----BEGIN PRIVATE KEY-----XXXXXXXXXXXX-----END PRIVATE KEY-----\n";

bool taskComplete = false;

#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
WiFiMulti multi;
#endif

void tokenStatusCallback(TokenInfo info);

void setup()
{

    Serial.begin(115200);
    Serial.println();
    Serial.println();

    Serial.printf("ESP Google Sheet Client v%s\n\n", ESP_GOOGLE_SHEET_CLIENT_VERSION);

#if defined(ESP32) || defined(ESP8266)
    WiFi.setAutoReconnect(true);
#endif

#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
    multi.addAP(WIFI_SSID, WIFI_PASSWORD);
    multi.run();
#else
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
#endif

    Serial.print("Connecting to Wi-Fi");
    unsigned long ms = millis();
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(300);
#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
        if (millis() - ms > 10000)
            break;
#endif
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();

    // Set the callback for Google API access token generation status (for debug only)
    GSheet.setTokenCallback(tokenStatusCallback);

    // The WiFi credentials are required for Pico W
    // due to it does not have reconnect feature.
#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
    GSheet.clearAP();
    GSheet.addAP(WIFI_SSID, WIFI_PASSWORD);
#endif

    // Begin the access token generation for Google API authentication
    GSheet.begin(CLIENT_EMAIL, PROJECT_ID, PRIVATE_KEY);
}

void loop()
{
    // Call ready() repeatedly in loop for authentication checking and processing
    bool ready = GSheet.ready();

    if (ready && !taskComplete)
    {
        // For basic FirebaseJson usage example, see examples/FirebaseJson/Create_Edit_Parse/Create_Edit_Parse.ino

        FirebaseJson response;
        // Instead of using FirebaseJson for response, you can use String for response to the functions
        // especially in low memory device that deserializing large JSON response may be failed as in ESP8266

        Serial.println("\nCreate spreadsheet...");
        Serial.println("------------------------");

        FirebaseJson spreadsheet;
        spreadsheet.set("properties/title", "Sheet1");
        spreadsheet.set("sheets/properties/title", "Sheet1");
        spreadsheet.set("sheets/properties/sheetId", 1);
        spreadsheet.set("sheets/properties/sheetType", "GRID");
        spreadsheet.set("sheets/properties/sheetType", "GRID");
        spreadsheet.set("sheets/properties/gridProperties/rowCount", 20);
        spreadsheet.set("sheets/properties/gridProperties/columnCount", 10);

        spreadsheet.set("sheets/developerMetadata/[0]/metadataValue", "test_value");
        spreadsheet.set("sheets/developerMetadata/[0]/metadataKey", "test_yey");
        spreadsheet.set("sheets/developerMetadata/[0]/visibility", "DOCUMENT");

        // For Google Sheet API ref doc, go to https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets/create

        bool success = GSheet.create(&response /* returned response */, &spreadsheet /* spreadsheet object */, USER_EMAIL /* your email that this spreadsheet shared to */);
        if (success)
            response.toString(Serial, true);
        else
            Serial.println(GSheet.errorReason());
        Serial.println();

#if defined(ESP32) || defined(ESP8266)
        Serial.println(ESP.getFreeHeap());
#elif defined(PICO_RP2040)
        Serial.println(rp2040.getFreeHeap());
#endif

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