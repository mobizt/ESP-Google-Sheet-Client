#ifndef GS_HELPER_H
#define GS_HELPER_H

#include <Arduino.h>
#include "mbfs/MB_MCU.h"
#include "ESP_Google_Sheet_Client_FS_Config.h"
#include <time.h>

#if !defined(__AVR__)
#include <vector>
#include <functional>
#endif

#include "GS_Const.h"
#if defined(ESP8266)
#include <Schedule.h>
#elif defined(MB_ARDUINO_PICO)
#include <WiFi.h>
#include <WiFiNTP.h>
#endif

#include "mbfs/MB_FS.h"

class GS_TCP_Client;

namespace Utils
{
    inline void idle()
    {
#if defined(ARDUINO_ESP8266_MAJOR) && defined(ARDUINO_ESP8266_MINOR) && defined(ARDUINO_ESP8266_REVISION) && ((ARDUINO_ESP8266_MAJOR == 3 && ARDUINO_ESP8266_MINOR >= 1) || ARDUINO_ESP8266_MAJOR > 3)
        esp_yield();
#else
        delay(0);
#endif
    }
};

namespace MemoryHelper
{

    template <typename T>
    inline T createBuffer(MB_FS *mbfs, size_t size, bool clear = true)
    {
        return reinterpret_cast<T>(mbfs->newP(size, clear));
    }

    template <typename T>
    inline T creatDownloadBuffer(MB_FS *mbfs, int &bufLen, bool clear = false)
    {
        if (bufLen < 512)
            bufLen = 512;

        if (bufLen > 1024 * 16)
            bufLen = 1024 * 16;

        return createBuffer<T>(mbfs, bufLen, clear);
    }

    inline void freeBuffer(MB_FS *mbfs, void *ptr)
    {
        mbfs->delP(&ptr);
    }

};

namespace TimeHelper
{

    inline time_t getTime(uint32_t *mb_ts, uint32_t *mb_ts_offset)
    {
        uint32_t &tm = *mb_ts;
#if defined(ENABLE_EXTERNAL_CLIENT) || defined(MB_ARDUINO_PICO)
        tm = *mb_ts_offset + millis() / 1000;

#if defined(MB_ARDUINO_PICO)
        if (tm < time(nullptr))
            tm = time(nullptr);
#endif

#elif defined(ESP32) || defined(ESP8266)
        tm = time(nullptr);
#endif
        return tm;
    }

    inline int setTimestamp(time_t ts)
    {
#if defined(ESP32) || defined(ESP8266)
        struct timeval tm = {ts, 0}; // sec, us
        return settimeofday((const timeval *)&tm, 0);
#endif
        return -1;
    }

    inline bool setTime(time_t ts, uint32_t *mb_ts, uint32_t *mb_ts_offset)
    {
        bool ret = false;

#if defined(ESP32) || defined(ESP8266)
        ret = setTimestamp(ts) == 0;
        *mb_ts = time(nullptr);
#else
        if (ts > GS_DEFAULT_TS)
        {
            *mb_ts_offset = ts - millis() / 1000;
            *mb_ts = ts;
            ret = true;
        }
#endif

        return ret;
    }

    inline bool updateClock(MB_NTP *ntp, uint32_t *mb_ts, uint32_t *mb_ts_offset)
    {
        uint32_t ts = ntp->getTime(2000 /* wait 10000 ms */);
        if (ts > 0)
            *mb_ts_offset = ts - millis() / 1000;

        time_t now = getTime(mb_ts, mb_ts_offset);

        bool rdy = now > GS_DEFAULT_TS;

#if defined(ESP32) || defined(ESP8266)
        if (rdy && time(nullptr) < now)
            setTime(now, mb_ts, mb_ts_offset);
#endif

        return rdy;
    }

    inline bool syncClock(MB_NTP *ntp, uint32_t *mb_ts, uint32_t *mb_ts_offset, float gmtOffset, gauth_cfg_t *config)
    {

        if (!config)
            return false;

        time_t now = getTime(mb_ts, mb_ts_offset);

        config->internal.clock_rdy = (unsigned long)now > GS_DEFAULT_TS;

        if (config->internal.clock_rdy && gmtOffset == config->internal.gmt_offset)
            return true;

        if (!config->internal.clock_rdy || gmtOffset != config->internal.gmt_offset)
        {
            if (config->internal.clock_rdy && gmtOffset != config->internal.gmt_offset)
                config->internal.clock_synched = false;

            if (!config->internal.clock_synched)
            {
                config->internal.clock_synched = true;

#if defined(ENABLE_EXTERNAL_CLIENT)

                updateClock(ntp, mb_ts, mb_ts_offset);

#else

#if defined(ESP32) || defined(ESP8266) || defined(MB_ARDUINO_PICO)

#if defined(MB_ARDUINO_PICO)
                NTP.begin("pool.ntp.org", "time.nist.gov");
                NTP.waitSet();

                now = time(nullptr);
                if (now > GS_DEFAULT_TS)
                    *mb_ts_offset = now - millis() / 1000;

#else
                configTime(gmtOffset * 3600, 0, "pool.ntp.org", "time.nist.gov");
#endif

#endif

#endif
            }
        }

        now = getTime(mb_ts, mb_ts_offset);

        config->internal.clock_rdy = (unsigned long)now > GS_DEFAULT_TS;
        if (config->internal.clock_rdy)
            config->internal.gmt_offset = gmtOffset;

        return config->internal.clock_rdy;
    }

};

namespace StringHelper
{

