
#ifndef GS_CONST_H
#define GS_CONST_H

#define GS_DEFAULT_AUTH_TOKEN_PRE_REFRESH_SECONDS 5 * 60

#define GS_DEFAULT_AUTH_TOKEN_EXPIRED_SECONDS 3600

#define GS_DEFAULT_REQUEST_TIMEOUT 2000

#define GS_DEFAULT_TS 1618971013

#define GS_TIME_SYNC_INTERVAL 5000

#define GS_MIN_TOKEN_GENERATION_ERROR_INTERVAL 5 * 1000

#define GS_MIN_NTP_SERVER_SYNC_TIME_OUT 15 * 1000

#define GS_MIN_TOKEN_GENERATION_BEGIN_STEP_INTERVAL 300

#define GS_MIN_SERVER_RESPONSE_TIMEOUT 1 * 1000
#define GS_DEFAULT_SERVER_RESPONSE_TIMEOUT 5 * 1000
#define GS_MAX_SERVER_RESPONSE_TIMEOUT 60 * 1000

#define GS_MIN_WIFI_RECONNECT_TIMEOUT 10 * 1000
#define GS_MAX_WIFI_RECONNECT_TIMEOUT 5 * 60 * 1000

#include <Arduino.h>
#include "mbfs/MB_MCU.h"

#if defined(ESP32) && !defined(ESP_ARDUINO_VERSION) /* ESP32 core < v2.0.x */
#include <sys/time.h>
#else
#include <time.h>
#endif

#include "ESP_Google_Sheet_Client_FS_Config.h"
#include "mbfs/MB_FS.h"
#include "auth/MB_NTP.h"
#if defined(ESP32)
#include "mbedtls/pk.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#endif

#if defined(ESP8266)

//__GNUC__
//__GNUC_MINOR__
//__GNUC_PATCHLEVEL__

#ifdef __GNUC__
#if __GNUC__ > 4 || __GNUC__ == 10
#include <string>
#define ESP8266_CORE_SDK_V3_X_X
#endif
#endif

#if __has_include(<core_esp8266_version.h>)
#include <core_esp8266_version.h>
#endif

#endif

#if defined __has_include

#if __has_include(<LwipIntfDev.h>)
#include <LwipIntfDev.h>
#endif

#if __has_include(<ENC28J60lwIP.h>) && defined(ENABLE_ESP8266_ENC28J60_ETH)
#define INC_ENC28J60_LWIP
#include <ENC28J60lwIP.h>
#define ESP8266_SPI_ETH_MODULE ENC28J60lwIP
#endif

#if __has_include(<W5100lwIP.h>) && defined(ENABLE_ESP8266_W5100_ETH)

#define INC_W5100_LWIP
// PIO compilation error
#include <W5100lwIP.h>
#define ESP8266_SPI_ETH_MODULE Wiznet5100lwIP
#endif

#if __has_include(<W5500lwIP.h>) && defined(ENABLE_ESP8266_W5500_ETH)
#define INC_W5500_LWIP
#include <W5500lwIP.h>
#define ESP8266_SPI_ETH_MODULE Wiznet5500lwIP
#endif

#endif

typedef enum
{
    esp_google_sheet_file_storage_type_undefined,
    esp_google_sheet_file_storage_type_flash,
    esp_google_sheet_file_storage_type_sd
} esp_google_sheet_file_storage_type;

typedef enum
{
    gs_tcp_client_type_undefined,
    gs_tcp_client_type_internal,
    gs_tcp_client_type_external

} gs_tcp_client_type;

typedef enum
{
    gs_cert_type_undefined = -1,
    gs_cert_type_none = 0,
    gs_cert_type_data,
    gs_cert_type_file

} gs_cert_type;

enum gauth_auth_token_status
{
    token_status_uninitialized,
    token_status_on_initialize,
    token_status_on_signing,
    token_status_on_request,
    token_status_on_refresh,
    token_status_ready,
    token_status_error,
    esp_signer_token_status_error = token_status_error
};

enum gauth_auth_token_type
{
    token_type_undefined,
    token_type_oauth2_access_token,
    token_type_refresh_token
};

