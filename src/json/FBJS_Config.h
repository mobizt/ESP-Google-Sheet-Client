#ifndef FirebaseJSON_CONFIG_H
#define FirebaseJSON_CONFIG_H
#include <Arduino.h>

/** Use PSRAM for supported ESP32/ESP8266 module */
#if defined(ESP32) || defined(ESP8266)
#define FIREBASEJSON_USE_PSRAM
#endif

#endif