    inline int strpos(const char *haystack, const char *needle, int offset)
    {
        if (!haystack || !needle)
            return -1;

        int hlen = strlen(haystack);
        int nlen = strlen(needle);

        if (hlen == 0 || nlen == 0)
            return -1;

        int hidx = offset, nidx = 0;
        while ((*(haystack + hidx) != '\0') && (*(needle + nidx) != '\0') && hidx < hlen)
        {
            if (*(needle + nidx) != *(haystack + hidx))
            {
                hidx++;
                nidx = 0;
            }
            else
            {
                nidx++;
                hidx++;
                if (nidx == nlen)
                    return hidx - nidx;
            }
        }

        return -1;
    }

    inline size_t getReservedLen(MB_FS *mbfs, size_t len)
    {
        return mbfs->getReservedLen(len);
    }

    inline void splitString(const MB_String &str, MB_VECTOR<MB_String> out, const char delim)
    {
        size_t current = 0, previous = 0;
        current = str.find(delim, 0);
        MB_String s;
        while (current != MB_String::npos)
        {
            s.clear();
            str.substr(s, previous, current - previous);
            s.trim();
            if (s.length() > 0)
                out.push_back(s);

            previous = current + 1;
            current = str.find(delim, previous);
            Utils::idle();
        }

        s.clear();

        if (previous > 0 && current == MB_String::npos)
            str.substr(s, previous, str.length() - previous);
        else
            s = str;
        s.trim();
        if (s.length() > 0)
            out.push_back(s);
        s.clear();
    }

    inline void pushTk(const MB_String &str, MB_VECTOR<MB_String> &tk)
    {
        MB_String s = str;
        s.trim();
        if (s.length() > 0)
            tk.push_back(s);
    }

    inline void splitTk(const MB_String &str, MB_VECTOR<MB_String> &tk, const char *delim)
    {
        size_t current, previous = 0;
        current = str.find(delim, previous);
        while (current != MB_String::npos)
        {
            pushTk(str.substr(previous, current - previous), tk);
            previous = current + strlen(delim);
            current = str.find(delim, previous);
        }
        pushTk(str.substr(previous, current - previous), tk);
    }

    inline bool find(const MB_String &src, PGM_P token, bool last, size_t offset, int &pos)
    {
        size_t ret = last ? src.find_last_of(pgm2Str(token), offset) : src.find(pgm2Str(token), offset);

        if (ret != MB_String::npos)
        {
            pos = ret;
            return true;
        }
        pos = -1;
        return false;
    }

    inline bool compare(const MB_String &src, int ofs, PGM_P token, bool caseInSensitive = false)
    {
        MB_String copy;
        src.substr(copy, ofs, strlen_P(token));
        return caseInSensitive ? (strcasecmp(pgm2Str(token), copy.c_str()) == 0) : (strcmp(pgm2Str(token), copy.c_str()) == 0);
    }

    /* convert string to boolean */
    inline bool str2Bool(const MB_String &v)
    {
        return v.length() > 0 && strcmp(v.c_str(), pgm2Str(gs_pgm_str_19 /* "true" */)) == 0;
    }

    inline MB_String intStr2Str(const MB_String &v)
    {
        return MB_String(atoi(v.c_str()));
    }

    inline MB_String boolStr2Str(const MB_String &v)
    {
        return MB_String(str2Bool(v.c_str()));
    }

    inline bool tokenSubString(const MB_String &src, MB_String &out, PGM_P token1, PGM_P token2,
                               int &ofs1, int ofs2, bool advanced)
    {
        size_t pos1 = src.find(pgm2Str(token1), ofs1);
        size_t pos2 = MB_String::npos;

        int len1 = strlen_P(token1);
        int len2 = 0;

        if (pos1 != MB_String::npos)
        {
            if (ofs2 > 0)
                pos2 = ofs2;
            else if (ofs2 == 0)
            {
                len2 = strlen_P(token2);
                pos2 = src.find(pgm2Str(token2), pos1 + len1 + 1);
            }
            else if (ofs2 == -1)
                ofs1 = pos1 + len1;

            if (pos2 == MB_String::npos)
                pos2 = src.length();

            if (pos2 != MB_String::npos)
            {
                // advanced the begin position before return
                if (advanced)
                    ofs1 = pos2 + len2;
                out = src.substr(pos1 + len1, pos2 - pos1 - len1);
                return true;
            }
        }

        return false;
    }

    inline bool tokenSubStringInt(const MB_String &buf, int &out, PGM_P token1, PGM_P token2, int &ofs1, int ofs2, bool advanced)
    {
        MB_String s;
        if (tokenSubString(buf, s, token1, token2, ofs1, ofs2, advanced))
        {
            out = atoi(s.c_str());
            return true;
        }
        return false;
    }

};

namespace URLHelper
{

    /* Append a parameter to URL */
    inline bool addParam(MB_String &url, PGM_P key, const MB_String &val, bool &hasParam, bool allowEmptyValue = false)
    {
        if (!allowEmptyValue && val.length() == 0)
            return false;

        MB_String _key(key);

        if (!hasParam && _key[0] == '&')
            _key[0] = '?';
        else if (hasParam && _key[0] == '?')
            _key[0] = '&';

        if (_key[0] != '?' && _key[0] != '&')
            url += !hasParam ? gs_pgm_str_28 /* "?" */ : gs_pgm_str_29 /* "&" */;

        if (_key[_key.length() - 1] != '=' && _key.find('=') == MB_String::npos)
            _key += gs_pgm_str_30; // "="

        url += _key;
        url += val;
        hasParam = true;
        return true;
    }

    /* Append the comma separated tokens as URL parameters */
    inline void addParamsTokens(MB_String &url, PGM_P key, MB_String val, bool &hasParam)
    {
        if (val.length() == 0)
            return;

        MB_VECTOR<MB_String> tk;
        StringHelper::splitTk(val, tk, ",");
        for (size_t i = 0; i < tk.size(); i++)
            addParam(url, key, tk[i], hasParam);
    }

