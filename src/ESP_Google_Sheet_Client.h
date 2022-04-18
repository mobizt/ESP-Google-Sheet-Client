#ifndef ESP_GOOGLE_SHEET_CLIENT_VERSION
#define ESP_GOOGLE_SHEET_CLIENT_VERSION "1.0.1"
#endif

/**
 * Google Sheet Client, ESP_Google_Sheet_Client.h v1.0.1
 * 
 * This library supports Espressif ESP8266 and ESP32 MCUs
 * 
 * Created April 18, 2022
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

#include <Arduino.h>

#ifndef ESP_Google_Sheet_Client_H
#define ESP_Google_Sheet_Client_H

#include "lib/ESPSigner/ESPSigner.h"

enum esp_google_sheet_file_storage_type
{
    esP_google_sheet_file_storage_type_flash,
    esP_google_sheet_file_storage_type_sd
};

class GSheetClass : public ESP_Signer
{
    friend class ESP_Google_Sheet_Client;
    friend class GSheet_Values;
    friend class GSheet_Sheets;
    friend class GSheet_Metadata;

public:
    GSheetClass();
    ~GSheetClass();

private:
    enum operation_type_t
    {
        operation_type_undefined,
        operation_type_range,
        operation_type_batch,
        operation_type_filter,
    };

    enum host_type_t
    {
        host_type_sheet,
        host_type_drive
    };

    SignerConfig config;
    int cert_addr = 0;
    bool cert_updated = false;
    MB_String certFile;
    esp_google_sheet_file_storage_type certFileStorageType;

    void auth(const char *client_email, const char *project_id, const char *private_key);
    void setTokenCallback(TokenStatusCallback callback);
    bool checkToken();
    bool get(FirebaseJson *response, const char *spreadsheetId, const char *range);
    bool batchGet(FirebaseJson *response, const char *spreadsheetId, const char *ranges, const char *majorDimension = "", const char *valueRenderOption = "", const char *dateTimeRenderOption = "");
    bool batchGetByDataFilter(FirebaseJson *response, const char *spreadsheetId, FirebaseJsonArray *dataFiltersArray, const char *majorDimension = "", const char *valueRenderOption = "", const char *dateTimeRenderOption = "");
    bool append(FirebaseJson *response, const char *spreadsheetId, const char *range, FirebaseJson *valueRange, const char *valueInputOption = "USER_ENTERED", const char *insertDataOption = "", const char *includeValuesInResponse = "", const char *responseValueRenderOption = "", const char *responseDateTimeRenderOption = "");
    bool update(FirebaseJson *response, const char *spreadsheetId, const char *range, FirebaseJson *valueRange, const char *valueInputOption = "USER_ENTERED", const char *includeValuesInResponse = "", const char *responseValueRenderOption = "", const char *responseDateTimeRenderOption = "");
    bool _batchUpdate(FirebaseJson *response, const char *spreadsheetId, FirebaseJsonArray *valueRangeArray, const char *valueInputOption = "USER_ENTERED", const char *includeValuesInResponse = "", const char *responseValueRenderOption = "", const char *responseDateTimeRenderOption = "");
    bool batchUpdateByDataFilter(FirebaseJson *response, const char *spreadsheetId, FirebaseJsonArray *DataFilterValueRangeArray, const char *valueInputOption = "USER_ENTERED", const char *includeValuesInResponse = "", const char *responseValueRenderOption = "", const char *responseDateTimeRenderOption = "");
    bool clear(FirebaseJson *response, const char *spreadsheetId, const char *range);
    bool batchClear(FirebaseJson *response, const char *spreadsheetId, const char *ranges);
    bool batchClearByDataFilter(FirebaseJson *response, const char *spreadsheetId, FirebaseJsonArray *dataFiltersArray);
    bool copyTo(FirebaseJson *response, const char *spreadsheetId, uint32_t sheetId, const char *destinationSpreadsheetId);
    bool batchUpdate(FirebaseJson *response, const char *spreadsheetId, FirebaseJsonArray *requestsArray, const char *includeSpreadsheetInResponse = "", const char *responseRanges = "", const char *responseIncludeGridData = "");
    bool create(FirebaseJson *response, FirebaseJson *spreadsheet, const char *sharedUserEmail);
    bool getMetadata(FirebaseJson *response, const char *spreadsheetId, uint32_t metadataId);
    bool searchMetadata(FirebaseJson *response, const char *spreadsheetId, FirebaseJsonArray *dataFiltersArray);
    bool deleteFile(FirebaseJson *response, const char *spreadsheetId, bool closeSession = true);
    bool getSpreadsheet(FirebaseJson *response, const char *spreadsheetId, const char *ranges, const char *includeGridData = "");
    bool getSpreadsheetByDataFilter(FirebaseJson *response, const char *spreadsheetId, FirebaseJsonArray *dataFiltersArray, const char *includeGridData = "");
    bool deleteFiles(FirebaseJson *response);
    bool listFiles(FirebaseJson *response, uint32_t pageSize = 5, const char *orderBy = "", const char *pageToken = "");
    void beginRequest(FirebaseJson *response, MB_String &req, host_type_t host_type);
    void addHeader(MB_String &req, host_type_t host_type, int len = -1);
    bool processRequest(FirebaseJson *response, MB_String &req, int &httpcode);
    void mUpdateInit(FirebaseJson *js, FirebaseJsonArray *rangeArr, const char *valueInputOption, const char *includeValuesInResponse, const char *responseValueRenderOption, const char *responseDateTimeRenderOption);
    bool mUpdate(bool append, operation_type_t type, FirebaseJson *response, const char *spreadsheetId, const char *range, FirebaseJson *valueRange, const char *valueInputOption = "USER_ENTERED", const char *insertDataOption = "", const char *includeValuesInResponse = "", const char *responseValueRenderOption = "", const char *responseDateTimeRenderOption = "");
    bool mClear(FirebaseJson *response, const char *spreadsheetId, const char *ranges, operation_type_t type);
    bool mGet(FirebaseJson *response, const char *spreadsheetId, const char *ranges, const char *majorDimension, const char *valueRenderOption, const char *dateTimeRenderOption, operation_type_t type);
    bool createPermission(FirebaseJson *response, const char *fileId, const char *role, const char *type, const char *email);
    bool setClock(float gmtOffset);
    void setSecure();
    void setCert(const char *ca);
    void setCertFile(const char *filename, esp_google_sheet_file_storage_type type);
};

class GSheet_Values
{
    friend class ESP_Google_Sheet_Client;

public:
    GSheet_Values(){};
    ~GSheet_Values(){};

    /** Get a range of values from a spreadsheet.
     * 
     * @param response (FirebaseJson) The returned response.
     * @param spreadsheetId (string) The ID of the spreadsheet to retrieve data from.
     * @param range (string) The A1 notation or R1C1 notation of the range to retrieve values from.
     * @return Boolean type status indicates the success of the operation.
     * 
     * For ref doc go to https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets.values/get
    */
    template <typename T1 = const char *, typename T2 = const char *>
    bool get(FirebaseJson *response, T1 spreadsheetId, T2 range)
    {
        if (!gsheet)
            return false;

        return gsheet->get(response, toString(spreadsheetId), toString(range));
    }

    /** Get one or more ranges of values from a spreadsheet.
     * 
     * @param response (FirebaseJson) The returned response.
     * @param spreadsheetId (string) The ID of the spreadsheet to retrieve data from.
     * @param ranges (string) The A1 notation or R1C1 notation of the range to retrieve values from. Ranges separated with comma ",".
     * @param majorDimension (enum string) The major dimension that results should use.
     * 
     * @note If the spreadsheet data is: A1=1,B1=2,A2=3,B2=4, 
     * then requesting range=A1:B2,majorDimension=ROWS returns [[1,2],[3,4]], 
     * whereas requesting range=A1:B2,majorDimension=COLUMNS returns [[1,3],[2,4]].
     * DIMENSION_UNSPECIFIED    The default value, do not use.
     * ROWS                     Operates on the rows of a sheet.
     * COLUMNS                  Operates on the columns of a sheet.
     * 
     * @param valueRenderOption (enum string) How values should be represented in the output. 
     * 
     * @note The default render option is ValueRenderOption.FORMATTED_VALUE.
     * FORMATTED_VALUE          Values will be calculated & formatted in the reply according to the cell's formatting. 
     *                          Formatting is based on the spreadsheet's locale, not the requesting user's locale. 
     *                          For example, if A1 is 1.23 and A2 is =A1 and formatted as currency, then A2 would 
     *                          return "$1.23".
     * UNFORMATTED_VALUE        Values will be calculated, but not formatted in the reply. 
     *                          For example, if A1 is 1.23 and A2 is =A1 and formatted as currency, 
     *                          then A2 would return the number 1.23.
     * FORMULA                  Values will not be calculated. The reply will include the formulas. 
     *                          For example, if A1 is 1.23 and A2 is =A1 and formatted as currency, 
     *                          then A2 would return "=A1".
     * 
     * @param dateTimeRenderOption (enum string) How dates, times, and durations should be represented in the output. 
     * 
     * @note This is ignored if valueRenderOption is FORMATTED_VALUE. 
     * The default dateTime render option is SERIAL_NUMBER.
     * SERIAL_NUMBER        Instructs date, time, datetime, and duration fields to be output as doubles in "serial number" format, as popularized by Lotus 1-2-3. 
     *                      The whole number portion of the value (left of the decimal) counts the days since December 30th 1899. 
     *                      The fractional portion (right of the decimal) counts the time as a fraction of the day. 
     *                      For example, January 1st 1900 at noon would be 2.5, 2 because it's 2 days after December 30st 1899, and .5 because noon is half a day. 
     *                      February 1st 1900 at 3pm would be 33.625. This correctly treats the year 1900 as not a leap year.
     * FORMATTED_STRING     Instructs date, time, datetime, and duration fields to be output as strings in their given number 
     *                      format (which is dependent on the spreadsheet locale).
     * 
     * @return Boolean type status indicates the success of the operation.
     * 
     * 
     * For ref doc go to https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets.values/batchGet
    */
    template <typename T1 = const char *, typename T2 = const char *, typename T3 = const char *, typename T4 = const char *, typename T5 = const char *>
    bool batchGet(FirebaseJson *response, T1 spreadsheetId, T2 ranges, T3 majorDimension = "", T4 valueRenderOption = "", T5 dateTimeRenderOption = "")
    {
        if (!gsheet)
            return false;

        return gsheet->batchGet(response, toString(spreadsheetId), toString(ranges), toString(majorDimension), toString(valueRenderOption), toString(dateTimeRenderOption));
    }

    /** Get one or more ranges of values that match the specified data filters.
     * 
     * @param response (FirebaseJson) The returned response.
     * @param spreadsheetId (string) The ID of the spreadsheet to retrieve data from.
     * @param dataFiltersArray (FirebaseJsonArray of DataFilter object) The data filters used to match the ranges of values to retrieve. 
     * Ranges that match any of the specified data filters are included in the response.
     * @param majorDimension (enum string) The major dimension that results should use.
     * 
     * @note If the spreadsheet data is: A1=1,B1=2,A2=3,B2=4, 
     * then requesting range=A1:B2,majorDimension=ROWS returns [[1,2],[3,4]], 
     * whereas requesting range=A1:B2,majorDimension=COLUMNS returns [[1,3],[2,4]].
     * DIMENSION_UNSPECIFIED    The default value, do not use.
     * ROWS                     Operates on the rows of a sheet.
     * COLUMNS                  Operates on the columns of a sheet.
     * 
     * @param valueRenderOption (enum string) How values should be represented in the output. 
     * 
     * @note The default render option is ValueRenderOption.FORMATTED_VALUE.
     * FORMATTED_VALUE          Values will be calculated & formatted in the reply according to the cell's formatting. 
     *                          Formatting is based on the spreadsheet's locale, not the requesting user's locale. 
     *                          For example, if A1 is 1.23 and A2 is =A1 and formatted as currency, then A2 would 
     *                          return "$1.23".
     * UNFORMATTED_VALUE        Values will be calculated, but not formatted in the reply. 
     *                          For example, if A1 is 1.23 and A2 is =A1 and formatted as currency, 
     *                          then A2 would return the number 1.23.
     * FORMULA                  Values will not be calculated. The reply will include the formulas. 
     *                          For example, if A1 is 1.23 and A2 is =A1 and formatted as currency, 
     *                          then A2 would return "=A1".
     * 
     * @param dateTimeRenderOption (enum string) How dates, times, and durations should be represented in the output. 
     * 
     * @note This is ignored if valueRenderOption is FORMATTED_VALUE. 
     * The default dateTime render option is SERIAL_NUMBER.
     * SERIAL_NUMBER        Instructs date, time, datetime, and duration fields to be output as doubles in "serial number" format, as popularized by Lotus 1-2-3. 
     *                      The whole number portion of the value (left of the decimal) counts the days since December 30th 1899. 
     *                      The fractional portion (right of the decimal) counts the time as a fraction of the day. 
     *                      For example, January 1st 1900 at noon would be 2.5, 2 because it's 2 days after December 30st 1899, and .5 because noon is half a day. 
     *                      February 1st 1900 at 3pm would be 33.625. This correctly treats the year 1900 as not a leap year.
     * FORMATTED_STRING     Instructs date, time, datetime, and duration fields to be output as strings in their given number 
     *                      format (which is dependent on the spreadsheet locale).
     * @return Boolean type status indicates the success of the operation.
     * 
     * 
     * For ref doc go to https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets.values/batchGetByDataFilter
    */
    template <typename T1 = const char *, typename T2 = const char *, typename T3 = const char *, typename T4 = const char *>
    bool batchGetByDataFilter(FirebaseJson *response, T1 spreadsheetId, FirebaseJsonArray *dataFiltersArray, T2 majorDimension, T3 valueRenderOption = "", T4 dateTimeRenderOption = "")
    {
        if (!gsheet)
            return false;

        return gsheet->batchGetByDataFilter(response, toString(spreadsheetId), dataFiltersArray, toString(majorDimension), toString(valueRenderOption), toString(dateTimeRenderOption));
    }

    /** Appends values to a spreadsheet.
     * 
     * @param response (FirebaseJson) The returned response.
     * @param spreadsheetId (string) The ID of the spreadsheet to update. 
     * @param range (string) The A1 notation of a range to search for a logical table of data. 
     * Values are appended after the last row of the table.
     * @param valueRange (FirebaseJson of valueRange object) The data within a range of the spreadsheet.
     * @param valueInputOption (enum string) How the input data should be interpreted. 
     * 
     * @note
     * INPUT_VALUE_OPTION_UNSPECIFIED   Default input value. This value must not be used.
     * RAW                              The values the user has entered will not be parsed and will be stored as-is.
     * USER_ENTERED                     The values will be parsed as if the user typed them into the UI. 
     *                                  Numbers will stay as numbers, but strings may be converted to numbers, dates, etc. following the same rules 
     *                                  that are applied when entering text into a cell via the Google Sheets UI.
     * 
     * @param insertDataOption (enum string) How the input data should be inserted.
     * 
     * @note
     * 
     * OVERWRITE            The new data overwrites existing data in the areas it is written. 
     *                      (Note: adding data to the end of the sheet will still insert new rows or columns so the data can be written.) or 
     * INSERT_ROWS          Rows are inserted for the new data.
     * 
     * @param includeValuesInResponse (boolean string). Determines if the update response should include the values of the cells that were appended. 
     * 
     * @note By default, responses do not include the updated values.
     * 
     * @param responseValueRenderOption (enum string) Determines how values in the response should be rendered. 
     * 
     * @note
     * The default render option is FORMATTED_VALUE.
     * FORMATTED_VALUE      Values will be calculated & formatted in the reply according to the cell's formatting. 
     *                      Formatting is based on the spreadsheet's locale, not the requesting user's locale. 
     *                      For example, if A1 is 1.23 and A2 is =A1 and formatted as currency, then A2 would return "$1.23".
     * UNFORMATTED_VALUE    Values will be calculated, but not formatted in the reply. 
     *                      For example, if A1 is 1.23 and A2 is =A1 and formatted as currency, then A2 would return the number 1.23.
     * FORMULA              Values will not be calculated. The reply will include the formulas. 
     *                      For example, if A1 is 1.23 and A2 is =A1 and formatted as currency, then A2 would return "=A1".
     * @param responseDateTimeRenderOption (enum string) Determines how dates, times, and durations in the response should be rendered. 
     * 
     * @note This is ignored if responseValueRenderOption is FORMATTED_VALUE. 
     * The default dateTime render option is SERIAL_NUMBER.
     * SERIAL_NUMBER        Instructs date, time, datetime, and duration fields to be output as doubles in "serial number" format, as popularized by Lotus 1-2-3. 
     *                      The whole number portion of the value (left of the decimal) counts the days since December 30th 1899. 
     *                      The fractional portion (right of the decimal) counts the time as a fraction of the day. 
     *                      For example, January 1st 1900 at noon would be 2.5, 2 because it's 2 days after December 30st 1899, and .5 because noon is half a day. 
     *                      February 1st 1900 at 3pm would be 33.625. This correctly treats the year 1900 as not a leap year.
     * FORMATTED_STRING     Instructs date, time, datetime, and duration fields to be output as strings in their given number 
     *                      format (which is dependent on the spreadsheet locale).
     * @return Boolean type status indicates the success of the operation.
     * 
     * 
     * For ref doc go to https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets.values/append
    */
    template <typename T1 = const char *, typename T2 = const char *, typename T3 = const char *, typename T4 = const char *, typename T5 = const char *, typename T6 = const char *, typename T7 = const char *>
    bool append(FirebaseJson *response, T1 spreadsheetId, T2 range, FirebaseJson *valueRange, T3 valueInputOption = "USER_ENTERED", T4 insertDataOption = "", T5 includeValuesInResponse = "", T6 responseValueRenderOption = "", T7 responseDateTimeRenderOption = "")
    {
        if (!gsheet)
            return false;

        return gsheet->append(response, toString(spreadsheetId), toString(range), valueRange, toString(valueInputOption), toString(insertDataOption), toString(includeValuesInResponse), toString(responseValueRenderOption), toString(responseDateTimeRenderOption));
    }

    /** Sets values in a range of a spreadsheet. 
     * 
     * @param response (FirebaseJson) The returned response.
     * @param spreadsheetId (string) The ID of the spreadsheet to update. 
     * @param range (string) The A1 notation of the values to update. 
     * @param valueRange (FirebaseJson of valueRange object) The data within a range of the spreadsheet.
     * @param valueInputOption (enum string) How the input data should be interpreted. 
     * 
     * @note
     * INPUT_VALUE_OPTION_UNSPECIFIED   Default input value. This value must not be used.
     * RAW                              The values the user has entered will not be parsed and will be stored as-is.
     * USER_ENTERED                     The values will be parsed as if the user typed them into the UI. 
     *                                  Numbers will stay as numbers, but strings may be converted to numbers, dates, etc. following the same rules 
     *                                  that are applied when entering text into a cell via the Google Sheets UI.
     * 
     * @param includeValuesInResponse (boolean string). Determines if the update response should include the values of the cells that were appended. 
     * 
     * @note By default, responses do not include the updated values.
     * 
     * @param responseValueRenderOption (enum string) Determines how values in the response should be rendered. 
     * 
     * @note
     * The default render option is FORMATTED_VALUE.
     * FORMATTED_VALUE      Values will be calculated & formatted in the reply according to the cell's formatting. 
     *                      Formatting is based on the spreadsheet's locale, not the requesting user's locale. 
     *                      For example, if A1 is 1.23 and A2 is =A1 and formatted as currency, then A2 would return "$1.23".
     * UNFORMATTED_VALUE    Values will be calculated, but not formatted in the reply. 
     *                      For example, if A1 is 1.23 and A2 is =A1 and formatted as currency, then A2 would return the number 1.23.
     * FORMULA              Values will not be calculated. The reply will include the formulas. 
     *                      For example, if A1 is 1.23 and A2 is =A1 and formatted as currency, then A2 would return "=A1".
     * @param responseDateTimeRenderOption (enum string) Determines how dates, times, and durations in the response should be rendered. 
     * 
     * @note This is ignored if responseValueRenderOption is FORMATTED_VALUE. 
     * The default dateTime render option is SERIAL_NUMBER.
     * SERIAL_NUMBER        Instructs date, time, datetime, and duration fields to be output as doubles in "serial number" format, as popularized by Lotus 1-2-3. 
     *                      The whole number portion of the value (left of the decimal) counts the days since December 30th 1899. 
     *                      The fractional portion (right of the decimal) counts the time as a fraction of the day. 
     *                      For example, January 1st 1900 at noon would be 2.5, 2 because it's 2 days after December 30st 1899, and .5 because noon is half a day. 
     *                      February 1st 1900 at 3pm would be 33.625. This correctly treats the year 1900 as not a leap year.
     * FORMATTED_STRING     Instructs date, time, datetime, and duration fields to be output as strings in their given number 
     *                      format (which is dependent on the spreadsheet locale).
     * @return Boolean type status indicates the success of the operation.
     * 
     * 
     * For ref doc go to https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets.values/update
    */
    template <typename T1 = const char *, typename T2 = const char *, typename T3 = const char *, typename T4 = const char *, typename T5 = const char *, typename T6 = const char *>
    bool update(FirebaseJson *response, T1 spreadsheetId, T2 range, FirebaseJson *valueRange, T3 valueInputOption = "USER_ENTERED", T4 includeValuesInResponse = "", T5 responseValueRenderOption = "", T6 responseDateTimeRenderOption = "")
    {
        if (!gsheet)
            return false;

        return gsheet->update(response, toString(spreadsheetId), toString(range), valueRange, toString(valueInputOption), toString(includeValuesInResponse), toString(responseValueRenderOption), toString(responseDateTimeRenderOption));
    }

    /** Sets values in one or more ranges of a spreadsheet.
     * 
     * @param response (FirebaseJson) The returned response.
     * @param spreadsheetId (string) The ID of the spreadsheet to update. 
     * @param valueRangeArray (FirebaseJsonArray of valueRange object) The data within a range of the spreadsheet.
     * @param valueInputOption (enum string) How the input data should be interpreted. 
     * 
     * @note
     * INPUT_VALUE_OPTION_UNSPECIFIED   Default input value. This value must not be used.
     * RAW                              The values the user has entered will not be parsed and will be stored as-is.
     * USER_ENTERED                     The values will be parsed as if the user typed them into the UI. 
     *                                  Numbers will stay as numbers, but strings may be converted to numbers, dates, etc. following the same rules 
     *                                  that are applied when entering text into a cell via the Google Sheets UI.
     * 
     * @param includeValuesInResponse (boolean string). Determines if the update response should include the values of the cells that were appended. 
     * 
     * @note By default, responses do not include the updated values.
     * 
     * @param responseValueRenderOption (enum string) Determines how values in the response should be rendered. 
     * 
     * @note
     * The default render option is FORMATTED_VALUE.
     * FORMATTED_VALUE      Values will be calculated & formatted in the reply according to the cell's formatting. 
     *                      Formatting is based on the spreadsheet's locale, not the requesting user's locale. 
     *                      For example, if A1 is 1.23 and A2 is =A1 and formatted as currency, then A2 would return "$1.23".
     * UNFORMATTED_VALUE    Values will be calculated, but not formatted in the reply. 
     *                      For example, if A1 is 1.23 and A2 is =A1 and formatted as currency, then A2 would return the number 1.23.
     * FORMULA              Values will not be calculated. The reply will include the formulas. 
     *                      For example, if A1 is 1.23 and A2 is =A1 and formatted as currency, then A2 would return "=A1".
     * @param responseDateTimeRenderOption (enum string) Determines how dates, times, and durations in the response should be rendered. 
     * 
     * @note This is ignored if responseValueRenderOption is FORMATTED_VALUE. 
     * The default dateTime render option is SERIAL_NUMBER.
     * SERIAL_NUMBER        Instructs date, time, datetime, and duration fields to be output as doubles in "serial number" format, as popularized by Lotus 1-2-3. 
     *                      The whole number portion of the value (left of the decimal) counts the days since December 30th 1899. 
     *                      The fractional portion (right of the decimal) counts the time as a fraction of the day. 
     *                      For example, January 1st 1900 at noon would be 2.5, 2 because it's 2 days after December 30st 1899, and .5 because noon is half a day. 
     *                      February 1st 1900 at 3pm would be 33.625. This correctly treats the year 1900 as not a leap year.
     * FORMATTED_STRING     Instructs date, time, datetime, and duration fields to be output as strings in their given number 
     *                      format (which is dependent on the spreadsheet locale).
     * @return Boolean type status indicates the success of the operation.
     * 
     * 
     * For ref doc go to https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets.values/batchUpdate
    */
    template <typename T1 = const char *, typename T2 = const char *, typename T3 = const char *, typename T4 = const char *, typename T5 = const char *>
    bool batchUpdate(FirebaseJson *response, T1 spreadsheetId, FirebaseJsonArray *valueRangeArray, T2 valueInputOption = "USER_ENTERED", T3 includeValuesInResponse = "", T4 responseValueRenderOption = "", T5 responseDateTimeRenderOption = "")
    {
        if (!gsheet)
            return false;

        return gsheet->_batchUpdate(response, toString(spreadsheetId), valueRangeArray, toString(valueInputOption), toString(includeValuesInResponse), toString(responseValueRenderOption), toString(responseDateTimeRenderOption));
    }

    /** Sets values in one or more ranges of a spreadsheet.
     * 
     * @param response (FirebaseJson) The returned response.
     * @param spreadsheetId (string) The ID of the spreadsheet to update. 
     * @param DataFilterValueRangeArray (FirebaseJsonArray of DataFilterValueRange object) The data within a range of the spreadsheet.
     * @param valueInputOption (enum string) How the input data should be interpreted. 
     * 
     * @note
     * INPUT_VALUE_OPTION_UNSPECIFIED   Default input value. This value must not be used.
     * RAW                              The values the user has entered will not be parsed and will be stored as-is.
     * USER_ENTERED                     The values will be parsed as if the user typed them into the UI. 
     *                                  Numbers will stay as numbers, but strings may be converted to numbers, dates, etc. following the same rules 
     *                                  that are applied when entering text into a cell via the Google Sheets UI.
     * 
     * @param includeValuesInResponse (boolean string). Determines if the update response should include the values of the cells that were appended. 
     * 
     * @note By default, responses do not include the updated values.
     * 
     * @param responseValueRenderOption (enum string) Determines how values in the response should be rendered. 
     * 
     * @note
     * The default render option is FORMATTED_VALUE.
     * FORMATTED_VALUE      Values will be calculated & formatted in the reply according to the cell's formatting. 
     *                      Formatting is based on the spreadsheet's locale, not the requesting user's locale. 
     *                      For example, if A1 is 1.23 and A2 is =A1 and formatted as currency, then A2 would return "$1.23".
     * UNFORMATTED_VALUE    Values will be calculated, but not formatted in the reply. 
     *                      For example, if A1 is 1.23 and A2 is =A1 and formatted as currency, then A2 would return the number 1.23.
     * FORMULA              Values will not be calculated. The reply will include the formulas. 
     *                      For example, if A1 is 1.23 and A2 is =A1 and formatted as currency, then A2 would return "=A1".
     * @param responseDateTimeRenderOption (enum string) Determines how dates, times, and durations in the response should be rendered. 
     * 
     * @note This is ignored if responseValueRenderOption is FORMATTED_VALUE. 
     * The default dateTime render option is SERIAL_NUMBER.
     * SERIAL_NUMBER        Instructs date, time, datetime, and duration fields to be output as doubles in "serial number" format, as popularized by Lotus 1-2-3. 
     *                      The whole number portion of the value (left of the decimal) counts the days since December 30th 1899. 
     *                      The fractional portion (right of the decimal) counts the time as a fraction of the day. 
     *                      For example, January 1st 1900 at noon would be 2.5, 2 because it's 2 days after December 30st 1899, and .5 because noon is half a day. 
     *                      February 1st 1900 at 3pm would be 33.625. This correctly treats the year 1900 as not a leap year.
     * FORMATTED_STRING     Instructs date, time, datetime, and duration fields to be output as strings in their given number 
     *                      format (which is dependent on the spreadsheet locale).
     * @return Boolean type status indicates the success of the operation.
     * 
     * 
     * For ref doc go to https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets.values/batchUpdateByDataFilter
    */
    template <typename T1 = const char *, typename T2 = const char *, typename T3 = const char *, typename T4 = const char *, typename T5 = const char *>
    bool batchUpdateByDataFilter(FirebaseJson *response, T1 spreadsheetId, FirebaseJsonArray *DataFilterValueRangeArray, T2 valueInputOption = "USER_ENTERED", T3 includeValuesInResponse = "", T4 responseValueRenderOption = "", T5 responseDateTimeRenderOption = "")
    {
        if (!gsheet)
            return false;

        return gsheet->batchUpdateByDataFilter(response, toString(spreadsheetId), DataFilterValueRangeArray, toString(valueInputOption), toString(includeValuesInResponse), toString(responseValueRenderOption), toString(responseDateTimeRenderOption));
    }

    /** Clears values from a spreadsheet. The caller must specify the spreadsheet ID and range. 
     * 
     * @param response (FirebaseJson) The returned response.
     * @param spreadsheetId (string) The ID of the spreadsheet to update. 
     * @param range (string) The A1 notation or R1C1 notation of the values to clear.
     * @return Boolean type status indicates the success of the operation.
     * 
     * 
     * For ref doc go to https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets.values/clear
    */
    template <typename T1 = const char *, typename T2 = const char *>
    bool clear(FirebaseJson *response, T1 spreadsheetId, T2 range)
    {
        if (!gsheet)
            return false;

        return gsheet->clear(response, toString(spreadsheetId), toString(range));
    }

    /** Clears one or more ranges of values from a spreadsheet. 
     * 
     * @param response (FirebaseJson) The returned response.
     * @param spreadsheetId (string) The ID of the spreadsheet to update. 
     * @param ranges (string) The ranges to clear, in A1 or R1C1 notation. Ranges separated with comma ",".
     * @return Boolean type status indicates the success of the operation.
     * 
     * 
     * For ref doc go to https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets.values/batchClear
    */
    template <typename T1 = const char *, typename T2 = const char *>
    bool batchClear(FirebaseJson *response, T1 spreadsheetId, T2 ranges)
    {
        if (!gsheet)
            return false;

        return gsheet->batchClear(response, toString(spreadsheetId), toString(ranges));
    }

    /** Clears one or more ranges of values from a spreadsheet. 
     * 
     * @param response (FirebaseJson) The returned response.
     * @param spreadsheetId (string) The ID of the spreadsheet to update. 
     * @param dataFiltersArray (FirebaseJsonArray of DataFilter object) The DataFilters used to determine which ranges to clear.
     * @return Boolean type status indicates the success of the operation.
     * 
     * 
     * For ref doc go to https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets.values/batchClearByDataFilter
    */
    template <typename T = const char *>
    bool batchClearByDataFilter(FirebaseJson *response, T spreadsheetId, FirebaseJsonArray *dataFiltersArray)
    {
        if (!gsheet)
            return false;

        return gsheet->batchClearByDataFilter(response, toString(spreadsheetId), dataFiltersArray);
    }