enum gauth_jwt_generation_step
{
    gauth_jwt_generation_step_begin,
    gauth_jwt_generation_step_encode_header_payload,
    gauth_jwt_generation_step_sign,
    gauth_jwt_generation_step_exchange
};

enum gs_request_method
{
    http_undefined,
    http_put,
    http_post,
    http_get,
    http_patch,
    http_delete
};

struct gs_no_eth_module_t
{
};

#ifndef ESP8266_SPI_ETH_MODULE
#define ESP8266_SPI_ETH_MODULE gs_no_eth_module_t
#endif

struct gs_wifi_credential_t
{
    MB_String ssid;
    MB_String password;
};

class esp_gs_wifi
{
public:
    esp_gs_wifi() { clearAP(); };
    ~esp_gs_wifi() { credentials.clear(); };
    void addAP(const String &ssid, const String &password)
    {
        gs_wifi_credential_t data;
        data.ssid = ssid;
        data.password = password;
        credentials.push_back(data);
    }
    void clearAP()
    {
        credentials.clear();
    }
    size_t size() { return credentials.size(); }

    gs_wifi_credential_t operator[](size_t index)
    {
        return credentials[index];
    }

    private:
    MB_List<gs_wifi_credential_t> credentials;
};

struct gs_url_info_t
{
    MB_String host;
    MB_String uri;
    MB_String auth;
};

struct gauth_service_account_data_info_t
{
    MB_String client_email;
    MB_String client_id;
    MB_String project_id;
    MB_String private_key_id;
    const char *private_key = "";
};

struct gauth_service_account_file_info_t
{
    MB_String path;
    mb_fs_mem_storage_type storage_type = mb_fs_mem_storage_type_flash;
};

struct gauth_service_account_t
{
    struct gauth_service_account_data_info_t data;
    struct gauth_service_account_file_info_t json;
};

struct gauth_auth_token_error_t
{
    MB_String message;
    int code = 0;
};

struct gauth_auth_token_info_t
{
    MB_String auth_type;
    MB_String jwt;
    unsigned long expires = 0;
    /* milliseconds count when last expiry time was set */
    unsigned long last_millis = 0;
    gauth_auth_token_type token_type = token_type_undefined;
    gauth_auth_token_status status = token_status_uninitialized;
    struct gauth_auth_token_error_t error;
};

typedef struct gauth_token_info_t
{
    gauth_auth_token_type type = token_type_undefined;
    gauth_auth_token_status status = token_status_uninitialized;
    struct gauth_auth_token_error_t error;
} TokenInfo;

struct gauth_token_signer_resources_t
{
    int step = 0;
    bool tokenTaskRunning = false;
    /* last token request milliseconds count */
    unsigned long lastReqMillis = 0;
    unsigned long preRefreshSeconds = GS_DEFAULT_AUTH_TOKEN_PRE_REFRESH_SECONDS;
    unsigned long expiredSeconds = GS_DEFAULT_AUTH_TOKEN_EXPIRED_SECONDS;
    /* request time out period (interval) */
    unsigned long reqTO = GS_DEFAULT_REQUEST_TIMEOUT;
    MB_String customHeaders;
    MB_String pk;
    size_t hashSize = 32; // SHA256 size (256 bits or 32 bytes)
    size_t signatureSize = 256;
#if defined(ESP32)
    uint8_t *hash = nullptr;
#elif defined(ESP8266) || defined(MB_ARDUINO_PICO)
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
#endif
    gauth_auth_token_info_t tokens;
};

typedef void (*TokenStatusCallback)(TokenInfo);

struct gs_chunk_state_info
{
    int state = 0;
    int chunkedSize = 0;
    int dataLen = 0;
};