    /* Append the path to URL */
    inline void addPath(MB_String &url, const MB_String &path)
    {
        if (path.length() > 0)
        {
            if (path[0] != '/')
                url += gs_pgm_str_31; // "/"
        }
        else
            url += gs_pgm_str_31; // "/"

        url += path;
    }

    inline void host2Url(MB_String &url, MB_String &host)
    {
        url = gs_pgm_str_32; // "https://"
        url += host;
    }

    inline void parse(MB_FS *mbfs, const MB_String &url, struct gs_url_info_t &info)
    {
        char *host = MemoryHelper::createBuffer<char *>(mbfs, url.length());
        char *uri = MemoryHelper::createBuffer<char *>(mbfs, url.length());
        char *auth = MemoryHelper::createBuffer<char *>(mbfs, url.length());

        int p1 = 0;
        int x = sscanf(url.c_str(), pgm2Str(gs_pgm_str_33), host, uri);
        x ? p1 = 8 : x = sscanf(url.c_str(), pgm2Str(gs_pgm_str_34), host, uri);
        x ? p1 = 7 : x = sscanf(url.c_str(), pgm2Str(gs_pgm_str_35), host, uri);

        size_t p2 = 0;
        if (x > 0)
        {
            p2 = MB_String(host).find(pgm2Str(gs_pgm_str_28), 0);
            if (p2 != MB_String::npos)
                x = sscanf(url.c_str() + p1, pgm2Str(gs_pgm_str_36), host, uri);
        }

        if (strlen(uri) > 0)
        {
            p2 = MB_String(uri).find(pgm2Str(gs_pgm_str_37), 0);
            if (p2 != MB_String::npos)
                x = sscanf(uri + p2 + 5, pgm2Str(gs_pgm_str_38), auth);
        }

        info.uri = uri;
        info.host = host;
        info.auth = auth;
        MemoryHelper::freeBuffer(mbfs, host);
        MemoryHelper::freeBuffer(mbfs, uri);
        MemoryHelper::freeBuffer(mbfs, auth);
    }

    inline void hexchar(char c, char &hex1, char &hex2)
    {
        hex1 = c / 16;
        hex2 = c % 16;
        hex1 += hex1 < 10 ? '0' : 'A' - 10;
        hex2 += hex2 < 10 ? '0' : 'A' - 10;
    }

    inline MB_String encode(const MB_String &s)
    {
        MB_String ret;
        ret.reserve(s.length() * 3 + 1);
        for (size_t i = 0, l = s.size(); i < l; i++)
        {
            char c = s[i];
            if ((c >= '0' && c <= '9') ||
                (c >= 'A' && c <= 'Z') ||
                (c >= 'a' && c <= 'z') ||
                c == '-' || c == '_' || c == '.' || c == '!' || c == '~' ||
                c == '*' || c == '\'' || c == '(' || c == ')')
            {
                ret += c;
            }
            else
            {
                ret += '%';
                char d1, d2;
                hexchar(c, d1, d2);
                ret += d1;
                ret += d2;
            }
        }
        ret.shrink_to_fit();
        return ret;
    }

};

namespace JsonHelper
{

    /* check for the JSON path or key */
    inline bool isJsonPath(PGM_P path)
    {
        return MB_String(path).find('/') != MB_String::npos;
    }

    /* convert comma separated tokens into JSON Array and set/add to JSON object */
    inline void addTokens(FirebaseJson *json, PGM_P key, const MB_String &tokens, const char *pre = "")
    {
        if (json && tokens.length() > 0)
        {
            FirebaseJsonArray arr;
            MB_VECTOR<MB_String> ta;
            StringHelper::splitTk(tokens, ta, ",");
            for (size_t i = 0; i < ta.size(); i++)
            {
                if (strlen(pre))
                {
                    MB_String s = pre;
                    s += ta[i];
                    arr.add(s);
                }
                else
                    arr.add(ta[i]);
            }

            if (ta.size() > 0)
            {
                if (isJsonPath(key))
                    json->set(pgm2Str(key), arr);
                else
                    json->add(pgm2Str(key), arr);
            }
        }
    }

    inline bool addString(FirebaseJson *json, PGM_P key, const MB_String &val)
    {
        if (json && val.length() > 0)
        {
            if (isJsonPath(key))
                json->set(pgm2Str(key), val);
            else
                json->add(pgm2Str(key), val);
            return true;
        }
        return false;
    }

    inline bool remove(FirebaseJson *json, PGM_P key)
    {
        if (json)
            return json->remove(pgm2Str(key));
        return false;
    }

    inline void addString(FirebaseJson *json, PGM_P key, const MB_String &val, bool &flag)
    {
        if (addString(json, key, val))
            flag = true;
    }

    inline void addBoolString(FirebaseJson *json, PGM_P key, const MB_String &val, bool &flag)
    {
        if (json && val.length() > 0)
        {
            if (isJsonPath(key))
                json->set(pgm2Str(key), strcmp(val.c_str(), pgm2Str(gs_pgm_str_19)) == 0 ? true : false);
            else
                json->add(pgm2Str(key), strcmp(val.c_str(), pgm2Str(gs_pgm_str_19)) == 0 ? true : false);
            flag = true;
        }
    }

    inline void addArrayString(FirebaseJson *json, PGM_P key, const MB_String &val, bool &flag)
    {
        if (val.length() > 0)
        {
            static FirebaseJsonArray arr;
            arr.clear();
            arr.setJsonArrayData(val);
            if (isJsonPath(key))
                json->set(pgm2Str(key), arr);
            else
                json->add(pgm2Str(key), arr);
            flag = true;
        }
    }