private:
    GSheetClass *gsheet = NULL;
    void init(GSheetClass *gsheet) { this->gsheet = gsheet; }

protected:
    template <typename T>
    auto toString(const T &val) -> typename mb_string::enable_if<mb_string::is_std_string<T>::value || mb_string::is_arduino_string<T>::value || mb_string::is_mb_string<T>::value || mb_string::is_same<T, StringSumHelper>::value, const char *>::type { return val.c_str(); }

    template <typename T>
    auto toString(T val) -> typename mb_string::enable_if<mb_string::is_const_chars<T>::value, const char *>::type { return val; }

    template <typename T>
    auto toString(T val) -> typename mb_string::enable_if<mb_string::fs_t<T>::value, const char *>::type { return (const char *)val; }

    template <typename T>
    auto toString(T val) -> typename mb_string::enable_if<mb_string::is_same<T, std::nullptr_t>::value, const char *>::type { return ""; }
};

class GSheet_Sheets
{
    friend class ESP_Google_Sheet_Client;

public:
    GSheet_Sheets(){};
    ~GSheet_Sheets(){};

    /** Copies a single sheet from a spreadsheet to another spreadsheet. 
     * 
     * @param response (FirebaseJson) The returned response.
     * @param spreadsheetId (string) The ID of the spreadsheet containing the sheet to copy. 
     * @param sheetId (integer) The ID of the sheet to copy.
     * @param destinationSpreadsheetId (string) The ID of the spreadsheet to copy the sheet to.
     * @return Boolean type status indicates the success of the operation.
     * 
     * @note The sheet id is the integer number which you can get it from gid parameter of spreadsheet URL when select the sheet tab.
     * For example, https://docs.google.com/spreadsheets/d/xxxxxxx/edit#gid=1180731163
     * The sheet id of above case is 1180731163
     * 
     * For ref doc go to https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets.sheets/copyTo
    */
    template <typename T1 = const char *, typename T2 = const char *>
    bool copyTo(FirebaseJson *response, T1 spreadsheetId, uint32_t sheetId, T2 destinationSpreadsheetId)
    {
        if (!gsheet)
            return false;

        return gsheet->copyTo(response, toString(spreadsheetId), sheetId, toString(destinationSpreadsheetId));
    }

private:
    GSheetClass *gsheet = NULL;
    void init(GSheetClass *gsheet) { this->gsheet = gsheet; }

protected:
    template <typename T>
    auto toString(const T &val) -> typename mb_string::enable_if<mb_string::is_std_string<T>::value || mb_string::is_arduino_string<T>::value || mb_string::is_mb_string<T>::value || mb_string::is_same<T, StringSumHelper>::value, const char *>::type { return val.c_str(); }

