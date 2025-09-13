/*
 * SPDX-FileCopyrightText: 2025 Suwatchai K. <suwatchai@outlook.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef GAUTH_MANAGER_CPP
#define GAUTH_MANAGER_CPP

#include <Arduino.h>
#include "mbfs/MB_MCU.h"
#include "GAuthManager.h"

GAuthManager::GAuthManager()
{
}

GAuthManager::~GAuthManager()
{
    end();
}

void GAuthManager::begin(esp_google_sheet_auth_cfg_t *cfg, MB_FS *mbfs, uint32_t *mb_ts, uint32_t *mb_ts_offset)
{
    this->config = cfg;
    this->mbfs = mbfs;
    this->mb_ts = mb_ts;
    this->mb_ts_offset = mb_ts_offset;
}

void GAuthManager::end()
{
    freeJson();
#if defined(ESP_GOOGLE_SHEET_CLIENT_HAS_WIFIMULTI)
    if (multi)
        delete multi;
    multi = nullptr;
#endif
    if (tcpClient)
        freeClient(&tcpClient);
}

void GAuthManager::newClient(GS_TCP_Client **client)
{

    freeClient(client);
    if (!*client)
    {
        *client = new GS_TCP_Client();

        if (_cli_type == esp_google_sheet_client_type_external_basic_client)
            (*client)->setClient(_cli, _net_con_cb, _net_stat_cb);
        else if (_cli_type == esp_google_sheet_client_type_external_gsm_client)
        {
#if defined(ESP_GOOGLE_SHEET_CLIENT_GSM_MODEM_IS_AVAILABLE)
            (*client)->setGSMClient(_cli, _modem, _pin.c_str(), _apn.c_str(), _user.c_str(), _password.c_str());
#endif
        }
        else
            (*client)->_client_type = _cli_type;
    }
}

void GAuthManager::freeClient(GS_TCP_Client **client)
{
    if (*client)
    {
        _cli_type = (*client)->type();
        _cli = (*client)->_basic_client;
        if (_cli_type == esp_google_sheet_client_type_external_basic_client)
        {
            _net_con_cb = (*client)->_network_connection_cb;
            _net_stat_cb = (*client)->_network_status_cb;
        }
        else if (_cli_type == esp_google_sheet_client_type_external_gsm_client)
        {
#if defined(ESP_GOOGLE_SHEET_CLIENT_GSM_MODEM_IS_AVAILABLE)
            _pin = (*client)->_pin;
            _apn = (*client)->_apn;
            _user = (*client)->_user;
            _password = (*client)->_password;
            _modem = (*client)->_modem;
#endif
        }
        delete *client;
    }
    *client = nullptr;
}

bool GAuthManager::parseSAFile()
{
    if (config->signer.pk.length() > 0)
        return false;

    int res = mbfs->open(config->service_account.json.path,
                         mbfs_type config->service_account.json.storage_type,
                         mb_fs_open_mode_read);

    if (res >= 0)
    {
        clearServiceAccountCreds();
        initJson();

        size_t len = res;
        char *buf = MemoryHelper::createBuffer<char *>(mbfs, len + 10);
        if (mbfs->available(mbfs_type config->service_account.json.storage_type))
        {
            if ((int)len == mbfs->read(mbfs_type config->service_account.json.storage_type, (uint8_t *)buf, len))
                jsonPtr->setJsonData(buf);
        }

        mbfs->close(mbfs_type config->service_account.json.storage_type);

        if (JsonHelper::parse(jsonPtr, resultPtr, gauth_pgm_str_1)) // type
        {
            if (resultPtr->to<MB_String>().find(pgm2Str(gauth_pgm_str_2 /* service_account */), 0) != MB_String::npos)
            {
                if (JsonHelper::parse(jsonPtr, resultPtr, gauth_pgm_str_3)) // project_id
                    config->service_account.data.project_id = resultPtr->to<const char *>();

                if (JsonHelper::parse(jsonPtr, resultPtr, gauth_pgm_str_4)) // private_key_id
                    config->service_account.data.private_key_id = resultPtr->to<const char *>();

                if (JsonHelper::parse(jsonPtr, resultPtr, gauth_pgm_str_5)) // private_key
                {
                    char *temp = MemoryHelper::createBuffer<char *>(mbfs, strlen(resultPtr->to<const char *>()));
                    size_t c = 0;
                    for (size_t i = 0; i < strlen(resultPtr->to<const char *>()); i++)
                    {
                        if (resultPtr->to<const char *>()[i] == '\\')
                        {
                            Utils::idle();
                            temp[c++] = '\n';
                            i++;
                        }
                        else
                            temp[c++] = resultPtr->to<const char *>()[i];
                    }
                    config->signer.pk = temp;
                    resultPtr->clear();
                    MemoryHelper::freeBuffer(mbfs, temp);
                }

                if (JsonHelper::parse(jsonPtr, resultPtr, gauth_pgm_str_6)) // client_email
                    config->service_account.data.client_email = resultPtr->to<const char *>();
                if (JsonHelper::parse(jsonPtr, resultPtr, gauth_pgm_str_7)) // client_id
                    config->service_account.data.client_id = resultPtr->to<const char *>();

                freeJson();

                MemoryHelper::freeBuffer(mbfs, buf);

                return true;
            }
        }

        freeJson();

        MemoryHelper::freeBuffer(mbfs, buf);
    }

    return false;
}

void GAuthManager::clearServiceAccountCreds()
{
    config->service_account.data.private_key = "";
    config->service_account.data.project_id.clear();
    config->service_account.data.private_key_id.clear();
    config->service_account.data.client_email.clear();
    config->signer.pk.clear();
}

bool GAuthManager::serviceAccountCredsReady()
{
    return (strlen_P(config->service_account.data.private_key) > 0 || config->signer.pk.length() > 0) &&
           config->service_account.data.client_email.length() > 0 &&
           config->service_account.data.project_id.length() > 0;
}

void GAuthManager::setTokenType(gauth_auth_token_type type)
{
    if (!config)
        return;
    config->signer.tokens.token_type = type;
}

time_t GAuthManager::getTime()
{
    return TimeHelper::getTime(mb_ts, mb_ts_offset);
}

bool GAuthManager::setTime(time_t ts)
{

#if defined(ESP8266) || defined(ESP32) || defined(MB_ARDUINO_PICO)

    if (TimeHelper::setTimestamp(ts, mb_ts_offset) == 0)
    {
        this->ts = time(nullptr);
        *mb_ts = this->ts;
        return true;
    }
    else
    {
        this->ts = time(nullptr);
        *mb_ts = this->ts;
    }

#else
    if (ts > ESP_GOOGLE_SHEET_CLIENT_DEFAULT_TS)
    {
        *mb_ts_offset = ts - millis() / 1000;
        this->ts = ts;
        *mb_ts = this->ts;
    }
#endif

    return false;
}