    inline void addObject(FirebaseJson *json, PGM_P key, FirebaseJson *val, bool clearAfterAdded)
    {
        if (json)
        {
            FirebaseJson js;

            if (!val)
                val = &js;

            if (isJsonPath(key))
                json->set(pgm2Str(key), *val);
            else
                json->add(pgm2Str(key), *val);

            if (clearAfterAdded && val)
                val->clear();
        }
    }

    inline void addNumberString(FirebaseJson *json, PGM_P key, const MB_String &val)
    {
        if (json && val.length() > 0)
        {
            if (isJsonPath(key))
                json->set(pgm2Str(key), atoi(val.c_str()));
            else
                json->add(pgm2Str(key), atoi(val.c_str()));
        }
    }

    inline void arrayAddObjectString(FirebaseJsonArray *arr, MB_String &val, bool clearAfterAdded)
    {
        if (arr && val.length() > 0)
        {
            FirebaseJson json(val);
            arr->add(json);
            if (clearAfterAdded)
                val.clear();
        }
    }

    inline void arrayAddObject(FirebaseJsonArray *arr, FirebaseJson *val, bool clearAfterAdded)
    {
        if (arr && val)
        {
            arr->add(*val);
            if (clearAfterAdded)
                val->clear();
        }
    }

    inline void addArray(FirebaseJson *json, PGM_P key, FirebaseJsonArray *val, bool clearAfterAdded)
    {
        if (json && val)
        {
            if (isJsonPath(key))
                json->set(pgm2Str(key), *val);
            else
                json->add(pgm2Str(key), *val);
            if (clearAfterAdded)
                val->clear();
        }
    }

    inline bool parse(FirebaseJson *json, FirebaseJsonData *result, PGM_P key)
    {
        bool ret = false;
        if (json && result)
        {
            result->clear();
            json->get(*result, pgm2Str(key));
            ret = result->success;
        }
        return ret;
    }

    inline bool setData(FirebaseJson *json, MB_String &val, bool clearAfterAdded)
    {
        bool ret = false;

        if (json && val.length() > 0)
            ret = json->setJsonData(val);

        if (clearAfterAdded)
            val.clear();

        return ret;
    }

    inline void toString(FirebaseJson *json, MB_String &out, bool clearSource, bool prettify = false)
    {
        if (json)
        {
            out.clear();
            json->toString(out, prettify);
            if (clearSource)
                json->clear();
        }
    }

    inline void clear(FirebaseJson *json)
    {
        if (json)
            json->clear();
    }

    inline void arrayClear(FirebaseJsonArray *arr)
    {
        if (arr)
            arr->clear();
    }

};

namespace Base64Helper
{

    inline int getBase64Len(int n)
    {
        int len = (4 * ceil(n / 3.0));
        return len;
    }

    inline int getBase64Padding(int n)
    {
        int pLen = getBase64Len(n);
        int uLen = ceil(4.0 * n / 3.0);
        return pLen - uLen;
    }

    inline size_t encodedLength(size_t len)
    {
        return ((len + 2) / 3 * 4) + 1;
    }

    inline int decodedLen(const char *src)
    {
        int len = strlen(src), i = len - 1, pad = 0;
        if (len < 4)
            return 0;
        while (i > 0 && src[i--] == '=')
        {
            pad++;
        }
        return (3 * (len / 4)) - pad;
    }

    inline unsigned char *creatBase64EncBuffer(MB_FS *mbfs, bool isURL)
    {
        unsigned char *base64EncBuf = MemoryHelper::createBuffer<unsigned char *>(mbfs, 65);
        strcpy_P((char *)base64EncBuf, (char *)gs_base64_table);
        if (isURL)
        {
            base64EncBuf[62] = '-';
            base64EncBuf[63] = '_';
        }
        return base64EncBuf;
    }

    inline unsigned char *creatBase64DecBuffer(MB_FS *mbfs)
    {
        unsigned char *base64DecBuf = MemoryHelper::createBuffer<unsigned char *>(mbfs, 256, false);
        memset(base64DecBuf, 0x80, 256);
        for (size_t i = 0; i < sizeof(gs_base64_table) - 1; i++)
            base64DecBuf[gs_base64_table[i]] = (unsigned char)i;
        base64DecBuf['='] = 0;
        return base64DecBuf;
    }

    template <typename T = uint8_t>
    inline bool writeOutput(MB_FS *mbfs, gs_base64_io_t<T> &out)
    {
        size_t write = out.bufWrite;
        out.bufWrite = 0;

        if (out.outC && out.outC->write((uint8_t *)out.outT, write) == write)
            return true;
        else if (out.filetype != mb_fs_mem_storage_type_undefined && mbfs->write(mbfs_type out.filetype,
                                                                                 (uint8_t *)out.outT, write) == (int)write)
            return true;
        return false;
    }

    template <typename T = uint8_t>
    inline bool setOutput(MB_FS *mbfs, uint8_t val, gs_base64_io_t<T> &out, T **pos)
    {
        if (out.outT)
        {
            if (out.outC || out.filetype != mb_fs_mem_storage_type_undefined)
            {
                out.outT[out.bufWrite++] = val;
                if (out.bufWrite == (int)out.bufLen && !writeOutput(mbfs, out))
                    return false;
            }
            else
                *(*pos)++ = (T)(val);
        }
        else if (out.outL)
            out.outL->push_back(val);

        return true;
    }

