/**
 * Google's OAuth2.0 Access token Generation class, Signer.h version 1.1.1
 * 
 * This library used RS256 for signing algorithm.
 * 
 * The signed JWT token will be generated and exchanged with the access token in the final generating process.
 * 
 * This library supports Espressif ESP8266 and ESP32
 * 
 * Created November 12, 2021
 * 
 * This work is a part of Firebase ESP Client library
 * Copyright (c) 2021 K. Suwatchai (Mobizt)
 * 
 * The MIT License (MIT)
 * Copyright (c) 2021 K. Suwatchai (Mobizt)
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

#ifndef ESP_SIGNER_CPP
#define ESP_SIGNER_CPP
#include "ESPSigner.h"

ESP_Signer::ESP_Signer()
{
}

ESP_Signer::~ESP_Signer()
{
    if (ut)
        delete ut;
}

void ESP_Signer::begin(SignerConfig *cfg)
{
    if (ut)
        delete ut;
    config = cfg;
    ut = new SignerUtils(config);
    config->signer.tokens.error.message.clear();

    config->_int.esp_signer_reconnect_wifi = WiFi.getAutoReconnect();

    if (config->service_account.json.path.length() > 0)
    {
        if (!parseSAFile())
            config->signer.tokens.status = esp_signer_token_status_uninitialized;
    }

    if (tokenSAReady())
        config->signer.tokens.token_type = esp_signer_token_type_oauth2_access_token;

    if (strlen_P(config->cert.data))
        config->_int.esp_signer_caCert = config->cert.data;

    if (config->cert.file.length() > 0)
    {
        if (config->cert.file_storage == esp_signer_mem_storage_type_sd && !config->_int.esp_signer_sd_rdy)
            config->_int.esp_signer_sd_rdy = ut->sdTest(config->_int.esp_signer_file);
        else if ((config->cert.file_storage == esp_signer_mem_storage_type_flash) && !config->_int.esp_signer_flash_rdy)
            ut->flashTest();
    }

    _token_processing_task_end_request = false;

    handleToken();
}

void ESP_Signer::end()
{
    if (!config || _token_processing_task_end_request)
        return;
    config->signer.tokens.expires = 0;
    _token_processing_task_end_request = true;
}

bool ESP_Signer::parseSAFile()
{
    if (config->signer.pk.length() > 0)
        return false;

    if (config->service_account.json.storage_type == esp_signer_mem_storage_type_sd && !config->_int.esp_signer_sd_rdy)
        config->_int.esp_signer_sd_rdy = ut->sdTest(config->_int.esp_signer_file);
    else if (config->service_account.json.storage_type == esp_signer_mem_storage_type_flash && !config->_int.esp_signer_flash_rdy)
        ut->flashTest();

    if (config->_int.esp_signer_sd_rdy || config->_int.esp_signer_flash_rdy)
    {
        if (config->service_account.json.storage_type == esp_signer_mem_storage_type_flash)
        {
            if (FLASH_FS.exists(config->service_account.json.path.c_str()))
                config->_int.esp_signer_file = FLASH_FS.open(config->service_account.json.path.c_str(), "r");
        }
        else
        {
            if (SD_FS.exists(config->service_account.json.path.c_str()))
                config->_int.esp_signer_file = SD_FS.open(config->service_account.json.path.c_str(), "r");
        }

        if (config->_int.esp_signer_file)
        {
            clearSA();
            config->signer.json = new FirebaseJson();
            config->signer.result = new FirebaseJsonData();
            char *tmp = nullptr;

            size_t len = config->_int.esp_signer_file.size();
            char *buf = (char *)ut->newP(len + 10);
            if (config->_int.esp_signer_file.available())
            {
                config->_int.esp_signer_file.readBytes(buf, len);
                config->signer.json->setJsonData(buf);
            }
            config->_int.esp_signer_file.close();
            ut->delP(&buf);

            if (parseJsonResponse(esp_signer_pgm_str_13))
            {
                if (ut->strposP(config->signer.result->to<const char *>(), esp_signer_pgm_str_14, 0) > -1)
                {
                    if (parseJsonResponse(esp_signer_pgm_str_15))
                        config->service_account.data.project_id = config->signer.result->to<const char *>();

                    if (parseJsonResponse(esp_signer_pgm_str_16))
                        config->service_account.data.private_key_id = config->signer.result->to<const char *>();

                    if (parseJsonResponse(esp_signer_pgm_str_17))
                    {
                        size_t len = strlen(config->signer.result->to<const char *>());
                        tmp = (char *)ut->newP(len);
                        size_t c = 0;
                        for (size_t i = 0; i < len; i++)
                        {
                            if (config->signer.result->to<const char *>()[i] == '\\')
                            {
                                ut->idle();
                                tmp[c++] = '\n';
                                i++;
                            }
                            else
                                tmp[c++] = config->signer.result->to<const char *>()[i];
                        }
                        config->signer.pk = tmp;
                        config->signer.result->clear();
                        ut->delP(&tmp);
                    }

                    if (parseJsonResponse(esp_signer_pgm_str_18))
                        config->service_account.data.client_email = config->signer.result->to<const char *>();

                    if (parseJsonResponse(esp_signer_pgm_str_19))
                        config->service_account.data.client_id = config->signer.result->to<const char *>();

                    delete config->signer.json;
                    delete config->signer.result;
                    return true;
                }
            }

            delete config->signer.json;
            delete config->signer.result;
        }
    }

    return false;
}

void ESP_Signer::clearSA()
{
    config->service_account.data.private_key = "";
    config->service_account.data.project_id.clear();
    config->service_account.data.private_key_id.clear();
    config->service_account.data.client_email.clear();
    config->signer.pk.clear();
}

bool ESP_Signer::tokenSAReady()
{
    if (!config)
        return false;
    return (strlen_P(config->service_account.data.private_key) > 0 || config->signer.pk.length() > 0) && config->service_account.data.client_email.length() > 0 && config->service_account.data.project_id.length() > 0;
}

bool ESP_Signer::handleToken()
{
    if (!config)
        return false;

#if defined(ESP8266)
    if ((config->cert.data != NULL || config->cert.file.length() > 0) && !config->_int.esp_signer_clock_rdy)
    {
        ut->idle();
        time_t now = time(nullptr);
        config->_int.esp_signer_clock_rdy = now > ut->default_ts;

        if (!config->_int.esp_signer_clock_rdy)
        {
            ut->setClock(config->time_zone);

            if (config->signer.tokens.status == esp_signer_token_status_uninitialized)
            {
                config->signer.tokens.status = esp_signer_token_status_on_initialize;
                config->signer.tokens.error.code = 0;
                config->signer.tokens.error.message.clear();
                config->_int.esp_signer_last_jwt_generation_error_cb_millis = 0;
                sendTokenStatusCB();
            }
            return false;
        }
    }
#endif

    if (config->signer.tokens.token_type == esp_signer_token_type_oauth2_access_token && isExpired())
    {

        if (config->signer.step == esp_signer_jwt_generation_step_begin)
        {

            if (!config->signer.tokenTaskRunning)
            {
                if (config->service_account.json.path.length() > 0 && config->signer.pk.length() == 0)
                {
                    if (!parseSAFile())
                        config->signer.tokens.status = esp_signer_token_status_uninitialized;
                }

                config->signer.tokens.status = esp_signer_token_status_on_initialize;
                config->signer.tokens.error.code = 0;
                config->signer.tokens.error.message.clear();
                config->_int.esp_signer_last_jwt_generation_error_cb_millis = 0;
                sendTokenStatusCB();

                _token_processing_task_enable = true;
                tokenProcessingTask();
            }
        }
    }

    if (config->signer.tokens.token_type == esp_signer_token_type_undefined)
        setTokenError(ESP_SIGNER_ERROR_TOKEN_NOT_READY);

    return config->signer.tokens.status == esp_signer_token_status_ready;
}

bool ESP_Signer::isExpired()
{
    if (!config)
        return false;

    //if the time was set (changed) after token has been generated, update its expiration
    if (config->signer.tokens.expires > 0 && config->signer.tokens.expires < ESP_DEFAULT_TS && time(nullptr) > ESP_DEFAULT_TS)
        config->signer.tokens.expires += time(nullptr) - (millis() - config->signer.tokens.last_millis) / 1000 - 60;

    if (config->signer.preRefreshSeconds > config->signer.tokens.expires && config->signer.tokens.expires > 0)
        config->signer.preRefreshSeconds = 60;

    return ((unsigned long)time(nullptr) > config->signer.tokens.expires - config->signer.preRefreshSeconds || config->signer.tokens.expires == 0);
}

void ESP_Signer::tokenProcessingTask()
{

#if defined(ESP32)

    if (config->signer.tokenTaskRunning)
        return;

    bool ret = false;

    config->signer.tokenTaskRunning = true;

    while (!ret && config->signer.tokens.status != esp_signer_token_status_ready)
    {
        delay(0);

        if (config->signer.step == esp_signer_jwt_generation_step_begin && (millis() - config->_int.esp_signer_last_jwt_begin_step_millis > config->timeout.tokenGenerationBeginStep || config->_int.esp_signer_last_jwt_begin_step_millis == 0))
        {
            config->_int.esp_signer_last_jwt_begin_step_millis = millis();
            ut->setClock(config->time_zone);
            time_t now = time(nullptr);
            config->_int.esp_signer_clock_rdy = now > ut->default_ts;

            if (config->_int.esp_signer_clock_rdy)
                config->signer.step = esp_signer_jwt_generation_step_encode_header_payload;
        }
        else if (config->signer.step == esp_signer_jwt_generation_step_encode_header_payload)
        {
            if (createJWT())
                config->signer.step = esp_signer_jwt_generation_step_sign;
        }
        else if (config->signer.step == esp_signer_jwt_generation_step_sign)
        {
            if (createJWT())
                config->signer.step = esp_signer_jwt_generation_step_exchange;
        }
        else if (config->signer.step == esp_signer_jwt_generation_step_exchange)
        {

            if (requestTokens())
            {
                config->signer.attempts = 0;
                _token_processing_task_enable = false;
                config->signer.step = esp_signer_jwt_generation_step_begin;
                ret = true;
            }
            else
            {
                config->signer.step = esp_signer_jwt_generation_step_begin;

                if (config->signer.attempts < config->max_token_generation_retry)
                    config->signer.attempts++;
                else
                {
                    config->signer.tokens.error.message.clear();
                    setTokenError(ESP_SIGNER_ERROR_TOKEN_EXCHANGE_MAX_RETRY_REACHED);
                    config->_int.esp_signer_last_jwt_generation_error_cb_millis = 0;
                    sendTokenStatusCB();
                    config->signer.attempts = 0;
                    config->signer.step = esp_signer_jwt_generation_step_begin;
                    ret = true;
                }
            }
        }
    }

    config->signer.tokenTaskRunning = false;

#elif defined(ESP8266)

    if (_token_processing_task_enable && config->signer.tokens.status != esp_signer_token_status_ready)
    {

        if (config->signer.step == esp_signer_jwt_generation_step_begin && (millis() - config->_int.esp_signer_last_jwt_begin_step_millis > 200 || config->_int.esp_signer_last_jwt_begin_step_millis == 0))
        {
            config->_int.esp_signer_last_jwt_begin_step_millis = millis();
            config->signer.tokenTaskRunning = true;
            ut->setClock(config->time_zone);
            time_t now = time(nullptr);
            config->_int.esp_signer_clock_rdy = now > ut->default_ts;

            if (config->_int.esp_signer_clock_rdy)
                config->signer.step = esp_signer_jwt_generation_step_encode_header_payload;
        }
        else if (config->signer.step == esp_signer_jwt_generation_step_encode_header_payload)
        {
            if (createJWT())
                config->signer.step = esp_signer_jwt_generation_step_sign;
        }
        else if (config->signer.step == esp_signer_jwt_generation_step_sign)
        {
            if (createJWT())
                config->signer.step = esp_signer_jwt_generation_step_exchange;
        }
        else if (config->signer.step == esp_signer_jwt_generation_step_exchange)
        {

            if (requestTokens())
            {
                config->signer.tokenTaskRunning = false;
                _token_processing_task_enable = false;
                config->signer.attempts = 0;
                config->signer.step = esp_signer_jwt_generation_step_begin;
                return;
            }
            else
            {
                if (config->signer.attempts < config->max_token_generation_retry)
                    config->signer.attempts++;
                else
                {
                    config->signer.tokens.error.message.clear();
                    setTokenError(ESP_SIGNER_ERROR_TOKEN_EXCHANGE_MAX_RETRY_REACHED);
                    config->_int.esp_signer_last_jwt_generation_error_cb_millis = 0;
                    sendTokenStatusCB();
                    config->signer.tokenTaskRunning = false;
                    _token_processing_task_enable = false;
                    config->signer.attempts = 0;
                    config->signer.step = esp_signer_jwt_generation_step_begin;
                    return;
                }
            }
        }

        set_scheduled_callback(std::bind(&ESP_Signer::tokenProcessingTask, this));
    }

#endif
}

void ESP_Signer::setTokenError(int code)
{
    if (code != 0)
        config->signer.tokens.status = esp_signer_token_status_error;
    else
    {
        config->signer.tokens.error.message.clear();
        config->signer.tokens.status = esp_signer_token_status_ready;
    }

    config->signer.tokens.error.code = code;

    if (config->signer.tokens.error.message.length() == 0)
    {
        config->_int.esp_signer_processing = false;
        switch (code)
        {
        case ESP_SIGNER_ERROR_TOKEN_SET_TIME:
            ut->appendP(config->signer.tokens.error.message, esp_signer_pgm_str_21, true);
            break;
        case ESP_SIGNER_ERROR_TOKEN_PARSE_PK:
            ut->appendP(config->signer.tokens.error.message, esp_signer_pgm_str_22, true);
            break;
        case ESP_SIGNER_ERROR_TOKEN_CREATE_HASH:
            ut->appendP(config->signer.tokens.error.message, esp_signer_pgm_str_23, true);
            break;
        case ESP_SIGNER_ERROR_TOKEN_SIGN:
            ut->appendP(config->signer.tokens.error.message, esp_signer_pgm_str_24, true);
            break;
        case ESP_SIGNER_ERROR_TOKEN_EXCHANGE:
            ut->appendP(config->signer.tokens.error.message, esp_signer_pgm_str_25, true);
            break;
        case ESP_SIGNER_ERROR_TOKEN_NOT_READY:
            ut->appendP(config->signer.tokens.error.message, esp_signer_pgm_str_26, true);
            break;
        case ESP_SIGNER_ERROR_TOKEN_EXCHANGE_MAX_RETRY_REACHED:
            ut->appendP(config->signer.tokens.error.message, esp_signer_pgm_str_27, true);
            break;
        case ESP_SIGNER_ERROR_TCP_ERROR_NOT_CONNECTED:
            ut->appendP(config->signer.tokens.error.message, esp_signer_pgm_str_28);
            break;
        case ESP_SIGNER_ERROR_TCP_ERROR_CONNECTION_LOST:
            ut->appendP(config->signer.tokens.error.message, esp_signer_pgm_str_29);
            break;
        case ESP_SIGNER_ERROR_HTTP_CODE_REQUEST_TIMEOUT:
            ut->appendP(config->signer.tokens.error.message, esp_signer_pgm_str_30);
            break;

        default:
            break;
        }
    }
}

bool ESP_Signer::sdBegin(int8_t ss, int8_t sck, int8_t miso, int8_t mosi)
{
    if (config)
    {
        config->_int.sd_config.sck = sck;
        config->_int.sd_config.miso = miso;
        config->_int.sd_config.mosi = mosi;
        config->_int.sd_config.ss = ss;
    }
#if defined(ESP32)
    if (ss > -1)
    {
        SPI.begin(sck, miso, mosi, ss);
        return SD_FS.begin(ss, SPI);
    }
    else
        return SD_FS.begin();
#elif defined(ESP8266)
    if (ss > -1)
        return SD_FS.begin(ss);
    else
        return SD_FS.begin(SD_CS_PIN);
#endif
    return false;
}

bool ESP_Signer::sdMMCBegin(const char *mountpoint, bool mode1bit, bool format_if_mount_failed)
{
#if defined(ESP32)
#if defined(CARD_TYPE_SD_MMC)
    if (config)
    {
        config->_int.sd_config.sd_mmc_mountpoint = mountpoint;
        config->_int.sd_config.sd_mmc_mode1bit = mode1bit;
        config->_int.sd_config.sd_mmc_format_if_mount_failed = format_if_mount_failed;
    }
    return SD_FS.begin(mountpoint, mode1bit, format_if_mount_failed);
#endif
#endif
    return false;
}

bool ESP_Signer::handleSignerError(int code, int httpCode)
{

    switch (code)
    {

    case 1:
        config->signer.tokens.error.message.clear();
        setTokenError(ESP_SIGNER_ERROR_TCP_ERROR_NOT_CONNECTED);
        config->_int.esp_signer_last_jwt_generation_error_cb_millis = 0;
        sendTokenStatusCB();
        break;
    case 2:

        if (config->signer.wcs->stream())
            config->signer.wcs->stream()->stop();

        config->signer.tokens.error.message.clear();
        setTokenError(ESP_SIGNER_ERROR_TCP_ERROR_CONNECTION_LOST);
        config->_int.esp_signer_last_jwt_generation_error_cb_millis = 0;
        sendTokenStatusCB();
        break;
    case 3:

        if (config->signer.wcs->stream())
            config->signer.wcs->stream()->stop();

        if (httpCode == 0)
        {
            setTokenError(ESP_SIGNER_ERROR_HTTP_CODE_REQUEST_TIMEOUT);
            config->signer.tokens.error.message.clear();
        }
        else
        {
            errorToString(httpCode, config->signer.tokens.error.message);
            setTokenError(httpCode);
        }
        config->_int.esp_signer_last_jwt_generation_error_cb_millis = 0;
        sendTokenStatusCB();

        break;

    default:
        break;
    }

    if (config->signer.wcs)
        delete config->signer.wcs;
    if (config->signer.json)
        delete config->signer.json;
    if (config->signer.result)
        delete config->signer.result;

    config->signer.wcs= NULL;
    config->signer.json = NULL;
    config->signer.result = NULL;

    config->_int.esp_signer_processing = false;

    if (code > 0 && code < 4)
    {
        config->signer.tokens.status = esp_signer_token_status_error;
        config->signer.tokens.error.code = code;
        return false;
    }
    else if (code <= 0)
    {
        config->signer.tokens.error.message.clear();
        config->signer.tokens.status = esp_signer_token_status_ready;
        config->signer.attempts = 0;
        config->signer.step = esp_signer_jwt_generation_step_begin;
        config->_int.esp_signer_last_jwt_generation_error_cb_millis = 0;
        if (code == 0)
            sendTokenStatusCB();
        return true;
    }

    return false;
}

void ESP_Signer::sendTokenStatusCB()
{
    tokenInfo.status = config->signer.tokens.status;
    tokenInfo.type = config->signer.tokens.token_type;
    tokenInfo.error = config->signer.tokens.error;

    if (config->token_status_callback)
    {
        if (millis() - config->_int.esp_signer_last_jwt_generation_error_cb_millis > config->timeout.tokenGenerationError || config->_int.esp_signer_last_jwt_generation_error_cb_millis == 0)
        {
            config->_int.esp_signer_last_jwt_generation_error_cb_millis = millis();
            config->token_status_callback(tokenInfo);
        }
    }
}

bool ESP_Signer::parseJsonResponse(PGM_P key_path)
{
    char *tmp = ut->strP(key_path);
    config->signer.result->clear();
    config->signer.json->get(*config->signer.result, tmp);
    ut->delP(&tmp);
    return config->signer.result->success;
}

bool ESP_Signer::handleServerResponse(int &httpCode)
{
    if (config->_int.esp_signer_reconnect_wifi)
        ut->reconnect(0);

    if (WiFi.status() != WL_CONNECTED && !ut->ethLinkUp(&config->spi_ethernet_module))
        return false;

    struct esp_signer_server_response_data_t response;

    unsigned long dataTime = millis();

    int chunkIdx = 0;
    int chunkBufSize = 0;
    int chunkedDataState = 0;
    int chunkedDataSize = 0;
    int chunkedDataLen = 0;
    MBSTRING header, payload;
    bool isHeader = false;

    WiFiClient *stream = config->signer.wcs->stream();

    while (stream->connected() && stream->available() == 0)
    {
        if (!ut->reconnect(dataTime))
        {
            if (stream)
                if (stream->connected())
                    stream->stop();
            return false;
        }

        ut->idle();
    }

    bool complete = false;
    unsigned long datatime = millis();
    while (!complete)
    {

        chunkBufSize = stream->available();

        if (chunkBufSize > 1 || !complete)
        {
            while (!complete)
            {
                ut->idle();

                if (config->_int.esp_signer_reconnect_wifi)
                    ut->reconnect(0);

                if (WiFi.status() != WL_CONNECTED && !ut->ethLinkUp(&config->spi_ethernet_module))
                {
                    if (stream)
                        if (stream->connected())
                            stream->stop();
                    return false;
                }
                chunkBufSize = stream->available();

                if (chunkBufSize > 0)
                {
                    if (chunkIdx == 0)
                    {
                        ut->readLine(stream, header);
                        int pos = 0;
                        char *tmp = ut->getHeader(header.c_str(), esp_signer_pgm_str_31, esp_signer_pgm_str_32, pos, 0);
                        if (tmp)
                        {
                            isHeader = true;
                            response.httpCode = atoi(tmp);
                            ut->delP(&tmp);
                        }
                    }
                    else
                    {
                        if (isHeader)
                        {
                            char *tmp = (char *)ut->newP(chunkBufSize);
                            int readLen = ut->readLine(stream, tmp, chunkBufSize);
                            bool headerEnded = false;

                            if (readLen == 1)
                                if (tmp[0] == '\r')
                                    headerEnded = true;

                            if (readLen == 2)
                                if (tmp[0] == '\r' && tmp[1] == '\n')
                                    headerEnded = true;

                            if (headerEnded)
                            {
                                isHeader = false;
                                ut->parseRespHeader(header.c_str(), response);
                                header.clear();
                            }
                            else
                                header += tmp;

                            ut->delP(&tmp);
                        }
                        else
                        {
                            if (!response.noContent)
                            {
                                if (response.isChunkedEnc)
                                    complete = ut->readChunkedData(stream, payload, chunkedDataState, chunkedDataSize, chunkedDataLen) < 0;
                                else
                                {
                                    chunkBufSize = 1024;
                                    if (stream->available() < chunkBufSize)
                                        chunkBufSize = stream->available();

                                    char *tmp = (char *)ut->newP(chunkBufSize + 1);
                                    int readLen = stream->readBytes(tmp, chunkBufSize);

                                    if (readLen > 0)
                                        payload += tmp;

                                    ut->delP(&tmp);
                                    complete = stream->available() <= 0;
                                }
                            }
                            else
                            {
                                while (stream->available() > 0)
                                    stream->read();
                                if (stream->available() <= 0)
                                    break;
                            }
                        }
                    }
                    chunkIdx++;
                }

                if (millis() - datatime > 5000)
                    complete = true;
            }
        }
    }

    if (stream->connected())
        stream->stop();

    httpCode = response.httpCode;

    if (payload.length() > 0 && !response.noContent)
    {

        config->signer.json->setJsonData(payload.c_str());
        payload.clear();
        return true;
    }

    return false;
}

bool ESP_Signer::createJWT()
{

    if (config->signer.step == esp_signer_jwt_generation_step_encode_header_payload)
    {
        config->signer.tokens.status = esp_signer_token_status_on_signing;
        config->signer.tokens.error.code = 0;
        config->signer.tokens.error.message.clear();
        config->_int.esp_signer_last_jwt_generation_error_cb_millis = 0;
        sendTokenStatusCB();

        config->signer.json = new FirebaseJson();
        config->signer.result = new FirebaseJsonData();

        unsigned long now = time(nullptr);

        config->signer.tokens.jwt.clear();

        //header
        char *tmp = ut->strP(esp_signer_pgm_str_33);
        char *tmp2 = ut->strP(esp_signer_pgm_str_34);
        config->signer.json->add(tmp, (const char *)tmp2);
        ut->delP(&tmp);
        ut->delP(&tmp2);
        tmp2 = ut->strP(esp_signer_pgm_str_35);
        tmp = ut->strP(esp_signer_pgm_str_36);
        config->signer.json->add(tmp, (const char *)tmp2);
        ut->delP(&tmp);
        ut->delP(&tmp2);

        MBSTRING hdr;
        config->signer.json->toString(hdr);
        size_t len = ut->base64EncLen(hdr.length());
        char *buf = (char *)ut->newP(len);
        ut->encodeBase64Url(buf, (unsigned char *)hdr.c_str(), hdr.length());
        config->signer.encHeader = buf;
        ut->delP(&buf);
        config->signer.encHeadPayload = config->signer.encHeader;
        hdr.clear();

        //payload
        config->signer.json->clear();
        tmp = ut->strP(esp_signer_pgm_str_37);
        config->signer.json->add(tmp, config->service_account.data.client_email.c_str());
        ut->delP(&tmp);
        tmp = ut->strP(esp_signer_pgm_str_38);
        config->signer.json->add(tmp, config->service_account.data.client_email.c_str());
        ut->delP(&tmp);
        tmp = ut->strP(esp_signer_pgm_str_39);
        MBSTRING t;
        ut->appendP(t, esp_signer_pgm_str_40);
        if (config->signer.tokens.token_type == esp_signer_token_type_oauth2_access_token)
        {
            ut->appendP(t, esp_signer_pgm_str_41);
            ut->appendP(t, esp_signer_pgm_str_42);
            ut->appendP(t, esp_signer_pgm_str_43);
            ut->appendP(t, esp_signer_pgm_str_44);
            ut->appendP(t, esp_signer_pgm_str_45);
        }

        config->signer.json->add(tmp, t.c_str());
        ut->delP(&tmp);

        tmp = ut->strP(esp_signer_pgm_str_46);
        config->signer.json->add(tmp, (int)now);
        ut->delP(&tmp);

        tmp = ut->strP(esp_signer_pgm_str_47);

        if (config->signer.expiredSeconds > 3600)
            config->signer.json->add(tmp, (int)(now + 3600));
        else
            config->signer.json->add(tmp, (int)(now + config->signer.expiredSeconds));

        ut->delP(&tmp);

        if (config->signer.tokens.token_type == esp_signer_token_type_oauth2_access_token)
        {

            MBSTRING s;

            if (config->signer.tokens.scope.length() > 0)
            {
                std::vector<MBSTRING> scopes = std::vector<MBSTRING>();
                ut->splitTk(config->signer.tokens.scope, scopes, ",");
                for (size_t i = 0; i < scopes.size(); i++)
                {
                    if (s.length() > 0)
                        ut->appendP(s, esp_signer_pgm_str_32);
                    s += scopes[i];
                    scopes[i].clear();
                }
                scopes.clear();
            }

            tmp = ut->strP(esp_signer_pgm_str_56);
            config->signer.json->add(tmp, s.c_str());
            ut->delP(&tmp);
        }

        MBSTRING payload;
        config->signer.json->toString(payload);

        len = ut->base64EncLen(payload.length());
        buf = (char *)ut->newP(len);
        ut->encodeBase64Url(buf, (unsigned char *)payload.c_str(), payload.length());
        config->signer.encPayload = buf;
        ut->delP(&buf);
        payload.clear();

        ut->appendP(config->signer.encHeadPayload, esp_signer_pgm_str_42);
        config->signer.encHeadPayload += config->signer.encPayload;

        config->signer.encHeader.clear();
        config->signer.encPayload.clear();

//create message digest from encoded header and payload
#if defined(ESP32)
        config->signer.hash = (uint8_t *)ut->newP(config->signer.hashSize);
        int ret = mbedtls_md(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), (const unsigned char *)config->signer.encHeadPayload.c_str(), config->signer.encHeadPayload.length(), config->signer.hash);
        if (ret != 0)
        {
            char *tmp = (char *)ut->newP(100);
            mbedtls_strerror(ret, tmp, 100);
            config->signer.tokens.error.message = tmp;
            config->signer.tokens.error.message.insert(0, (const char *)FPSTR("mbedTLS, mbedtls_md: "));
            ut->delP(&tmp);
            setTokenError(ESP_SIGNER_ERROR_TOKEN_CREATE_HASH);
            sendTokenStatusCB();
            ut->delP(&config->signer.hash);
            return false;
        }
#elif defined(ESP8266)
        config->signer.hash = (char *)ut->newP(config->signer.hashSize);
        br_sha256_context mc;
        br_sha256_init(&mc);
        br_sha256_update(&mc, config->signer.encHeadPayload.c_str(), config->signer.encHeadPayload.length());
        br_sha256_out(&mc, config->signer.hash);
#endif

        config->signer.tokens.jwt = config->signer.encHeadPayload;
        ut->appendP(config->signer.tokens.jwt, esp_signer_pgm_str_42);
        config->signer.encHeadPayload.clear();

        delete config->signer.json;
        delete config->signer.result;
    }
    else if (config->signer.step == esp_signer_jwt_generation_step_sign)
    {
        config->signer.tokens.status = esp_signer_token_status_on_signing;

#if defined(ESP32)
        config->signer.pk_ctx = new mbedtls_pk_context();
        mbedtls_pk_init(config->signer.pk_ctx);

        //parse priv key
        int ret = 0;
        if (config->signer.pk.length() > 0)
            ret = mbedtls_pk_parse_key(config->signer.pk_ctx, (const unsigned char *)config->signer.pk.c_str(), config->signer.pk.length() + 1, NULL, 0);
        else if (strlen_P(config->service_account.data.private_key) > 0)
            ret = mbedtls_pk_parse_key(config->signer.pk_ctx, (const unsigned char *)config->service_account.data.private_key, strlen_P(config->service_account.data.private_key) + 1, NULL, 0);

        if (ret != 0)
        {
            char *tmp = (char *)ut->newP(100);
            mbedtls_strerror(ret, tmp, 100);
            config->signer.tokens.error.message = tmp;
            config->signer.tokens.error.message.insert(0, (const char *)FPSTR("mbedTLS, mbedtls_pk_parse_key: "));
            ut->delP(&tmp);
            setTokenError(ESP_SIGNER_ERROR_TOKEN_PARSE_PK);
            sendTokenStatusCB();
            mbedtls_pk_free(config->signer.pk_ctx);
            ut->delP(&config->signer.hash);
            delete config->signer.pk_ctx;
            return false;
        }

        //generate RSA signature from private key and message digest
        config->signer.signature = (unsigned char *)ut->newP(config->signer.signatureSize);
        size_t sigLen = 0;
        config->signer.entropy_ctx = new mbedtls_entropy_context();
        config->signer.ctr_drbg_ctx = new mbedtls_ctr_drbg_context();
        mbedtls_entropy_init(config->signer.entropy_ctx);
        mbedtls_ctr_drbg_init(config->signer.ctr_drbg_ctx);
        mbedtls_ctr_drbg_seed(config->signer.ctr_drbg_ctx, mbedtls_entropy_func, config->signer.entropy_ctx, NULL, 0);

        ret = mbedtls_pk_sign(config->signer.pk_ctx, MBEDTLS_MD_SHA256, (const unsigned char *)config->signer.hash, config->signer.hashSize, config->signer.signature, &sigLen, mbedtls_ctr_drbg_random, config->signer.ctr_drbg_ctx);
        if (ret != 0)
        {
            char *tmp = (char *)ut->newP(100);
            mbedtls_strerror(ret, tmp, 100);
            config->signer.tokens.error.message = tmp;
            config->signer.tokens.error.message.insert(0, (const char *)FPSTR("mbedTLS, mbedtls_pk_sign: "));
            ut->delP(&tmp);
            setTokenError(ESP_SIGNER_ERROR_TOKEN_SIGN);
            sendTokenStatusCB();
        }
        else
        {
            config->signer.encSignature.clear();
            size_t len = ut->base64EncLen(config->signer.signatureSize);
            char *buf = (char *)ut->newP(len);
            ut->encodeBase64Url(buf, config->signer.signature, config->signer.signatureSize);
            config->signer.encSignature = buf;
            ut->delP(&buf);

            config->signer.tokens.jwt += config->signer.encSignature;
            config->signer.pk.clear();
            config->signer.encSignature.clear();
        }

        ut->delP(&config->signer.signature);
        ut->delP(&config->signer.hash);
        mbedtls_pk_free(config->signer.pk_ctx);
        mbedtls_entropy_free(config->signer.entropy_ctx);
        mbedtls_ctr_drbg_free(config->signer.ctr_drbg_ctx);
        delete config->signer.pk_ctx;
        delete config->signer.entropy_ctx;
        delete config->signer.ctr_drbg_ctx;

        if (ret != 0)
            return false;
#elif defined(ESP8266)
        //RSA private key
        BearSSL::PrivateKey *pk = nullptr;
        ut->idle();
        //parse priv key
        if (config->signer.pk.length() > 0)
            pk = new BearSSL::PrivateKey((const char *)config->signer.pk.c_str());
        else if (strlen_P(config->service_account.data.private_key) > 0)
            pk = new BearSSL::PrivateKey((const char *)config->service_account.data.private_key);

        if (!pk)
        {
            setTokenError(ESP_SIGNER_ERROR_TOKEN_PARSE_PK);
            config->signer.tokens.error.message.insert(0, (const char *)FPSTR("BearSSL, PrivateKey: "));
            sendTokenStatusCB();
            return false;
        }

        if (!pk->isRSA())
        {
            setTokenError(ESP_SIGNER_ERROR_TOKEN_PARSE_PK);
            config->signer.tokens.error.message.insert(0, (const char *)FPSTR("BearSSL, isRSA: "));
            sendTokenStatusCB();
            delete pk;
            return false;
        }

        const br_rsa_private_key *br_rsa_key = pk->getRSA();

        //generate RSA signature from private key and message digest
        config->signer.signature = new unsigned char[config->signer.signatureSize];

        ut->idle();
        int ret = br_rsa_i15_pkcs1_sign(BR_HASH_OID_SHA256, (const unsigned char *)config->signer.hash, br_sha256_SIZE, br_rsa_key, config->signer.signature);
        ut->idle();
        ut->delP(&config->signer.hash);

        size_t len = ut->base64EncLen(config->signer.signatureSize);
        char *buf = (char *)ut->newP(len);
        ut->encodeBase64Url(buf, config->signer.signature, config->signer.signatureSize);
        config->signer.encSignature = buf;
        ut->delP(&buf);
        ut->delP(&config->signer.signature);
        delete pk;
        //get the signed JWT
        if (ret > 0)
        {
            config->signer.tokens.jwt += config->signer.encSignature;
            config->signer.pk.clear();
            config->signer.encSignature.clear();
        }
        else
        {
            setTokenError(ESP_SIGNER_ERROR_TOKEN_SIGN);
            config->signer.tokens.error.message.insert(0, (const char *)FPSTR("BearSSL, br_rsa_i15_pkcs1_sign: "));
            sendTokenStatusCB();
            return false;
        }
#endif
    }

    return true;
}

bool ESP_Signer::requestTokens()
{

    if (config->_int.esp_signer_reconnect_wifi)
        ut->reconnect(0);

    if (WiFi.status() != WL_CONNECTED && !ut->ethLinkUp(&config->spi_ethernet_module))
        return false;

    ut->idle();

    if (config->signer.tokens.status == esp_signer_token_status_on_request || config->signer.tokens.status == esp_signer_token_status_on_refresh || time(nullptr) < ut->default_ts || config->_int.esp_signer_processing)
        return false;

    config->signer.tokens.status = esp_signer_token_status_on_request;
    config->_int.esp_signer_processing = true;
    config->signer.tokens.error.code = 0;
    config->signer.tokens.error.message.clear();
    config->_int.esp_signer_last_jwt_generation_error_cb_millis = 0;
    sendTokenStatusCB();

    if (config->signer.wcs)
        delete config->signer.wcs;

    config->signer.wcs = new ESP_SIGNER_TCP_Client();
    config->signer.wcs->setCACert(nullptr);

    config->signer.json = new FirebaseJson();
    config->signer.result = new FirebaseJsonData();

    MBSTRING host;
    ut->appendP(host, esp_signer_pgm_str_48);
    ut->appendP(host, esp_signer_pgm_str_42);
    ut->appendP(host, esp_signer_pgm_str_43);

    ut->idle();
#if defined(ESP8266)
    ut->ethDNSWorkAround(&ut->config->spi_ethernet_module, host.c_str(), 443);
#endif

    config->signer.wcs->begin(host.c_str(), 443);


    MBSTRING req;
    ut->appendP(req, esp_signer_pgm_str_57);
    ut->appendP(req, esp_signer_pgm_str_32);

    if (config->signer.tokens.token_type == esp_signer_token_type_oauth2_access_token)
    {
        char *tmp = ut->strP(esp_signer_pgm_str_58);
        char *tmp2 = ut->strP(esp_signer_pgm_str_59);
        config->signer.json->add(tmp, (const char *)tmp2);
        ut->delP(&tmp);
        ut->delP(&tmp2);
        tmp = ut->strP(esp_signer_pgm_str_60);
        config->signer.json->add(tmp, config->signer.tokens.jwt.c_str());
        ut->delP(&tmp);

        ut->appendP(req, esp_signer_pgm_str_44);
        ut->appendP(req, esp_signer_pgm_str_45);
        ut->appendP(req, esp_signer_pgm_str_61);
        ut->appendP(req, esp_signer_pgm_str_62);
        ut->appendP(req, esp_signer_pgm_str_41);
    }

    ut->appendP(req, esp_signer_pgm_str_42);
    ut->appendP(req, esp_signer_pgm_str_43);

    ut->appendP(req, esp_signer_pgm_str_4);
    ut->appendP(req, esp_signer_pgm_str_64);
    ut->appendP(req, esp_signer_pgm_str_65);
    req += NUM2S(strlen(config->signer.json->raw())).get();
    ut->appendP(req, esp_signer_pgm_str_4);
    ut->appendP(req, esp_signer_pgm_str_66);
    ut->appendP(req, esp_signer_pgm_str_67);
    ut->appendP(req, esp_signer_pgm_str_4);
    ut->appendP(req, esp_signer_pgm_str_4);

    req += config->signer.json->raw();

    int ret = config->signer.wcs->send(req.c_str());
    req.clear();
    if (ret < 0)
        return handleSignerError(2);

    struct esp_signer_auth_token_error_t error;

    int httpCode = 0;
    if (handleServerResponse(httpCode))
    {
        config->signer.tokens.jwt.clear();
        if (parseJsonResponse(esp_signer_pgm_str_68))
        {

            error.code = config->signer.result->to<int>();
            config->signer.tokens.status = esp_signer_token_status_error;

            if (parseJsonResponse(esp_signer_pgm_str_69))
                error.message = config->signer.result->to<const char *>();
        }
        else if (parseJsonResponse(esp_signer_pgm_str_113))
        {

            error.code = -1;
            config->signer.tokens.status = esp_signer_token_status_error;

            if (parseJsonResponse(esp_signer_pgm_str_12))
                error.message = config->signer.result->to<const char *>();
        }

        if (error.code != 0 && config->signer.tokens.token_type == esp_signer_token_type_oauth2_access_token)
        {
            //new jwt needed as it is already cleared
            config->signer.step = esp_signer_jwt_generation_step_encode_header_payload;
        }

        config->signer.tokens.error = error;
        tokenInfo.status = config->signer.tokens.status;
        tokenInfo.type = config->signer.tokens.token_type;
        tokenInfo.error = config->signer.tokens.error;
        config->_int.esp_signer_last_jwt_generation_error_cb_millis = 0;
        if (error.code != 0)
            sendTokenStatusCB();

        if (error.code == 0)
        {
            if (config->signer.tokens.token_type == esp_signer_token_type_oauth2_access_token)
            {
                if (parseJsonResponse(esp_signer_pgm_str_70))
                    config->signer.tokens.access_token = config->signer.result->to<const char *>();

                if (parseJsonResponse(esp_signer_pgm_str_71))
                    config->signer.tokens.auth_type = config->signer.result->to<const char *>();

                if (parseJsonResponse(esp_signer_pgm_str_72))
                    getExpiration(config->signer.result->to<const char *>());
            }
            return handleSignerError(0);
        }
        return handleSignerError(4);
    }

    return handleSignerError(3, httpCode);
}

void ESP_Signer::getExpiration(const char *exp)
{
    time_t ts = time(nullptr);
    unsigned long ms = millis();
    config->signer.tokens.expires = ts + atoi(exp);
    config->signer.tokens.last_millis = ms;
}

void ESP_Signer::checkToken()
{
    if (!config)
        return;

    //if the time was set (changed) after token has been generated, update its expiration
    if (config->signer.tokens.expires > 0 && config->signer.tokens.expires < ESP_DEFAULT_TS && time(nullptr) > ESP_DEFAULT_TS)
        config->signer.tokens.expires += time(nullptr) - (millis() - config->signer.tokens.last_millis) / 1000 - 60;

    if (config->signer.preRefreshSeconds > config->signer.tokens.expires && config->signer.tokens.expires > 0)
        config->signer.preRefreshSeconds = 60;

    if (_token_processing_task_end_request && config->signer.tokens.status == esp_signer_token_status_ready)
    {

        config->signer.pk.clear();
        config->signer.tokens.jwt.clear();
        config->signer.tokens.access_token.clear();
        config->signer.tokens.token_type = esp_signer_token_type_undefined;

        config->signer.tokens.status = esp_signer_token_status_uninitialized;
        config->signer.tokens.error.code = 0;
        config->signer.tokens.error.message.clear();
        config->_int.esp_signer_last_jwt_generation_error_cb_millis = 0;
        config->signer.tokens.status = esp_signer_token_status_uninitialized;
        _token_processing_task_end_request = false;
        _token_processing_task_enable = false;
    }

    if (config->signer.tokens.token_type == esp_signer_token_type_oauth2_access_token && ((unsigned long)time(nullptr) > config->signer.tokens.expires - config->signer.preRefreshSeconds || config->signer.tokens.expires == 0))
        handleToken();
}

bool ESP_Signer::tokenReady()
{
    if (!config)
        return false;

    checkToken();
    return config->signer.tokens.status == esp_signer_token_status_ready;
};

String ESP_Signer::accessToken()
{
    if (!config)
        return "";
    return config->signer.tokens.access_token.c_str();
}

String ESP_Signer::getTokenType(TokenInfo info)
{
    if (!config)
        return "";

    MBSTRING s;
    switch (info.type)
    {
    case esp_signer_token_type_undefined:
        ut->appendP(s, esp_signer_pgm_str_49);
        break;
    case esp_signer_token_type_oauth2_access_token:
        ut->appendP(s, esp_signer_pgm_str_50);
        break;
    default:
        break;
    }
    return s.c_str();
}

String ESP_Signer::getTokenType()
{
    return getTokenType(tokenInfo);
}

String ESP_Signer::getTokenStatus(TokenInfo info)
{
    if (!config)
        return "";

    MBSTRING s;
    switch (info.status)
    {
    case esp_signer_token_status_uninitialized:
        ut->appendP(s, esp_signer_pgm_str_51);
        break;

    case esp_signer_token_status_on_initialize:
        ut->appendP(s, esp_signer_pgm_str_52);
        break;
    case esp_signer_token_status_on_signing:
        ut->appendP(s, esp_signer_pgm_str_53);
        break;
    case esp_signer_token_status_on_request:
        ut->appendP(s, esp_signer_pgm_str_54);
        break;
    case esp_signer_token_status_on_refresh:
        ut->appendP(s, esp_signer_pgm_str_55);
        break;
    case esp_signer_token_status_ready:
        ut->appendP(s, esp_signer_pgm_str_112);
        break;
    case esp_signer_token_status_error:
        ut->appendP(s, esp_signer_pgm_str_113);
        break;
    default:
        break;
    }
    return s.c_str();
}

String ESP_Signer::getTokenStatus()
{
    return getTokenStatus(tokenInfo);
}

String ESP_Signer::getTokenError(TokenInfo info)
{
    if (!config)
        return "";

    MBSTRING s;
    ut->appendP(s, esp_signer_pgm_str_114);
    s += NUM2S(info.error.code).get();
    ut->appendP(s, esp_signer_pgm_str_115);
    s += info.error.message;
    return s.c_str();
}

String ESP_Signer::getTokenError()
{
    return getTokenError(tokenInfo);
}

unsigned long ESP_Signer::getExpiredTimestamp()
{
    return config->signer.tokens.expires;
}

void ESP_Signer::refreshToken()
{
    config->signer.tokens.expires = 0;
    checkToken();
}

void ESP_Signer::errorToString(int httpCode, MBSTRING &buff)
{
    buff.clear();

    if (config)
    {
        if (config->signer.tokens.status == esp_signer_token_status_error || config->signer.tokens.error.code != 0)
        {
            buff = config->signer.tokens.error.message;
            return;
        }
    }

    switch (httpCode)
    {
    case ESP_SIGNER_ERROR_TCP_ERROR_CONNECTION_REFUSED:
        ut->appendP(buff, esp_signer_pgm_str_73);
        return;
    case ESP_SIGNER_ERROR_TCP_ERROR_SEND_HEADER_FAILED:
        ut->appendP(buff, esp_signer_pgm_str_74);
        return;
    case ESP_SIGNER_ERROR_TCP_ERROR_SEND_PAYLOAD_FAILED:
        ut->appendP(buff, esp_signer_pgm_str_75);
        return;
    case ESP_SIGNER_ERROR_TCP_ERROR_NOT_CONNECTED:
        ut->appendP(buff, esp_signer_pgm_str_28);
        return;
    case ESP_SIGNER_ERROR_TCP_ERROR_CONNECTION_LOST:
        ut->appendP(buff, esp_signer_pgm_str_29);
        return;
    case ESP_SIGNER_ERROR_TCP_ERROR_NO_HTTP_SERVER:
        ut->appendP(buff, esp_signer_pgm_str_76);
        return;
    case ESP_SIGNER_ERROR_HTTP_CODE_BAD_REQUEST:
        ut->appendP(buff, esp_signer_pgm_str_77);
        return;
    case ESP_SIGNER_ERROR_HTTP_CODE_NON_AUTHORITATIVE_INFORMATION:
        ut->appendP(buff, esp_signer_pgm_str_78);
        return;
    case ESP_SIGNER_ERROR_HTTP_CODE_NO_CONTENT:
        ut->appendP(buff, esp_signer_pgm_str_79);
        return;
    case ESP_SIGNER_ERROR_HTTP_CODE_MOVED_PERMANENTLY:
        ut->appendP(buff, esp_signer_pgm_str_80);
        return;
    case ESP_SIGNER_ERROR_HTTP_CODE_USE_PROXY:
        ut->appendP(buff, esp_signer_pgm_str_81);
        return;
    case ESP_SIGNER_ERROR_HTTP_CODE_TEMPORARY_REDIRECT:
        ut->appendP(buff, esp_signer_pgm_str_82);
        return;
    case ESP_SIGNER_ERROR_HTTP_CODE_PERMANENT_REDIRECT:
        ut->appendP(buff, esp_signer_pgm_str_83);
        return;
    case ESP_SIGNER_ERROR_HTTP_CODE_UNAUTHORIZED:
        ut->appendP(buff, esp_signer_pgm_str_84);
        return;
    case ESP_SIGNER_ERROR_HTTP_CODE_FORBIDDEN:
        ut->appendP(buff, esp_signer_pgm_str_85);
        return;
    case ESP_SIGNER_ERROR_HTTP_CODE_NOT_FOUND:
        ut->appendP(buff, esp_signer_pgm_str_86);
        return;
    case ESP_SIGNER_ERROR_HTTP_CODE_METHOD_NOT_ALLOWED:
        ut->appendP(buff, esp_signer_pgm_str_87);
        return;
    case ESP_SIGNER_ERROR_HTTP_CODE_NOT_ACCEPTABLE:
        ut->appendP(buff, esp_signer_pgm_str_88);
        return;
    case ESP_SIGNER_ERROR_HTTP_CODE_PROXY_AUTHENTICATION_REQUIRED:
        ut->appendP(buff, esp_signer_pgm_str_89);
        return;
    case ESP_SIGNER_ERROR_HTTP_CODE_REQUEST_TIMEOUT:
        ut->appendP(buff, esp_signer_pgm_str_30);
        return;
    case ESP_SIGNER_ERROR_HTTP_CODE_LENGTH_REQUIRED:
        ut->appendP(buff, esp_signer_pgm_str_90);
        return;
    case ESP_SIGNER_ERROR_HTTP_CODE_TOO_MANY_REQUESTS:
        ut->appendP(buff, esp_signer_pgm_str_91);
        return;
    case ESP_SIGNER_ERROR_HTTP_CODE_REQUEST_HEADER_FIELDS_TOO_LARGE:
        ut->appendP(buff, esp_signer_pgm_str_92);
        return;
    case ESP_SIGNER_ERROR_HTTP_CODE_INTERNAL_SERVER_ERROR:
        ut->appendP(buff, esp_signer_pgm_str_93);
        return;
    case ESP_SIGNER_ERROR_HTTP_CODE_BAD_GATEWAY:
        ut->appendP(buff, esp_signer_pgm_str_94);
        return;
    case ESP_SIGNER_ERROR_HTTP_CODE_SERVICE_UNAVAILABLE:
        ut->appendP(buff, esp_signer_pgm_str_95);
        return;
    case ESP_SIGNER_ERROR_HTTP_CODE_GATEWAY_TIMEOUT:
        ut->appendP(buff, esp_signer_pgm_str_96);
        return;
    case ESP_SIGNER_ERROR_HTTP_CODE_HTTP_VERSION_NOT_SUPPORTED:
        ut->appendP(buff, esp_signer_pgm_str_97);
        return;
    case ESP_SIGNER_ERROR_HTTP_CODE_NETWORK_AUTHENTICATION_REQUIRED:
        ut->appendP(buff, esp_signer_pgm_str_98);
        return;
    case ESP_SIGNER_ERROR_HTTP_CODE_PRECONDITION_FAILED:
        ut->appendP(buff, esp_signer_pgm_str_99);
        return;
    case ESP_SIGNER_ERROR_TCP_RESPONSE_PAYLOAD_READ_TIMED_OUT:
        ut->appendP(buff, esp_signer_pgm_str_100);
        return;
    case ESP_SIGNER_ERROR_TCP_ERROR_CONNECTION_INUSED:
        ut->appendP(buff, esp_signer_pgm_str_101);
        return;
    case ESP_SIGNER_ERROR_BUFFER_OVERFLOW:
        ut->appendP(buff, esp_signer_pgm_str_102);
        return;
    case ESP_SIGNER_ERROR_HTTP_CODE_PAYLOAD_TOO_LARGE:
        ut->appendP(buff, esp_signer_pgm_str_103);
        return;
    case ESP_SIGNER_ERROR_FILE_IO_ERROR:
        ut->appendP(buff, esp_signer_pgm_str_104);
        return;
    case ESP_SIGNER_ERROR_FILE_NOT_FOUND:
        ut->appendP(buff, esp_signer_pgm_str_105);
        return;
    case ESP_SIGNER_ERROR_TOKEN_NOT_READY:
        ut->appendP(buff, esp_signer_pgm_str_26);
        return;
    case ESP_SIGNER_ERROR_UNINITIALIZED:
        ut->appendP(buff, esp_signer_pgm_str_106);
        return;
    default:
        return;
    }
}

bool ESP_Signer::setSystemTime(time_t ts)
{
    return ut->setTimestamp(ts) == 0;
}

ESP_Signer Signer = ESP_Signer();

#endif