    template <typename T>
    auto toString(T val) -> typename mb_string::enable_if<mb_string::is_const_chars<T>::value, const char *>::type { return val; }

    template <typename T>
    auto toString(T val) -> typename mb_string::enable_if<mb_string::fs_t<T>::value, const char *>::type { return (const char *)val; }

    template <typename T>
    auto toString(T val) -> typename mb_string::enable_if<mb_string::is_same<T, std::nullptr_t>::value, const char *>::type { return ""; }
};

class GSheet_Metadata
{
    friend class ESP_Google_Sheet_Client;

public:
    GSheet_Metadata(){};
    ~GSheet_Metadata(){};

    /** Get the developer metadata with the specified ID. 
     * 
     * @param response (FirebaseJson) The returned response.
     * @param spreadsheetId (string) The ID of the spreadsheet to retrieve metadata from.
     * @param metadataId (integer) The ID of the developer metadata to retrieve.
     * @return Boolean type status indicates the success of the operation.
     * 
     * For ref doc go to https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets.developerMetadata/get
    */
    template <typename T = const char *>
    bool get(FirebaseJson *response, T spreadsheetId, uint32_t metadataId)
    {
        if (!gsheet)
            return false;

        return gsheet->getMetadata(response, toString(spreadsheetId), metadataId);
    }