    template <typename T>
    inline bool decode(MB_FS *mbfs, unsigned char *base64DecBuf, const char *src, size_t len, gs_base64_io_t<T> &out)
    {
        // the maximum chunk size that writes to output is limited by out.bufLen, the minimum is depending on the source length
        bool ret = false;
        unsigned char *block = MemoryHelper::createBuffer<unsigned char *>(mbfs, 4, false);
        unsigned char temp;
        size_t i, count;
        int pad = 0;
        size_t extra_pad;
        T *pos = out.outT ? (T *)&out.outT[0] : nullptr;
        if (len == 0)
            len = strlen(src);

        count = 0;

        for (i = 0; i < len; i++)
        {
            if ((uint8_t)base64DecBuf[(uint8_t)src[i]] != 0x80)
                count++;
        }

        if (count == 0)
            goto skip;

        extra_pad = (4 - count % 4) % 4;
        count = 0;
        for (i = 0; i < len + extra_pad; i++)
        {
            unsigned char val;

            if (i >= len)
                val = '=';
            else
                val = src[i];

            temp = base64DecBuf[val];

            if (temp == 0x80)
                continue;

            if (val == '=')
                pad++;

            block[count] = temp;
            count++;
            if (count == 4)
            {

                setOutput(mbfs, (block[0] << 2) | (block[1] >> 4), out, &pos);

                count = 0;
                if (pad)
                {
                    if (pad == 1)
                        setOutput(mbfs, (block[1] << 4) | (block[2] >> 2), out, &pos);
                    else if (pad > 2)
                        goto skip;

                    break;
                }
                else
                {
                    setOutput(mbfs, (block[1] << 4) | (block[2] >> 2), out, &pos);
                    setOutput(mbfs, (block[2] << 6) | block[3], out, &pos);
                }
            }
        }

        // write remaining
        if (out.bufWrite > 0 && !writeOutput(mbfs, out))
            goto skip;

        ret = true;

    skip:
        MemoryHelper::freeBuffer(mbfs, block);
        return ret;
    }

    template <typename T>
    inline bool encodeLast(MB_FS *mbfs, unsigned char *base64EncBuf, const unsigned char *in, size_t len,
                           gs_base64_io_t<T> &out, T **pos)
    {
        if (len > 2)
            return false;

        if (!setOutput(mbfs, base64EncBuf[in[0] >> 2], out, pos))
            return false;

        if (len == 1)
        {
            if (!setOutput(mbfs, base64EncBuf[(in[0] & 0x03) << 4], out, pos))
                return false;
            if (!setOutput(mbfs, '=', out, pos))
                return false;
        }
        else
        {
            if (!setOutput(mbfs, base64EncBuf[((in[0] & 0x03) << 4) | (in[1] >> 4)], out, pos))
                return false;
            if (!setOutput(mbfs, base64EncBuf[(in[1] & 0x0f) << 2], out, pos))
                return false;
        }

        if (!setOutput(mbfs, '=', out, pos))
            return false;

        return true;
    }

    template <typename T>
    inline bool encode(MB_FS *mbfs, unsigned char *base64EncBuf, uint8_t *src, size_t len,
                       gs_base64_io_t<T> &out, bool writeAllRemaining = true)
    {
        const unsigned char *end, *in;

        T *pos = out.outT ? (T *)&out.outT[0] : nullptr;
        in = src;
        end = src + len;

        while (end - in >= 3)
        {
            if (!setOutput(mbfs, base64EncBuf[in[0] >> 2], out, &pos))
                return false;
            if (!setOutput(mbfs, base64EncBuf[((in[0] & 0x03) << 4) | (in[1] >> 4)], out, &pos))
                return false;
            if (!setOutput(mbfs, base64EncBuf[((in[1] & 0x0f) << 2) | (in[2] >> 6)], out, &pos))
                return false;
            if (!setOutput(mbfs, base64EncBuf[in[2] & 0x3f], out, &pos))
                return false;
            in += 3;
        }

        if (end - in && !encodeLast(mbfs, base64EncBuf, in, end - in, out, &pos))
            return false;

        if (writeAllRemaining && out.bufWrite > 0 && !writeOutput(mbfs, out))
            return false;

        return true;
    }
    template <typename T>
    inline bool decodeToArray(MB_FS *mbfs, const MB_String &src, MB_VECTOR<T> &val)
    {
        gs_base64_io_t<T> out;
        out.outL = &val;
        unsigned char *base64DecBuf = creatBase64DecBuffer(mbfs);
        bool ret = decode<T>(mbfs, base64DecBuf, src.c_str(), src.length(), out);
        MemoryHelper::freeBuffer(mbfs, base64DecBuf);
        return ret;
    }

    inline bool decodeToFile(MB_FS *mbfs, const char *src, size_t len, mbfs_file_type type)
    {
        gs_base64_io_t<uint8_t> out;
        out.filetype = type;
        uint8_t *buf = MemoryHelper::createBuffer<uint8_t *>(mbfs, out.bufLen);
        out.outT = buf;
        unsigned char *base64DecBuf = creatBase64DecBuffer(mbfs);
        bool ret = decode<uint8_t>(mbfs, base64DecBuf, src, strlen(src), out);
        MemoryHelper::freeBuffer(mbfs, buf);
        MemoryHelper::freeBuffer(mbfs, base64DecBuf);
        return ret;
    }

    inline void encodeUrl(MB_FS *mbfs, char *encoded, unsigned char *string, size_t len)
    {
        size_t i;
        char *p = encoded;
        unsigned char *base64EncBuf = creatBase64EncBuffer(mbfs, true);

        for (i = 0; i < len - 2; i += 3)
        {
            *p++ = base64EncBuf[(string[i] >> 2) & 0x3F];
            *p++ = base64EncBuf[((string[i] & 0x3) << 4) | ((int)(string[i + 1] & 0xF0) >> 4)];
            *p++ = base64EncBuf[((string[i + 1] & 0xF) << 2) | ((int)(string[i + 2] & 0xC0) >> 6)];
            *p++ = base64EncBuf[string[i + 2] & 0x3F];
        }

        if (i < len)
        {
            *p++ = base64EncBuf[(string[i] >> 2) & 0x3F];
            if (i == (len - 1))
                *p++ = base64EncBuf[((string[i] & 0x3) << 4)];
            else
            {
                *p++ = base64EncBuf[((string[i] & 0x3) << 4) | ((int)(string[i + 1] & 0xF0) >> 4)];
                *p++ = base64EncBuf[((string[i + 1] & 0xF) << 2)];
            }
        }

        *p++ = '\0';

        MemoryHelper::freeBuffer(mbfs, base64EncBuf);
    }