bool GAuthManager::isExpired()
{
    if (!config)
        return false;

    time_t now = 0;

    // adjust the expiry time when needed
    adjustTime(now);

    // time is up or expiry time was reset or unset?
    return (now > (int)(config->signer.tokens.expires - config->signer.preRefreshSeconds) || config->signer.tokens.expires == 0);
}

void GAuthManager::adjustTime(time_t &now)
{
    now = getTime(); // returns timestamp (synched) or millis/1000 (unsynched)

    // if time has changed (synched or manually set) after token has been generated, update its expiration
    if (config->signer.tokens.expires > 0 && config->signer.tokens.expires < ESP_GOOGLE_SHEET_CLIENT_DEFAULT_TS && now > ESP_GOOGLE_SHEET_CLIENT_DEFAULT_TS)
        /* new expiry time (timestamp) = current timestamp - total seconds since last token request - 60 */
        config->signer.tokens.expires += now - (millis() - config->signer.tokens.last_millis) / 1000 - 60;

    // pre-refresh seconds should not greater than the expiry time
    if (config->signer.preRefreshSeconds > config->signer.tokens.expires && config->signer.tokens.expires > 0)
        config->signer.preRefreshSeconds = 60;
}

bool GAuthManager::readyToRequest()
{
    bool ret = false;
    // To detain the next request using lat request millis
    if (config && (millis() - config->signer.lastReqMillis > config->signer.reqTO || config->signer.lastReqMillis == 0))
    {
        config->signer.lastReqMillis = millis();
        ret = true;
    }

    return ret;
}

bool GAuthManager::readyToRefresh()
{
    if (!config)
        return false;
    // To detain the next request using lat request millis
    return millis() - config->internal.last_request_token_cb_millis > 5000;
}

bool GAuthManager::readyToSync()
{
    bool ret = false;
    // To detain the next synching using lat synching millis
    if (config && millis() - config->internal.last_time_sync_millis > ESP_GOOGLE_SHEET_CLIENT_TIME_SYNC_INTERVAL)
    {
        config->internal.last_time_sync_millis = millis();
        ret = true;
    }

    return ret;
}

bool GAuthManager::isSyncTimeOut()
{
    bool ret = false;
    // If device time was not synched in time
    if (config && millis() - config->internal.last_ntp_sync_timeout_millis > config->timeout.ntpServerRequest)
    {
        config->internal.last_ntp_sync_timeout_millis = millis();
        ret = true;
    }

    return ret;
}

bool GAuthManager::isErrorCBTimeOut()
{
    bool ret = false;
    // To detain the next error callback
    if (config &&
        (millis() - config->internal.last_jwt_generation_error_cb_millis > config->timeout.tokenGenerationError ||
         config->internal.last_jwt_generation_error_cb_millis == 0))
    {
        config->internal.last_jwt_generation_error_cb_millis = millis();
        ret = true;
    }

    return ret;
}

bool GAuthManager::handleToken()
{

    if (!config)
        return false;

    // time is up or expiey time reset or unset
    bool exp = isExpired();

    // Handle user assigned tokens (access tokens)

    // Handle the signed jwt token generation, request and refresh the token

    // If expiry time is up or reset/unset, start the process
    if (exp)
    {

        // Handle the jwt token processing

        // If it is the first step and no task is currently running
        if (!config->signer.tokenTaskRunning)
        {
            if (config->signer.step == gauth_jwt_generation_step_begin)
            {
                bool use_sa_key_file = false, valid_key_file = false;
                // If service account key json file assigned and no private key parsing data
                if (config->service_account.json.path.length() > 0 && config->signer.pk.length() == 0)
                {
                    use_sa_key_file = true;
                    // Parse the private key from service account json file
                    valid_key_file = parseSAFile();
                }

                // Check the SA creds
                if (!serviceAccountCredsReady())
                {
                    config->signer.tokens.status = token_status_error;
                    if (use_sa_key_file && !valid_key_file)
                    {
                        errorToString(ESP_GOOGLE_SHEET_CLIENT_ERROR_SERVICE_ACCOUNT_JSON_FILE_PARSING_ERROR, config->signer.tokens.error.message);
                        config->signer.tokens.error.code = ESP_GOOGLE_SHEET_CLIENT_ERROR_SERVICE_ACCOUNT_JSON_FILE_PARSING_ERROR;
                    }
                    else
                    {
                        errorToString(ESP_GOOGLE_SHEET_CLIENT_ERROR_MISSING_SERVICE_ACCOUNT_CREDENTIALS, config->signer.tokens.error.message);
                        config->signer.tokens.error.code = ESP_GOOGLE_SHEET_CLIENT_ERROR_MISSING_SERVICE_ACCOUNT_CREDENTIALS;
                    }
                    sendTokenStatusCB();
                    return false;
                }

                // If no token status set, set the states
                if (config->signer.tokens.status != token_status_on_initialize)
                {
                    config->signer.tokens.status = token_status_on_initialize;
                    config->signer.tokens.error.code = 0;
                    config->signer.tokens.error.message.clear();
                    config->internal.last_jwt_generation_error_cb_millis = 0;
                    sendTokenStatusCB();
                }
            }

            // If service account creds are ready, set the token processing task started flag and run the task
            _token_processing_task_enable = true;
            tokenProcessingTask();
        }
    }

    return config->signer.tokens.status == token_status_ready;
}

void GAuthManager::initJson()
{
    if (!jsonPtr)
        jsonPtr = new FirebaseJson();
    if (!resultPtr)
        resultPtr = new FirebaseJsonData();
}

void GAuthManager::freeJson()
{
    if (jsonPtr)
        delete jsonPtr;
    if (resultPtr)
        delete resultPtr;
    jsonPtr = nullptr;
    resultPtr = nullptr;
}

void GAuthManager::tryGetTime()
{
    if (!tcpClient || config->internal.clock_rdy)
        return;

    _cli_type = tcpClient->type();

    if (tcpClient->type() == esp_google_sheet_client_type_external_gsm_client)
    {
        uint32_t _time = tcpClient->gprsGetTime();
        if (_time > 0)
        {
            *mb_ts = _time;
            TimeHelper::setTimestamp(_time, mb_ts_offset);
            config->internal.clock_rdy = TimeHelper::clockReady(mb_ts, mb_ts_offset);
        }
    }
    else
        TimeHelper::syncClock(mb_ts, mb_ts_offset, config->time_zone, config);
}