struct gs_tcp_response_handler_t
{
    // the chunk index of all data that is being process
    int chunkIdx = 0;
    // the payload chunk index that is being process
    int pChunkIdx = 0;
    // the total bytes of http response payload to read
    int payloadLen = 0;
    // the total bytes of base64 decoded data from response payload
    int decodedPayloadLen = 0;
    // the current size of chunk data to read from client
    int chunkBufSize = 0;
    // the amount of http response payload that read,
    // compare with the content length header value for finishing check
    int payloadRead = 0;
    // status showed that the http headers was found and is being read
    bool isHeader = false;
    // status showed that the http headers was completely read
    bool headerEnded = false;
    // the prefered size of chunk data to read from client
    size_t defaultChunkSize = 0;
    // keep the auth token generation error
    struct gauth_auth_token_error_t error;
    // keep the http header or the first line of stream event data
    MB_String header;
    // time out checking for execution
    unsigned long dataTime = 0;
    // pointer to payload
    MB_String *payload = nullptr;
    // data is already in receive buffer (must be int)
    int bufferAvailable = 0;
    // data in receive buffer is base64 file data
    bool isBase64File = false;
    // the base64 encoded string downloaded anount
    int downloadByteLen = 0;
    // pad (=) length checking from tail of encoded string of file/blob data
    int base64PadLenTail = 0;
    // pad (=) length checking from base64 encoded string signature (begins with "file,base64, and "blob,base64,)
    // of file/blob data
    int base64PadLenSignature = 0;
    // the tcp client pointer
    Client *client = nullptr;
    // the chunk state info
    gs_chunk_state_info chunkState;

public:
    int available()
    {
        if (client)
            return client->available();
        return false;
    }
};

struct gs_server_response_data_t
{
    int httpCode = 0;
    // Must not be negative
    int payloadLen = 0;
    // The response content length, must not be negative as it uses to determine
    // the available data to read in event-stream
    // and content length specific read in http response
    int contentLen = 0;
    int chunkRange = 0;
    bool redirect = false;
    bool isChunkedEnc = false;
    bool noContent = false;
    MB_String location;
    MB_String contentType;
    MB_String connection;
    MB_String eventPath;
    MB_String eventType;
    MB_String eventData;
    MB_String etag;
    MB_String pushName;
    MB_String fbError;
    MB_String transferEnc;
};

template <typename T>
struct gs_base64_io_t
{
    // the total bytes of data in output buffer
    int bufWrite = 0;
    // the size of output buffer
    size_t bufLen = 1024;
    // for file, the type of filesystem to write
    mbfs_file_type filetype = mb_fs_mem_storage_type_undefined;
    // for T array
    T *outT = nullptr;
    // for T vector
    MB_VECTOR<T> *outL = nullptr;
    // for client
    Client *outC = nullptr;
};

struct gauth_auth_cert_t
{
    const char *data = "";
    MB_String file;
    mb_fs_mem_storage_type file_storage = mb_fs_mem_storage_type_flash;
};

struct gauth_cfg_int_t
{
    bool processing = false;
    bool rtoken_requested = false;

    bool reconnect_wifi = false;
    unsigned long last_reconnect_millis = 0;
    unsigned long last_jwt_begin_step_millis = 0;
    unsigned long last_jwt_generation_error_cb_millis = 0;
    unsigned long last_request_token_cb_millis = 0;
    unsigned long last_stream_timeout_cb_millis = 0;
    unsigned long last_time_sync_millis = 0;
    unsigned long last_ntp_sync_timeout_millis = 0;
    bool clock_rdy = false;
    uint16_t email_crc = 0, password_crc = 0, client_email_crc = 0, project_id_crc = 0, priv_key_crc = 0;

    /* flag set when NTP time server synching has been started */
    bool clock_synched = false;
    float gmt_offset = 0;
    bool auth_uri = false;

    MB_String auth_token;
    MB_String refresh_token;
    MB_String client_id;
    MB_String client_secret;
};

struct gauth_client_timeout_t
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

typedef struct gauth_spi_ethernet_module_t
{
#if defined(ESP8266) && defined(ESP8266_CORE_SDK_V3_X_X)
#ifdef INC_ENC28J60_LWIP
    ENC28J60lwIP *enc28j60 = nullptr;
#endif
#ifdef INC_W5100_LWIP
    Wiznet5100lwIP *w5100 = nullptr;
#endif
#ifdef INC_W5500_LWIP
    Wiznet5500lwIP *w5500 = nullptr;
#endif
#elif defined(INC_CYW43_LWIP)

#endif
} SPI_ETH_Module;

