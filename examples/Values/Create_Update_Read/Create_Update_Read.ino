
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

//This example shows how to create the spreadsheet, update and read the values.

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

//Your email to share access to spreadsheet
#define USER_EMAIL "USER_EMAIL"

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

        Serial.println("\nCreate spreadsheet...");
        Serial.println("------------------------");

        FirebaseJson spreadsheet;
        spreadsheet.set("properties/title", "Test - Create Update and Read");

        String spreadsheetId, spreadsheetURL;
        bool success = false;

        //For Google Sheet API ref doc, go to https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets/create

        success = GSheet.create(&response /* returned response */, &spreadsheet /* spreadsheet object */, USER_EMAIL /* your email that this spreadsheet shared to */);
        response.toString(Serial, true);
        Serial.println();

        if (success)
        {

            //Get the spreadsheet id from already created file.
            FirebaseJsonData result;
            response.get(result, FPSTR("spreadsheetId")); //parse or deserialize the JSON response
            if (result.success)
                spreadsheetId = result.to<const char *>();

            //Get the spreadsheet URL.
            result.clear();
            response.get(result, FPSTR("spreadsheetUrl")); //parse or deserialize the JSON response
            if (result.success)
            {
                spreadsheetURL = result.to<const char *>();
                Serial.println("\nThe spreadsheet URL");
                Serial.println(spreadsheetURL);
            }

            //If you assign the spreadsheet id from your own spreadsheet,
            //to update and read, you need to set share access to the Service Account's CLIENT_EMAIL

            Serial.println("\nUpdate spreadsheet values...");
            Serial.println("------------------------------");

            FirebaseJson valueRange;

            valueRange.add("range", "Sheet1!A1:C3");
            valueRange.add("majorDimension", "COLUMNS");
            valueRange.set("values/[0]/[0]", "A1"); //column 1/row 1
            valueRange.set("values/[0]/[1]", "A2"); //column 1/row 2
            valueRange.set("values/[0]/[2]", "A3"); //column 1/row 3
            valueRange.set("values/[1]/[0]", "B1"); //column 2/row 1
            valueRange.set("values/[1]/[1]", "B2"); //column 2/row 2
            valueRange.set("values/[1]/[2]", "B3"); //column 2/row 3
            valueRange.set("values/[2]/[0]", "C1"); //column 3/row 1
            valueRange.set("values/[2]/[1]", "C2"); //column 3/row 2
            valueRange.set("values/[2]/[2]", "C3"); //column 3/row 3

            //For Google Sheet API ref doc, go to https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets.values/update

            success = GSheet.values.update(&response /* returned response */, spreadsheetId /* spreadsheet Id to update */, "Sheet1!A1:C3" /* range to update */, &valueRange /* data to update */);
            response.toString(Serial, true);
            Serial.println();

            Serial.println("\nUpdate spreadsheet values...");
            Serial.println("------------------------------");

            valueRange.clear();

            valueRange.add("range", "Sheet1!G1:I3");
            valueRange.add("majorDimension", "ROWS");
            valueRange.set("values/[0]/[0]", "G1"); //row 1/column 7
            valueRange.set("values/[1]/[0]", "G2"); //row 2/column 7
            valueRange.set("values/[2]/[0]", "G3"); //row 3/column 7
            valueRange.set("values/[0]/[1]", "H1"); //row 1/column 8
            valueRange.set("values/[1]/[1]", "H2"); //row 2/column 8
            valueRange.set("values/[2]/[1]", "H3"); //row 3/column 8
            valueRange.set("values/[0]/[2]", "I1"); //row 1/column 9
            valueRange.set("values/[1]/[2]", "I2"); //row 2/column 9
            valueRange.set("values/[2]/[2]", "I3"); //row 3/column 9

            success = GSheet.values.update(&response /* returned response */, spreadsheetId /* spreadsheet Id to update */, "Sheet1!G1:I3" /* range to update */, &valueRange /* data to update */);
            response.toString(Serial, true);
            Serial.println();

            if (success)
            {

                Serial.println("\nGet spreadsheet values...");
                Serial.println("------------------------------");

                //For Google Sheet API ref doc, go to https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets.values/get

                success = GSheet.values.get(&response /* returned response */, spreadsheetId /* spreadsheet Id to read */, "Sheet1!A1:C3" /* range to read */);
                response.toString(Serial, true);
                Serial.println();

                Serial.println("\nGet spreadsheet values...");
                Serial.println("------------------------------");

                success = GSheet.values.get(&response /* returned response */, spreadsheetId /* spreadsheet Id to read */, "Sheet1!G1:I3" /* range to read */);
                response.toString(Serial, true);
                Serial.println();
            }
        }

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