void GAuthManager::tokenProcessingTask()
{
    // We don't have to use memory reserved tasks e.g., RTOS task in ESP32 for this JWT
    // All tasks can be processed in a finite loop.

    // return when task is currently running
    if (config->signer.tokenTaskRunning)
        return;

    bool ret = false;

    config->signer.tokenTaskRunning = true;

    time_t now = getTime();

    Utils::idle();
    // check time if clock synching once set in the JWT token generating process (during beginning step)
    // or valid time required for SSL handshake in ESP8266
    if (!config->internal.clock_rdy)
    {
        if (readyToSync())
        {
            if (isSyncTimeOut())
            {
                config->signer.tokens.error.message.clear();
                if (_cli_type == esp_google_sheet_client_type_internal_basic_client)
                    setTokenError(ESP_GOOGLE_SHEET_CLIENT_ERROR_NTP_SYNC_TIMED_OUT);
                else
                    setTokenError(ESP_GOOGLE_SHEET_CLIENT_ERROR_SYS_TIME_IS_NOT_READY);
                sendTokenStatusCB();
                config->signer.tokens.status = token_status_on_initialize;
                config->internal.last_jwt_generation_error_cb_millis = 0;
            }

            // reset flag to allow clock synching execution again in ut->syncClock if clocck synching was timed out
            config->internal.clock_synched = false;
            reconnect();
        }

        // check or set time again
        tryGetTime();

        // exit task immediately if time is not ready synched
        // which handleToken function should run repeatedly to enter this function again.
        if (!config->internal.clock_rdy)
        {
            config->signer.tokenTaskRunning = false;
            return;
        }
    }

    // create signed JWT token and exchange with auth token
    if (config->signer.step == gauth_jwt_generation_step_begin &&
        (millis() - config->internal.last_jwt_begin_step_millis > config->timeout.tokenGenerationBeginStep ||
         config->internal.last_jwt_begin_step_millis == 0))
    {

        // time must be set first
        tryGetTime();
        config->internal.last_jwt_begin_step_millis = millis();

        if (config->internal.clock_rdy)
            config->signer.step = gauth_jwt_generation_step_encode_header_payload;
    }
    // encode the JWT token
    else if (config->signer.step == gauth_jwt_generation_step_encode_header_payload)
    {
        if (createJWT())
            config->signer.step = gauth_jwt_generation_step_sign;
    }
    // sign the JWT token
    else if (config->signer.step == gauth_jwt_generation_step_sign)
    {
        if (createJWT())
            config->signer.step = gauth_jwt_generation_step_exchange;
    }
    // sending JWT token requst for auth token
    else if (config->signer.step == gauth_jwt_generation_step_exchange)
    {

        if (readyToRefresh())
        {
            // sending a new request
            ret = requestTokens(false);

            // send error cb
            if (!reconnect())
                handleTaskError(ESP_GOOGLE_SHEET_CLIENT_ERROR_TCP_ERROR_CONNECTION_LOST);

            // reset state and exit loop
            config->signer.step = ret || getTime() - now > 3599 ? gauth_jwt_generation_step_begin : gauth_jwt_generation_step_exchange;

            _token_processing_task_enable = false;
            ret = true;
        }
    }

    // reset task running status
    config->signer.tokenTaskRunning = false;
}

bool GAuthManager::refreshToken()
{

    if (!config)
        return false;

    if (config->signer.tokens.status == token_status_on_request ||
        config->signer.tokens.status == token_status_on_refresh ||
        config->internal.processing)
        return false;

    if (config->internal.refresh_token.length() == 0 && config->internal.auth_token.length() == 0)
        return false;

    if (!initClient(gauth_pgm_str_8 /* "securetoken" */, token_status_on_refresh))
        return false;

    jsonPtr->add(pgm2Str(gauth_pgm_str_9 /* "grantType" */), pgm2Str(gauth_pgm_str_10 /* "refresh_token" */));
    jsonPtr->add(pgm2Str(gauth_pgm_str_11 /* "refreshToken" */), config->internal.refresh_token.c_str());

    MB_String req;
    HttpHelper::addRequestHeaderFirst(req, http_post);

    req += gauth_pgm_str_12; // "/v1/token?Key=""
    req += config->api_key;
    HttpHelper::addRequestHeaderLast(req);

    HttpHelper::addGAPIsHostHeader(req, gauth_pgm_str_8 /* "securetoken" */);
    HttpHelper::addUAHeader(req);
    HttpHelper::addContentLengthHeader(req, strlen(jsonPtr->raw()));
    HttpHelper::addContentTypeHeader(req, gauth_pgm_str_13 /* "application/json" */);
    HttpHelper::addNewLine(req);

    req += jsonPtr->raw(); // {"grantType":"refresh_token","refreshToken":"<refresh token>"}

    tcpClient->send(req.c_str());

    req.clear();
    if (response_code < 0)
        return handleTaskError(ESP_GOOGLE_SHEET_CLIENT_ERROR_TCP_ERROR_CONNECTION_LOST);

    struct gauth_auth_token_error_t error;

    int httpCode = ESP_GOOGLE_SHEET_CLIENT_ERROR_HTTP_CODE_REQUEST_TIMEOUT;
    MB_String payload;
    if (handleResponse(tcpClient, httpCode, payload))
    {
        if (JsonHelper::parse(jsonPtr, resultPtr, gauth_pgm_str_14 /* "error/code" */))
        {
            error.code = resultPtr->to<int>();
            config->signer.tokens.status = token_status_error;

            if (JsonHelper::parse(jsonPtr, resultPtr, gauth_pgm_str_15 /* "error/message" */))
                error.message = resultPtr->to<const char *>();
        }

        config->signer.tokens.error = error;
        tokenInfo.status = config->signer.tokens.status;
        tokenInfo.error = config->signer.tokens.error;
        config->internal.last_jwt_generation_error_cb_millis = 0;
        if (error.code != 0)
            sendTokenStatusCB();

        if (error.code == 0)
        {

            if (JsonHelper::parse(jsonPtr, resultPtr, gauth_pgm_str_19 /* "expires_in" */))
                getExpiration(resultPtr->to<const char *>());

            return handleTaskError(ESP_GOOGLE_SHEET_CLIENT_ERROR_TOKEN_COMPLETE_NOTIFY);
        }

        return handleTaskError(ESP_GOOGLE_SHEET_CLIENT_ERROR_TOKEN_ERROR_UNNOTIFY);
    }

    return handleTaskError(ESP_GOOGLE_SHEET_CLIENT_ERROR_HTTP_CODE_REQUEST_TIMEOUT, httpCode);
}