struct gauth_cfg_t
{
    uint32_t mb_ts = 0;

    struct gauth_service_account_t service_account;
    float time_zone = 0;
    struct gauth_auth_cert_t cert;
    struct gauth_token_signer_resources_t signer;
    struct gauth_cfg_int_t internal;
    TokenStatusCallback token_status_callback = NULL;
    gauth_spi_ethernet_module_t spi_ethernet_module;
    struct gauth_client_timeout_t timeout;

    MB_String api_key;
    MB_String client_id;
    MB_String client_secret;

    gauth_token_info_t tokenInfo;
    gauth_auth_token_error_t error;

    struct esp_gs_wifi wifi;
};

#if !defined(__AVR__)
typedef std::function<void(void)> callback_function_t;
#endif

typedef void (*GS_NetworkConnectionRequestCallback)(void);
typedef void (*GS_NetworkStatusRequestCallback)(void);
typedef void (*GS_ResponseCallback)(const char *);

static const unsigned char gs_base64_table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const char gauth_pgm_str_1[] PROGMEM = "type";
static const char gauth_pgm_str_2[] PROGMEM = "service_account";
static const char gauth_pgm_str_3[] PROGMEM = "project_id";
static const char gauth_pgm_str_4[] PROGMEM = "private_key_id";
static const char gauth_pgm_str_5[] PROGMEM = "private_key";
static const char gauth_pgm_str_6[] PROGMEM = "client_email";
static const char gauth_pgm_str_7[] PROGMEM = "client_id";
static const char gauth_pgm_str_8[] PROGMEM = "securetoken";
static const char gauth_pgm_str_9[] PROGMEM = "grantType";
static const char gauth_pgm_str_10[] PROGMEM = "refresh_token";
static const char gauth_pgm_str_11[] PROGMEM = "refreshToken";
static const char gauth_pgm_str_12[] PROGMEM = "/v1/token?Key=";
static const char gauth_pgm_str_13[] PROGMEM = "application/json";
static const char gauth_pgm_str_14[] PROGMEM = "error/code";
static const char gauth_pgm_str_15[] PROGMEM = "error/message";
static const char gauth_pgm_str_16[] PROGMEM = "id_token";
static const char gauth_pgm_str_18[] PROGMEM = "refresh_token";
static const char gauth_pgm_str_19[] PROGMEM = "expires_in";
static const char gauth_pgm_str_20[] PROGMEM = "alg";
static const char gauth_pgm_str_21[] PROGMEM = "RS256";
static const char gauth_pgm_str_22[] PROGMEM = "typ";
static const char gauth_pgm_str_23[] PROGMEM = "JWT";
static const char gauth_pgm_str_24[] PROGMEM = "iss";
static const char gauth_pgm_str_25[] PROGMEM = "sub";
static const char gauth_pgm_str_26[] PROGMEM = "https://";
static const char gauth_pgm_str_27[] PROGMEM = "oauth2";
static const char gauth_pgm_str_28[] PROGMEM = "/";
static const char gauth_pgm_str_29[] PROGMEM = "token";
static const char gauth_pgm_str_30[] PROGMEM = "aud";
static const char gauth_pgm_str_31[] PROGMEM = "iat";
static const char gauth_pgm_str_32[] PROGMEM = "exp";
static const char gauth_pgm_str_33[] PROGMEM = "scope";
static const char gauth_pgm_str_34[] PROGMEM = "https://www.googleapis.com/auth/drive.metadata https://www.googleapis.com/auth/drive.appdata https://www.googleapis.com/auth/spreadsheets https://www.googleapis.com/auth/drive https://www.googleapis.com/auth/drive.file";
static const char gauth_pgm_str_35[] PROGMEM = ".";
static const char gauth_pgm_str_36[] PROGMEM = "www";
static const char gauth_pgm_str_37[] PROGMEM = "client_secret";
static const char gauth_pgm_str_38[] PROGMEM = "grant_type";
static const char gauth_pgm_str_39[] PROGMEM = "urn:ietf:params:oauth:grant-type:jwt-bearer";
static const char gauth_pgm_str_40[] PROGMEM = "assertion";
static const char gauth_pgm_str_41[] PROGMEM = "oauth2";
static const char gauth_pgm_str_42[] PROGMEM = "error";
static const char gauth_pgm_str_43[] PROGMEM = "error_description";
static const char gauth_pgm_str_44[] PROGMEM = "access_token";
static const char gauth_pgm_str_45[] PROGMEM = "Bearer ";