    /** Get all developer metadata matching the specified DataFilter.
     * 
     * @param response (FirebaseJson) The returned response.
     * @param spreadsheetId (string) The ID of the spreadsheet to retrieve metadata from. 
     * @param dataFiltersArray (FirebaseJsonArray of DataFilter object) The data filters describing the criteria used to determine which 
     * DeveloperMetadata entries to return. DeveloperMetadata matching any of the specified filters are included in the response.
     * @return Boolean type status indicates the success of the operation.
     * 
     * For ref doc go to https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets.developerMetadata/search
    */
    template <typename T = const char *>
    bool search(FirebaseJson *response, T spreadsheetId, FirebaseJsonArray *dataFiltersArray)
    {
        if (!gsheet)
            return false;

        return gsheet->searchMetadata(response, toString(spreadsheetId), dataFiltersArray);
    }

private:
    GSheetClass *gsheet = NULL;
    void init(GSheetClass *gsheet) { this->gsheet = gsheet; }

protected:
    template <typename T>
    auto toString(const T &val) -> typename mb_string::enable_if<mb_string::is_std_string<T>::value || mb_string::is_arduino_string<T>::value || mb_string::is_mb_string<T>::value || mb_string::is_same<T, StringSumHelper>::value, const char *>::type { return val.c_str(); }