    inline MB_String encodeToString(MB_FS *mbfs, uint8_t *src, size_t len)
    {
        MB_String str;
        char *encoded = MemoryHelper::createBuffer<char *>(mbfs, encodedLength(len) + 1);
        gs_base64_io_t<char> out;
        out.outT = encoded;
        unsigned char *base64EncBuf = creatBase64EncBuffer(mbfs, false);
        if (encode<char>(mbfs, base64EncBuf, (uint8_t *)src, len, out))
            str = encoded;
        MemoryHelper::freeBuffer(mbfs, encoded);
        MemoryHelper::freeBuffer(mbfs, base64EncBuf);
        return str;
    }

    inline bool encodeToClient(Client *client, MB_FS *mbfs, size_t bufSize, uint8_t *data, size_t len)
    {
        gs_base64_io_t<uint8_t> out;
        out.outC = client;
        uint8_t *buf = MemoryHelper::createBuffer<uint8_t *>(mbfs, out.bufLen);
        out.outT = buf;
        unsigned char *base64EncBuf = creatBase64EncBuffer(mbfs, false);
        bool ret = encode<uint8_t>(mbfs, base64EncBuf, (uint8_t *)data, len, out);
        MemoryHelper::freeBuffer(mbfs, buf);
        MemoryHelper::freeBuffer(mbfs, base64EncBuf);
        return ret;
    }
};

namespace HttpHelper
{
    inline void addNewLine(MB_String &header)
    {
        header += gs_pgm_str_1; // "\r\n"
    }

    inline void addGAPIsHost(MB_String &str, PGM_P sub)
    {
        str += sub;
        if (str[str.length() - 1] != '.')
            str += gs_pgm_str_2; // "."
        str += gs_pgm_str_3;     // "googleapis.com"
    }

    inline void addGAPIsHostHeader(MB_String &header, PGM_P sub)
    {
        header += gs_pgm_str_4; // "Host: "
        addGAPIsHost(header, sub);
        addNewLine(header);
    }

    inline void addHostHeader(MB_String &header, PGM_P host)
    {
        header += gs_pgm_str_4; // "Host: "
        header += host;
        addNewLine(header);
    }

    inline void addContentTypeHeader(MB_String &header, PGM_P v)
    {
        header += gs_pgm_str_5; // "Content-Type: "
        header += v;
        header += gs_pgm_str_1; // "\r\n"
    }

    inline void addContentLengthHeader(MB_String &header, size_t len)
    {
        header += gs_pgm_str_6; // "Content-Length: "
        header += len;
        addNewLine(header);
    }

    inline void addUAHeader(MB_String &header)
    {
        header += gs_pgm_str_7; // "User-Agent: ESP\r\n"
    }

    inline void addConnectionHeader(MB_String &header, bool keepAlive)
    {
        header += keepAlive ? gs_pgm_str_8 /* "Connection: keep-alive\r\n" */
                            : gs_pgm_str_9 /* "Connection: close\r\n" */;
    }

    /* Append the string with first request line (HTTP method) */
    inline bool addRequestHeaderFirst(MB_String &header, gs_request_method method)
    {
        bool post = false;
        switch (method)
        {
        case http_get:
            header += gs_pgm_str_10; // "GET"
            break;
        case http_post:
            header += gs_pgm_str_11; // "POST"
            post = true;
            break;

        case http_patch:
            header += gs_pgm_str_12; // "PATCH"
            post = true;
            break;

        case http_delete:
            header += gs_pgm_str_13; // "DELETE"
            break;

        case http_put:
            header += gs_pgm_str_14; // "PUT"
            break;

        default:
            break;
        }

        if (method == http_get || method == http_post || method == http_patch || method == http_delete || method == http_put)
            header += gs_pgm_str_15; // " "

        return post;
    }

    /* Append the string with last request line (HTTP version) */
    inline void addRequestHeaderLast(MB_String &header)
    {
        header += gs_pgm_str_16; // " HTTP/1.1\r\n"
    }

    /* Append the string with first part of Authorization header */
    inline void addAuthHeaderFirst(MB_String &header)
    {
        header += gs_pgm_str_17; // "Authorization: "
        header += gs_pgm_str_18; // "Bearer "
    }

    inline void parseRespHeader(const MB_String &src, struct gs_server_response_data_t &response)
    {
        int beginPos = 0;

        MB_String out;

        if (response.httpCode != -1)
        {

            StringHelper::tokenSubString(src, response.connection,
                                         gs_pgm_str_20 /* "Connection: " */,
                                         gs_pgm_str_1 /* "\r\n" */, beginPos, 0, false);
            StringHelper::tokenSubString(src, response.contentType,
                                         gs_pgm_str_21 /* "Content-Type: " */,
                                         gs_pgm_str_1 /* "\r\n" */, beginPos, 0, false);
            StringHelper::tokenSubStringInt(src, response.contentLen,
                                            gs_pgm_str_22 /* "Content-Length: " */,
                                            gs_pgm_str_1 /* "\r\n" */, beginPos, 0, false);
            StringHelper::tokenSubString(src, response.etag,
                                         gs_pgm_str_23 /* "ETag: " */,
                                         gs_pgm_str_1 /* "\r\n" */, beginPos, 0, false);
            response.payloadLen = response.contentLen;

            if (StringHelper::tokenSubString(src, response.transferEnc,
                                             gs_pgm_str_24 /* "Transfer-Encoding: " */,
                                             gs_pgm_str_1 /* "\r\n" */, beginPos, 0, false) &&
                StringHelper::compare(response.transferEnc, 0, gs_pgm_str_25 /* "chunked" */))
                response.isChunkedEnc = true;

            if (response.httpCode == GS_ERROR_HTTP_CODE_OK ||
                response.httpCode == GS_ERROR_HTTP_CODE_TEMPORARY_REDIRECT ||
                response.httpCode == GS_ERROR_HTTP_CODE_PERMANENT_REDIRECT ||
                response.httpCode == GS_ERROR_HTTP_CODE_MOVED_PERMANENTLY ||
                response.httpCode == GS_ERROR_HTTP_CODE_FOUND)
                StringHelper::tokenSubString(src, response.location,
                                             gs_pgm_str_26 /* "Location: " */,
                                             gs_pgm_str_1 /* "\r\n" */, beginPos, 0, false);

            if (response.httpCode == GS_ERROR_HTTP_CODE_NO_CONTENT)
                response.noContent = true;
        }
    }