void GAuthManager::setTokenError(int code)
{
    config->signer.tokens.error.message.clear();
    config->signer.tokens.status = code == 0 ? token_status_ready : token_status_error;
    config->signer.tokens.error.code = code;
    config->internal.processing = false;
    errorToString(code, config->signer.tokens.error.message);
}

bool GAuthManager::handleTaskError(int code, int httpCode)
{
    // Close TCP connection and unlock used flag
    tcpClient->stop();
    config->internal.processing = false;

    switch (code)
    {

    case ESP_GOOGLE_SHEET_CLIENT_ERROR_TCP_ERROR_CONNECTION_LOST:

        // Show error based on connection status
        config->signer.tokens.error.message.clear();
        setTokenError(code);
        config->internal.last_jwt_generation_error_cb_millis = 0;
        sendTokenStatusCB();
        break;
    case ESP_GOOGLE_SHEET_CLIENT_ERROR_HTTP_CODE_REQUEST_TIMEOUT:

        // Request time out?
        if (httpCode == 0)
        {
            // Show error based on request time out
            setTokenError(code);
        }
        else
        {
            // Show error from response http code
            errorToString(httpCode, config->signer.tokens.error.message);
            setTokenError(httpCode);
        }

        config->internal.last_jwt_generation_error_cb_millis = 0;
        sendTokenStatusCB();

        break;

    default:
        break;
    }

    // Free memory
    tcpClient->stop();
    freeJson();

    // reset token processing state
    if (code == ESP_GOOGLE_SHEET_CLIENT_ERROR_TOKEN_COMPLETE_NOTIFY || code == ESP_GOOGLE_SHEET_CLIENT_ERROR_TOKEN_COMPLETE_UNNOTIFY)
    {
        config->signer.tokens.error.message.clear();
        config->signer.tokens.status = token_status_ready;
        config->signer.step = gauth_jwt_generation_step_begin;
        config->internal.last_jwt_generation_error_cb_millis = 0;
        if (code == ESP_GOOGLE_SHEET_CLIENT_ERROR_TOKEN_COMPLETE_NOTIFY)
            sendTokenStatusCB();

        return true;
    }

    return false;
}

void GAuthManager::sendTokenStatusCB()
{
    tokenInfo.status = config->signer.tokens.status;
    tokenInfo.type = config->signer.tokens.token_type;
    tokenInfo.error = config->signer.tokens.error;

    if (config->token_status_callback && isErrorCBTimeOut())
        config->token_status_callback(tokenInfo);
}

bool GAuthManager::handleResponse(GS_TCP_Client *client, int &httpCode, MB_String &payload, bool stopSession)
{
    if (!reconnect(client))
        return false;

    MB_String header;

    struct esp_google_sheet_server_response_data_t response;
    struct esp_google_sheet_tcp_response_handler_t tcpHandler;

    HttpHelper::intTCPHandler(client, tcpHandler, 2048, 2048, nullptr);

    while (client->connected() && client->available() == 0)
    {
        Utils::idle();
        if (!reconnect(client, tcpHandler.dataTime))
            return false;
    }

    bool complete = false;

    tcpHandler.chunkBufSize = tcpHandler.defaultChunkSize;

    char *pChunk = MemoryHelper::createBuffer<char *>(mbfs, tcpHandler.chunkBufSize + 1);

    while (tcpHandler.available() || !complete)
    {
        Utils::idle();

        if (!reconnect(client, tcpHandler.dataTime))
            break;

        if (!HttpHelper::readStatusLine(mbfs, client, tcpHandler, response))
        {
            // The next chunk data can be the remaining http header
            if (tcpHandler.isHeader)
            {
                // Read header, complete?
                if (HttpHelper::readHeader(mbfs, client, tcpHandler, response))
                {
                    if (response.httpCode == ESP_GOOGLE_SHEET_CLIENT_ERROR_HTTP_CODE_NO_CONTENT)
                        tcpHandler.error.code = 0;

                    if (Utils::isNoContent(&response))
                        break;
                }
            }
            else
            {
                memset(pChunk, 0, tcpHandler.chunkBufSize + 1);

                // Read the avilable data
                // chunk transfer encoding?
                if (response.isChunkedEnc)
                    tcpHandler.bufferAvailable = HttpHelper::readChunkedData(mbfs, client,
                                                                             pChunk, nullptr, tcpHandler);
                else
                    tcpHandler.bufferAvailable = HttpHelper::readLine(client,
                                                                      pChunk, tcpHandler.chunkBufSize);

                if (tcpHandler.bufferAvailable > 0)
                {
                    tcpHandler.payloadRead += tcpHandler.bufferAvailable;
                    payload += pChunk;
                }

                if (Utils::isChunkComplete(&tcpHandler, &response, complete) ||
                    Utils::isResponseComplete(&tcpHandler, &response, complete))
                {

                    break;
                }
            }
        }
    }

    // To make sure all chunks read
    if (response.isChunkedEnc)
        client->flush();

    MemoryHelper::freeBuffer(mbfs, pChunk);

    if (stopSession && client->connected())
        client->stop();

    httpCode = response.httpCode;

    if (jsonPtr && payload.length() > 0 && !response.noContent)
    {
        // Just a simple JSON which is suitable for parsing in low memory device
        jsonPtr->setJsonData(payload.c_str());
        return true;
    }

    return httpCode == ESP_GOOGLE_SHEET_CLIENT_ERROR_HTTP_CODE_OK;
}