    template <typename T>
    auto toString(T val) -> typename mb_string::enable_if<mb_string::is_const_chars<T>::value, const char *>::type { return val; }

    template <typename T>
    auto toString(T val) -> typename mb_string::enable_if<mb_string::fs_t<T>::value, const char *>::type { return (const char *)val; }

    template <typename T>
    auto toString(T val) -> typename mb_string::enable_if<mb_string::is_same<T, std::nullptr_t>::value, const char *>::type { return ""; }
};

class ESP_Google_Sheet_Client
{

public:
    ESP_Google_Sheet_Client()
    {
        gsheet = new GSheetClass();
    };

    ~ESP_Google_Sheet_Client()
    {
        if (gsheet)
            delete gsheet;
    };

    /** Begin the Google API authentication. 
     * 
     * @param client_email (string) The Service Account's client email.
     * @param project_id (string) The project ID. 
     * @param private_key (string) The Service Account's private key.
     * 
    */
    template <typename T1 = const char *, typename T2 = const char *, typename T3 = const char *>
    void begin(T1 client_email, T2 project_id, T3 private_key)
    {
        values.init(gsheet);
        sheets.init(gsheet);
        developerMetadata.init(gsheet);
        gsheet->auth(toString(client_email), toString(project_id), toString(private_key));
    }

