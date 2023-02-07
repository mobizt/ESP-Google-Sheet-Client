# Arduino Google Sheet Client Library for ESP32, ESP8266 and Raspberry Pi Pico (RP2040)


Arduino Google Sheet Client Library for ESP32, ESP8266 and Raspberry Pi Pico (RP2040).

This library allows devices to authenticate and communicate with Google Sheet APIs using the Service Account.

Devices will be able to read, update, append and clear sheet values. Create, read, list, and delete the spreadsheet are also supported.


The spreadsheet that created using this library, owned by the Service Account and shared access to the user.


You can create, edit and deploy the Apps Script code via extension of spreadsheet that created by this library except for run the script due to permission denied.


Spreadsheet created or owned by you, needed to share the access with Service Account's client email then library can read, and edit except for delete the user's spreadsheet due to permission denied.


## Dependencies


This library required **ESP8266, ESP32 and Raspberry Pi Pico Arduino Core SDK** to be installed.

To install device SDK, in Arduino IDE, ESP8266, ESP32 and Pico Core SDK can be installed through **Boards Manager**. 

In PlatfoemIO IDE, ESP32 and ESP8266 devices's Core SDK can be installed through **PIO Home** > **Platforms** > **Espressif 8266 or Espressif 32**.


### RP2040 Arduino SDK installation

For Arduino IDE, the Arduino-Pico SDK can be installed from Boards Manager by searching pico and choose Raspberry Pi Pico/RP2040 to install.

For PlatformIO, the Arduino-Pico SDK can be installed via platformio.ini

```ini
[env:rpipicow]
platform = https://github.com/maxgerhardt/platform-raspberrypi.git
board = rpipicow
framework = arduino
board_build.core = earlephilhower
monitor_speed = 115200
board_build.filesystem_size = 1m
```

