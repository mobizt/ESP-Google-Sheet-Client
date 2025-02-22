
/**
 * The MIT License (MIT)
 * Copyright (c) 2025 K. Suwatchai (Mobizt)
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

#ifndef GS_TCP_Client_H
#define GS_TCP_Client_H

#include <Arduino.h>
#include "../GS_Error.h"
#include "../GS_Const.h"
#include "../mbfs/MB_FS.h"
#include "../GS_Helper.h"
#if __has_include(<ESP_SSLClient.h>)
#include <ESP_SSLClient.h>
#else
#include "./client/SSLClient/ESP_SSLClient.h"
#endif
#include "../GS_Network.h"

#if defined(ESP32)
#include "IPAddress.h"
#include "lwip/sockets.h"

#if defined(ESP_GOOGLE_SHEET_CLIENT_WIFI_IS_AVAILABLE)
#define WIFI_HAS_HOST_BY_NAME
#endif
#include "../client/WiFiClientImpl.h"
#define BASE_WIFICLIENT WiFiClientImpl

#elif defined(ESP_GOOGLE_SHEET_CLIENT_WIFI_IS_AVAILABLE)
#include "WiFiClient.h"
#define BASE_WIFICLIENT WiFiClient
#endif

#pragma GCC diagnostic ignored "-Wdelete-non-virtual-dtor"
#pragma GCC diagnostic ignored "-Wunused-variable"

typedef enum
{
  esp_google_sheet_client_type_undefined,
  esp_google_sheet_client_type_internal_basic_client,
  esp_google_sheet_client_type_external_basic_client,
  esp_google_sheet_client_type_external_gsm_client

} esp_google_sheet_client_type;

class GS_TCP_Client : public Client
{
  friend class GAuthManager;
  friend class ESP_Google_Sheet_Client;

public:
  GS_TCP_Client()
  {
    _tcp_client = new ESP_SSLClient();
  }

  virtual ~GS_TCP_Client()
  {
    clear();
    if (_tcp_client)
      delete (ESP_SSLClient *)_tcp_client;
    _tcp_client = nullptr;
  }

  /**
   * Set the client.
   * @param client The Client interface.
   */
  void setClient(Client *client, ESP_GOOGLE_SHEET_CLIENT_NetworkConnectionRequestCallback networkConnectionCB,
                 ESP_GOOGLE_SHEET_CLIENT_NetworkStatusRequestCallback networkStatusCB)
  {

    clear();
    _basic_client = client;
    _client_type = esp_google_sheet_client_type_external_basic_client;
    _network_connection_cb = networkConnectionCB;
    _network_status_cb = networkStatusCB;
  }

  /** Assign TinyGsm Clients.
   *
   * @param client The pointer to TinyGsmClient.
   * @param modem The pointer to TinyGsm modem object. Modem should be initialized and/or set mode before transfering data
   * @param pin The SIM pin.
   * @param apn The GPRS APN (Access Point Name).
   * @param user The GPRS user.
   * @param password The GPRS password.
   */
  void setGSMClient(Client *client, void *modem = nullptr, const char *pin = nullptr, const char *apn = nullptr, const char *user = nullptr, const char *password = nullptr)
  {
#if defined(ESP_GOOGLE_SHEET_CLIENT_GSM_MODEM_IS_AVAILABLE)
    _pin = pin;
    _apn = apn;
    _user = user;
    _password = password;
    _modem = modem;
    _basic_client = client;
    _client_type = esp_google_sheet_client_type_external_gsm_client;
#endif
  }

  void setX509Time(time_t now)
  {
    if (_tcp_client)
    {
      _tcp_client->setX509Time(now);
      if (now > ESP_SSLCLIENT_VALID_TIMESTAMP)
        _clock_ready = true;
    }
  }

  /**
   * Set Root CA certificate to verify.
   * @param caCert The certificate.
   */
  void setCACert(const char *caCert)
  {
    if (caCert)
    {
      if (_x509)
        delete _x509;

      _x509 = new X509List(caCert);
      _tcp_client->setTrustAnchors(_x509);

      setCertType(esp_google_sheet_cert_type_data);
    }
    else
    {
      setCertType(esp_google_sheet_cert_type_none);
      setInSecure();
    }
  }

  /**
   * Set Root CA certificate to verify.
   * @param certFile The certificate file path.
   * @param storageType The storage type mb_fs_mem_storage_type_flash or mb_fs_mem_storage_type_sd.
   * @return true when certificate loaded successfully.
   */
  bool setCertFile(const char *certFile, mb_fs_mem_storage_type storageType)
  {
    if (!_mbfs)
      return false;

    if (_clock_ready && strlen(certFile) > 0)
    {
      MB_String filename = certFile;
      if (filename.length() > 0)
      {
        if (filename[0] != '/')
          filename.prepend('/');
      }

      int len = _mbfs->open(filename, storageType, mb_fs_open_mode_read);
      if (len > -1)
      {
        uint8_t *der = (uint8_t *)_mbfs->newP(len);
        if (_mbfs->available(storageType))
          _mbfs->read(storageType, der, len);
        _mbfs->close(storageType);

        if (_x509)
          delete _x509;

        _x509 = new X509List(der, len);
        _tcp_client->setTrustAnchors(_x509);
        _mbfs->delP(&der);

        setCertType(esp_google_sheet_cert_type_file);
      }
    }

    return getCertType() == esp_google_sheet_cert_type_file;
  }

  /**
   * Set TCP connection time out in seconds.
   * @param timeoutSec The time out in seconds.
   */
  void setTimeout(uint32_t timeoutSec)
  {
    _tcp_client->setTimeout(timeoutSec);
  }

  /**  Set the BearSSL IO buffer size.
   *
   * @param rx The BearSSL receive buffer size in bytes.
   * @param tx The BearSSL trasmit buffer size in bytes.
   */
  void setIOBufferSize(int rx, int tx)
  {
    _rx_size = rx;
    _tx_size = tx;
  }

  /**
   * Get the ethernet link status.
   * @return true for link up or false for link down.
   */
  bool ethLinkUp()
  {
    bool ret = false;

#if defined(ESP_GOOGLE_SHEET_CLIENT_ETH_IS_AVAILABLE)

#if defined(ESP32)
    if (strcmp(ETH.localIP().toString().c_str(), (const char *)MBSTRING_FLASH_MCR("0.0.0.0")) != 0)
    {
      ETH.linkUp();
      ret = true;
    }
#elif defined(ESP8266) || defined(MB_ARDUINO_PICO)
    if (!eth && _config)
      eth = &(_config->spi_ethernet_module);

    if (!eth)
      return false;

    bool ret = false;
#if defined(ESP8266) && defined(ESP8266_CORE_SDK_V3_X_X)

#if defined(INC_ENC28J60_LWIP)
    if (eth->enc28j60)
    {
      ret = eth->enc28j60->status() == WL_CONNECTED;
      goto ex;
    }
#endif
#if defined(INC_W5100_LWIP)
    if (eth->w5100)
    {
      ret = eth->w5100->status() == WL_CONNECTED;
      goto ex;
    }
#endif
#if defined(INC_W5500_LWIP)
    if (eth->w5500)
    {
      ret = eth->w5500->status() == WL_CONNECTED;
      goto ex;
    }
#endif

#elif defined(MB_ARDUINO_PICO)

#endif

    return ret;

#if defined(INC_ENC28J60_LWIP) || defined(INC_W5100_LWIP) || defined(INC_W5500_LWIP)
  ex:
#endif

    // workaround for ESP8266 Ethernet
    delayMicroseconds(0);

    return ret;
#endif

#endif

    return ret;
  }

  /**
   * Checking for valid IP.
   * @return true for valid.
   */
  bool validIP(IPAddress ip)
  {
    char buf[16];
    sprintf(buf, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
    return strcmp(buf, "0.0.0.0") != 0;
  }

  void ethDNSWorkAround(SPI_ETH_Module *eth, const char *host, uint16_t port)
  {
#if defined(ESP_GOOGLE_SHEET_CLIENT_ETH_IS_AVAILABLE)
    if (!eth)
      return;

#if defined(ESP8266) && defined(ESP8266_CORE_SDK_V3_X_X)

#if defined(INC_ENC28J60_LWIP)
    if (eth->enc28j60)
      goto ex;
#endif
#if defined(INC_W5100_LWIP)
    if (eth->w5100)
      goto ex;
#endif
#if defined(INC_W5500_LWIP)
    if (eth->w5500)
      goto ex;
#endif

#elif defined(MB_ARDUINO_PICO)

#endif

    return;

#if defined(INC_ENC28J60_LWIP) || defined(INC_W5100_LWIP) || defined(INC_W5500_LWIP)
  ex:
#if defined(ESP_GOOGLE_SHEET_CLIENT_WIFI_IS_AVAILABLE)
    BASE_WIFICLIENT _client;
    _client.connect(host, port);
    _client.stop();
#endif
#endif
#endif
  }

  /**
   * Get the network status.
   * @return true for connected or false for not connected.
   */
  bool networkReady()
  {

    // We will not invoke the network status request when device has built-in WiFi or Ethernet and it is connected.

    if (_client_type == esp_google_sheet_client_type_external_gsm_client)
    {
      _network_status = gprsConnected();
      if (!_network_status)
        gprsConnect();
    }
    else if (WiFI_CONNECTED || ethLinkUp())
      _network_status = true;
    else if (_client_type == esp_google_sheet_client_type_external_basic_client)
    {
      if (!_network_status_cb)
        _last_error = 1;
      else
        _network_status_cb();
    }

    return _network_status;
  }

  /**
   * Reconnect the network.
   */
  void networkReconnect()
  {

    if (_client_type == esp_google_sheet_client_type_external_basic_client)
    {
#if defined(ESP_GOOGLE_SHEET_CLIENT_HAS_WIFI_DISCONNECT)
      // We can reconnect WiFi when device connected via built-in WiFi that supports reconnect
      if (WiFI_CONNECTED)
      {
        WiFi.reconnect();
        return;
      }

#endif

      if (_network_connection_cb)
        _network_connection_cb();
    }
    else if (_client_type == esp_google_sheet_client_type_external_gsm_client)
    {
      gprsDisconnect();
      gprsConnect();
    }
    else if (_client_type == esp_google_sheet_client_type_internal_basic_client)
    {

#if defined(ESP_GOOGLE_SHEET_CLIENT_WIFI_IS_AVAILABLE)
#if defined(ESP32) || defined(ESP8266)
      WiFi.reconnect();
#else
      if (_wifi_multi && _wifi_multi->credentials.size())
        _wifi_multi->reconnect();
#endif
#endif
    }
  }

  /**
   * Disconnect the network.
   */
  void networkDisconnect() {}

  /**
   * Get the Client type.
   * @return The  esp_google_sheet_client_type enum value.
   */
  esp_google_sheet_client_type type() { return _client_type; }

  /**
   * Get the Client initialization status.
   * @return The initialization status.
   */
  bool isInitialized()
  {
    bool rdy = true;
#if !defined(ESP_GOOGLE_SHEET_CLIENT_WIFI_IS_AVAILABLE)

    if (_client_type == esp_google_sheet_client_type_external_basic_client &&
        (!_network_connection_cb || !_network_status_cb))
      rdy = false;
    else if (_client_type != esp_google_sheet_client_type_external_basic_client &&
             _client_type != esp_google_sheet_client_type_external_gsm_client)
      rdy = false;
#else
    // assume external client is WiFiClient and network status request callback is not required
    // when device was connected to network using on board WiFi
    if (_client_type == esp_google_sheet_client_type_external_basic_client &&
        (!_network_connection_cb || (!_network_status_cb && !WiFI_CONNECTED && !ethLinkUp())))
    {
      rdy = false;
    }

#endif

    if (!rdy)
    {
      if (!_network_connection_cb)
        setError(ESP_GOOGLE_SHEET_CLIENT_ERROR_TCP_CLIENT_MISSING_NETWORK_CONNECTION_CB);

      if (!WiFI_CONNECTED && !ethLinkUp())
      {
        if (!_network_status_cb)
          setError(ESP_GOOGLE_SHEET_CLIENT_ERROR_TCP_CLIENT_MISSING_NETWORK_STATUS_CB);
      }
    }

    return rdy;
  }

  /**
   * Set Root CA certificate to verify.
   * @param name The host name.
   * @param ip The ip address result.
   * @return 1 for success or 0 for failed.
   */
  int hostByName(const char *name, IPAddress &ip)
  {
#if defined(ESP_GOOGLE_SHEET_CLIENT_WIFI_IS_AVAILABLE)
    return WiFi.hostByName(name, ip);
#else
    return 1;
#endif
  }

  /**
   * Store the host name and port.
   * @param host The host name to connect.
   * @param port The port to connect.
   * @return true.
   */
  bool begin(const char *host, uint16_t port, int *response_code)
  {
    _host = host;
    _port = port;
    _tcp_client->setBufferSizes(_rx_size, _tx_size);
     _tcp_client->setSessionTimeout(150);
    _last_error = 0;
    this->response_code = response_code;
    return true;
  }

  void setInsecure()
  {
    _tcp_client->setInsecure();
  };

  void setBufferSizes(int rx, int tx)
  {
    _rx_size = rx;
    _tx_size = tx;
  }

  operator bool()
  {
    return connected();
  }

  /**
   * Get the TCP connection status.
   * @return true for connected or false for not connected.
   */
  uint8_t connected() { return _tcp_client && _tcp_client->connected(); };

  bool connect()
  {
    if (!_tcp_client)
      return false;

    _tcp_client->enableSSL(true);

    _last_error = 0;

    if (connected())
    {
      flush();
      return true;
    }

    if (!_basic_client)
    {
      if (_client_type == esp_google_sheet_client_type_external_basic_client)
      {
        _last_error = 1;
        return false;
      }
      else if (_client_type != esp_google_sheet_client_type_external_gsm_client)
      {
// Device has no built-in WiFi, external client required.
#if defined(ESP_GOOGLE_SHEET_CLIENT_WIFI_IS_AVAILABLE)
        _basic_client = new BASE_WIFICLIENT();
        _client_type = esp_google_sheet_client_type_internal_basic_client;
#else
        _last_error = 1;
        return false;
#endif
      }
    }

    _tcp_client->setClient(_basic_client);
    _tcp_client->setDebugLevel(2);
    if (!_tcp_client->connect(_host.c_str(), _port))
      return setError(ESP_GOOGLE_SHEET_CLIENT_ERROR_TCP_ERROR_CONNECTION_REFUSED);

#if defined(ESP_GOOGLE_SHEET_CLIENT_WIFI_IS_AVAILABLE) && (defined(ESP32) || defined(ESP8266) || defined(MB_ARDUINO_PICO))
    if (_client_type == esp_google_sheet_client_type_internal_basic_client)
      reinterpret_cast<BASE_WIFICLIENT *>(_basic_client)->setNoDelay(true);
#endif

    // For TCP keepalive should work in ESP8266 core > 3.1.2.
    // https://github.com/esp8266/Arduino/pull/8940

    if (_client_type == esp_google_sheet_client_type_internal_basic_client)
    {
      if (isKeepAliveSet())
      {
#if defined(ESP_GOOGLE_SHEET_CLIENT_WIFI_IS_AVAILABLE)

#if defined(ESP8266)
        if (_tcpKeepIdleSeconds == 0 || _tcpKeepIntervalSeconds == 0 || _tcpKeepCount == 0)
          reinterpret_cast<BASE_WIFICLIENT *>(_basic_client)->disableKeepAlive();
        else
          reinterpret_cast<BASE_WIFICLIENT *>(_basic_client)->keepAlive(_tcpKeepIdleSeconds, _tcpKeepIntervalSeconds, _tcpKeepCount);

#elif defined(ESP32)

        if (_tcpKeepIdleSeconds == 0 || _tcpKeepIntervalSeconds == 0 || _tcpKeepCount == 0)
        {
          _tcpKeepIdleSeconds = 0;
          _tcpKeepIntervalSeconds = 0;
          _tcpKeepCount = 0;
        }

        bool success = setOption(TCP_KEEPIDLE, &_tcpKeepIdleSeconds) > -1 &&
                       setOption(TCP_KEEPINTVL, &_tcpKeepIntervalSeconds) > -1 &&
                       setOption(TCP_KEEPCNT, &_tcpKeepCount) > -1;
        if (!success)
          _isKeepAlive = false;
#endif

#endif
      }
    }

    bool ret = connected();

    if (!ret)
      stop();

    return ret;
  }

  /**
   * Stop TCP connection.
   */
  void stop()
  {
    if (_tcp_client)
      _tcp_client->stop();
  }

  int setError(int code)
  {
    if (!response_code)
      return -1000;

    *response_code = code;
    return *response_code;
  }

  size_t write(const uint8_t *data, size_t size)
  {

    if (!_tcp_client)
      return setError(ESP_GOOGLE_SHEET_CLIENT_ERROR_TCP_CLIENT_NOT_INITIALIZED);

    if (!data || size == 0)
      return setError(ESP_GOOGLE_SHEET_CLIENT_ERROR_TCP_ERROR_SEND_REQUEST_FAILED);

    if (!networkReady())
      return setError(ESP_GOOGLE_SHEET_CLIENT_ERROR_TCP_ERROR_NOT_CONNECTED);

    if (!_tcp_client->connected() && !connect())
      return setError(ESP_GOOGLE_SHEET_CLIENT_ERROR_TCP_ERROR_CONNECTION_REFUSED);

    int toSend = _chunkSize;
    int sent = 0;
    while (sent < (int)size)
    {
      if (sent + toSend > (int)size)
        toSend = size - sent;

      if ((int)_tcp_client->write(data + sent, toSend) != toSend)
        return ESP_GOOGLE_SHEET_CLIENT_ERROR_TCP_ERROR_SEND_REQUEST_FAILED;

      sent += toSend;
    }

    setError(ESP_GOOGLE_SHEET_CLIENT_ERROR_HTTP_CODE_OK);

    return size;
  }

  size_t write(uint8_t v)
  {
    uint8_t buf[1];
    buf[0] = v;
    return write(buf, 1);
  }

  /**
   * The TCP data send function.
   * @param data The data to send.
   * @return The size of data that was successfully sent or 0 for error.
   */
  int send(const char *data) { return write((uint8_t *)data, strlen(data)); }

  /**
   * The TCP data print function.
   * @param data The data to print.
   * @return The size of data that was successfully print or 0 for error.
   */
  int print(const char *data) { return send(data); }

  /**
   * The TCP data print function.
   * @param data The data to print.
   * @return The size of data that was successfully print or 0 for error.
   */
  int print(int data)
  {
    char buf[64];
    memset(buf, 0, 64);
    sprintf(buf, (const char *)FPSTR("%d"), data);
    int ret = send(buf);
    return ret;
  }

  /**
   * The TCP data print with new line function.
   * @param data The data to print.
   * @return The size of data that was successfully print or 0 for error.
   */
  int println(const char *data)
  {
    int len = send(data);
    if (len < 0)
      return len;
    int sz = send((const char *)FPSTR("\r\n"));
    if (sz < 0)
      return sz;
    return len + sz;
  }

  /**
   * The TCP data print with new line function.
   * @param data The data to print.
   * @return The size of data that was successfully print or 0 for error.
   */
  int println(int data)
  {
    char buf[64];
    memset(buf, 0, 64);
    sprintf(buf, (const char *)FPSTR("%d\r\n"), data);
    int ret = send(buf);
    return ret;
  }

  int available()
  {
    if (!_tcp_client)
      return setError(ESP_GOOGLE_SHEET_CLIENT_ERROR_TCP_CLIENT_NOT_INITIALIZED);

    return _tcp_client->available();
  }

  /**
   * The TCP data read function.
   * @return The read value or -1 for error.
   */
  int read()
  {
    if (!_basic_client)
      return setError(ESP_GOOGLE_SHEET_CLIENT_ERROR_TCP_CLIENT_NOT_INITIALIZED);

    return _tcp_client->read();
  }

  int read(uint8_t *buf, size_t len)
  {
    return readBytes(buf, len);
  }

  /**
   * The TCP data read function.
   * @param buf The data buffer.
   * @param len The length of data that read.
   * @return The size of data that was successfully read or negative value for error.
   */
  int readBytes(uint8_t *buf, int len)
  {
    if (!_basic_client)
      return setError(ESP_GOOGLE_SHEET_CLIENT_ERROR_TCP_CLIENT_NOT_INITIALIZED);

    return _tcp_client->read(buf, len);
  }

  /**
   * The TCP data read function.
   * @param buf The data buffer.
   * @param len The length of data that read.
   * @return The size of data that was successfully read or negative value for error.
   */
  int readBytes(char *buf, int len)
  {
    return readBytes((uint8_t *)buf, len);
  }

  /**
   * Wait for all receive buffer data read.
   */
  void flush()
  {
    if (_tcp_client)
      _tcp_client->flush();
  }

  /**
   * Set the network status which should call in side the networkStatusRequestCallback function.
   * @param status The status of network.
   */
  void setNetworkStatus(bool status)
  {
    _network_status = status;
  }

  int peek()
  {
    if (!_tcp_client)
      return 0;
    return _tcp_client->peek();
  }

  int connect(IPAddress ip, uint16_t port)
  {
    _ip = ip;
    _port = port;
    return connect();
  }

  int connect(const char *host, uint16_t port)
  {
    _host = host;
    _port = port;
    return connect();
  }

#if defined(ESP32_ARDUINO_CORE_CLIENT_CONNECT_HAS_TMO)
  int connect(IPAddress ip, uint16_t port, int32_t timeout)
  {
    _tcp_client->setTimeout(timeout);
    return connect(ip, port);
  }

  int connect(const char *host, uint16_t port, int32_t timeout)
  {
    _tcp_client->setTimeout(timeout);
    return connect(host, port);
  }
#endif

  void setConfig(esp_google_sheet_auth_cfg_t *config, MB_FS *mbfs)
  {
    _config = config;
    _mbfs = mbfs;
  }

  void setClockStatus(bool status)
  {
    _clock_ready = status;
  }

  void setCertType(esp_google_sheet_cert_type type) { _cert_type = type; }

  esp_google_sheet_cert_type getCertType() { return _cert_type; }

  unsigned long tcpTimeout()
  {
    if (_tcp_client)
      return 1000 * _tcp_client->getTimeout();
    return 0;
  }

  void disconnect() {};

  void keepAlive(int tcpKeepIdleSeconds, int tcpKeepIntervalSeconds, int tcpKeepCount)
  {
    _tcpKeepIdleSeconds = tcpKeepIdleSeconds;
    _tcpKeepIntervalSeconds = tcpKeepIntervalSeconds;
    _tcpKeepCount = tcpKeepCount;
    _isKeepAlive = tcpKeepIdleSeconds > 0 && tcpKeepIntervalSeconds > 0 && tcpKeepCount > 0;
  }

  bool isKeepAliveSet() { return _tcpKeepIdleSeconds > -1 && _tcpKeepIntervalSeconds > -1 && _tcpKeepCount > -1; };

  bool isKeepAlive() { return _isKeepAlive; };

  void clear()
  {
    if (_basic_client && _client_type == esp_google_sheet_client_type_internal_basic_client)
    {
#if defined(ESP_GOOGLE_SHEET_CLIENT_WIFI_IS_AVAILABLE)
      delete (BASE_WIFICLIENT *)_basic_client;
#else
      delete _basic_client;
#endif
      _basic_client = nullptr;
    }
  }

  void setWiFi(esp_google_sheet_wifi *wifi) { _wifi_multi = wifi; }

  bool gprsConnect()
  {
#if defined(ESP_GOOGLE_SHEET_CLIENT_GSM_MODEM_IS_AVAILABLE)
    TinyGsm *gsmModem = (TinyGsm *)_modem;
    if (gsmModem)
    {
      // Unlock your SIM card with a PIN if needed
      if (_pin.length() && gsmModem->getSimStatus() != 3)
        gsmModem->simUnlock(_pin.c_str());

#if defined(TINY_GSM_MODEM_XBEE)
      // The XBee must run the gprsConnect function BEFORE waiting for network!
      gsmModem->gprsConnect(_apn.c_str(), _user.c_str(), _password.c_str());
#endif

      if (_last_error == 0)
        Serial.print((const char *)MBSTRING_FLASH_MCR("Waiting for network..."));

      if (!gsmModem->waitForNetwork())
      {

        if (_last_error == 0)
          Serial.println((const char *)MBSTRING_FLASH_MCR(" fail"));

        _last_error = 1;
        _network_status = false;
        return false;
      }

      if (_last_error == 0)
        Serial.println((const char *)MBSTRING_FLASH_MCR(" success"));

      if (gsmModem->isNetworkConnected())
      {
        if (_last_error == 0)
        {
          Serial.print((const char *)MBSTRING_FLASH_MCR("Connecting to "));
          Serial.print(_apn.c_str());
        }

        _network_status = gsmModem->gprsConnect(_apn.c_str(), _user.c_str(), _password.c_str()) &&
                          gsmModem->isGprsConnected();

        if (_last_error == 0)
        {
          if (_network_status)
            Serial.println((const char *)MBSTRING_FLASH_MCR(" success"));
          else
            Serial.println((const char *)MBSTRING_FLASH_MCR(" fail"));
        }
      }

      if (!_network_status)
        _last_error = 1;

      return _network_status;
    }

#endif
    return false;
  }

  bool gprsConnected()
  {
#if defined(ESP_GOOGLE_SHEET_CLIENT_GSM_MODEM_IS_AVAILABLE)
    TinyGsm *gsmModem = (TinyGsm *)_modem;
    _network_status = gsmModem && gsmModem->isGprsConnected();
#endif
    return _network_status;
  }

  bool gprsDisconnect()
  {
#if defined(ESP_GOOGLE_SHEET_CLIENT_GSM_MODEM_IS_AVAILABLE)
    TinyGsm *gsmModem = (TinyGsm *)_modem;
    _network_status = gsmModem && gsmModem->gprsDisconnect();
#endif
    return !_network_status;
  }

  uint32_t gprsGetTime()
  {
#if defined(ESP_GOOGLE_SHEET_CLIENT_GSM_MODEM_IS_AVAILABLE) && defined(TINY_GSM_MODEM_HAS_TIME)
    if (!gprsConnected())
      return 0;
    TinyGsm *gsmModem = (TinyGsm *)_modem;
    int year3 = 0;
    int month3 = 0;
    int day3 = 0;
    int hour3 = 0;
    int min3 = 0;
    int sec3 = 0;
    float timezone = 0;
    for (int8_t i = 5; i; i--)
    {
      if (gsmModem->getNetworkTime(&year3, &month3, &day3, &hour3, &min3, &sec3, &timezone))
      {
        // We have to subtract the local GSM network timezone to get GMT time
        return TimeHelper::getTimestamp(year3, month3, day3, hour3, min3, sec3) - timezone * 3600;
      }
    }
#endif
    return 0;
  }

  int setOption(int option, int *value)
  {
#if defined(ESP32) && defined(ESP_GOOGLE_SHEET_CLIENT_WIFI_IS_AVAILABLE)
    // Actually we wish to use setSocketOption directly but it is ambiguous in old ESP32 core v1.0.x.;
    // Use setOption instead for old core support.
    return reinterpret_cast<BASE_WIFICLIENT *>(_basic_client)->setOption(option, value);
#endif
    return 0;
  }

  void setInSecure()
  {
    _tcp_client->setInsecure();
  }

private:
  // lwIP TCP Keepalive idle in seconds.
  int _tcpKeepIdleSeconds = -1;
  // lwIP TCP Keepalive interval in seconds.
  int _tcpKeepIntervalSeconds = -1;
  // lwIP TCP Keepalive count.
  int _tcpKeepCount = -1;
  bool _isKeepAlive = false;

  ESP_SSLClient *_tcp_client = nullptr;
  X509List *_x509 = nullptr;

  MB_String _host;
  uint16_t _port = 443;
  IPAddress _ip;

  MB_FS *_mbfs = nullptr;
  Client *_basic_client = nullptr;
  esp_google_sheet_wifi *_wifi_multi = nullptr;

  ESP_GOOGLE_SHEET_CLIENT_NetworkConnectionRequestCallback _network_connection_cb = NULL;
  ESP_GOOGLE_SHEET_CLIENT_NetworkStatusRequestCallback _network_status_cb = NULL;

#if defined(ESP_GOOGLE_SHEET_CLIENT_HAS_WIFIMULTI)
  WiFiMulti *_multi = nullptr;
#endif
#if defined(ESP_GOOGLE_SHEET_CLIENT_GSM_MODEM_IS_AVAILABLE)
  MB_String _pin, _apn, _user, _password;
  void *_modem = nullptr;
#endif
  int _chunkSize = 1024;
  bool _clock_ready = false;
  int _last_error = 0;
  volatile bool _network_status = false;
  int _rx_size = 1024, _tx_size = 512;
  int *response_code = nullptr;
  esp_google_sheet_auth_cfg_t *_config = nullptr;

  esp_google_sheet_cert_type _cert_type = esp_google_sheet_cert_type_undefined;
  esp_google_sheet_client_type _client_type = esp_google_sheet_client_type_undefined;

#if defined(ESP8266) || defined(MB_ARDUINO_PICO)
  SPI_ETH_Module *eth = NULL;
#endif
};

#endif /* GS_TCP_Client_H */