bool GAuthManager::createJWT()
{
    if (config->signer.step == gauth_jwt_generation_step_encode_header_payload)
    {
        config->signer.tokens.status = token_status_on_signing;
        config->signer.tokens.error.code = 0;
        config->signer.tokens.error.message.clear();
        config->internal.last_jwt_generation_error_cb_millis = 0;
        sendTokenStatusCB();

        time_t now = getTime();

        initJson();

        config->signer.tokens.jwt.clear();

        // header
        // {"alg":"RS256","typ":"JWT"}
        jsonPtr->add(pgm2Str(gauth_pgm_str_20 /* "alg" */), pgm2Str(gauth_pgm_str_21 /* "RS256" */));
        jsonPtr->add(pgm2Str(gauth_pgm_str_22 /* "typ" */), pgm2Str(gauth_pgm_str_23 /* "JWT" */));

        size_t len = Base64Helper::encodedLength(strlen(jsonPtr->raw()));
        char *buf = MemoryHelper::createBuffer<char *>(mbfs, len);
        Base64Helper::encodeUrl(mbfs, buf, (unsigned char *)jsonPtr->raw(), strlen(jsonPtr->raw()));
        config->signer.encHeader = buf;
        MemoryHelper::freeBuffer(mbfs, buf);
        config->signer.encHeadPayload = config->signer.encHeader;

        // payload
        // {"iss":"<email>","sub":"<email>","aud":"<audience>","iat":<timstamp>,"exp":<expire>,"scope":"<scope>"}
        // {"iss":"<email>","sub":"<email>","aud":"<audience>","iat":<timstamp>,"exp":<expire>,"uid":"<uid>","claims":"<claims>"}
        jsonPtr->clear();
        jsonPtr->add(pgm2Str(gauth_pgm_str_24 /* "iss" */), config->service_account.data.client_email.c_str());
        jsonPtr->add(pgm2Str(gauth_pgm_str_25 /* "sub" */), config->service_account.data.client_email.c_str());

        MB_String t = gauth_pgm_str_26; // "https://"
        HttpHelper::addGAPIsHost(t, gauth_pgm_str_27 /* "oauth2" */);
        t += gauth_pgm_str_28; // "/"
        t += gauth_pgm_str_29; // "token"

        jsonPtr->add(pgm2Str(gauth_pgm_str_30 /* "aud" */), t.c_str());
        jsonPtr->add(pgm2Str(gauth_pgm_str_31 /* "iat" */), (int)now);

        if (config->signer.expiredSeconds > 3600)
            jsonPtr->add(pgm2Str(gauth_pgm_str_32 /* "exp" */), (int)(now + 3600));
        else
            jsonPtr->add(pgm2Str(gauth_pgm_str_32 /* "exp" */), (int)(now + config->signer.expiredSeconds));

        jsonPtr->add(pgm2Str(gauth_pgm_str_33 /* "scope" */), pgm2Str(gauth_pgm_str_34));

        len = Base64Helper::encodedLength(strlen(jsonPtr->raw()));
        buf = MemoryHelper::createBuffer<char *>(mbfs, len);
        Base64Helper::encodeUrl(mbfs, buf, (unsigned char *)jsonPtr->raw(), strlen(jsonPtr->raw()));
        config->signer.encPayload = buf;
        MemoryHelper::freeBuffer(mbfs, buf);

        config->signer.encHeadPayload += gauth_pgm_str_35; // "."
        config->signer.encHeadPayload += config->signer.encPayload;

        config->signer.encHeader.clear();
        config->signer.encPayload.clear();

        // create message digest from encoded header and payload
        config->signer.hash = MemoryHelper::createBuffer<char *>(mbfs, config->signer.hashSize);
        br_sha256_context mc;
        br_sha256_init(&mc);
        br_sha256_update(&mc, config->signer.encHeadPayload.c_str(), config->signer.encHeadPayload.length());
        br_sha256_out(&mc, config->signer.hash);

        config->signer.tokens.jwt = config->signer.encHeadPayload;
        config->signer.tokens.jwt += gauth_pgm_str_35; // "."
        config->signer.encHeadPayload.clear();

        freeJson();
    }
    else if (config->signer.step == gauth_jwt_generation_step_sign)
    {
        config->signer.tokens.status = token_status_on_signing;

        // RSA private key
        PrivateKey *pk = nullptr;
        Utils::idle();
        // parse priv key
        if (config->signer.pk.length() > 0)
            pk = new PrivateKey((const char *)config->signer.pk.c_str());
        else if (strlen_P(config->service_account.data.private_key) > 0)
            pk = new PrivateKey((const char *)config->service_account.data.private_key);

        if (!pk)
        {
            setTokenError(ESP_GOOGLE_SHEET_CLIENT_ERROR_TOKEN_PARSE_PK);
            config->signer.tokens.error.message.insert(0, (const char *)FPSTR("BearSSL, PrivateKey: "));
            sendTokenStatusCB();
            return false;
        }

        if (!pk->isRSA())
        {
            setTokenError(ESP_GOOGLE_SHEET_CLIENT_ERROR_TOKEN_PARSE_PK);
            config->signer.tokens.error.message.insert(0, (const char *)FPSTR("BearSSL, isRSA: "));
            sendTokenStatusCB();
            delete pk;
            pk = nullptr;
            return false;
        }

        const br_rsa_private_key *br_rsa_key = pk->getRSA();

        // generate RSA signature from private key and message digest
        config->signer.signature = new unsigned char[config->signer.signatureSize];

        Utils::idle();
        int ret = br_rsa_i15_pkcs1_sign(BR_HASH_OID_SHA256, (const unsigned char *)config->signer.hash,
                                        br_sha256_SIZE, br_rsa_key, config->signer.signature);
        Utils::idle();
        MemoryHelper::freeBuffer(mbfs, config->signer.hash);

        size_t len = Base64Helper::encodedLength(config->signer.signatureSize);
        char *buf = MemoryHelper::createBuffer<char *>(mbfs, len);
        Base64Helper::encodeUrl(mbfs, buf, config->signer.signature, config->signer.signatureSize);
        config->signer.encSignature = buf;
        MemoryHelper::freeBuffer(mbfs, buf);
        MemoryHelper::freeBuffer(mbfs, config->signer.signature);
        delete pk;
        pk = nullptr;

        // get the signed JWT
        if (ret > 0)
        {
            config->signer.tokens.jwt += config->signer.encSignature;
            config->signer.pk.clear();
            config->signer.encSignature.clear();
        }
        else
        {
            setTokenError(ESP_GOOGLE_SHEET_CLIENT_ERROR_TOKEN_SIGN);
            config->signer.tokens.error.message.insert(0, (const char *)FPSTR("BearSSL, br_rsa_i15_pkcs1_sign: "));
            sendTokenStatusCB();
            return false;
        }
    }

    return true;
}

bool GAuthManager::initClient(PGM_P subDomain, gauth_auth_token_status status)
{

    Utils::idle();

    if (status != token_status_uninitialized)
    {
        config->signer.tokens.status = status;
        config->internal.processing = true;
        config->signer.tokens.error.code = 0;
        config->signer.tokens.error.message.clear();
        config->internal.last_jwt_generation_error_cb_millis = 0;
        config->internal.last_request_token_cb_millis = millis();
        sendTokenStatusCB();
    }

    // stop the TCP session
    tcpClient->stop();

    tcpClient->setCACert(nullptr);

    if (!reconnect(tcpClient))
        return false;

    tcpClient->setBufferSizes(2048, 1024);

    initJson();

    MB_String host;
    HttpHelper::addGAPIsHost(host, subDomain);

    Utils::idle();
    tcpClient->begin(host.c_str(), 443, &response_code);

    time_t now = getTime();

    tcpClient->setX509Time(now);

    return true;
}