    inline int getStatusCode(const MB_String &header, int &pos)
    {
        int code = 0;
        StringHelper::tokenSubStringInt(header, code,
                                        gs_pgm_str_27 /* "HTTP/1.1 " */,
                                        gs_pgm_str_15 /* " " */, pos, 0, false);
        return code;
    }

    inline void getCustomHeaders(MB_String &header, const MB_String &tokens)
    {
        if (tokens.length() > 0)
        {
            MB_VECTOR<MB_String> headers;
            StringHelper::splitTk(tokens, headers, ",");
            for (size_t i = 0; i < headers.size(); i++)
            {
                size_t p1 = headers[i].find(F("X-Firebase-"));
                size_t p2 = headers[i].find(':');
                size_t p3 = headers[i].find(F("-ETag"));

                if (p1 != MB_String::npos && p2 != MB_String::npos && p2 > p1 && p3 == MB_String::npos)
                {
                    header += headers[i];
                    addNewLine(header);
                }
                headers[i].clear();
            }
            headers.clear();
        }
    }

    inline void intTCPHandler(GS_TCP_Client *client, struct gs_tcp_response_handler_t &tcpHandler,
                              size_t defaultChunkSize, size_t respSize, MB_String *payload)
    {
        // set the client before calling available
        tcpHandler.client = (Client *)client;
        tcpHandler.payloadLen = 0;
        tcpHandler.payloadRead = 0;
        tcpHandler.chunkBufSize = tcpHandler.available(); // client must be set before calling
        tcpHandler.defaultChunkSize = respSize;
        tcpHandler.error.code = -1;
        tcpHandler.defaultChunkSize = defaultChunkSize;
        tcpHandler.bufferAvailable = 0;
        tcpHandler.header.clear();
        tcpHandler.dataTime = millis();
        tcpHandler.payload = payload;
    }

    inline int readLine(Client *client, char *buf, int bufLen)
    {
        if (!client)
            return 0;

        int res = -1;
        char c = 0;
        int idx = 0;
        if (!client)
            return idx;
        while (client->available() && idx < bufLen)
        {
            if (!client)
                break;

            Utils::idle();

            res = client->read();
            if (res > -1)
            {
                c = (char)res;
                buf[idx++] = c;
                if (c == '\n')
                    return idx;
            }
        }
        return idx;
    }

    inline int readLine(Client *client, MB_String &buf)
    {
        if (!client)
            return 0;

        int res = -1;
        char c = 0;
        int idx = 0;
        if (!client)
            return idx;
        while (client->available())
        {
            if (!client)
                break;

            Utils::idle();

            res = client->read();
            if (res > -1)
            {
                c = (char)res;
                buf += c;
                idx++;
                if (c == '\n')
                    return idx;
            }
        }
        return idx;
    }

    inline uint32_t hex2int(const char *hex)
    {
        uint32_t val = 0;
        while (*hex)
        {
            // get current character then increment
            uint8_t byte = *hex++;
            // transform hex character to the 4bit equivalent number, using the ascii table indexes
            if (byte >= '0' && byte <= '9')
                byte = byte - '0';
            else if (byte >= 'a' && byte <= 'f')
                byte = byte - 'a' + 10;
            else if (byte >= 'A' && byte <= 'F')
                byte = byte - 'A' + 10;
            // shift 4 to make space for new digit, and add the 4 bits of the new digit
            val = (val << 4) | (byte & 0xF);
        }
        return val;
    }

    // Returns -1 when complete
    inline int readChunkedData(MB_FS *mbfs, Client *client, char *out1, MB_String *out2,
                               struct gs_tcp_response_handler_t &tcpHandler)
    {
        if (!client)
            return 0;

        int bufLen = tcpHandler.chunkBufSize;
        char *buf = nullptr;
        int p1 = 0;
        int olen = 0;

