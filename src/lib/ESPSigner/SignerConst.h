
/**
 * Created April 23, 2022
 *
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

#ifndef FB_COMMON_H_
#define FB_COMMON_H_

#include <Arduino.h>
#include <SPI.h>
#include <time.h>
#include <vector>
#include <functional>

#include <FS.h>
#include "./json/FirebaseJson.h"

#if defined(ESP32)
#include <WiFi.h>
#include "./wcs/esp32/ESP_Signer_TCP_Client.h"
#elif defined(ESP8266)
#include <Schedule.h>
#include <ets_sys.h>
#include <ESP8266WiFi.h>
#include "./wcs/esp8266/ESP_Signer_TCP_Client.h"
#define FS_NO_GLOBALS
#endif

#define SD_CS_PIN 15
#define MAX_REDIRECT 5
#define WIFI_RECONNECT_TIMEOUT 10000
#define MAX_EXCHANGE_TOKEN_ATTEMPTS 5
#define ESP_DEFAULT_TS 1618971013
#define ESP_SIGNER_DEFAULT_RESPONSE_BUFFER_SIZE 2560

enum esp_signer_mem_storage_type
{
    esp_signer_mem_storage_type_undefined,
    esp_signer_mem_storage_type_flash,
    esp_signer_mem_storage_type_sd
};

enum esp_signer_auth_token_status
{
    esp_signer_token_status_uninitialized,
    esp_signer_token_status_on_initialize,
    esp_signer_token_status_on_signing,
    esp_signer_token_status_on_request,
    esp_signer_token_status_on_refresh,
    esp_signer_token_status_ready,
    esp_signer_token_status_error
};

enum esp_signer_auth_token_type
{
    esp_signer_token_type_undefined,
    esp_signer_token_type_oauth2_access_token
};

enum esp_signer_jwt_generation_step
{
    esp_signer_jwt_generation_step_begin,
    esp_signer_jwt_generation_step_encode_header_payload,
    esp_signer_jwt_generation_step_sign,
    esp_signer_jwt_generation_step_exchange
};

typedef struct esp_signer_spi_ethernet_module_t
{
#if defined(ESP8266) && defined(ESP8266_CORE_SDK_V3_X_X)
#ifdef INC_ENC28J60_LWIP
    ENC28J60lwIP *enc28j60;
#endif
#ifdef INC_W5100_LWIP
    Wiznet5100lwIP *w5100;
#endif
#ifdef INC_W5500_LWIP
    Wiznet5500lwIP *w5500;
#endif
#endif
} SPI_ETH_Module;

struct esp_signer_url_info_t
{
    MB_String host;
    MB_String uri;
};

struct esp_signer_server_response_data_t
{
    int httpCode = -1;
    int payloadLen = -1;
    int contentLen = -1;
    int chunkRange = 0;
    int payloadOfs = 0;
    bool noContent = false;
    bool isChunkedEnc = false;
    MB_String location = "";
    MB_String contentType = "";
    MB_String connection = "";
    MB_String transferEnc = "";
};

struct esp_signer_auth_token_error_t
{
    MB_String message;
    int code = 0;
};

struct esp_signer_auth_token_info_t
{
    MB_String access_token;
    MB_String auth_type;
    MB_String jwt;
    MB_String scope;
    unsigned long expires = 0;
    unsigned long last_millis = 0;
    esp_signer_auth_token_type token_type = esp_signer_token_type_undefined;
    esp_signer_auth_token_status status = esp_signer_token_status_uninitialized;
    struct esp_signer_auth_token_error_t error;
};

struct esp_signer_service_account_data_info_t
{
    MB_String client_email;
    MB_String client_id;
    MB_String project_id;
    MB_String private_key_id;
    const char *private_key = "";
};

struct esp_signer_auth_signin_user_t
{
    MB_String email;
    MB_String password;
};

struct esp_signer_auth_cert_t
{
    const char *data = "";
    MB_String file;
    esp_signer_mem_storage_type file_storage = esp_signer_mem_storage_type_flash;
};

struct esp_signer_service_account_file_info_t
{
    MB_String path;
    esp_signer_mem_storage_type storage_type = esp_signer_mem_storage_type_flash;
};

struct esp_signer_service_account_t
{
    struct esp_signer_service_account_data_info_t data;
    struct esp_signer_service_account_file_info_t json;
};

struct esp_signer_token_signer_resources_t
{
    int step = 0;
    bool tokenTaskRunning = false;
    unsigned long lastReqMillis = 0;
    unsigned long preRefreshSeconds = 60;
    unsigned long expiredSeconds = 3600;
    unsigned long reqTO = 2000;
    MB_String pk;
    size_t hashSize = 32; // SHA256 size (256 bits or 32 bytes)
    size_t signatureSize = 256;
#if defined(ESP32)
    uint8_t *hash = nullptr;
#elif defined(ESP8266)
    char *hash = nullptr;
#endif
    unsigned char *signature = nullptr;
    MB_String encHeader;
    MB_String encPayload;
    MB_String encHeadPayload;
    MB_String encSignature;
#if defined(ESP32)
    mbedtls_pk_context *pk_ctx = nullptr;
    mbedtls_entropy_context *entropy_ctx = nullptr;
    mbedtls_ctr_drbg_context *ctr_drbg_ctx = nullptr;
    ESP_SIGNER_TCP_Client *wcs = nullptr;
#elif defined(ESP8266)
    ESP_SIGNER_TCP_Client *wcs = nullptr;
#endif
    FirebaseJson *json = nullptr;
    FirebaseJsonData *result = nullptr;
    struct esp_signer_auth_token_info_t tokens;
};

struct esp_signer_cfg_int_t
{
    struct esp_signer_sd_config_info_t sd_config;
    fs::File esp_signer_file;
    bool esp_signer_sd_rdy = false;
    bool esp_signer_flash_rdy = false;
    bool esp_signer_sd_used = false;
    bool esp_signer_reconnect_wifi = false;
    unsigned long esp_signer_last_reconnect_millis = 0;
    unsigned long esp_signer_last_jwt_begin_step_millis = 0;
    uint16_t esp_signer_reconnect_tmo = WIFI_RECONNECT_TIMEOUT;
    bool esp_signer_clock_rdy = false;
    bool esp_signer_clock_synched = false;
    float esp_signer_gmt_offset = 0;
    const char *esp_signer_caCert = nullptr;
    bool esp_signer_processing = false;
    unsigned long esp_signer_last_jwt_generation_error_cb_millis = 0;
    unsigned long esp_signer_last_time_sync_millis = 0;
    unsigned long esp_signer_last_ntp_sync_timeout_millis = 0;

#if defined(ESP32)
    TaskHandle_t token_processing_task_handle = NULL;
#endif
};

struct esp_signer_client_timeout_t
{
    // WiFi reconnect timeout (interval) in ms (10 sec - 5 min) when WiFi disconnected.
    uint16_t wifiReconnect = 10 * 1000;

    // Socket connection and ssl handshake timeout in ms (1 sec - 1 min).
    unsigned long socketConnection = 10 * 1000;

    // unused.
    unsigned long sslHandshake = 0;

    // Server response read timeout in ms (1 sec - 1 min).
    unsigned long serverResponse = 10 * 1000;

    uint16_t tokenGenerationBeginStep = 300;

    uint16_t tokenGenerationError = 5 * 1000;

    uint16_t ntpServerRequest = 15 * 1000;
};

typedef struct token_info_t
{
    esp_signer_auth_token_type type = esp_signer_token_type_undefined;
    esp_signer_auth_token_status status = esp_signer_token_status_uninitialized;
    struct esp_signer_auth_token_error_t error;
} TokenInfo;

typedef void (*TokenStatusCallback)(TokenInfo);

struct esp_signer_cfg_t
{
    struct esp_signer_service_account_t service_account;
    float time_zone = 0;
    struct esp_signer_auth_cert_t cert;
    struct esp_signer_token_signer_resources_t signer;
    struct esp_signer_cfg_int_t internal;
    TokenStatusCallback token_status_callback = NULL;
    int8_t max_token_generation_retry = MAX_EXCHANGE_TOKEN_ATTEMPTS;
    SPI_ETH_Module spi_ethernet_module;
    struct esp_signer_client_timeout_t timeout;
};

struct esp_signer_session_info_t
{
    bool buffer_ovf = false;
    bool chunked_encoding = false;
    bool connected = false;
    MB_String host = "";
    unsigned long last_conn_ms = 0;
    const uint32_t conn_timeout = 3 * 60 * 1000;

    uint16_t resp_size = 2048;
    int http_code = -1000;
    int content_length = 0;
    MB_String error = "";

#if defined(ESP8266)
    uint16_t bssl_rx_size = 512;
    uint16_t bssl_tx_size = 512;
#endif
};

typedef struct esp_signer_cfg_t SignerConfig;

typedef std::function<void(void)> esp_signer_callback_function_t;

// static const char esp_signer_pgm_str_1[] PROGMEM = "true";
// static const char esp_signer_pgm_str_2[] PROGMEM = "double";
static const char esp_signer_pgm_str_3[] PROGMEM = "Connection: ";
static const char esp_signer_pgm_str_4[] PROGMEM = "\r\n";
static const char esp_signer_pgm_str_5[] PROGMEM = "Content-Type: ";
static const char esp_signer_pgm_str_6[] PROGMEM = "Content-Length: ";
static const char esp_signer_pgm_str_7[] PROGMEM = "Transfer-Encoding: ";
static const char esp_signer_pgm_str_8[] PROGMEM = "chunked";
static const char esp_signer_pgm_str_9[] PROGMEM = "Location: ";
static const char esp_signer_pgm_str_10[] PROGMEM = ";";
static const char esp_signer_pgm_str_11[] PROGMEM = "0.0.0.0";
static const char esp_signer_pgm_str_12[] PROGMEM = "error_description";
static const char esp_signer_pgm_str_13[] PROGMEM = "type";
static const char esp_signer_pgm_str_14[] PROGMEM = "service_account";
static const char esp_signer_pgm_str_15[] PROGMEM = "project_id";
static const char esp_signer_pgm_str_16[] PROGMEM = "private_key_id";
static const char esp_signer_pgm_str_17[] PROGMEM = "private_key";
static const char esp_signer_pgm_str_18[] PROGMEM = "client_email";
static const char esp_signer_pgm_str_19[] PROGMEM = "client_id";
static const char esp_signer_pgm_str_20[] PROGMEM = "tokenProcessingTask";
static const char esp_signer_pgm_str_21[] PROGMEM = "system time was not set";
static const char esp_signer_pgm_str_22[] PROGMEM = "RSA private key parsing failed";
static const char esp_signer_pgm_str_23[] PROGMEM = "create message digest";
static const char esp_signer_pgm_str_24[] PROGMEM = "JWT token signing failed";
static const char esp_signer_pgm_str_25[] PROGMEM = "token exchange failed";
static const char esp_signer_pgm_str_26[] PROGMEM = "token is not ready";
static const char esp_signer_pgm_str_27[] PROGMEM = "max token generation retry reached";
static const char esp_signer_pgm_str_28[] PROGMEM = "not connected";
static const char esp_signer_pgm_str_29[] PROGMEM = "connection lost";
static const char esp_signer_pgm_str_30[] PROGMEM = "request timed out";
static const char esp_signer_pgm_str_31[] PROGMEM = "HTTP/1.1 ";
static const char esp_signer_pgm_str_32[] PROGMEM = " ";
static const char esp_signer_pgm_str_33[] PROGMEM = "alg";
static const char esp_signer_pgm_str_34[] PROGMEM = "RS256";
static const char esp_signer_pgm_str_35[] PROGMEM = "JWT";
static const char esp_signer_pgm_str_36[] PROGMEM = "typ";
static const char esp_signer_pgm_str_37[] PROGMEM = "iss";
static const char esp_signer_pgm_str_38[] PROGMEM = "sub";
static const char esp_signer_pgm_str_39[] PROGMEM = "aud";
static const char esp_signer_pgm_str_40[] PROGMEM = "https://";
static const char esp_signer_pgm_str_41[] PROGMEM = "oauth2";
static const char esp_signer_pgm_str_42[] PROGMEM = ".";
static const char esp_signer_pgm_str_43[] PROGMEM = "googleapis.com";
static const char esp_signer_pgm_str_44[] PROGMEM = "/";
static const char esp_signer_pgm_str_45[] PROGMEM = "token";
static const char esp_signer_pgm_str_46[] PROGMEM = "iat";
static const char esp_signer_pgm_str_47[] PROGMEM = "exp";
static const char esp_signer_pgm_str_48[] PROGMEM = "www";
static const char esp_signer_pgm_str_49[] PROGMEM = "undefined";
static const char esp_signer_pgm_str_50[] PROGMEM = "OAuth2.0 access token";
static const char esp_signer_pgm_str_51[] PROGMEM = "uninitialized";
static const char esp_signer_pgm_str_52[] PROGMEM = "on initializing";
static const char esp_signer_pgm_str_53[] PROGMEM = "on signing";
static const char esp_signer_pgm_str_54[] PROGMEM = "on exchange request";
static const char esp_signer_pgm_str_55[] PROGMEM = "on refreshing";
static const char esp_signer_pgm_str_56[] PROGMEM = "scope";
static const char esp_signer_pgm_str_57[] PROGMEM = "POST";
static const char esp_signer_pgm_str_58[] PROGMEM = "grant_type";
static const char esp_signer_pgm_str_59[] PROGMEM = "urn:ietf:params:oauth:grant-type:jwt-bearer";
static const char esp_signer_pgm_str_60[] PROGMEM = "assertion";
static const char esp_signer_pgm_str_61[] PROGMEM = " HTTP/1.1\r\n";
static const char esp_signer_pgm_str_62[] PROGMEM = "Host: ";
// static const char esp_signer_pgm_str_63[] PROGMEM = "\r\n";
static const char esp_signer_pgm_str_64[] PROGMEM = "User-Agent: ESP\r\n";
static const char esp_signer_pgm_str_65[] PROGMEM = "Content-Length: ";
static const char esp_signer_pgm_str_66[] PROGMEM = "Content-Type: ";
static const char esp_signer_pgm_str_67[] PROGMEM = "application/json";
static const char esp_signer_pgm_str_68[] PROGMEM = "error/code";
static const char esp_signer_pgm_str_69[] PROGMEM = "error/message";
static const char esp_signer_pgm_str_70[] PROGMEM = "access_token";
static const char esp_signer_pgm_str_71[] PROGMEM = "token_type";
static const char esp_signer_pgm_str_72[] PROGMEM = "expires_in";
static const char esp_signer_pgm_str_73[] PROGMEM = "connection refused";
static const char esp_signer_pgm_str_74[] PROGMEM = "send header failed";
static const char esp_signer_pgm_str_75[] PROGMEM = "send payload failed";
static const char esp_signer_pgm_str_76[] PROGMEM = "no HTTP server";
static const char esp_signer_pgm_str_77[] PROGMEM = "bad request";
static const char esp_signer_pgm_str_78[] PROGMEM = "non-authoriative information";
static const char esp_signer_pgm_str_79[] PROGMEM = "no content";
static const char esp_signer_pgm_str_80[] PROGMEM = "moved permanently";
static const char esp_signer_pgm_str_81[] PROGMEM = "use proxy";
static const char esp_signer_pgm_str_82[] PROGMEM = "temporary redirect";
static const char esp_signer_pgm_str_83[] PROGMEM = "permanent redirect";
static const char esp_signer_pgm_str_84[] PROGMEM = "unauthorized";
static const char esp_signer_pgm_str_85[] PROGMEM = "forbidden";
static const char esp_signer_pgm_str_86[] PROGMEM = "not found";
static const char esp_signer_pgm_str_87[] PROGMEM = "method not allow";
static const char esp_signer_pgm_str_88[] PROGMEM = "not acceptable";
static const char esp_signer_pgm_str_89[] PROGMEM = "proxy authentication required";
static const char esp_signer_pgm_str_90[] PROGMEM = "length required";
static const char esp_signer_pgm_str_91[] PROGMEM = "too many requests";
static const char esp_signer_pgm_str_92[] PROGMEM = "request header fields too larg";
static const char esp_signer_pgm_str_93[] PROGMEM = "internal server error";
static const char esp_signer_pgm_str_94[] PROGMEM = "bad gateway";
static const char esp_signer_pgm_str_95[] PROGMEM = "service unavailable";
static const char esp_signer_pgm_str_96[] PROGMEM = "gateway timeout";
static const char esp_signer_pgm_str_97[] PROGMEM = "http version not support";
static const char esp_signer_pgm_str_98[] PROGMEM = "network authentication required";
static const char esp_signer_pgm_str_99[] PROGMEM = "precondition failed";
static const char esp_signer_pgm_str_100[] PROGMEM = "read timed out";
static const char esp_signer_pgm_str_101[] PROGMEM = "http connection was used by other processes";
static const char esp_signer_pgm_str_102[] PROGMEM = "data buffer overflow";
static const char esp_signer_pgm_str_103[] PROGMEM = "payload too large";
static const char esp_signer_pgm_str_104[] PROGMEM = "File I/O error";
static const char esp_signer_pgm_str_105[] PROGMEM = "File not found";
static const char esp_signer_pgm_str_106[] PROGMEM = "Token generation was not initialized";
static const char esp_signer_pgm_str_107[] PROGMEM = "https://%[^/]/%s";
static const char esp_signer_pgm_str_108[] PROGMEM = "http://%[^/]/%s";
static const char esp_signer_pgm_str_109[] PROGMEM = "%[^/]/%s";
static const char esp_signer_pgm_str_110[] PROGMEM = "%[^?]?%s";
static const char esp_signer_pgm_str_111[] PROGMEM = "?";
static const char esp_signer_pgm_str_112[] PROGMEM = "ready";
static const char esp_signer_pgm_str_113[] PROGMEM = "error";
static const char esp_signer_pgm_str_114[] PROGMEM = "code: ";
static const char esp_signer_pgm_str_115[] PROGMEM = ", message: ";
static const char esp_signer_pgm_str_116[] PROGMEM = "NTP server time synching failed";

static const unsigned char esp_signer_base64_table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char esp_signer_boundary_table[] PROGMEM = "=_abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

#endif