bool GAuthManager::requestTokens(bool refresh)
{
    time_t now = getTime();

    if (config->signer.tokens.status == token_status_on_request ||
        config->signer.tokens.status == token_status_on_refresh ||
        ((unsigned long)now < ESP_GOOGLE_SHEET_CLIENT_DEFAULT_TS && !refresh) ||
        config->internal.processing)
        return false;

    if (!initClient(gauth_pgm_str_36 /* "www" */, refresh ? token_status_on_refresh : token_status_on_request))
        return false;

    MB_String req;
    HttpHelper::addRequestHeaderFirst(req, http_post);

    if (refresh)
    {
        // {"client_id":"<client id>","client_secret":"<client secret>","grant_type":"refresh_token",
        // "refresh_token":"<refresh token>"}
        jsonPtr->add(pgm2Str(gauth_pgm_str_7 /* "client_id" */), config->internal.client_id.c_str());
        jsonPtr->add(pgm2Str(gauth_pgm_str_37 /* "client_secret" */), config->internal.client_secret.c_str());

        jsonPtr->add(pgm2Str(gauth_pgm_str_38 /* "grant_type" */), pgm2Str(gauth_pgm_str_10 /* "refresh_token" */));
        jsonPtr->add(pgm2Str(gauth_pgm_str_18 /* "refresh_token" */), config->internal.refresh_token.c_str());
    }
    else
    {

        // rfc 7523, JWT Bearer Token Grant Type Profile for OAuth 2.0

        // {"grant_type":"urn:ietf:params:oauth:grant-type:jwt-bearer","assertion":"<signed jwt token>"}
        jsonPtr->add(pgm2Str(gauth_pgm_str_38 /* "grant_type" */),
                     pgm2Str(gauth_pgm_str_39 /* "urn:ietf:params:oauth:grant-type:jwt-bearer" */));
        jsonPtr->add(pgm2Str(gauth_pgm_str_40 /* "assertion" */), config->signer.tokens.jwt.c_str());
    }

    req += gauth_pgm_str_28; // "/"
    req += gauth_pgm_str_29; // "token"
    HttpHelper::addRequestHeaderLast(req);
    HttpHelper::addGAPIsHostHeader(req, gauth_pgm_str_41 /* "oauth2" */);

    HttpHelper::addUAHeader(req);
    HttpHelper::addContentLengthHeader(req, strlen(jsonPtr->raw()));
    HttpHelper::addContentTypeHeader(req, gauth_pgm_str_13 /* "application/json" */);
    HttpHelper::addNewLine(req);

    req += jsonPtr->raw();

    tcpClient->send(req.c_str());

    req.clear();

    if (response_code < 0)
        return handleTaskError(ESP_GOOGLE_SHEET_CLIENT_ERROR_TCP_ERROR_CONNECTION_LOST);

    struct gauth_auth_token_error_t error;

    int httpCode = ESP_GOOGLE_SHEET_CLIENT_ERROR_HTTP_CODE_REQUEST_TIMEOUT;
    MB_String payload;
    if (handleResponse(tcpClient, httpCode, payload))
    {

        config->signer.tokens.jwt.clear();
        if (JsonHelper::parse(jsonPtr, resultPtr, gauth_pgm_str_14 /* "error/code" */))
        {
            error.code = resultPtr->to<int>();
            config->signer.tokens.status = token_status_error;

            if (JsonHelper::parse(jsonPtr, resultPtr, gauth_pgm_str_15 /* "error/message" */))
                error.message = resultPtr->to<const char *>();
        }
        else if (JsonHelper::parse(jsonPtr, resultPtr, gauth_pgm_str_42 /* "error" */))
        {
            error.code = -1;
            config->signer.tokens.status = token_status_error;

            if (JsonHelper::parse(jsonPtr, resultPtr, gauth_pgm_str_43 /* "error_description" */))
                error.message = resultPtr->to<const char *>();
        }

        if (error.code != 0)
        {
            // new jwt needed as it is already cleared
            config->signer.step = gauth_jwt_generation_step_encode_header_payload;
        }

        config->signer.tokens.error = error;
        tokenInfo.status = config->signer.tokens.status;
        tokenInfo.error = config->signer.tokens.error;
        config->internal.last_jwt_generation_error_cb_millis = 0;

        if (error.code != 0)
            sendTokenStatusCB();

        if (error.code == 0)
        {

            if (JsonHelper::parse(jsonPtr, resultPtr, gauth_pgm_str_44 /* "access_token" */))
                config->internal.auth_token = resultPtr->to<const char *>();

            if (JsonHelper::parse(jsonPtr, resultPtr, gauth_pgm_str_19 /* "expires_in" */))
                getExpiration(resultPtr->to<const char *>());

            return handleTaskError(ESP_GOOGLE_SHEET_CLIENT_ERROR_TOKEN_COMPLETE_NOTIFY);
        }
        return handleTaskError(ESP_GOOGLE_SHEET_CLIENT_ERROR_TOKEN_ERROR_UNNOTIFY);
    }

    return handleTaskError(ESP_GOOGLE_SHEET_CLIENT_ERROR_HTTP_CODE_REQUEST_TIMEOUT, httpCode);
}

void GAuthManager::getExpiration(const char *exp)
{
    time_t now = getTime();
    unsigned long ms = millis();
    config->signer.tokens.expires = now + atoi(exp);
    config->signer.tokens.last_millis = ms;
}

void GAuthManager::checkToken()
{
    if (!config)
        return;

    if (isExpired())
        handleToken();
}

bool GAuthManager::tokenReady()
{
    if (!config)
        return false;

    checkToken();

    // call checkToken to send callback before checking connection.
    if (!reconnect())
        return false;

    return config->signer.tokens.status == token_status_ready;
};

String GAuthManager::getTokenType(TokenInfo info)
{
    if (!config)
        return String();

    MB_String s;
    switch (info.type)
    {
    case token_type_undefined:
        s = esp_google_sheet_pgm_str_39;
        break;
    case token_type_oauth2_access_token:
        s = esp_google_sheet_pgm_str_40;
        break;
    default:
        break;
    }
    return s.c_str();
}

String GAuthManager::getTokenType()
{
    return getTokenType(tokenInfo);
}

String GAuthManager::getTokenStatus(TokenInfo info)
{
    if (!config)
        return String();

    MB_String s;
    switch (info.status)
    {
    case token_status_uninitialized:
        s = esp_google_sheet_pgm_str_41;
        break;

    case token_status_on_initialize:
        s = esp_google_sheet_pgm_str_42;
        break;
    case token_status_on_signing:
        s = esp_google_sheet_pgm_str_43;
        break;
    case token_status_on_request:
        s = esp_google_sheet_pgm_str_44;
        break;
    case token_status_on_refresh:
        s = esp_google_sheet_pgm_str_45;
        break;
    case token_status_ready:
        s = esp_google_sheet_pgm_str_49;
        break;
    case token_status_error:
        s = esp_google_sheet_pgm_str_46;
        break;
    default:
        break;
    }
    return s.c_str();
}