        if (tcpHandler.chunkState.state == 0)
        {
            tcpHandler.chunkState.state = 1;
            tcpHandler.chunkState.chunkedSize = -1;
            tcpHandler.chunkState.dataLen = 0;

            MB_String s;
            int readLen = 0;

            if (out2)
                readLen = readLine(client, s);
            else if (out1)
            {
                buf = MemoryHelper::createBuffer<char *>(mbfs, bufLen);
                readLen = readLine(client, buf, bufLen);
            }

            if (readLen)
            {
                if (out1)
                    s = buf;

                p1 = StringHelper::strpos(s.c_str(), (const char *)MBSTRING_FLASH_MCR(";"), 0);
                if (p1 == -1)
                    p1 = StringHelper::strpos(s.c_str(), (const char *)MBSTRING_FLASH_MCR("\r\n"), 0);

                if (p1 != -1)
                {
                    if (out2)
                        tcpHandler.chunkState.chunkedSize = hex2int(s.substr(0, p1).c_str());
                    else if (out1)
                    {
                        char *temp = MemoryHelper::createBuffer<char *>(mbfs, p1 + 1);
                        memcpy(temp, buf, p1);
                        tcpHandler.chunkState.chunkedSize = hex2int(temp);
                        MemoryHelper::freeBuffer(mbfs, temp);
                    }
                }

                // last chunk
                if (tcpHandler.chunkState.chunkedSize < 1)
                    olen = -1;
            }
            else
                tcpHandler.chunkState.state = 0;

            if (out1)
                MemoryHelper::freeBuffer(mbfs, buf);
        }
        else
        {
            if (tcpHandler.chunkState.chunkedSize > -1)
            {
                MB_String s;
                int readLen = 0;

                if (out2)
                    readLen = readLine(client, s);
                else if (out1)
                {
                    buf = MemoryHelper::createBuffer<char *>(mbfs, bufLen);
                    readLen = readLine(client, buf, bufLen);
                }

                if (readLen > 0)
                {
                    // chunk may contain trailing
                    if (tcpHandler.chunkState.dataLen + readLen - 2 < tcpHandler.chunkState.chunkedSize)
                    {
                        tcpHandler.chunkState.dataLen += readLen;
                        if (out2)
                            *out2 += s;
                        else if (out1)
                            memcpy(out1, buf, readLen);

                        olen = readLen;
                    }
                    else
                    {
                        if (tcpHandler.chunkState.chunkedSize - tcpHandler.chunkState.dataLen > 0)
                        {
                            if (out2)
                                *out2 += s;
                            else if (out1)
                                memcpy(out1, buf, tcpHandler.chunkState.chunkedSize - tcpHandler.chunkState.dataLen);
                        }

                        tcpHandler.chunkState.dataLen = tcpHandler.chunkState.chunkedSize;
                        tcpHandler.chunkState.state = 0;
                        olen = readLen;
                    }
                }
                // if all chunks read, returns -1
                else if (tcpHandler.chunkState.dataLen == tcpHandler.chunkState.chunkedSize)
                    olen = -1;

                if (out1)
                    MemoryHelper::freeBuffer(mbfs, buf);
            }
        }

        return olen;
    }

    inline bool readStatusLine(MB_FS *mbfs, Client *client, struct gs_tcp_response_handler_t &tcpHandler,
                               struct gs_server_response_data_t &response)
    {
        tcpHandler.chunkIdx++;

        if (!tcpHandler.isHeader && tcpHandler.chunkIdx > 1)
            tcpHandler.pChunkIdx++;

        if (tcpHandler.chunkIdx > 1)
            return false;

        // the first chunk (line) can be http response status or already connected stream payload
        char *hChunk = MemoryHelper::createBuffer<char *>(mbfs, tcpHandler.chunkBufSize);
        int readLen = readLine(client, hChunk, tcpHandler.chunkBufSize);
        if (readLen > 0)
            tcpHandler.header += hChunk;

        int pos = 0;
        int status = HttpHelper::getStatusCode(hChunk, pos);
        if (status > 0)
        {
            // http response status
            tcpHandler.isHeader = true;
            response.httpCode = status;
        }

        MemoryHelper::freeBuffer(mbfs, hChunk);
        return true;
    }

    inline bool readHeader(MB_FS *mbfs, Client *client, struct gs_tcp_response_handler_t &tcpHandler,
                           struct gs_server_response_data_t &response)
    {
        // do not check of the config here to allow legacy fcm to work

        char *hChunk = MemoryHelper::createBuffer<char *>(mbfs, tcpHandler.chunkBufSize);
        int readLen = readLine(client, hChunk, tcpHandler.chunkBufSize);

        // check is it the end of http header (\n or \r\n)?
        if ((readLen == 1 && hChunk[0] == '\r') || (readLen == 2 && hChunk[0] == '\r' && hChunk[1] == '\n'))
            tcpHandler.headerEnded = true;

        if (tcpHandler.headerEnded)
        {
            // parse header string to get the header field
            tcpHandler.isHeader = false;
            HttpHelper::parseRespHeader(tcpHandler.header, response);
        }
        // accumulate the remaining header field
        else if (readLen > 0)
            tcpHandler.header += hChunk;

        MemoryHelper::freeBuffer(mbfs, hChunk);
        return tcpHandler.headerEnded;
    }

};

namespace Utils
{

    inline uint16_t calCRC(MB_FS *mbfs, const char *buf)
    {
        return mbfs->calCRC(buf);
    }

    inline bool isNoContent(gs_server_response_data_t *response)
    {
        return !response->isChunkedEnc && response->contentLen == 0;
    }

    inline bool isResponseTimeout(gs_tcp_response_handler_t *tcpHandler, bool &complete)
    {
        if (millis() - tcpHandler->dataTime > 5000)
        {
            // Read all remaining data
            tcpHandler->client->flush();
            complete = true;
        }
        return complete;
    }

    inline bool isResponseComplete(gs_tcp_response_handler_t *tcpHandler, gs_server_response_data_t *response, bool &complete, bool check = true)
    {
        if (check && !response->isChunkedEnc &&
            (tcpHandler->bufferAvailable < 0 || tcpHandler->payloadRead >= response->contentLen))
        {
            // Read all remaining data
            tcpHandler->client->flush();
            complete = true;
        }
        return complete;
    }

    inline bool isChunkComplete(gs_tcp_response_handler_t *tcpHandler, gs_server_response_data_t *response, bool &complete)
    {
        if (response->isChunkedEnc && tcpHandler->bufferAvailable < 0)
        {
            // Read all remaining data
            tcpHandler->client->flush();
            complete = true;
        }
        return complete;
    }
};

#endif