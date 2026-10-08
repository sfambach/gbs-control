/* PersWiFiManager
   version 3.0.1
   https://r-downing.github.io/PersWiFiManager/

   modified for inclusion in gbs-control
   see /3rdparty/PersWiFiManager/ for original code and license
*/
#include "config.h"
#if defined(ESP8266) && ENABLE_WIFI
#ifndef PERSWIFIMANAGER_H
#define PERSWIFIMANAGER_H

#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>

#define WIFI_CONNECT_TIMEOUT 45
// in AP mode with stored credentials: retry the stored network every .._INTERVAL for .._DURATION seconds
#define WIFI_AP_RETRY_INTERVAL 60
#define WIFI_AP_RETRY_DURATION 15

class PersWiFiManager
{

public:
    typedef std::function<void(void)> WiFiChangeHandlerFunction;

    PersWiFiManager(AsyncWebServer &s, DNSServer &d);

    bool attemptConnection(const String &ssid = "", const String &pass = "");

    void setupWiFiHandlers();

    bool begin(const String &ssid = "", const String &pass = "");

    String getApSsid();

    void setApCredentials(const String &apSsid, const String &apPass = "");

    void setConnectNonBlock(bool b);

    void handleWiFi();

    void startApMode();

    void onConnect(WiFiChangeHandlerFunction fn);

    void onAp(WiFiChangeHandlerFunction fn);

private:
    AsyncWebServer *_server;
    DNSServer *_dnsServer;
    String _apSsid, _apPass;

    bool _connectNonBlock;
    unsigned long _connectStartTime;
    unsigned long _apRetryTime = 0; // != 0: AP mode fallback active, time of last retry change
    bool _apRetrying = false;       // currently trying the stored network (AP + STA)

    void handleApRetry();

    WiFiChangeHandlerFunction _connectHandler;
    WiFiChangeHandlerFunction _apHandler;

}; //class

#endif
#endif
