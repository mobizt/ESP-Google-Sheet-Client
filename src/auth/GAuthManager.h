/**
 * Google Sheet Client, GAuthManager v1.0.2
 *
 * This library supports Espressif ESP8266, ESP32 and Raspberry Pi Pico MCUs.
 *
 * Created March 5, 2023
 *
 * The MIT License (MIT)
 * Copyright (c) 2022 K. Suwatchai (Mobizt)
 *
 *
 * Permission is hereby granted, free of charge, to any person returning a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef GAUTH_MANAGER_H
#define GAUTH_MANAGER_H
#include <Arduino.h>
#include "mbfs/MB_MCU.h"
#include "client/GS_TCP_Client.h"
#include <FS.h>
#include "mbfs/MB_FS.h"
#include "MB_NTP.h"
#include "GS_Const.h"

class GAuthManager
{
    friend class GSheetClass;
    friend class ESP_Google_Sheet_Client;
    friend class GSheet_Values;
    friend class GSheet_Sheets;
    friend class GSheet_Metadata;

public:
    GAuthManager();
    ~GAuthManager();

private:
    GS_TCP_Client *tcpClient = nullptr;
    bool localTCPClient = false;
    gauth_cfg_t *config = nullptr;
    MB_FS *mbfs = nullptr;
    uint32_t *mb_ts = nullptr;
    uint32_t *mb_ts_offset = nullptr;
    MB_NTP ntpClient;
    UDP *udp= nullptr;
    float gmtOffset = 0;
#if defined(ESP8266)
    callback_function_t esp8266_cb = nullptr;
#endif
    TokenInfo tokenInfo;
    bool _token_processing_task_enable = false;
    FirebaseJson *jsonPtr = nullptr;
    FirebaseJsonData *resultPtr = nullptr;
    int response_code = 0;
    time_t ts = 0;
    bool autoReconnectWiFi = true;
    unsigned long last_reconnect_millis = 0;
    uint16_t reconnect_tmo = 10 * 1000;

    /* intitialize the class */
    void begin(gauth_cfg_t *cfg, MB_FS *mbfs, uint32_t *mb_ts, uint32_t *mb_ts_offset);
    void end();
    void newClient(GS_TCP_Client **client);
    void freeClient(GS_TCP_Client **client);
    /* parse service account json file for private key */
    bool parseSAFile();
    /* clear service account credentials */
    void clearServiceAccountCreds();
    /* check for sevice account credentials */
    bool serviceAccountCredsReady();
    /* check for time is up or expiry time was reset or unset? */
    bool isExpired();
    /* Adjust the expiry time if system time synched or set. Adjust pre-refresh seconds to not exceed */
    void adjustTime(time_t &now);
    /* auth token was never been request or the last request was timed out */
    bool readyToRequest();
    /* is the time to refresh the token */
    bool readyToRefresh();
    /* is the time to sync clock */
    bool readyToSync();
    /* time synching timed out */
    bool isSyncTimeOut();
    /* error callback timed out */
    bool isErrorCBTimeOut();
    /* handle the auth tokens generation */
    bool handleToken();
    /* init the temp use Json objects */
    void initJson();
    /* free the temp use Json objects */
    void freeJson();
    /* exchane the auth token with the refresh token */
    bool refreshToken();
    /* set the token status by error code */
    void setTokenError(int code);
    /* handle the token processing task error */
    bool handleTaskError(int code, int httpCode = 0);
    // parse the auth token response
    bool handleResponse(GS_TCP_Client *client, int &httpCode, MB_String &payload, bool stopSession = true);
    /* process the tokens (generation, signing, request and refresh) */
    void tokenProcessingTask();
    bool checkUDP(UDP *udp, bool &ret, bool &_token_processing_task_enable, float gmtOffset);
    /* encode and sign the JWT token */
    bool createJWT();
    /* request or refresh the token */
    bool requestTokens(bool refresh);
    /* check the token ready status and process the token tasks */
    void checkToken();
    /* parse expiry time from string */
    void getExpiration(const char *exp);
    /* return error string from code */
    void errorToString(int httpCode, MB_String &buff);
    /* check the token ready status and process the token tasks and returns the status */
    bool tokenReady();
    /* error status callback */
    void sendTokenStatusCB();
    /* prepare or initialize the external/internal TCP client */
    bool initClient(PGM_P subDomain, gauth_auth_token_status status = token_status_uninitialized);
    /* get system time */
    time_t getTime();
    /* set the system time */
    bool setTime(time_t ts);
    /* set the WiFi (or network) auto reconnection option */
    void setAutoReconnectWiFi(bool reconnect);

    void setTokenType(gauth_auth_token_type type);
    String getTokenType(TokenInfo info);
    String getTokenType();
    String getTokenStatus(TokenInfo info);
    String getTokenStatus();
    String getTokenError(TokenInfo info);
    void reset();
    void refresh();
    String getTokenError();
    unsigned long getExpiredTimestamp();
    bool reconnect(GS_TCP_Client *client, unsigned long dataTime = 0);
    bool reconnect();

#if defined(ESP8266)
    void set_scheduled_callback(callback_function_t callback)
    {
        esp8266_cb = std::move([callback]()
                               { schedule_function(callback); });
        esp8266_cb();
    }
#endif

#if defined(MB_ARDUINO_PICO)
#if __has_include(<WiFiMulti.h>)
#define HAS_WIFIMULTI
    WiFiMulti *multi = nullptr;
#endif
#endif
};

#endif