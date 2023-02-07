
/**
 * Created by K. Suwatchai (Mobizt)
 *
 * Email: k_suwatchai@hotmail.com
 *
 * Github: https://github.com/mobizt
 *
 * Copyright (c) 2023 mobizt
 *
 */

// This example shows how to connect to Google API via ethernet using external SSL client
// This example is for Raspberri Pi Pico and W5500 Ethernet module.

/**
 *
 * The W5500 Ethernet module and RPI2040 Pico board, SPI 0 port wiring connection.
 *
 * Raspberry Pi Pico                        W5500
 *
 * GPIO 16 - SPI 0 MISO                     SO
 * GPIO 19 - SPI 0 MOSI                     SI
 * GPIO 18 - SPI 0 SCK                      SCK
 * GPIO 17 - SPI 0 CS                       CS
 * GPIO 20 - W5500 Reset                    Reset
 * GND                                      GND
 * 3V3                                      VCC
 *
 */

/**
 * Do not forget to defines the following macro in ESP_Google_Sheet_Client_FS_Config.h
 * 
 * #define ESP_GOOGLE_SHEET_CLIENT_ENABLE_EXTERNAL_CLIENT
 * 
 */

#include <Arduino.h>
#include <ESP_Google_Sheet_Client.h>

// https://github.com/mobizt/ESP_SSLClient
#include <ESP_SSLClient.h>

#include <Ethernet.h>

// For how to create Service Account and how to use the library, go to https://github.com/mobizt/ESP-Google-Sheet-Client

#define PROJECT_ID "PROJECT_ID"

// Service Account's client email
#define CLIENT_EMAIL "CLIENT_EMAIL"

// Service Account's private key
const char PRIVATE_KEY[] PROGMEM = "-----BEGIN PRIVATE KEY-----XXXXXXXXXXXX-----END PRIVATE KEY-----\n";

#define WIZNET_RESET_PIN 20       // Connect W5500 Reset pin to GPIO 20 of Raspberry Pi Pico
#define WIZNET_CS_PIN PIN_SPI0_SS // Connect W5500 CS pin to SPI 0's SS (GPIO 17) of Raspberry Pi Pico

/* 5. Define MAC */
uint8_t Eth_MAC[] = {0x02, 0xF0, 0x0D, 0xBE, 0xEF, 0x01};

bool gsheetSetupReady = false;

bool taskComplete = false;

void setupGsheet();

void tokenStatusCallback(TokenInfo info);

// Define the basic client
// The network interface devices that can be used to handle SSL data should
// have large memory buffer up to 1k - 2k or more, otherwise the SSL/TLS handshake
// will fail.
EthernetClient basic_client;

// This is the wrapper client that utilized the basic client for io and
// provides the mean for the data encryption and decryption before sending to or after read from the io.
// The most probable failures are related to the basic client itself that may not provide the buffer
// that large enough for SSL data.
// The SSL client can do nothing for this case, you should increase the basic client buffer memory.
ESP_SSLClient ssl_client;

// UDP Client for NTP Time synching
EthernetUDP udpClient;

void ResetEthernet()
{
    Serial.println("Resetting WIZnet W5500 Ethernet Board...  ");
    pinMode(WIZNET_RESET_PIN, OUTPUT);
    digitalWrite(WIZNET_RESET_PIN, HIGH);
    delay(200);
    digitalWrite(WIZNET_RESET_PIN, LOW);
    delay(50);
    digitalWrite(WIZNET_RESET_PIN, HIGH);
    delay(200);
}

void networkConnection()
{
    Ethernet.init(WIZNET_CS_PIN);

    ResetEthernet();

    Serial.println("Starting Ethernet connection...");
    Ethernet.begin(Eth_MAC);

    unsigned long to = millis();

    while (Ethernet.linkStatus() == LinkOFF || millis() - to < 2000)
    {
        delay(100);
    }

    if (Ethernet.linkStatus() == LinkON)
    {
        Serial.print("Connected with IP ");
        Serial.println(Ethernet.localIP());
    }
    else
    {
        Serial.println("Can't connected");
    }
}

// Define the callback function to handle server status acknowledgement
void networkStatusRequestCallback()
{
    // Set the network status
    GSheet.setNetworkStatus(Ethernet.linkStatus() == LinkON);
}

void setup()
{

    Serial.begin(115200);

    delay(5000);

    Serial.printf("ESP Google Sheet Client v%s\n\n", ESP_GOOGLE_SHEET_CLIENT_VERSION);

    networkConnection();

    ssl_client.setClient(&basic_client);
    ssl_client.setInsecure();
}

void loop()
{

    if (!gsheetSetupReady)
        setupGsheet();

    bool ready = GSheet.ready();

    if (ready && !taskComplete)
    {

        // Google sheet code here

        taskComplete = true;
    }
}

void setupGsheet()
{
    // Set the callback for Google API access token generation status (for debug only)
    GSheet.setTokenCallback(tokenStatusCallback);

    /* Assign the pointer to global defined external SSL Client object and required callback functions */
    GSheet.setExternalClient(&ssl_client, networkConnection, networkStatusRequestCallback);

    /* Assign UDP client and gmt offset for NTP time synching when using external SSL client */
    GSheet.setUDPClient(&udpClient, 3);

    // Set the seconds to refresh the auth token before expire (60 to 3540, default is 300 seconds)
    GSheet.setPrerefreshSeconds(10 * 60);

    // Begin the access token generation for Google API authentication
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