String GAuthManager::getTokenStatus()
{
    return getTokenStatus(tokenInfo);
}

String GAuthManager::getTokenError(TokenInfo info)
{
    if (!config)
        return String();

    MB_String s = esp_google_sheet_pgm_str_47;
    s += info.error.code;
    s += esp_google_sheet_pgm_str_48;
    s += info.error.message;
    return s.c_str();
}

void GAuthManager::reset()
{
    if (config)
    {
        config->internal.client_id.clear();
        config->internal.client_secret.clear();
        config->internal.auth_token.clear();
        config->internal.refresh_token.clear();
        config->signer.lastReqMillis = 0;
        config->internal.last_jwt_generation_error_cb_millis = 0;
        config->signer.tokens.expires = 0;
        config->internal.rtoken_requested = false;

        config->internal.client_email_crc = 0;
        config->internal.project_id_crc = 0;
        config->internal.priv_key_crc = 0;
        config->internal.email_crc = 0;
        config->internal.password_crc = 0;

        config->signer.tokens.status = token_status_uninitialized;
    }
}

void GAuthManager::refresh()
{
    if (config)
    {
        config->signer.lastReqMillis = 0;
        config->signer.tokens.expires = 0;

        if (config)
        {
            config->internal.rtoken_requested = false;
            this->requestTokens(true);
        }
        else
            config->internal.rtoken_requested = true;
    }
}

String GAuthManager::getTokenError()
{
    return getTokenError(tokenInfo);
}

unsigned long GAuthManager::getExpiredTimestamp()
{
    if (!config)
        return 0;

    return config->signer.tokens.expires;
}

bool GAuthManager::reconnect(GS_TCP_Client *client, unsigned long dataTime)
{
    if (!client)
        return false;

    if (dataTime > 0)
    {
        unsigned long tmo = ESP_GOOGLE_SHEET_CLIENT_DEFAULT_SERVER_RESPONSE_TIMEOUT;
        if (config->timeout.serverResponse < ESP_GOOGLE_SHEET_CLIENT_MIN_SERVER_RESPONSE_TIMEOUT ||
            config->timeout.serverResponse > ESP_GOOGLE_SHEET_CLIENT_MAX_SERVER_RESPONSE_TIMEOUT)
            config->timeout.serverResponse = ESP_GOOGLE_SHEET_CLIENT_DEFAULT_SERVER_RESPONSE_TIMEOUT;

        tmo = config->timeout.serverResponse;

        if (millis() - dataTime > tmo)
        {
            response_code = ESP_GOOGLE_SHEET_CLIENT_ERROR_TCP_RESPONSE_PAYLOAD_READ_TIMED_OUT;
            return false;
        }
    }

    bool status = client->networkReady();

    if (!status)
    {
        client->stop();

        if (autoReconnectWiFi)
        {
            if (config->timeout.wifiReconnect < ESP_GOOGLE_SHEET_CLIENT_MIN_WIFI_RECONNECT_TIMEOUT ||
                config->timeout.wifiReconnect > ESP_GOOGLE_SHEET_CLIENT_MAX_WIFI_RECONNECT_TIMEOUT)
                config->timeout.wifiReconnect = ESP_GOOGLE_SHEET_CLIENT_MIN_WIFI_RECONNECT_TIMEOUT;

            if (millis() - config->internal.last_reconnect_millis > config->timeout.wifiReconnect)
            {

                if (config->signer.tokens.status != token_status_ready && !tcpClient->isInitialized())
                {

                    config->signer.tokens.error.message.clear();
                    setTokenError(ESP_GOOGLE_SHEET_CLIENT_ERROR_EXTERNAL_CLIENT_NOT_INITIALIZED);
                    sendTokenStatusCB();
                }
                client->networkReconnect();
                config->internal.last_reconnect_millis = millis();
            }
        }

        status = client->networkReady();

        if (!status)
            response_code = ESP_GOOGLE_SHEET_CLIENT_ERROR_TCP_ERROR_CONNECTION_LOST;
    }

    return status;
}

bool GAuthManager::reconnect()
{
    return reconnect(tcpClient);
}

