/* PersWiFiManager
   version 3.0.1
   https://r-downing.github.io/PersWiFiManager/

   modified for inclusion in gbs-control
   see /3rdparty/PersWiFiManager/ for original code and license
*/
#include "config.h"
#if (defined(ESP8266) || defined(ESP32)) && ENABLE_WIFI
#ifndef PERSWIFIMANAGER_H
#define PERSWIFIMANAGER_H

#include "platform.h" // wifi, async tcp, web server
#include <DNSServer.h>

// WIFI_CONNECT_TIMEOUT, WIFI_AP_RETRY_INTERVAL, WIFI_AP_RETRY_DURATION: see config.h

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

    // connect to a new network right away, without a restart; a running access point stays on
    void connectKeepAp(const String &ssid, const String &pass);
    // station state for the web ui: "connected", "connecting", "failed" or "off"
    const char *staState();
    bool apActive();
    // switch the access point off; only while the station is connected (returns false otherwise)
    bool stopAp();

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
    bool _connectFailed = false;    // last connection attempt timed out / failed
    bool _connectNotified = false; // connect handler called (once per boot)

    void handleApRetry();

    WiFiChangeHandlerFunction _connectHandler;
    WiFiChangeHandlerFunction _apHandler;

}; //class

#endif
#endif