    /** Set the Root certificate data for server authorization.
     * @param ca PEM format certificate string.
    */
    void setCert(const char *ca) { gsheet->setCert(ca); }

    /** Set the Root certificate file for server authorization.
     * @param filename PEM format certificate file name incuded path.
     * @param storageType The storage type of certificate file. esp_google_sheet_file_storage_type_flash or esp_google_sheet_file_storage_type_sd
    */
    template <typename T = const char *>
    void setCertFile(T filename, esp_google_sheet_file_storage_type storageType) { gsheet->setCertFile(toString(filename, storageType)); }

    /** Set the OAuth2.0 token generation status callback. 
     * 
     * @param callback The callback function that accepts the TokenInfo as argument.
     * 
    */
    void setTokenCallback(TokenStatusCallback callback)
    {
        gsheet->setTokenCallback(callback);
    }

    /** Get the authentication ready status and process the authentication. 
     * 
     * @note This function should be called repeatedly in loop.
     * 
    */
    bool ready()
    {
        return gsheet->checkToken();
    }

    /**
     * Get the generated access token.
     * 
     * @return String of OAuth2.0 access token.
     * 
    */
    String accessToken() { return gsheet->accessToken(); }

    /**
     * Get the token type string.
     * 
     * @param info The TokenInfo structured data contains token info.
     * @return token type String.
     * 
    */
    String getTokenType() { return gsheet->getTokenType(); }
    String getTokenType(TokenInfo info) { return gsheet->getTokenType(info); }