void GAuthManager::errorToString(int httpCode, MB_String &buff)
{
    buff.clear();

    if (&config->signer.tokens.error.message != &buff &&
        (response_code > 200 || config->signer.tokens.status == token_status_error || config->signer.tokens.error.code != 0))
    {
        buff = config->signer.tokens.error.message;
        return;
    }

    switch (httpCode)
    {
    case ESP_GOOGLE_SHEET_CLIENT_ERROR_TCP_ERROR_CONNECTION_REFUSED:
        buff += F("connection refused");
        return;
    case ESP_GOOGLE_SHEET_CLIENT_ERROR_TCP_ERROR_SEND_REQUEST_FAILED:
        buff += F("send request failed");
        return;
    case ESP_GOOGLE_SHEET_CLIENT_ERROR_TCP_ERROR_NOT_CONNECTED:
        buff += F("not connected");
        return;
    case ESP_GOOGLE_SHEET_CLIENT_ERROR_TCP_ERROR_CONNECTION_LOST:
        buff += F("connection lost");
        return;
    case ESP_GOOGLE_SHEET_CLIENT_ERROR_TCP_ERROR_NO_HTTP_SERVER:
        buff += F("no HTTP server");
        return;
    case ESP_GOOGLE_SHEET_CLIENT_ERROR_TCP_CLIENT_MISSING_NETWORK_CONNECTION_CB:
        buff += F("network connection callback is required");
        return;
    case ESP_GOOGLE_SHEET_CLIENT_ERROR_TCP_CLIENT_MISSING_NETWORK_STATUS_CB:
        buff += F("network connection status callback is required");
        return;
    case ESP_GOOGLE_SHEET_CLIENT_ERROR_TCP_CLIENT_NOT_INITIALIZED:
        buff += F("client and/or necessary callback functions are not yet assigned");
        return;
    case ESP_GOOGLE_SHEET_CLIENT_ERROR_HTTP_CODE_BAD_REQUEST:
        buff += F("bad request");
        return;
    case ESP_GOOGLE_SHEET_CLIENT_ERROR_HTTP_CODE_NON_AUTHORITATIVE_INFORMATION:
        buff += F("non-authoriative information");
        return;
    case ESP_GOOGLE_SHEET_CLIENT_ERROR_HTTP_CODE_NO_CONTENT:
        buff += F("no content");
        return;
    case ESP_GOOGLE_SHEET_CLIENT_ERROR_HTTP_CODE_MOVED_PERMANENTLY:
        buff += F("moved permanently");
        return;
    case ESP_GOOGLE_SHEET_CLIENT_ERROR_HTTP_CODE_USE_PROXY:
        buff += F("use proxy");
        return;
    case ESP_GOOGLE_SHEET_CLIENT_ERROR_HTTP_CODE_TEMPORARY_REDIRECT:
        buff += F("temporary redirect");
        return;
    case ESP_GOOGLE_SHEET_CLIENT_ERROR_HTTP_CODE_PERMANENT_REDIRECT:
        buff += F("permanent redirect");
        return;
    case ESP_GOOGLE_SHEET_CLIENT_ERROR_HTTP_CODE_UNAUTHORIZED:
        buff += F("unauthorized");
        return;
    case ESP_GOOGLE_SHEET_CLIENT_ERROR_HTTP_CODE_FORBIDDEN:
        buff += F("forbidden");
        return;
    case ESP_GOOGLE_SHEET_CLIENT_ERROR_HTTP_CODE_NOT_FOUND:
        buff += F("not found");
        return;
    case ESP_GOOGLE_SHEET_CLIENT_ERROR_HTTP_CODE_METHOD_NOT_ALLOWED:
        buff += F("method not allow");
        return;
    case ESP_GOOGLE_SHEET_CLIENT_ERROR_HTTP_CODE_NOT_ACCEPTABLE:
        buff += F("not acceptable");
        return;
    case ESP_GOOGLE_SHEET_CLIENT_ERROR_HTTP_CODE_PROXY_AUTHENTICATION_REQUIRED:
        buff += F("proxy authentication required");
        return;
    case ESP_GOOGLE_SHEET_CLIENT_ERROR_HTTP_CODE_REQUEST_TIMEOUT:
        buff += F("request timed out");
        return;
    case ESP_GOOGLE_SHEET_CLIENT_ERROR_HTTP_CODE_LENGTH_REQUIRED:
        buff += F("length required");
        return;
    case ESP_GOOGLE_SHEET_CLIENT_ERROR_HTTP_CODE_TOO_MANY_REQUESTS:
        buff += F("too many requests");
        return;
    case ESP_GOOGLE_SHEET_CLIENT_ERROR_HTTP_CODE_REQUEST_HEADER_FIELDS_TOO_LARGE:
        buff += F("request header fields too larg");
        return;
    case ESP_GOOGLE_SHEET_CLIENT_ERROR_HTTP_CODE_INTERNAL_SERVER_ERROR:
        buff += F("internal server error");
        return;
    case ESP_GOOGLE_SHEET_CLIENT_ERROR_HTTP_CODE_BAD_GATEWAY:
        buff += F("bad gateway");
        return;
    case ESP_GOOGLE_SHEET_CLIENT_ERROR_HTTP_CODE_SERVICE_UNAVAILABLE:
        buff += F("service unavailable");
        return;
    case ESP_GOOGLE_SHEET_CLIENT_ERROR_HTTP_CODE_GATEWAY_TIMEOUT:
        buff += F("gateway timeout");
        return;
    case ESP_GOOGLE_SHEET_CLIENT_ERROR_HTTP_CODE_HTTP_VERSION_NOT_SUPPORTED:
        buff += F("http version not support");
        return;
    case ESP_GOOGLE_SHEET_CLIENT_ERROR_HTTP_CODE_NETWORK_AUTHENTICATION_REQUIRED:
        buff += F("network authentication required");
        return;
    case ESP_GOOGLE_SHEET_CLIENT_ERROR_HTTP_CODE_PRECONDITION_FAILED:
        buff += F("Precondition Failed (ETag does not match)");
        return;
    case ESP_GOOGLE_SHEET_CLIENT_ERROR_TCP_RESPONSE_PAYLOAD_READ_TIMED_OUT:
        buff += F("response read timed out");
        return;
    case ESP_GOOGLE_SHEET_CLIENT_ERROR_TCP_RESPONSE_READ_FAILED:
        buff += F("Response read failed.");
        return;
    case ESP_GOOGLE_SHEET_CLIENT_ERROR_TOKEN_NOT_READY:
        buff += F("token is not ready (revoked or expired)");
        return;
    case ESP_GOOGLE_SHEET_CLIENT_ERROR_TOKEN_SET_TIME:
        buff += F("system time was not set");
        break;
    case ESP_GOOGLE_SHEET_CLIENT_ERROR_TOKEN_PARSE_PK:
        buff += F("RSA private key parsing failed");
        break;
    case ESP_GOOGLE_SHEET_CLIENT_ERROR_TOKEN_SIGN:
        buff += F("JWT token signing failed");
        break;
    case ESP_GOOGLE_SHEET_CLIENT_ERROR_TOKEN_EXCHANGE:
        buff += F("token exchange failed");
        break;

#if defined(MBFS_FLASH_FS) || defined(MBFS_SD_FS)

    case MB_FS_ERROR_FLASH_STORAGE_IS_NOT_READY:
        buff += F("Flash Storage is not ready.");
        return;

    case MB_FS_ERROR_SD_STORAGE_IS_NOT_READY:
        buff += F("SD Storage is not ready.");
        return;

    case MB_FS_ERROR_FILE_STILL_OPENED:
        buff += F("File is still opened.");
        return;

    case MB_FS_ERROR_FILE_NOT_FOUND:
        buff += F("File not found.");
        return;
#endif

    case ESP_GOOGLE_SHEET_CLIENT_ERROR_NTP_SYNC_TIMED_OUT:
        buff += F("NTP server time synching failed");
        return;
    case ESP_GOOGLE_SHEET_CLIENT_ERROR_SYS_TIME_IS_NOT_READY:
        buff += F("System time or library reference time was not set. Use GSheet.setSystemTime to set time.");
        return;
    case ESP_GOOGLE_SHEET_CLIENT_ERROR_EXTERNAL_CLIENT_NOT_INITIALIZED:
        buff += F("External client is not yet initialized");
        return;

    case ESP_GOOGLE_SHEET_CLIENT_ERROR_MISSING_SERVICE_ACCOUNT_CREDENTIALS:
        buff += F("The Service Account Credentials are missing.");
        return;
    case ESP_GOOGLE_SHEET_CLIENT_ERROR_SERVICE_ACCOUNT_JSON_FILE_PARSING_ERROR:
        buff += F("Unable to parse Service Account JSON file. Please check file name, storage type and its content.");
        return;
    default:
        buff += F("unknown error");
        return;
    }
}

#endif