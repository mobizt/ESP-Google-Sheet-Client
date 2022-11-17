/**
 * Google Sheet Client, ESP_Google_Sheet_Client.cpp v1.2.0
 *
 * This library supports Espressif ESP8266 and ESP32 MCUs
 *
 * Created November 17, 2022
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

#ifndef GSheetClass_CPP
#define GSheetClass_CPP

#include "ESP_Google_Sheet_Client.h"

GSheetClass::GSheetClass()
{
}

GSheetClass::~GSheetClass()
{
    if (config.signer.wcs)
        delete config.signer.wcs;
    if (config.signer.json)
        delete config.signer.json;
    if (config.signer.result)
        delete config.signer.result;

    config.signer.wcs = NULL;
    config.signer.json = NULL;
    config.signer.result = NULL;
}

void GSheetClass::auth(const char *client_email, const char *project_id, const char *private_key)
{
    config.service_account.data.client_email = client_email;
    config.service_account.data.project_id = project_id;
    config.service_account.data.private_key = private_key;
    config.signer.expiredSeconds = 3600;
    config.signer.preRefreshSeconds = 60;
    config.internal.esp_signer_reconnect_wifi = WiFi.getAutoReconnect();
    config.signer.tokens.scope = (const char *)FPSTR("https://www.googleapis.com/auth/drive.metadata,https://www.googleapis.com/auth/drive.appdata,https://www.googleapis.com/auth/spreadsheets,https://www.googleapis.com/auth/drive,https://www.googleapis.com/auth/drive.file");
    this->begin(&config);
}

void GSheetClass::setTokenCallback(TokenStatusCallback callback)
{
    config.token_status_callback = callback;
}

bool GSheetClass::checkToken()
{
    return this->tokenReady();
}

bool GSheetClass::setClock(float gmtOffset)
{

    if (time(nullptr) > ESP_DEFAULT_TS && gmtOffset == config.internal.esp_signer_gmt_offset)
        return true;

    time_t now = time(nullptr);

    config.internal.esp_signer_clock_rdy = now > ESP_DEFAULT_TS;

    if (!config.internal.esp_signer_clock_rdy || gmtOffset != config.internal.esp_signer_gmt_offset)
    {
        configTime(gmtOffset * 3600, 0, "pool.ntp.org", "time.nist.gov");

        now = time(nullptr);
        unsigned long timeout = millis();
        while (now < ESP_DEFAULT_TS)
        {
            now = time(nullptr);
            if (now > ESP_DEFAULT_TS || millis() - timeout > 1000)
                break;
            delay(10);
        }
    }

    config.internal.esp_signer_clock_rdy = now > ESP_DEFAULT_TS;
    if (config.internal.esp_signer_clock_rdy)
        config.internal.esp_signer_gmt_offset = gmtOffset;

    return config.internal.esp_signer_clock_rdy;
}

void GSheetClass::beginRequest(MB_String &req, host_type_t host_type)
{

    if (!config.signer.wcs)
    {
        config.signer.wcs = new ESP_SIGNER_TCP_Client();
        config.signer.wcs->setCACert(nullptr);
    }

#if defined(ESP8266)
    if (host_type == host_type_sheet)
        ut->ethDNSWorkAround(&config.spi_ethernet_module, (const char *)FPSTR("sheets.googleapis.com"), 443);
    else if (host_type == host_type_drive)
        ut->ethDNSWorkAround(&config.spi_ethernet_module, (const char *)FPSTR("www.googleapis.com"), 443);
#endif

    if (host_type == host_type_sheet)
        config.signer.wcs->begin((const char *)FPSTR("sheets.googleapis.com"), 443);
    else if (host_type == host_type_drive)
        config.signer.wcs->begin((const char *)FPSTR("www.googleapis.com"), 443);

    setSecure();
}

void GSheetClass::addHeader(MB_String &req, host_type_t host_type, int len)
{
    req += FPSTR(" HTTP/1.1\r\n");
    if (host_type == host_type_sheet)
        req += FPSTR("Host: sheets.googleapis.com\r\n");
    else if (host_type == host_type_drive)
        req += FPSTR("Host: www.googleapis.com\r\n");
    req += FPSTR("Authorization: Bearer ");
    req += config.signer.tokens.access_token;
    req += FPSTR("\r\n");

    if (len > -1)
    {
        req += FPSTR("Content-Length: ");
        req += len;
        req += FPSTR("\r\n");

        req += FPSTR("Content-Type: application/json\r\n");
    }

    req += FPSTR("Connection: keep-alive\r\n");
    req += FPSTR("Keep-Alive: timeout=30, max=100\r\n");
    req += FPSTR("Accept-Encoding: identity;q=1,chunked;q=0.1,*;q=0\r\n");
}

void GSheetClass::setCert(const char *ca)
{
    int addr = reinterpret_cast<int>(ca);
    if (addr != cert_addr)
    {
        cert_updated = true;
        cert_addr = addr;
    }
}

void GSheetClass::setCertFile(const char *filename, esp_google_sheet_file_storage_type type)
{
    certFile = filename;
    certFileStorageType = type;
    cert_addr = 0;
    cert_updated = false;
}

void GSheetClass::setSecure()
{
    if (!config.signer.wcs)
        return;

    config.signer.wcs->timeout = 5 * 1000;

    if (config.signer.wcs->_certType == -1 || cert_updated)
    {

        if (!config.internal.esp_signer_clock_rdy && (cert_addr > 0))
        {

#if defined(ESP8266)
            setClock(0);
#endif
        }

        config.signer.wcs->_clockReady = config.internal.esp_signer_clock_rdy;

        if (certFile.length() > 0)
        {

#if defined(ESP8266)
            if (config.internal.sd_config.ss == -1)
                config.internal.sd_config.ss = SD_CS_PIN;
#endif
            int type = certFileStorageType == esP_google_sheet_file_storage_type_flash ? 1 : 2;
            config.signer.wcs->setCACertFile(certFile.c_str(), type, config.internal.sd_config);
        }
        else
        {
            if (cert_addr > 0)
            {
                config.signer.wcs->setCACert(reinterpret_cast<const char *>(cert_addr));
            }
            else
                config.signer.wcs->setCACert(NULL);
        }

        cert_updated = false;
    }
}

bool GSheetClass::processRequest(MB_String &req, MB_String &response, int &httpcode)
{

    int ret = config.signer.wcs->send(req.c_str());
    req.clear();
    config.signer.reuseSession = true;

    if (ret == 0)
    {
        if (this->handleServerResponse(httpcode, response))
            ret = 1;
    }

    config.signer.reuseSession = false;

    return ret;
}

bool GSheetClass::mGet(MB_String &response, const char *spreadsheetId, const char *ranges, const char *majorDimension, const char *valueRenderOption, const char *dateTimeRenderOption, operation_type_t type)
{
    if (!checkToken())
        return false;

    MB_String req;
    int httpcode = 0;

    beginRequest(req, host_type_sheet);

    if (type == operation_type_range)
    {
        req = FPSTR("GET /v4/spreadsheets/");
        req += spreadsheetId;
        req += FPSTR("/values/");
        req += ranges;

        addHeader(req, host_type_sheet);
        req += FPSTR("\r\n");
    }
    else if (type == operation_type_batch)
    {
        req = FPSTR("GET /v4/spreadsheets/");
        req += spreadsheetId;
        req += FPSTR("/values:batchGet");

        std::vector<MB_String> rngs = std::vector<MB_String>();
        MB_String rng = ranges;
        ut->splitTk(rng, rngs, ",");

        if (rngs.size() == 0)
            return false;

        MB_String s;

        for (size_t i = 0; i < rngs.size(); i++)
        {
            if (s.length() > 0)
                s += FPSTR("&");
            else
                s += FPSTR("?");

            s += FPSTR("ranges=");
            s += rngs[i].c_str();
        }

        req += s;
        s.clear();

        if (strlen(majorDimension) > 0)
        {
            req += FPSTR("&majorDimension=");
            req += majorDimension;
        }

        if (strlen(valueRenderOption) > 0)
        {
            req += FPSTR("&valueRenderOption=");
            req += valueRenderOption;
        }

        if (strlen(dateTimeRenderOption) > 0)
        {
            req += FPSTR("&dateTimeRenderOption=");
            req += dateTimeRenderOption;
        }

        addHeader(req, host_type_sheet);
        req += FPSTR("\r\n");
    }
    else if (type == operation_type_filter)
    {
        req = FPSTR("POST /v4/spreadsheets/");
        req += spreadsheetId;
        req += FPSTR("/values:batchGetByDataFilter");

        addHeader(req, host_type_sheet, strlen(ranges));

        req += FPSTR("\r\n");

        req += ranges;
    }

    return processRequest(req, response, httpcode);
}

bool GSheetClass::isError(MB_String &response)
{
    config.signer.json = new FirebaseJson();
    bool ret = false;
    if (config.signer.json->setJsonData(response))
    {
        config.signer.result = new FirebaseJsonData();
        ret = parseJsonResponse(esp_signer_pgm_str_68) || parseJsonResponse(esp_signer_pgm_str_113);
        delete config.signer.result;
        config.signer.result = nullptr;
    }

    delete config.signer.json;
    config.signer.json = nullptr;

    return ret;
}

bool GSheetClass::get(MB_String &response, const char *spreadsheetId, const char *range)
{
    return mGet(response, spreadsheetId, range, "", "", "", operation_type_range);
}

bool GSheetClass::batchGet(MB_String &response, const char *spreadsheetId, const char *ranges, const char *majorDimension, const char *valueRenderOption, const char *dateTimeRenderOption)
{
    return mGet(response, spreadsheetId, ranges, majorDimension, valueRenderOption, dateTimeRenderOption, operation_type_batch);
}

bool GSheetClass::batchGetByDataFilter(MB_String &response, const char *spreadsheetId, FirebaseJsonArray *dataFiltersArray, const char *majorDimension, const char *valueRenderOption, const char *dateTimeRenderOption)
{
    if (!checkToken())
        return false;

    if (dataFiltersArray)
    {
        FirebaseJson js;
        js.add(FPSTR("dataFilters"), *dataFiltersArray);

        if (strlen(majorDimension) > 0)
            js.add(FPSTR("majorDimension"), majorDimension);
        if (strlen(valueRenderOption) > 0)
            js.add(FPSTR("valueRenderOption"), valueRenderOption);
        if (strlen(dateTimeRenderOption) > 0)
            js.add(FPSTR("dateTimeRenderOption"), dateTimeRenderOption);
        return mGet(response, spreadsheetId, js.raw(), "", "", "", operation_type_filter);
    }

    return false;
}

bool GSheetClass::append(MB_String &response, const char *spreadsheetId, const char *range, FirebaseJson *valueRange, const char *valueInputOption, const char *insertDataOption, const char *includeValuesInResponse, const char *responseValueRenderOption, const char *responseDateTimeRenderOption)
{
    return mUpdate(true, operation_type_range, response, spreadsheetId, range, valueRange, valueInputOption, insertDataOption, includeValuesInResponse, responseValueRenderOption, responseDateTimeRenderOption);
}

bool GSheetClass::update(MB_String &response, const char *spreadsheetId, const char *range, FirebaseJson *valueRange, const char *valueInputOption, const char *includeValuesInResponse, const char *responseValueRenderOption, const char *responseDateTimeRenderOption)
{
    return mUpdate(false, operation_type_range, response, spreadsheetId, range, valueRange, valueInputOption, "", includeValuesInResponse, responseValueRenderOption, responseDateTimeRenderOption);
}

bool GSheetClass::_batchUpdate(MB_String &response, const char *spreadsheetId, FirebaseJsonArray *valueRangeArray, const char *valueInputOption, const char *includeValuesInResponse, const char *responseValueRenderOption, const char *responseDateTimeRenderOption)
{
    if (valueRangeArray)
    {
        FirebaseJson js;
        mUpdateInit(&js, valueRangeArray, valueInputOption, includeValuesInResponse, responseValueRenderOption, responseDateTimeRenderOption);
        return mUpdate(false, operation_type_batch, response, spreadsheetId, "", &js, "", "", "", "", "");
    }

    return false;
}

bool GSheetClass::batchUpdateByDataFilter(MB_String &response, const char *spreadsheetId, FirebaseJsonArray *DataFilterValueRangeArray, const char *valueInputOption, const char *includeValuesInResponse, const char *responseValueRenderOption, const char *responseDateTimeRenderOption)
{
    if (DataFilterValueRangeArray)
    {
        FirebaseJson js;
        mUpdateInit(&js, DataFilterValueRangeArray, valueInputOption, includeValuesInResponse, responseValueRenderOption, responseDateTimeRenderOption);
        return mUpdate(false, operation_type_filter, response, spreadsheetId, "", &js, "", "", "", "", "");
    }

    return false;
}

void GSheetClass::mUpdateInit(FirebaseJson *js, FirebaseJsonArray *rangeArr, const char *valueInputOption, const char *includeValuesInResponse, const char *responseValueRenderOption, const char *responseDateTimeRenderOption)
{
    js->add(FPSTR("data"), *rangeArr);

    if (strlen(valueInputOption) > 0)
        js->add(FPSTR("valueInputOption"), valueInputOption);

    if (strlen(includeValuesInResponse) > 0)
    {
        if (strcmp(includeValuesInResponse, (const char *)FPSTR("true")) == 0)
            js->add(FPSTR("includeValuesInResponse"), true);
        else
            js->add(FPSTR("includeValuesInResponse"), false);
    }

    if (strlen(responseValueRenderOption) > 0)
        js->add(FPSTR("responseValueRenderOption"), responseValueRenderOption);

    if (strlen(responseDateTimeRenderOption) > 0)
        js->add(FPSTR("responseDateTimeRenderOption"), responseDateTimeRenderOption);
}

bool GSheetClass::mUpdate(bool append, operation_type_t type, MB_String &response, const char *spreadsheetId, const char *range, FirebaseJson *valueRange, const char *valueInputOption, const char *insertDataOption, const char *includeValuesInResponse, const char *responseValueRenderOption, const char *responseDateTimeRenderOption)
{
    if (!checkToken())
        return false;

    MB_String req;
    int httpcode = 0;

    beginRequest(req, host_type_sheet);

    if (append || type == operation_type_batch || type == operation_type_filter)
        req = FPSTR("POST /v4/spreadsheets/");
    else
        req = FPSTR("PUT /v4/spreadsheets/");

    req += spreadsheetId;
    req += FPSTR("/values");

    if (type == operation_type_range)
    {
        req += FPSTR("/");
        req += range;
    }
    else if (type == operation_type_batch)
        req += FPSTR(":batchUpdate");
    else if (type == operation_type_filter)
        req += FPSTR(":batchUpdateByDataFilter");

    if (append)
        req += FPSTR(":append");

    if (append || type == operation_type_range)
    {
        req += FPSTR("?valueInputOption=");
        req += valueInputOption;

        if (strlen(insertDataOption) > 0)
        {
            req += FPSTR("&insertDataOption=");
            req += insertDataOption;
        }

        if (strlen(includeValuesInResponse) > 0)
        {
            req += FPSTR("&includeValuesInResponse=");
            req += includeValuesInResponse;
        }

        if (strlen(responseValueRenderOption) > 0)
        {
            req += FPSTR("&responseValueRenderOption=");
            req += responseValueRenderOption;
        }

        if (strlen(responseDateTimeRenderOption) > 0)
        {
            req += FPSTR("&responseDateTimeRenderOption=");
            req += responseDateTimeRenderOption;
        }
    }

    if (valueRange)
        addHeader(req, host_type_sheet, strlen(valueRange->raw()));
    else
        addHeader(req, host_type_sheet, 0);

    req += FPSTR("\r\n");
    if (valueRange)
        req += valueRange->raw();

    return processRequest(req, response, httpcode);
}

bool GSheetClass::clear(MB_String &response, const char *spreadsheetId, const char *range)
{
    return mClear(response, spreadsheetId, range, operation_type_range);
}

bool GSheetClass::batchClear(MB_String &response, const char *spreadsheetId, const char *ranges)
{
    return mClear(response, spreadsheetId, ranges, operation_type_batch);
}

bool GSheetClass::batchClearByDataFilter(MB_String &response, const char *spreadsheetId, FirebaseJsonArray *dataFiltersArray)
{
    FirebaseJson js;
    js.add(FPSTR("dataFilters"), *dataFiltersArray);
    return mClear(response, spreadsheetId, js.raw(), operation_type_filter);
}

bool GSheetClass::mClear(MB_String &response, const char *spreadsheetId, const char *ranges, operation_type_t type)
{
    if (!checkToken())
        return false;

    MB_String req;
    int httpcode = 0;

    beginRequest(req, host_type_sheet);

    req = FPSTR("POST /v4/spreadsheets/");
    req += spreadsheetId;

    if (strlen(ranges) > 0)
    {
        if (type == operation_type_range)
        {

            req += FPSTR("/values/");
            req += ranges;
            req += FPSTR(":clear");

            addHeader(req, host_type_sheet, 2);

            req += FPSTR("\r\n{}");
        }
        else
        {
            if (type == operation_type_batch)
            {
                req += FPSTR("/values:batchClear");
                std::vector<MB_String> rngs = std::vector<MB_String>();
                MB_String rng = ranges;
                ut->splitTk(rng, rngs, ",");

                if (rngs.size() == 0)
                    return false;

                FirebaseJson r;
                MB_String tmp;

                for (size_t i = 0; i < rngs.size(); i++)
                {
                    tmp = (const char *)FPSTR("ranges/[");
                    tmp += i;
                    tmp += (const char *)FPSTR("]");

                    r.set(tmp.c_str(), rngs[i].c_str());
                }
                tmp.clear();

                addHeader(req, host_type_sheet, strlen(r.raw()));

                req += FPSTR("\r\n");

                req += r.raw();
                r.clear();
            }
            else if (type == operation_type_filter)
            {
                req += FPSTR("/values:batchClearByDataFilter");

                addHeader(req, host_type_sheet, strlen(ranges));

                req += FPSTR("\r\n");

                req += ranges;
            }
        }
    }

    return processRequest(req, response, httpcode);
}

bool GSheetClass::copyTo(MB_String &response, const char *spreadsheetId, uint32_t sheetId, const char *destinationSpreadsheetId)
{
    if (!checkToken())
        return false;

    MB_String req;
    int httpcode = 0;

    beginRequest(req, host_type_sheet);

    req = FPSTR("POST /v4/spreadsheets/");
    req += spreadsheetId;

    req += FPSTR("/sheets/");
    req += sheetId;
    req += FPSTR(":copyTo");

    MB_String s;
    s = FPSTR("{\"destinationSpreadsheetId\":\"");
    s += destinationSpreadsheetId;
    s += "\"}";

    addHeader(req, host_type_sheet, s.length());

    req += FPSTR("\r\n");
    req += s;

    return processRequest(req, response, httpcode);
}

bool GSheetClass::batchUpdate(MB_String &response, const char *spreadsheetId, FirebaseJsonArray *requestsArray, const char *includeSpreadsheetInResponse, const char *responseRanges, const char *responseIncludeGridData)
{
    if (!checkToken())
        return false;

    MB_String req;
    int httpcode = 0;

    beginRequest(req, host_type_sheet);

    req = FPSTR("POST /v4/spreadsheets/");
    req += spreadsheetId;

    req += FPSTR(":batchUpdate");

    FirebaseJson js;

    if (requestsArray)
    {
        js.add(FPSTR("requests"), *requestsArray);

        if (strlen(responseIncludeGridData) > 0)
        {
            if (strcmp(responseIncludeGridData, (const char *)FPSTR("true")) == 0)
                js.add(FPSTR("responseIncludeGridData"), true);
            else
                js.add(FPSTR("responseIncludeGridData"), false);
        }

        if (strlen(includeSpreadsheetInResponse) > 0)
        {
            if (strcmp(includeSpreadsheetInResponse, (const char *)FPSTR("true")) == 0)
                js.add(FPSTR("includeSpreadsheetInResponse"), true);
            else
                js.add(FPSTR("includeSpreadsheetInResponse"), false);
        }

        if (strlen(responseRanges) > 0)
        {
            std::vector<MB_String> rngs = std::vector<MB_String>();
            MB_String rng = responseRanges;
            ut->splitTk(rng, rngs, ",");

            if (rngs.size() == 0)
                return false;
            MB_String tmp;

            for (size_t i = 0; i < rngs.size(); i++)
            {
                tmp = (const char *)FPSTR("responseRanges/[");
                tmp += i;
                tmp += (const char *)FPSTR("]");

                js.set(tmp.c_str(), rngs[i].c_str());
            }

            tmp.clear();
        }

        addHeader(req, host_type_sheet, strlen(js.raw()));

        req += FPSTR("\r\n");
        req += js.raw();

        return processRequest(req, response, httpcode);
    }

    return false;
}

bool GSheetClass::create(MB_String &response, FirebaseJson *spreadsheet, const char *sharedUserEmail)
{
    if (!checkToken())
        return false;

    MB_String req;
    int httpcode = 0;

    beginRequest(req, host_type_sheet);

    req = FPSTR("POST /v4/spreadsheets");

    addHeader(req, host_type_sheet, strlen(spreadsheet->raw()));

    req += FPSTR("\r\n");
    req += spreadsheet->raw();

    return processRequest(req, response, httpcode);
}
bool GSheetClass::getMetadata(MB_String &response, const char *spreadsheetId, uint32_t metadataId)
{
    if (!checkToken())
        return false;

    MB_String req;
    int httpcode = 0;

    beginRequest(req, host_type_sheet);

    req = FPSTR("GET /v4/spreadsheets/");
    req += spreadsheetId;
    req += FPSTR("/developerMetadata/");
    req += metadataId;

    addHeader(req, host_type_sheet);

    req += FPSTR("\r\n");

    return processRequest(req, response, httpcode);
}

bool GSheetClass::searchMetadata(MB_String &response, const char *spreadsheetId, FirebaseJsonArray *dataFiltersArray)
{
    if (!checkToken())
        return false;

    if (dataFiltersArray)
    {
        MB_String req;
        int httpcode = 0;

        beginRequest(req, host_type_sheet);

        req = FPSTR("POST /v4/spreadsheets/");
        req += spreadsheetId;
        req += FPSTR("/developerMetadata:search");

        FirebaseJson js;
        js.add(FPSTR("dataFilters"), *dataFiltersArray);

        addHeader(req, host_type_sheet, strlen(js.raw()));

        req += FPSTR("\r\n");

        req += js.raw();

        return processRequest(req, response, httpcode);
    }

    return false;
}

bool GSheetClass::getSpreadsheet(MB_String &response, const char *spreadsheetId, const char *ranges, const char *includeGridData)
{
    if (!checkToken())
        return false;

    MB_String req;
    int httpcode = 0;

    beginRequest(req, host_type_drive);

    req = FPSTR("GET /v4/spreadsheets/");
    req += spreadsheetId;

    MB_String s;

    if (strlen(ranges) > 0)
    {
        std::vector<MB_String> rngs = std::vector<MB_String>();
        MB_String rng = ranges;
        ut->splitTk(rng, rngs, ",");

        if (rngs.size() == 0)
            return false;

        for (size_t i = 0; i < rngs.size(); i++)
        {
            if (s.length() > 0)
                s += FPSTR("&");
            else
                s += FPSTR("?");

            s += FPSTR("ranges=");
            s += rngs[i].c_str();
        }

        req += s;
    }

    if (strlen(includeGridData) > 0)
    {
        if (s.length() > 0)
            s += FPSTR("&");
        else
            s += FPSTR("?");

        if (strcmp(includeGridData, (const char *)FPSTR("true")) == 0)
            s += FPSTR("true");
        else
            s += FPSTR("false");
    }

    s.clear();

    addHeader(req, host_type_sheet);

    req += FPSTR("\r\n");

    return processRequest(req, response, httpcode);
}

bool GSheetClass::getSpreadsheetByDataFilter(MB_String &response, const char *spreadsheetId, FirebaseJsonArray *dataFiltersArray, const char *includeGridData)
{
    if (!checkToken())
        return false;

    if (dataFiltersArray)
    {
        MB_String req;
        int httpcode = 0;

        beginRequest(req, host_type_drive);

        req = FPSTR("POST /v4/spreadsheets/");
        req += spreadsheetId;
        req += ":getByDataFilter";

        FirebaseJson js;
        js.add(FPSTR("dataFilters"), *dataFiltersArray);

        if (strlen(includeGridData) > 0)
        {
            if (strcmp(includeGridData, (const char *)FPSTR("true")) == 0)
                js.add(FPSTR("includeGridData"), true);
        }
        addHeader(req, host_type_sheet, strlen(js.raw()));

        req += FPSTR("\r\n");
        req += js.raw();
        return processRequest(req, response, httpcode);
    }

    return false;
}

bool GSheetClass::deleteFile(MB_String &response, const char *spreadsheetId, bool closeSession)
{
    if (!checkToken())
        return false;

    if (closeSession)
    {
        if (config.signer.wcs)
            delete config.signer.wcs;
        config.signer.wcs = NULL;
    }

    MB_String req;
    int httpcode = 0;

    beginRequest(req, host_type_drive);

    req = FPSTR("DELETE /drive/v3/files/");
    req += spreadsheetId;

    addHeader(req, host_type_drive);

    req += FPSTR("\r\n");

    processRequest(req, response, httpcode);

    if (closeSession)
    {
        if (config.signer.wcs)
            delete config.signer.wcs;
        config.signer.wcs = NULL;
    }

    return httpcode == 204;
}

bool GSheetClass::deleteFiles(MB_String &response)
{

    if (!checkToken())
        return false;

    bool ret = listFiles(response);

    if (ret)
    {
        ret = false;
        FirebaseJsonData result;
        FirebaseJsonArray arr;
        FirebaseJson js(response);
        js.get(result, FPSTR("files"));
        if (result.success)
        {
            if (result.typeNum == FirebaseJson::JSON_ARRAY)
            {
                result.getArray(arr);

                ret = arr.size() > 0;

                for (size_t i = 0; i < arr.size(); i++)
                {
                    result.clear();
                    arr.get(result, i);
                    if (result.success)
                    {
                        result.getJSON(js);
                        result.clear();
                        js.get(result, FPSTR("id"));
                        if (result.success)
                        {
                            if (!deleteFile(response, result.to<const char *>(), false))
                            {
                                ret = false;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    return ret;
}

bool GSheetClass::listFiles(MB_String &response, uint32_t pageSize, const char *orderBy, const char *pageToken)
{
    if (!checkToken())
        return false;

    if (config.signer.wcs)
        delete config.signer.wcs;
    config.signer.wcs = NULL;

    MB_String req;
    int httpcode = 0;

    if (pageSize == 0 || pageSize > 10)
        pageSize = 10;

    beginRequest(req, host_type_drive);

    req = FPSTR("GET /drive/v3/files?pageSize=");
    req += pageSize;

    if (strlen(orderBy) > 0)
    {
        req += FPSTR("&orderBy=");
        req += orderBy;
    }

    if (strlen(pageToken) > 0)
    {
        req += FPSTR("&pageToken=");
        req += pageToken;
    }

    addHeader(req, host_type_drive);

    req += FPSTR("\r\n");

    bool ret = processRequest(req, response, httpcode);

    if (config.signer.wcs)
        delete config.signer.wcs;
    config.signer.wcs = NULL;

    return ret;
}

MB_String GSheetClass::mGetValue(MB_String &response, const char *key)
{
    MB_String _key = "\"";
    _key += key;
    _key += "\"";

    size_t p1 = response.find(_key);
    if (p1 != MB_String::npos)
    {
        size_t p2 = response.find(':', p1 + _key.length());

        if (p2 != MB_String::npos)
        {
            size_t p3 = response.find('"', p2 + 1);
            if (p3 != MB_String::npos)
            {
                size_t p4 = response.find('"', p3 + 1);
                return response.substr(p3 + 1, p4 - p3 - 1);
            }
        }
    }
    return MB_String();
}

bool GSheetClass::createPermission(MB_String &response, const char *fileid, const char *role, const char *type, const char *email)
{
    if (config.signer.wcs)
        delete config.signer.wcs;
    config.signer.wcs = NULL;

    MB_String req;
    int httpcode = 0;

    beginRequest(req, host_type_drive);

    req = FPSTR("POST /drive/v3/files/");
    req += fileid;
    req += FPSTR("/permissions?supportsAllDrives=true");

    if (strcmp(role, (const char *)FPSTR("owner")) == 0)
        req += FPSTR("&transferOwnership=true");

    FirebaseJson js;
    js.add((const char *)FPSTR("role"), role);
    js.add((const char *)FPSTR("type"), type);
    js.add((const char *)FPSTR("emailAddress"), email);

    addHeader(req, host_type_drive, strlen(js.raw()));

    req += FPSTR("\r\n");
    req += js.raw();

    bool ret = processRequest(req, response, httpcode);

    if (config.signer.wcs)
        delete config.signer.wcs;
    config.signer.wcs = NULL;

    return ret;
}

ESP_Google_Sheet_Client GSheet = ESP_Google_Sheet_Client();

#endif