    /**
     * Get the token status string.
     * 
     * @param info The TokenInfo structured data contains token info.
     * @return token status String.
     * 
    */
    String getTokenStatus() { return gsheet->getTokenStatus(); }
    String getTokenStatus(TokenInfo info) { return gsheet->getTokenStatus(info); }

    /**
     * Get the token generation error string.
     * 
     * @param info The TokenInfo structured data contains token info.
     * @return token generation error String.
     * 
    */
    String getTokenError() { return gsheet->getTokenError(); }
    String getTokenError(TokenInfo info) { return gsheet->getTokenError(info); }

    /**
     * Get the token expiration timestamp (seconds from midnight Jan 1, 1970).
     * 
     * @return timestamp.
     * 
    */
    unsigned long getExpiredTimestamp() { return gsheet->getExpiredTimestamp(); }

    /** Applies one or more updates to the spreadsheet. 
     * 
     * @param response (FirebaseJson) The returned response.
     * @param spreadsheetId (string) The spreadsheet to apply the updates to. 
     * @param requestsArray (FirebaseJsonArray of Request oobject) A list of updates to apply to the spreadsheet. 
     * 
     * @note Requests will be applied in the order they are specified. If any request is not valid, no requests will be applied.
     * 
     * @param includeSpreadsheetInResponse (boolean string) Determines if the update response should include the spreadsheet resource.
     * @param responseRanges (string) Limits the ranges included in the response spreadsheet. Meaningful only if includeSpreadsheetInResponse is 'true'.
     * @param responseIncludeGridData (boolean string) True if grid data should be returned. Meaningful only if includeSpreadsheetInResponse is 'true'. This parameter is ignored if a field mask was set in the request.
     * @return Boolean type status indicates the success of the operation.
     * 
     * For ref doc go to https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets/batchUpdate
     * 
    */
    template <typename T1 = const char *, typename T2 = const char *, typename T3 = const char *, typename T4 = const char *>
    bool batchUpdate(FirebaseJson *response, T1 spreadsheetId, FirebaseJsonArray *requestsArray, T2 includeSpreadsheetInResponse = "", T3 responseRanges = "", T4 responseIncludeGridData = "")
    {
        return gsheet->batchUpdate(response, toString(spreadsheetId), requestsArray, toString(includeSpreadsheetInResponse), toString(responseRanges), toString(responseIncludeGridData));
    }

    /** Creates a spreadsheet, returning the newly created spreadsheet. 
     * 
     * @param response (FirebaseJson) The returned response.
     * @param spreadsheet (FirebaseJson) The spreadsheet object. 
     * @param sharedUserEmail (string) Email of user to share the access. 
     * 
     * @note Drive API should be enabled at, https://console.cloud.google.com/apis/library/drive.googleapis.com
     * 
     * @return Boolean type status indicates the success of the operation.
     * 
     * For ref doc go to https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets/create
     * 
    */
    template <typename T = const char *>
    bool create(FirebaseJson *response, FirebaseJson *spreadsheet, T sharedUserEmail)
    {
        return gsheet->create(response, spreadsheet, toString(sharedUserEmail));
    }