static const char gs_pgm_str_1[] PROGMEM = "\r\n";
static const char gs_pgm_str_2[] PROGMEM = ".";
static const char gs_pgm_str_3[] PROGMEM = "googleapis.com";
static const char gs_pgm_str_4[] PROGMEM = "Host: ";
static const char gs_pgm_str_5[] PROGMEM = "Content-Type: ";
static const char gs_pgm_str_6[] PROGMEM = "Content-Length: ";
static const char gs_pgm_str_7[] PROGMEM = "User-Agent: ESP\r\n";
static const char gs_pgm_str_8[] PROGMEM = "Connection: keep-alive\r\n";
static const char gs_pgm_str_9[] PROGMEM = "Connection: close\r\n";
static const char gs_pgm_str_10[] PROGMEM = "GET";
static const char gs_pgm_str_11[] PROGMEM = "POST";
static const char gs_pgm_str_12[] PROGMEM = "PATCH";
static const char gs_pgm_str_13[] PROGMEM = "DELETE";
static const char gs_pgm_str_14[] PROGMEM = "PUT";
static const char gs_pgm_str_15[] PROGMEM = " ";
static const char gs_pgm_str_16[] PROGMEM = " HTTP/1.1\r\n";
static const char gs_pgm_str_17[] PROGMEM = "Authorization: ";
static const char gs_pgm_str_18[] PROGMEM = "Bearer ";
static const char gs_pgm_str_19[] PROGMEM = "true";
static const char gs_pgm_str_20[] PROGMEM = "Connection: ";
static const char gs_pgm_str_21[] PROGMEM = "Content-Type: ";
static const char gs_pgm_str_22[] PROGMEM = "Content-Length: ";
static const char gs_pgm_str_23[] PROGMEM = "ETag: ";
static const char gs_pgm_str_24[] PROGMEM = "Transfer-Encoding: ";
static const char gs_pgm_str_25[] PROGMEM = "chunked";
static const char gs_pgm_str_26[] PROGMEM = "Location: ";
static const char gs_pgm_str_27[] PROGMEM = "HTTP/1.1 ";
static const char gs_pgm_str_28[] PROGMEM = "?";
static const char gs_pgm_str_29[] PROGMEM = "&";
static const char gs_pgm_str_30[] PROGMEM = "=";
static const char gs_pgm_str_31[] PROGMEM = "/";
static const char gs_pgm_str_32[] PROGMEM = "https://";
static const char gs_pgm_str_33[] PROGMEM = "https://%[^/]/%s";
static const char gs_pgm_str_34[] PROGMEM = "http://%[^/]/%s";
static const char gs_pgm_str_35[] PROGMEM = "%[^/]/%s";
static const char gs_pgm_str_36[] PROGMEM = "%[^?]?%s";
static const char gs_pgm_str_37[] PROGMEM = "auth=";
static const char gs_pgm_str_38[] PROGMEM = "%[^&]";
static const char gs_pgm_str_39[] PROGMEM = "undefined";
static const char gs_pgm_str_40[] PROGMEM = "OAuth2.0 access token";
static const char gs_pgm_str_41[] PROGMEM = "uninitialized";
static const char gs_pgm_str_42[] PROGMEM = "on initializing";
static const char gs_pgm_str_43[] PROGMEM = "on signing";
static const char gs_pgm_str_44[] PROGMEM = "on exchange request";
static const char gs_pgm_str_45[] PROGMEM = "on refreshing";
static const char gs_pgm_str_46[] PROGMEM = "error";
static const char gs_pgm_str_47[] PROGMEM = "code: ";
static const char gs_pgm_str_48[] PROGMEM = ", message: ";
static const char gs_pgm_str_49[] PROGMEM = "ready";

#endif