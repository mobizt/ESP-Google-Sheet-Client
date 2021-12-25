#ifndef ESP_GOOGLE_SHEET_CLIENT_FS_CONFIG_H_
#define ESP_GOOGLE_SHEET_CLIENT_FS_CONFIG_H_

/**
 * To use other flash file systems
 * 
 * LittleFS File system
 * 
 * #include <LittleFS.h>
 * #define DEFAULT_FLASH_FS LittleFS //For LitteFS
 * 
 * 
 * FAT File system
 * 
 * #include <FFat.h>
 * #define DEFAULT_FLASH_FS FFat  //For ESP32 FAT
 * 
*/
#ifndef DEFAULT_FLASH_FS
#define DEFAULT_FLASH_FS SPIFFS
#endif
/**
 * To use SD card file systems with different hardware interface
 * e.g. SDMMC hardware bus on the ESP32
 * https://github.com/espressif/arduino-esp32/tree/master/libraries/SD#faq
 * 
 #include <SD_MMC.h>
 #define DEFAULT_SD_FS SD_MMC //For ESP32 SDMMC
 #define CARD_TYPE_SD_MMC 1 //For ESP32 SDMMC
 * 
*/
#ifndef DEFAULT_SD_FS
#include <SD.h>
#define DEFAULT_SD_FS SD
#define CARD_TYPE_SD 1
#endif

//For ESP32, format SPIFFS or FFat if mounting failed
#ifndef FORMAT_FLASH_IF_MOUNT_FAILED
#define FORMAT_FLASH_IF_MOUNT_FAILED 1
#endif
/** Use PSRAM for supported ESP32/ESP8266 module */
#if defined(ESP32) || defined(ESP8266)
#define ESP_GOOGLE_SHEET_CLIENT_USE_PSRAM
#endif

#if !defined(ESP_SIGNER_USE_PSRAM) && defined(ESP_GOOGLE_SHEET_CLIENT_USE_PSRAM)
#define ESP_SIGNER_USE_PSRAM ESP_GOOGLE_SHEET_CLIENT_USE_PSRAM
#endif

#endif