See this Arduino-Pico SDK [documentation](https://arduino-pico.readthedocs.io/en/latest/) for more information.




## Prerequisites

This library used the Service Account crendentials to create the short lived access token for Google API authentication, which will be expired in 1 hour and will be refresh automatically.

You need to create the Service Account private key and enable Google Sheet and Google Drive APIs for your project.

To enable Google Sheet API for your project, go to https://console.cloud.google.com/apis/library/sheets.googleapis.com

![Create SA](/media/images/API_Enable1.png)

To enable Google Drive API for your project, go to https://console.cloud.google.com/apis/library/drive.googleapis.com

![Create SA](/media/images/API_Enable2.png)


In case the spreadsheet that works with this library was created or owned by you, you need to share that spreadsheet to the Service Account client email.

If the spreadsheet created by this library, it will set the share access to your account automatically, you will get the notification email of the file sharing.


## How to Create Service Account Private Key


Go to [Google Cloud Console](https://console.cloud.google.com/projectselector2/iam-admin/settings).

1. Choose or create project to create Service Account.

2. Choose Service Accounts

![Select Project](/media/images/GC_Select_Project.png)

3. Click at + CREAT SERVICE ACCOUNT.

![Create SA](/media/images/GC_Create_SA.png)

4. Enter the Service account name, 

5. Service account ID and

6. Click at CREATE AND CONTINUE

![Create SA2](/media/images/GC_Create_SA2.png)

7. Select Role.

8. Click at CONTINUE.

![Create SA3](/media/images/GC_Create_SA3.png)

9. Click at DONE.

![Create SA4](/media/images/GC_Create_SA4.png)

10. Choose service account that recently created from the list.

![Create SA5](/media/images/GC_Create_SA5.png)

11. Choose KEYS.

![Create SA6](/media/images/GC_Create_SA6.png)

12. Click ADD KEY and choose Create new key.

![Create SA7](/media/images/GC_Create_SA7.png)

13. Choose JSON for Key type and click CREATE. 


![Create SA8](/media/images/GC_Create_SA8.png)

14. Private key will be created for this service account and downloaded to your computer, click CLOSE. 


![Create SA9](/media/images/GC_Create_SA9.png)

In the following stepts (15-16) for saving the Service Account Credential in flash memory at compile time.

If you want to allow library to read the Service Account JSON key file directly at run time, skip these steps.

15. Open the .json file that is already downloaded with text editor.

```json
{
  "type": "service_account",
  "project_id": "...",
  "private_key_id": "...",
  "private_key": "-----BEGIN PRIVATE KEY-----\n...\n-----END PRIVATE KEY-----\n",
  "client_email": "...",
  "client_id": "...",
  "auth_uri": "https://accounts.google.com/o/oauth2/auth",
  "token_uri": "https://oauth2.googleapis.com/token",
  "auth_provider_x509_cert_url": "https://www.googleapis.com/oauth2/v1/certs",
  "client_x509_cert_url": "..."
}

```
16. Copy project_id, client_email, private_key_id and private_key from .json file and paste to these defines in the example.

```cpp
#define PROJECT_ID "..." //Taken from "project_id" key in JSON file.
#define CLIENT_EMAIL "..." //Taken from "client_email" key in JSON file.
const char PRIVATE_KEY[] PROGMEM = "-----BEGIN PRIVATE KEY-----\n...\n-----END PRIVATE KEY-----\n"; //Taken from "private_key" key in JSON file.
```






## Installation


### Using Library Manager

At Arduino IDE, go to menu **Sketch** -> **Include Library** -> **Manage Libraries...**

In Library Manager Window, search **"sheet"** in the search form then select **"ESP Google Spreadsheet Client"**. 

Click **"Install"** button.



For PlatformIO IDE, using the following command.

**pio lib install "ESP Google Sheet Client""**

Or at **PIO Home** -> **Library** -> **Registry** then search **ESP Google Sheet Client**.



### Manual installation

For Arduino IDE, download zip file from the repository (Github page) by select **Clone or download** dropdown at the top of repository, select **Download ZIP** 

From Arduino IDE, select menu **Sketch** -> **Include Library** -> **Add .ZIP Library...**.

Choose **ESP-Google-Sheet-Client-master.zip** that previously downloaded.

Go to menu **Files** -> **Examples** -> **ESP-Google-Sheet-Client-master** and choose one from examples.



## Usages


See [all examples](/examples) for complete usages.


```cpp

#include <Arduino.h>
#if defined(ESP32) || defined(ARDUINO_RASPBERRY_PI_PICO_W)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

#include <ESP_Google_Sheet_Client.h>

#define WIFI_SSID "WIFI_AP"
#define WIFI_PASSWORD "WIFI_PASSWORD"

#define PROJECT_ID "PROJECT_ID"

//Service Account's client email
#define CLIENT_EMAIL "CLIENT_EMAIL"

//Service Account's private key
const char PRIVATE_KEY[] PROGMEM = "-----BEGIN PRIVATE KEY-----XXXXXXXXXXXX-----END PRIVATE KEY-----\n";

bool taskComplete = false;

// For Pico, WiFiMulti is recommended.
#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
WiFiMulti multi;
#endif

void setup()
{

    Serial.begin(115200);
    Serial.println();
    Serial.println();

    // Set auto reconnect WiFi or network connection
#if defined(ESP32) || defined(ESP8266)
    WiFi.setAutoReconnect(true);
#endif

// Connect to WiFi or network
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


    // The WiFi credentials are required for Pico W
    // due to it does not have reconnect feature.
#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
    GSheet.clearAP();
    GSheet.addAP(WIFI_SSID, WIFI_PASSWORD);
    // You can add many WiFi credentials here
#endif

    //Begin the access token generation for Google API authentication
    GSheet.begin(CLIENT_EMAIL, PROJECT_ID, PRIVATE_KEY);

    // In case SD/SD_MMC storage file access, mount the SD/SD_MMC card.
    // SD_Card_Mounting(); // See src/GS_SDHelper.h

    // Or begin with the Service Account JSON file that uploaded to the Filesystem image or stored in SD memory card.
    // GSheet.begin("path/to/serviceaccount/json/file", esp_google_sheet_file_storage_type_flash /* or esp_google_sheet_file_storage_type_sd */);
}


void loop()
{
    //Call ready() repeatedly in loop for authentication checking and processing
    bool ready = GSheet.ready();

    if (ready && !taskComplete)
    {
        //For basic FirebaseJson usage example, see examples/FirebaseJson/Create_Edit_Parse/Create_Edit_Parse.ino

        //If you assign the spreadsheet id from your own spreadsheet,
        //you need to set share access to the Service Account's CLIENT_EMAIL

        FirebaseJson response;
        // Instead of using FirebaseJson for response, you can use String for response to the functions 
        // especially in low memory device that deserializing large JSON response may be failed as in ESP8266

        Serial.println("Get spreadsheet values from range...");
        Serial.println("---------------------------------------------------------------");

        bool success = GSheet.values.get(&response /* returned response */, "<spreadsheetId>" /* spreadsheet Id to read */, "Sheet1!A1:A3" /* range to read */);
        response.toString(Serial, true);
        Serial.println();

        taskComplete = true;
    }
}

```




## IDE Configuaration for ESP8266 MMU - Adjust the Ratio of ICACHE to IRAM

### Arduino IDE

When you update the ESP8266 Arduino Core SDK to v3.0.0, the memory can be configurable from Arduino IDE board settings.

By default MMU **option 1** was selected, the free Heap can be low and may not suitable for the SSL client usage in this library.

To increase the Heap, choose the MMU **option 3**, 16KB cache + 48KB IRAM and 2nd Heap (shared).

![Arduino IDE config](/media/images/ArduinoIDE.png)

To use external Heap from 1 Mbit SRAM 23LC1024, choose the MMU **option 5**, 128K External 23LC1024.

![MMU VM 128K](/media/images/ESP8266_VM.png)

To use external Heap from PSRAM, choose the MMU **option 6**, 1M External 64 MBit PSRAM.

The connection between SRAM/PSRAM and ESP8266

```
23LC1024/ESP-PSRAM64                ESP8266

CS (Pin 1)                          GPIO15
SCK (Pin 6)                         GPIO14
MOSI (Pin 5)                        GPIO13
MISO (Pin 2)                        GPIO12
/HOLD (Pin 7 on 23LC1024 only)      3V3
Vcc (Pin 8)                         3V3
Vcc (Pin 4)                         GND
```


### PlatformIO IDE

By default the balanced ratio (32KB cache + 32KB IRAM) configuration is used.

To increase the heap, **PIO_FRAMEWORK_ARDUINO_MMU_CACHE16_IRAM48_SECHEAP_SHARED** build flag should be assigned in platformio.ini.

```ini
[env:d1_mini]
platform = espressif8266
build_flags = -D PIO_FRAMEWORK_ARDUINO_MMU_CACHE16_IRAM48_SECHEAP_SHARED
board = d1_mini
framework = arduino
monitor_speed = 115200
```

And to use external Heap from 1 Mbit SRAM 23LC1024 and 64 Mbit PSRAM, **PIO_FRAMEWORK_ARDUINO_MMU_EXTERNAL_128K** and **PIO_FRAMEWORK_ARDUINO_MMU_EXTERNAL_1024K** build flags should be assigned respectively.

The supportedd MMU build flags in PlatformIO.

- **PIO_FRAMEWORK_ARDUINO_MMU_CACHE16_IRAM48**

   16KB cache + 48KB IRAM (IRAM)

- **PIO_FRAMEWORK_ARDUINO_MMU_CACHE16_IRAM48_SECHEAP_SHARED**

   16KB cache + 48KB IRAM and 2nd Heap (shared)

- **PIO_FRAMEWORK_ARDUINO_MMU_CACHE16_IRAM32_SECHEAP_NOTSHARED**

   16KB cache + 32KB IRAM + 16KB 2nd Heap (not shared)

- **PIO_FRAMEWORK_ARDUINO_MMU_EXTERNAL_128K**

   128K External 23LC1024

- **PIO_FRAMEWORK_ARDUINO_MMU_EXTERNAL_1024K**

   1M External 64 MBit PSRAM

- **PIO_FRAMEWORK_ARDUINO_MMU_CUSTOM**

   Disables default configuration and expects user-specified flags


To use PSRAM/SRAM for internal memory allocation which you can config to use it via [**ESP_Google_Sheet_Client_FS_Config.h**](src/ESP_Google_Sheet_Client_FS_Config.h) with this macro.

```cpp
#define ESP_GOOGLE_SHEET_CLIENT_USE_PSRAM
```

   
### Test code for MMU

```cpp

#include <Arduino.h>
#include <umm_malloc/umm_heap_select.h>

void setup() 
{
  Serial.begin(115200);
  HeapSelectIram ephemeral;
  Serial.printf("IRAM free: %6d bytes\r\n", ESP.getFreeHeap());
  {
    HeapSelectDram ephemeral;
    Serial.printf("DRAM free: %6d bytes\r\n", ESP.getFreeHeap());
  }

  ESP.setExternalHeap();
  Serial.printf("External free: %d\n", ESP.getFreeHeap());
  ESP.resetHeap();
}

void loop() {
  // put your main code here, to run repeatedly:
}

```


### Use PSRAM on ESP32


To enable PSRAM in ESP32 module with on-board PSRAM chip, in Arduino IDE

![Enable PSRAM in ESP32](/media/images/ESP32-PSRAM.png)


In PlatformIO in VSCode IDE, add the following build_flags in your project's platformio.ini file

```ini
build_flags = -DBOARD_HAS_PSRAM -mfix-esp32-psram-cache-issue
```

*When config the IDE or add the build flags to use PSRAM in the ESP32 dev boards that do not have on-board PSRAM chip, your device will be crashed (reset).


To use PSRAM for internal memory allocation which you can config to use it via [**ESP_Google_Sheet_Client_FS_Config.h**](src/ESP_Google_Sheet_Client_FS_Config.h) with this macro.

```cpp
#define ESP_GOOGLE_SHEET_CLIENT_USE_PSRAM
```

## Functions Description

### Global Functions

#### Begin the Google API authentication. 

param **`client_email`** (string) The Service Account's client email.

param **`project_id`** (string) The project ID. 

param **`private_key`** (string) The Service Account's private key.

param **`eth`** (optional for ESP8266 only) The pointer to ESP8266 lwIP network class e.g. ENC28J60lwIP, Wiznet5100lwIP and Wiznet5500lwIP.

```cpp
void begin(<string> client_email, <string> project_id, <string> private_key, <ESP8266_spi_eth_module> *eth = nullptr);
```



#### Begin the Google API authentication. 

param **`service_account_file`** (string) The Service Account's JSON key file.

param **`storage_type`** (esp_google_sheet_file_storage_type) The JSON key file storage type e.g. esp_google_sheet_file_storage_type_flash and esp_google_sheet_file_storage_type_sd.

param **`eth`** (optional for ESP8266 only) The pointer to ESP8266 lwIP network class e.g. ENC28J60lwIP, Wiznet5100lwIP and Wiznet5500lwIP.

```cpp
void begin(<string> service_account_file, esp_google_sheet_file_storage_type storage_type, <ESP8266_spi_eth_module> *eth = nullptr);
```



#### Set the OAuth2.0 token generation status callback. 

param **`callback`** The callback function that accepts the TokenInfo as argument.
 
```cpp
void setTokenCallback(TokenStatusCallback callback);
```

#### Add the WiFi Access point credentials for connection resume (non-ESP device only).

param **`param`** ssid The access point ssid.

param **`param`** password The access point password.
```cpp
void addAP(T1 ssid, T2 password);
```

#### Clear all WiFi Access points credentials (non-ESP device only).

```cpp
void clearAP();
```

#### Assign external Arduino Client and required callback fumctions.

param **`client`** The pointer to Arduino Client derived class of SSL Client.

param **`networkConnectionCB`** The function that handles the network connection.

param **`networkStatusCB`** The function that handle the network connection status acknowledgement.

```cpp
void setExternalClient(Client *client, GS_NetworkConnectionRequestCallback networkConnectionCB,
                           GS_NetworkStatusRequestCallback networkStatusCB);
```

####  Assign UDP client and gmt offset for NTP time synching when using external SSL client

param **`client`** The pointer to UDP client based on the network type.

param **`gmtOffset`** The GMT time offset.

```cpp
void setUDPClient(UDP *client, float gmtOffset = 0);
```

####  Set the network status acknowledgement.

param **`status`** The network status.

```cpp
void setNetworkStatus(bool status);
```


####  Set the seconds to refesh auth token before it expires.

param **`seconds`** The seconds (60 sec to 3540 sec) that auth token will refresh before expired.

Default value is 300 seconds.

```cpp
void setPrerefreshSeconds(uint16_t seconds);
```



#### Set the Root certificate data for server authorization 

param **`ca`** PEM format certificate string.
 
```cpp
void setCert(const char *ca);
```


#### Set the Root certificate file for server authorization. 

param **`filename`** PEM format certificate file name included path.

param **`storageType`** The storage type of certificate file. esp_google_sheet_file_storage_type_flash or esp_google_sheet_file_storage_type_sd

```cpp
void setCertFile(<string> filename, esp_google_sheet_file_storage_type storageType);
```


#### Get the authentication ready status and process the authentication. 

Note: This function should be called repeatedly in loop.

```cpp
bool ready();
```


#### Get the generated access token.

retuen **`String`** of OAuth2.0 access token.

```cpp
String accessToken();
```


#### Get the token type string.

param  **`info`** The TokenInfo structured data contains token info.

retuen **`String`** of token type.

```cpp
String getTokenType();

String getTokenType(TokenInfo info);
```


#### Get the token status string.

param  **`info`** The TokenInfo structured data contains token info.

retuen **`String`** of token status.

```cpp
String getTokenStatus();

String getTokenStatus(TokenInfo info);
```


#### Get the token generation error string.

param  **`info`** The TokenInfo structured data contains token info.

retuen **`String`** of token error.

```cpp
String getTokenError();

String getTokenError(TokenInfo info);
```


#### Get the token expiration timestamp (seconds from midnight Jan 1, 1970).

retuen **`unsigned long`** of timestamp.

```cpp
unsigned long getExpiredTimestamp();
```

#### Force the token to expire immediately and refresh.

```cpp
void refreshToken();
```


#### Reset stored config and auth credentials.

```cpp
void reset();
```

#### Initiate SD card with SPI port configuration.

param **`ss`** The SPI Chip/Slave Select pin.

param **`sck`** The SPI Clock pin.

param **`miso`** The SPI MISO pin.

param **`mosi`** The SPI MOSI pin.

aram **`frequency`** The SPI frequency.

return **`boolean`** The boolean value indicates the success of operation.

```cpp
bool sdBegin(int8_t ss = -1, int8_t sck = -1, int8_t miso = -1, int8_t mosi = -1, uint32_t frequency = 4000000);
```


#### Initiate SD card with SD FS configurations (ESP8266 only).

param **`ss`** SPI Chip/Slave Select pin.

param **`sdFSConfig`** The pointer to SDFSConfig object (ESP8266 only).

return **`boolean`** type status indicates the success of the operation.

```cpp
  bool sdBegin(SDFSConfig *sdFSConfig);
```


#### Initiate SD card with chip select and SPI configuration (ESP32 only).

param **`ss`** The SPI Chip/Slave Select pin.

param **`spiConfig`** The pointer to SPIClass object for SPI configuartion.

param **`frequency`** The SPI frequency.

return **`boolean`** The boolean value indicates the success of operation.

```cpp
bool sdBegin(int8_t ss, SPIClass *spiConfig = nullptr, uint32_t frequency = 4000000);
```


#### Initiate SD card with SdFat SPI and pins configurations (with SdFat included only).

param **`sdFatSPIConfig`** The pointer to SdSpiConfig object for SdFat SPI configuration.

param **`ss`** The SPI Chip/Slave Select pin.

param **`sck`** The SPI Clock pin.

param **`miso`** The SPI MISO pin.

param **`mosi`** The SPI MOSI pin.

return **`boolean`** The boolean value indicates the success of operation.

```cpp
 bool sdBegin(SdSpiConfig *sdFatSPIConfig, int8_t ss = -1, int8_t sck = -1, int8_t miso = -1, int8_t mosi = -1);
```


#### Initiate SD card with SdFat SDIO configuration (with SdFat included only).

param **`sdFatSDIOConfig`** The pointer to SdioConfig object for SdFat SDIO configuration.

return **`boolean`** The boolean value indicates the success of operation.

```cpp
 bool sdBegin(SdioConfig *sdFatSDIOConfig);
```


#### Initialize the SD_MMC card (ESP32 only).

param **`mountpoint`** The mounting point.

param **`mode1bit`** Allow 1 bit data line (SPI mode).

param **`format_if_mount_failed`** Format SD_MMC card if mount failed.

return **`Boolean`** type status indicates the success of the operation.

```cpp
bool sdMMCBegin(const char *mountpoint = "/sdcard", bool mode1bit = false, bool format_if_mount_failed = false);
```

#### Applies one or more updates to the spreadsheet. 

param **`response`** (FirebaseJson or String) The returned response.

param **`spreadsheetId`** (string) The spreadsheet to apply the updates to.

param **`requestsArray`** (FirebaseJsonArray of Request oobject) A list of updates to apply to the spreadsheet. 

Note: Requests will be applied in the order they are specified. If any request is not valid, no requests will be applied.

param **`includeSpreadsheetInResponse`** (boolean string) Determines if the update response should include the spreadsheet resource.

param **`responseRanges`** (string) Limits the ranges included in the response spreadsheet. Meaningful only if includeSpreadsheetInResponse is 'true'.

param **`responseIncludeGridData`** (boolean string) True if grid data should be returned. Meaningful only if includeSpreadsheetInResponse is 'true'. This parameter is ignored if a field mask was set in the request.

return **`Boolean`** type status indicates the success of the operation.

For ref doc go to https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets/batchUpdate

```cpp
bool batchUpdate(FirebaseJson *response, <string> spreadsheetId, FirebaseJsonArray *requestsArray, <string> includeSpreadsheetInResponse = "", <string> responseRanges = "", <string> responseIncludeGridData = "");
```

```cpp
bool batchUpdate(String *response, <string> spreadsheetId, FirebaseJsonArray *requestsArray, <string> includeSpreadsheetInResponse = "", <string> responseRanges = "", <string> responseIncludeGridData = "");
```


#### Creates a spreadsheet, returning the newly created spreadsheet. 

param **`response`** (FirebaseJson or String) The returned response.

param **`spreadsheet`** (FirebaseJson) The spreadsheet object.

param **`sharedUserEmail`** (string) Email of user to share the access. 

Note Google Drive API should be enabled at, https://console.cloud.google.com/apis/library/drive.googleapis.com

return **`Boolean`** type status indicates the success of the operation.

For ref doc go to https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets/create

```cpp
bool create(FirebaseJson *response, FirebaseJson *spreadsheet, <string> sharedUserEmail);
```

```cpp
bool create(String *response, FirebaseJson *spreadsheet, <string> sharedUserEmail);
```


#### Get the spreadsheet at the given ID. 

param **`response`** (FirebaseJson or String) The returned response.

param **`spreadsheetId`** (FirebaseJson) The spreadsheet to request. 

param **`ranges`** (string) The ranges to retrieve from the spreadsheet. Ranges separated with comma ",". 

param **`includeGridData`** (boolean string) True if grid data should be returned. 

Note This parameter is ignored if a field mask was set in the request. 

return **`Boolean`** type status indicates the success of the operation.

For ref doc, go to https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets/get
  
```cpp
bool get(FirebaseJson *response, <string> spreadsheetId, <string> ranges = "", <string> includeGridData = "");
```

```cpp
bool get(String *response, <string> spreadsheetId, <string> ranges = "", <string> includeGridData = "");
```



#### Get the spreadsheet at the given ID. 

param **`response`** (FirebaseJson or String) The returned response.

param **`spreadsheetId`** (FirebaseJson) The spreadsheet to request. 

return **`Boolean`** type status indicates the success of the operation.

For ref doc, go to https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets/getByDataFilter

```cpp
bool getByDataFilter(FirebaseJson *response, <string> spreadsheetId, FirebaseJsonArray *dataFiltersArray, <string> includeGridData = "");
```

```cpp
bool getByDataFilter(String *response, <string> spreadsheetId, FirebaseJsonArray *dataFiltersArray, <string> includeGridData = "");
```



#### Delete a spreadsheet from Google Drive. 

param **`response`** (FirebaseJson or String) The returned response.

param **`spreadsheetId`** (FirebaseJson) The ID of spreadsheet to delete. 

param **`closeSession`** (boolean) Close the session after delete. 

return **`Boolean`** type status indicates the success of the operation.

Note The response may be empty.

```cpp
bool deleteFile(FirebaseJson *response, <string> spreadsheetId, bool closeSession = true);
```

```cpp
bool deleteFile(String *response, <string> spreadsheetId, bool closeSession = true);
```



#### Delete spreadsheets from Google Drive. 

param **`response`** (FirebaseJson or String) The returned response.

Note This will delete last 5 spreadsheets at a time.

return **`Boolean`** type status indicates the success of the operation.

Note: The response may be empty.

```cpp
bool deleteFiles(FirebaseJson *response);
```

```cpp
bool deleteFiles(String *response);
```


#### List the spreadsheets in Google Drive. 

param **`response`** (FirebaseJson or String) The returned response.

param **`pageSize`** (integer) The maximum number of files to return per page. 

param **`orderBy`** (string) A comma-separated list of sort keys. 

Note: Valid keys are 'createdTime', 'folder', 'modifiedByMeTime', 'modifiedTime', 'name', 'name_natural', 'quotaBytesUsed', 'recency', 'sharedWithMeTime', 'starred', and 'viewedByMeTime'. 

Each key sorts ascending by default, but may be reversed with the 'desc' modifier. 

Example usage: ?orderBy=folder,modifiedTime desc,name. 

param **`pageToken`** (string) The token for continuing a previous list request on the next page. 

Note: This should be set to the value of 'nextPageToken' from the previous response.

return **`Boolean`** type status indicates the success of the operation.

```cpp
bool listFiles(FirebaseJson *response, uint32_t pageSize = 5, <string> orderBy = "createdTime%20desc", <string> pageToken = "");
```

```cpp
bool listFiles(String *response, uint32_t pageSize = 5, <string> orderBy = "createdTime%20desc", <string> pageToken = "");
```


### Spreadsheets.Values member functions

#### Get a range of values from a spreadsheet.

param **`response`** (FirebaseJson or String) The returned response.

param **`spreadsheetId`** (string) The ID of the spreadsheet to retrieve data from.

param **`range`** (string) The A1 notation or R1C1 notation of the range to retrieve values from.

return **`Boolean`** type status indicates the success of the operation.

For ref doc go to https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets.values/get

```cpp
bool get(FirebaseJson *response, <string> spreadsheetId, <string> range);
```

```cpp
bool get(String *response, <string> spreadsheetId, <string> range);
```


#### Get one or more ranges of values from a spreadsheet.

param **`response`** (FirebaseJson or String) The returned response.

param **`spreadsheetId`** (string) The ID of the spreadsheet to retrieve data from.

param **`ranges`** (string) The A1 notation or R1C1 notation of the range to retrieve values from. Ranges separated with comma ",".

param **`majorDimension`** (enum string) The major dimension that results should use.

Note If the spreadsheet data is: A1=1,B1=2,A2=3,B2=4, then requesting range=A1:B2,majorDimension=ROWS returns [[1,2],[3,4]], whereas requesting range=A1:B2,majorDimension=COLUMNS returns [[1,3],[2,4]].

**DIMENSION_UNSPECIFIED** The default value, do not use.

**ROWS** Operates on the rows of a sheet.

**COLUMNS** Operates on the columns of a sheet.

param **`valueRenderOption`** (enum string) How values should be represented in the output. 

Note: The default render option is ValueRenderOption.FORMATTED_VALUE.

**FORMATTED_VALUE** Values will be calculated & formatted in the reply according to the cell's formatting. 

Formatting is based on the spreadsheet's locale, not the requesting user's locale. 

For example, if A1 is 1.23 and A2 is =A1 and formatted as currency, then A2 would return "$1.23".

**UNFORMATTED_VALUE** Values will be calculated, but not formatted in the reply. 

For example, if A1 is 1.23 and A2 is =A1 and formatted as currency, then A2 would return the number 1.23.

**FORMULA** Values will not be calculated. The reply will include the formulas. 

For example, if A1 is 1.23 and A2 is =A1 and formatted as currency, then A2 would return "=A1".
 
param **`dateTimeRenderOption`** (enum string) How dates, times, and durations should be represented in the output. 

Note: This is ignored if valueRenderOption is FORMATTED_VALUE. 

The default dateTime render option is SERIAL_NUMBER.
**SERIAL_NUMBER** Instructs date, time, datetime, and duration fields to be output as doubles in "serial number" format, as popularized by Lotus 1-2-3. 

The whole number portion of the value (left of the decimal) counts the days since December 30th 1899. 

The fractional portion (right of the decimal) counts the time as a fraction of the day. 

For example, January 1st 1900 at noon would be 2.5, 2 because it's 2 days after December 30st 1899, and .5 because noon is half a day. 

February 1st 1900 at 3pm would be 33.625. This correctly treats the year 1900 as not a leap year.

**FORMATTED_STRING** Instructs date, time, datetime, and duration fields to be output as strings in their given number format (which is dependent on the spreadsheet locale).

return **`Boolean`** type status indicates the success of the operation.

For ref doc go to https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets.values/batchGet
  
```cpp
bool batchGet(FirebaseJson *response, <string> spreadsheetId, <string> ranges, <string> majorDimension = "", <string> valueRenderOption = "", <string> dateTimeRenderOption = "");
```

```cpp
bool batchGet(String *response, <string> spreadsheetId, <string> ranges, <string> majorDimension = "", <string> valueRenderOption = "", <string> dateTimeRenderOption = "");
```


#### Get one or more ranges of values that match the specified data filters.

param **`response`** (FirebaseJson or String) The returned response.

param **`spreadsheetId`** (string) The ID of the spreadsheet to retrieve data from.

param **`dataFiltersArray`** (FirebaseJsonArray of DataFilter object) The data filters used to match the ranges of values to retrieve. Ranges that match any of the specified data filters are included in the response.

param **`majorDimension`** (enum string) The major dimension that results should use.

Note: If the spreadsheet data is: A1=1,B1=2,A2=3,B2=4, then requesting range=A1:B2,majorDimension=ROWS returns [[1,2],[3,4] whereas requesting range=A1:B2,majorDimension=COLUMNS returns [[1,3],[2,4]].

**DIMENSION_UNSPECIFIED** The default value, do not use.

**ROWS** Operates on the rows of a sheet.

**COLUMNS** Operates on the columns of a sheet.

param **`valueRenderOption`** (enum string) How values should be represented in the output. 

Note: The default render option is ValueRenderOption.FORMATTED_VALUE.

**FORMATTED_VALUE** Values will be calculated & formatted in the reply according to the cell's formatting. 

Formatting is based on the spreadsheet's locale, not the requesting user's locale. 

For example, if A1 is 1.23 and A2 is =A1 and formatted as currency, then A2 would return "$1.23".
**UNFORMATTED_VALUE** Values will be calculated, but not formatted in the reply. 

For example, if A1 is 1.23 and A2 is =A1 and formatted as currency, then A2 would return the number 1.23.
**FORMULA** Values will not be calculated. The reply will include the formulas. 

For example, if A1 is 1.23 and A2 is =A1 and formatted as currency, then A2 would return "=A1".

param **`dateTimeRenderOption`** (enum string) How dates, times, and durations should be represented in the output. 

Note: This is ignored if valueRenderOption is FORMATTED_VALUE. 

The default dateTime render option is SERIAL_NUMBER.

**SERIAL_NUMBER** Instructs date, time, datetime, and duration fields to be output as doubles in "serial number" format, as popularized by Lotus 1-2-3. 

The whole number portion of the value (left of the decimal) counts the days since December 30th 1899. 

The fractional portion (right of the decimal) counts the time as a fraction of the day. 

For example, January 1st 1900 at noon would be 2.5, 2 because it's 2 days after December 30st 1899, and .5 because noon is half a day.

February 1st 1900 at 3pm would be 33.625. This correctly treats the year 1900 as not a leap year.

**FORMATTED_STRING** Instructs date, time, datetime, and duration fields to be output as strings in their given number  format (which is dependent on the spreadsheet locale).

return **`Boolean`** type status indicates the success of the operation.

For ref doc go to https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets.values/batchGetByDataFilter

```cpp
bool batchGetByDataFilter(FirebaseJson *response, <string> spreadsheetId, FirebaseJsonArray *dataFiltersArray, <string> majorDimension, <string> valueRenderOption = "", <string> dateTimeRenderOption = "");
```

```cpp
bool batchGetByDataFilter(String *response, <string> spreadsheetId, FirebaseJsonArray *dataFiltersArray, <string> majorDimension, <string> valueRenderOption = "", <string> dateTimeRenderOption = "");
```


#### Appends values to a spreadsheet.

param **`response`** (FirebaseJson or String) The returned response.

param **`spreadsheetId`** (string) The ID of the spreadsheet to update.

param **`range`** (string) The A1 notation of a range to search for a logical table of data. 

Values are appended after the last row of the table.

param **`valueRange`** (FirebaseJson of valueRange object) The data within a range of the spreadsheet.

param **`valueInputOption`** (enum string) How the input data should be interpreted. 

Note:

**INPUT_VALUE_OPTION_UNSPECIFIED** Default input value. This value must not be used.

**RAW** The values the user has entered will not be parsed and will be stored as-is.

**USER_ENTERED** The values will be parsed as if the user typed them into the UI. 

Numbers will stay as numbers, but strings may be converted to numbers, dates, etc. following the same rules that are applied when entering text into a cell via the Google Sheets UI.

param **`insertDataOption`** (enum string) How the input data should be inserted.

Note:

**OVERWRITE** The new data overwrites existing data in the areas it is written. 
(Note: adding data to the end of the sheet will still insert new rows or columns so the data can be written.) or 

**INSERT_ROWS** Rows are inserted for the new data.

param **`includeValuesInResponse`** (boolean string). Determines if the update response should include the values of the cells that were appended. 

Note: By default, responses do not include the updated values.

param **`responseValueRenderOption`** (enum string) Determines how values in the response should be rendered. 

Note:

The default render option is FORMATTED_VALUE.

**FORMATTED_VALUE** Values will be calculated & formatted in the reply according to the cell's formatting. 

Formatting is based on the spreadsheet's locale, not the requesting user's locale. 

For example, if A1 is 1.23 and A2 is =A1 and formatted as currency, then A2 would return "$1.23".

**UNFORMATTED_VALUE** Values will be calculated, but not formatted in the reply. 

For example, if A1 is 1.23 and A2 is =A1 and formatted as currency, then A2 would return the number 1.23.

**FORMULA** Values will not be calculated. The reply will include the formulas. 

For example, if A1 is 1.23 and A2 is =A1 and formatted as currency, then A2 would return "=A1".

param **`responseDateTimeRenderOption`** (enum string) Determines how dates, times, and durations in the response should be rendered. 

Note: This is ignored if responseValueRenderOption is FORMATTED_VALUE. 


The default dateTime render option is SERIAL_NUMBER.

**SERIAL_NUMBER** Instructs date, time, datetime, and duration fields to be output as doubles in "serial number" format, as popularized by Lotus 1-2-3. 

The whole number portion of the value (left of the decimal) counts the days since December 30th 1899. 

The fractional portion (right of the decimal) counts the time as a fraction of the day. 

For example, January 1st 1900 at noon would be 2.5, 2 because it's 2 days after December 30st 1899, and .5 because noon is half a day. 

February 1st 1900 at 3pm would be 33.625. This correctly treats the year 1900 as not a leap year.

**FORMATTED_STRING** Instructs date, time, datetime, and duration fields to be output as strings in their given number format (which is dependent on the spreadsheet locale).
 
 return **`Boolean`** type status indicates the success of the operation.
 
 For ref doc go to https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets.values/append
   
```cpp
bool append(FirebaseJson *response, <string> spreadsheetId, <string> range, FirebaseJson *valueRange, <string> valueInputOption = "USER_ENTERED", <string> insertDataOption = "", <string> includeValuesInResponse = "", <string> responseValueRenderOption = "", <string> responseDateTimeRenderOption = "");
```

```cpp
bool append(String *response, <string> spreadsheetId, <string> range, FirebaseJson *valueRange, <string> valueInputOption = "USER_ENTERED", <string> insertDataOption = "", <string> includeValuesInResponse = "", <string> responseValueRenderOption = "", <string> responseDateTimeRenderOption = "");
```

    
#### Sets values in a range of a spreadsheet. 
 
param **`response`** (FirebaseJson or String) The returned response.

param **`spreadsheetId`** (string) The ID of the spreadsheet to update.

param **`range`** (string) The A1 notation of the values to update.

param **`valueRange`** (FirebaseJson of valueRange object) The data within a range of the spreadsheet.

param **`valueInputOption`** (enum string) How the input data should be interpreted. 

Note:

**INPUT_VALUE_OPTION_UNSPECIFIED** Default input value. This value must not be used.

**RAW** The values the user has entered will not be parsed and will be stored as-is.

**USER_ENTERED** The values will be parsed as if the user typed them into the UI. 

Numbers will stay as numbers, but strings may be converted to numbers, dates, etc. following the same rules that are applied when entering text into a cell via the Google Sheets UI.

param **`includeValuesInResponse`** (boolean string). Determines if the update response should include the values of the cells that were appended. 

Note: By default, responses do not include the updated values.
 
param **`responseValueRenderOption`** (enum string) Determines how values in the response should be rendered. 

Note:

The default render option is FORMATTED_VALUE.

**FORMATTED_VALUE** Values will be calculated & formatted in the reply according to the cell's formatting. 

Formatting is based on the spreadsheet's locale, not the requesting user's locale. 

For example, if A1 is 1.23 and A2 is =A1 and formatted as currency, then A2 would return "$1.23".

**UNFORMATTED_VALUE** Values will be calculated, but not formatted in the reply. 

For example, if A1 is 1.23 and A2 is =A1 and formatted as currency, then A2 would return the number 1.23.

**FORMULA** Values will not be calculated. The reply will include the formulas. 

For example, if A1 is 1.23 and A2 is =A1 and formatted as currency, then A2 would return "=A1".

param **`responseDateTimeRenderOption`** (enum string) Determines how dates, times, and durations in the response should be rendered. 

Note: This is ignored if responseValueRenderOption is FORMATTED_VALUE. 

The default dateTime render option is SERIAL_NUMBER.

**SERIAL_NUMBER** Instructs date, time, datetime, and duration fields to be output as doubles in "serial number" format, as popularized by Lotus 1-2-3. 

The whole number portion of the value (left of the decimal) counts the days since December 30th 1899. 

The fractional portion (right of the decimal) counts the time as a fraction of the day. 

For example, January 1st 1900 at noon would be 2.5, 2 because it's 2 days after December 30st 1899, and .5 because noon is half a day. 

February 1st 1900 at 3pm would be 33.625. This correctly treats the year 1900 as not a leap year.

**FORMATTED_STRING** Instructs date, time, datetime, and duration fields to be output as strings in their given number format (which is dependent on the spreadsheet locale).

return **`Boolean`** type status indicates the success of the operation.

For ref doc go to https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets.values/update

```cpp
bool update(FirebaseJson *response, <string> spreadsheetId, <string> range, FirebaseJson *valueRange, <string> valueInputOption = "USER_ENTERED", <string> includeValuesInResponse = "", <string> responseValueRenderOption = "", <string> responseDateTimeRenderOption = "");
```

```cpp
bool update(String *response, <string> spreadsheetId, <string> range, FirebaseJson *valueRange, <string> valueInputOption = "USER_ENTERED", <string> includeValuesInResponse = "", <string> responseValueRenderOption = "", <string> responseDateTimeRenderOption = "");
```


#### Sets values in one or more ranges of a spreadsheet.

param **`response`** (FirebaseJson or String) The returned response.

param **`spreadsheetId`** (string) The ID of the spreadsheet to update.

param **`valueRangeArray`** (FirebaseJsonArray of valueRange object) The data within a range of the spreadsheet.

param **`valueInputOption`** (enum string) How the input data should be interpreted. 

Note:

**INPUT_VALUE_OPTION_UNSPECIFIED** Default input value. This value must not be used.

**RAW** The values the user has entered will not be parsed and will be stored as-is.

**USER_ENTERED** The values will be parsed as if the user typed them into the UI. 

Numbers will stay as numbers, but strings may be converted to numbers, dates, etc. following the same rules that are applied when entering text into a cell via the Google Sheets UI.

param **`includeValuesInResponse`** (boolean string). Determines if the update response should include the values of the cells that were appended. 

Note: By default, responses do not include the updated values.

param **`responseValueRenderOption`** (enum string) Determines how values in the response should be rendered. 

Note:

The default render option is FORMATTED_VALUE.

**FORMATTED_VALUE** Values will be calculated & formatted in the reply according to the cell's formatting.

Formatting is based on the spreadsheet's locale, not the requesting user's locale. 

For example, if A1 is 1.23 and A2 is =A1 and formatted as currency, then A2 would return "$1.23".

**UNFORMATTED_VALUE** Values will be calculated, but not formatted in the reply. 

For example, if A1 is 1.23 and A2 is =A1 and formatted as currency, then A2 would return the number 1.23.

**FORMULA** Values will not be calculated. The reply will include the formulas. 

For example, if A1 is 1.23 and A2 is =A1 and formatted as currency, then A2 would return "=A1".

param **`responseDateTimeRenderOption`** (enum string) Determines how dates, times, and durations in the response should be rendered. 

Note: This is ignored if responseValueRenderOption is FORMATTED_VALUE. 

The default dateTime render option is SERIAL_NUMBER.

**SERIAL_NUMBER** Instructs date, time, datetime, and duration fields to be output as doubles in "serial number" format, as popularized by Lotus 1-2-3. 

The whole number portion of the value (left of the decimal) counts the days since December 30th 1899. 

The fractional portion (right of the decimal) counts the time as a fraction of the day. 

For example, January 1st 1900 at noon would be 2.5, 2 because it's 2 days after December 30st 1899, and .5 because noon is half a day. 

February 1st 1900 at 3pm would be 33.625. This correctly treats the year 1900 as not a leap year.

**FORMATTED_STRING** Instructs date, time, datetime, and duration fields to be output as strings in their given number 

return **`Boolean`** type status indicates the success of the operation.

For ref doc go to https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets.values/batchUpdate

```cpp
bool batchUpdate(FirebaseJson *response, <string> spreadsheetId, FirebaseJsonArray *valueRangeArray, <string> valueInputOption = "USER_ENTERED", <string> includeValuesInResponse = "", <string> responseValueRenderOption = "", <string> responseDateTimeRenderOption = "");
```

```cpp
bool batchUpdate(String *response, <string> spreadsheetId, FirebaseJsonArray *valueRangeArray, <string> valueInputOption = "USER_ENTERED", <string> includeValuesInResponse = "", <string> responseValueRenderOption = "", <string> responseDateTimeRenderOption = "");
```


#### Sets values in one or more ranges of a spreadsheet.

param **`response`** (FirebaseJson or String) The returned response.

param **`spreadsheetId`** (string) The ID of the spreadsheet to update.

param **`DataFilterValueRangeArray`** (FirebaseJsonArray of DataFilterValueRange object) The data within a range of the spreadsheet.

param **`valueInputOption`** (enum string) How the input data should be interpreted. 

Note:

**INPUT_VALUE_OPTION_UNSPECIFIED** Default input value. This value must not be used.

**RAW** The values the user has entered will not be parsed and will be stored as-is.

**USER_ENTERED** The values will be parsed as if the user typed them into the UI. 

Numbers will stay as numbers, but strings may be converted to numbers, dates, etc. following the same rules that are applied when entering text into a cell via the Google Sheets UI.

param **`includeValuesInResponse`** (boolean string). Determines if the update response should include the values of the cells that were appended. 

Note: By default, responses do not include the updated values.

param **`responseValueRenderOption`** (enum string) Determines how values in the response should be rendered. 

Note:

The default render option is FORMATTED_VALUE.

**FORMATTED_VALUE** Values will be calculated & formatted in the reply according to the cell's formatting. 

Formatting is based on the spreadsheet's locale, not the requesting user's locale. 

For example, if A1 is 1.23 and A2 is =A1 and formatted as currency, then A2 would return "$1.23".

**UNFORMATTED_VALUE** Values will be calculated, but not formatted in the reply. 

For example, if A1 is 1.23 and A2 is =A1 and formatted as currency, then A2 would return the number 1.23.

**FORMULA** Values will not be calculated. The reply will include the formulas. 

For example, if A1 is 1.23 and A2 is =A1 and formatted as currency, then A2 would return "=A1".

param **`responseDateTimeRenderOption`** (enum string) Determines how dates, times, and durations in the response should be rendered. 

Note: This is ignored if responseValueRenderOption is FORMATTED_VALUE. 

The default dateTime render option is SERIAL_NUMBER.

**SERIAL_NUMBER** Instructs date, time, datetime, and duration fields to be output as doubles in "serial number" format, as popularized by Lotus 1-2-3. 

The whole number portion of the value (left of the decimal) counts the days since December 30th 1899. 

The fractional portion (right of the decimal) counts the time as a fraction of the day. 

For example, January 1st 1900 at noon would be 2.5, 2 because it's 2 days after December 30st 1899, and .5 because noon is half a day. 

February 1st 1900 at 3pm would be 33.625. This correctly treats the year 1900 as not a leap year.

**FORMATTED_STRING** Instructs date, time, datetime, and duration fields to be output as strings in their given number format (which is dependent on the spreadsheet locale).

return **`Boolean`** type status indicates the success of the operation.

For ref doc go to https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets.values/batchUpdateByDataFilter

```cpp
bool batchUpdateByDataFilter(FirebaseJson *response, <string> spreadsheetId, FirebaseJsonArray *DataFilterValueRangeArray, <string> valueInputOption = "USER_ENTERED", <string> includeValuesInResponse = "", <string> responseValueRenderOption = "", <string> responseDateTimeRenderOption = "");
```

```cpp
bool batchUpdateByDataFilter(String *response, <string> spreadsheetId, FirebaseJsonArray *DataFilterValueRangeArray, <string> valueInputOption = "USER_ENTERED", <string> includeValuesInResponse = "", <string> responseValueRenderOption = "", <string> responseDateTimeRenderOption = "");
```


#### Clears values from a spreadsheet. The caller must specify the spreadsheet ID and range. 

param **`response`** (FirebaseJson or String) The returned response.

param **`spreadsheetId`** (string) The ID of the spreadsheet to update. 

param **`range`** (string) The A1 notation or R1C1 notation of the values to clear.

return **`Boolean`** type status indicates the success of the operation.

For ref doc go to https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets.values/clear

```cpp
bool clear(FirebaseJson *response, <string> spreadsheetId, <string> range);
```

```cpp
bool clear(String *response, <string> spreadsheetId, <string> range);
```

#### Clears one or more ranges of values from a spreadsheet. 

param **`response`** (FirebaseJson or String) The returned response.

param **`spreadsheetId`** (string) The ID of the spreadsheet to update.

param **`ranges`** (string) The ranges to clear, in A1 or R1C1 notation. Ranges separated with comma ",".

return **`Boolean`** type status indicates the success of the operation.

For ref doc go to https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets.values/batchClear

```cpp
bool batchClear(FirebaseJson *response, <string> spreadsheetId, <string> ranges);
```

```cpp
bool batchClear(String *response, <string> spreadsheetId, <string> ranges);
```

#### Clears one or more ranges of values from a spreadsheet. 

param **`response`** (FirebaseJson or String) The returned response.

param **`spreadsheetId`** (string) The ID of the spreadsheet to update.

param **`dataFiltersArray`** (FirebaseJsonArray of DataFilter object) The DataFilters used to determine which ranges to clear.

return **`Boolean`** type status indicates the success of the operation.

For ref doc go to https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets.values/batchClearByDataFilter

```cpp
bool batchClearByDataFilter(FirebaseJson *response, <string> spreadsheetId, FirebaseJsonArray *dataFiltersArray);
```

```cpp
bool batchClearByDataFilter(String *response, <string> spreadsheetId, FirebaseJsonArray *dataFiltersArray);
```


### Spreadsheets.Sheets Member Functions

#### Copies a single sheet from a spreadsheet to another spreadsheet. 

param **`response`** (FirebaseJson or String) The returned response.

param **`spreadsheetId`** (string) The ID of the spreadsheet containing the sheet to copy.

param **`sheetId`** (integer) The ID of the sheet to copy.

param **`destinationSpreadsheetId`** (string) The ID of the spreadsheet to copy the sheet to.

return **`Boolean`** type status indicates the success of the operation.

Note: The sheet id is the integer number which you can get it from gid parameter of spreadsheet URL when select the sheet tab.

For example, https://docs.google.com/spreadsheets/d/xxxxxxx/edit#gid=1180731163

The sheet id of above case is 1180731163

For ref doc go to https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets.sheets/copyTo

```cpp
bool copyTo(FirebaseJson *response, <string> spreadsheetId, uint32_t sheetId, <string> destinationSpreadsheetId);
```

```cpp
bool copyTo(String *response, <string> spreadsheetId, uint32_t sheetId, <string> destinationSpreadsheetId);
```


### Spreadsheets.DeveloperMetadata Member Functions

#### Get the developer metadata with the specified ID. 

param **`response`** (FirebaseJson or String) The returned response.

param **`spreadsheetId`** (string) The ID of the spreadsheet to retrieve metadata from.

return **`Boolean`** type status indicates the success of the operation.

For ref doc go to https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets.developerMetadata/get

```cpp
bool get(FirebaseJson *response, <string> spreadsheetId, uint32_t metadataId);
```

```cpp
bool get(String *response, <string> spreadsheetId, uint32_t metadataId);
```

#### Get all developer metadata matching the specified DataFilter.

param **`response`** (FirebaseJson or String) The returned response.

param **`spreadsheetId`** (string) The ID of the spreadsheet to retrieve metadata from. 

param **`dataFiltersArray`** (FirebaseJsonArray of DataFilter object) The data filters describing the criteria used to determine which DeveloperMetadata entries to return. 
     
DeveloperMetadata matching any of the specified filters are included in the response.

return **`Boolean`** type status indicates the success of the operation.

For ref doc go to https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets.developerMetadata/search

```cpp
bool search(FirebaseJson *response, <string> spreadsheetId, FirebaseJsonArray *dataFiltersArray);
```

```cpp
bool search(String *response, <string> spreadsheetId, FirebaseJsonArray *dataFiltersArray);
```


## License

The MIT License (MIT)

Copyright (C) 2023 K. Suwatchai (Mobizt)


Permission is hereby granted, free of charge, to any person returning a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.