    /** Get the spreadsheet at the given ID. 
     * 
     * @param response (FirebaseJson) The returned response.
     * @param spreadsheetId (FirebaseJson) The spreadsheet to request. 
     * @param ranges (string) The ranges to retrieve from the spreadsheet. Ranges separated with comma ",". 
     * @param includeGridData (boolean string) True if grid data should be returned. 
     * 
     * @note This parameter is ignored if a field mask was set in the request. 
     * 
     * @return Boolean type status indicates the success of the operation.
     * 
     * For ref doc, go to https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets/get
     * 
    */
    template <typename T1 = const char *, typename T2 = const char *, typename T3 = const char *>
    bool get(FirebaseJson *response, T1 spreadsheetId, T2 ranges = "", T3 includeGridData = "")
    {
        return gsheet->getSpreadsheet(response, toString(spreadsheetId), toString(ranges), toString(includeGridData));
    }

    /** Get the spreadsheet at the given ID. 
     * 
     * @param response (FirebaseJson) The returned response.
     * @param spreadsheetId (FirebaseJson) The spreadsheet to request. 
     * 
     * @return Boolean type status indicates the success of the operation.
     * 
     * For ref doc, go to https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets/getByDataFilter
     * 
    */
    template <typename T1 = const char *, typename T2 = const char *>
    bool getByDataFilter(FirebaseJson *response, T1 spreadsheetId, FirebaseJsonArray *dataFiltersArray, T2 includeGridData = "")
    {
        return gsheet->getSpreadsheetByDataFilter(response, toString(spreadsheetId), dataFiltersArray, toString(includeGridData));
    }

    /** Delete a spreadsheet from Google Drive. 
     * 
     * @param response (FirebaseJson) The returned response.
     * @param spreadsheetId (FirebaseJson) The ID of spreadsheet to delete. 
     * @param closeSession (boolean) Close the session after delete. 
     * 
     * @return Boolean type status indicates the success of the operation.
     * 
     * @note The response may be empty.
     * 
    */
    template <typename T = const char *>
    bool deleteFile(FirebaseJson *response, T spreadsheetId, bool closeSession = true)
    {
        return gsheet->deleteFile(response, toString(spreadsheetId), closeSession);
    }

    /** Delete spreadsheets from Google Drive. 
     * 
     * @param response (FirebaseJson) The returned response.
     * 
     * @note This will delete last 5 spreadsheets at a time.
     * 
     * @return Boolean type status indicates the success of the operation.
     * 
     * @note The response may be empty.
     * 
    */
    bool deleteFiles(FirebaseJson *response)
    {
        return gsheet->deleteFiles(response);
    }

    /** List the spreadsheets in Google Drive. 
     * 
     * @param response (FirebaseJson) The returned response.
     * @param pageSize (integer) The maximum number of files to return per page. 
     * @param orderBy (string) A comma-separated list of sort keys. 
     * 
     * @note Valid keys are 'createdTime', 'folder', 'modifiedByMeTime', 'modifiedTime', 'name', 'name_natural', 
     * 'quotaBytesUsed', 'recency', 'sharedWithMeTime', 'starred', and 'viewedByMeTime'. 
     * Each key sorts ascending by default, but may be reversed with the 'desc' modifier. 
     * Example usage: ?orderBy=folder,modifiedTime desc,name. 
     * 
     * @param pageToken (string) The token for continuing a previous list request on the next page. 
     * 
     * @note This should be set to the value of 'nextPageToken' from the previous response.
     * 
     * @return Boolean type status indicates the success of the operation.
     * 
    */
    template <typename T1 = const char *, typename T2 = const char *>
    bool listFiles(FirebaseJson *response, uint32_t pageSize = 5, T1 orderBy = "createdTime%20desc", T2 pageToken = "")
    {
        return gsheet->listFiles(response, pageSize, toString(orderBy), toString(pageToken));
    }

    /** SD card config with GPIO pins.
     * 
     * @param ss -   SPI Chip/Slave Select pin.
     * @param sck -  SPI Clock pin.
     * @param miso - SPI MISO pin.
     * @param mosi - SPI MOSI pin.
     * @return Boolean type status indicates the success of the operation.
    */
    bool sdBegin(int8_t ss = -1, int8_t sck = -1, int8_t miso = -1, int8_t mosi = -1) { return gsheet->sdBegin(ss, sck, miso, mosi); }

    /** Initialize the SD_MMC card (ESP32 only).
     * @param mountpoint The mounting point.
     * @param mode1bit Allow 1 bit data line (SPI mode).
     * @param format_if_mount_failed Format SD_MMC card if mount failed.
     * @return The boolean value indicates the success of operation.
    */
    template <typename T = const char *>
    bool sdMMCBegin(T mountpoint = "/sdcard", bool mode1bit = false, bool format_if_mount_failed = false) { return gsheet->sdMMCBegin(toString(mountpoint), mode1bit, format_if_mount_failed); }

    GSheet_Values values;
    GSheet_Sheets sheets;
    GSheet_Metadata developerMetadata;

private:
    GSheetClass *gsheet = NULL;

protected:
    template <typename T>
    auto toString(const T &val) -> typename mb_string::enable_if<mb_string::is_std_string<T>::value || mb_string::is_arduino_string<T>::value || mb_string::is_mb_string<T>::value || mb_string::is_same<T, StringSumHelper>::value, const char *>::type { return val.c_str(); }

    template <typename T>
    auto toString(T val) -> typename mb_string::enable_if<mb_string::is_const_chars<T>::value, const char *>::type { return val; }

    template <typename T>
    auto toString(T val) -> typename mb_string::enable_if<mb_string::fs_t<T>::value, const char *>::type { return (const char *)val; }

    template <typename T>
    auto toString(T val) -> typename mb_string::enable_if<mb_string::is_same<T, std::nullptr_t>::value, const char *>::type { return ""; }
};

extern ESP_Google_Sheet_Client GSheet;

#endif