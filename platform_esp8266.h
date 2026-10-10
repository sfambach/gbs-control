#ifndef PLATFORM_ESP8266_H_
#define PLATFORM_ESP8266_H_

// ESP8266 (Wemos D1 mini, NodeMCU ...), see platform.h

#include <Arduino.h>
#include "FS.h"
#include <ESP8266WiFi.h>
#if ENABLE_WIFI
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266mDNS.h>
#endif

#define GBS_PLATFORM_NAME "ESP8266"

#define GBS_IRAM_ATTR ICACHE_RAM_ATTR

#define GBS_FS SPIFFS
#define gbsFsBegin() SPIFFS.begin()

// fast ESP8266 digitalRead (21 cycles vs 77), *should* work with all possible input pins
// but only "D7" and "D6" have been tested so far
#define GBS_FAST_DIGITAL_READ(x) ((GPIO_REG_READ(GPIO_IN_ADDRESS) >> x) & 1)

// read in interrupt handlers, must be inline
#define gbsCycleCount(var) __asm__ __volatile__("rsr %0,ccount" : "=a"(var))

// a vsync measurement busy-waits for up to a few frames
#define GBS_MEASURE_BEGIN() ESP.wdtDisable()
#define GBS_MEASURE_ARMED() WiFi.setSleepMode(WIFI_LIGHT_SLEEP)
#define GBS_MEASURE_END()      \
    do {                       \
        ESP.wdtEnable(0);      \
        WiFi.setSleepMode(WIFI_NONE_SLEEP); \
    } while (0)

// don't use restart(), messes up websocket reconnects
#define gbsRestart() ESP.reset()
#define gbsResetReason() ESP.getResetInfo()

// attachInterrupt() / detachInterrupt() may be called from an ISR (used by framesync.h)
#define GBS_ISR_CAN_REATTACH 1

#define gbsWifiSetHostname(name) WiFi.hostname(name)
#define gbsWifiStoredSsid() WiFi.SSID()
#define gbsMdnsUpdate() MDNS.update()
#define gbsMdnsBegin(name) MDNS.begin(name, WiFi.localIP())
#define gbsMdnsAnnounce() MDNS.announce()
#define GBS_WIFI_ENC_OPEN ENC_TYPE_NONE

// called with the disconnect reason code
inline void gbsWifiOnDisconnected(void (*callback)(int reason))
{
    static void (*disconnectedCallback)(int) = nullptr;
    static WiFiEventHandler handler;
    disconnectedCallback = callback;
    handler = WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected &event) {
        if (disconnectedCallback) {
            disconnectedCallback(event.reason);
        }
    });
}
#define GBS_WEBSOCKET_NEEDS_LOOP 0
#define gbsWifiOff()            \
    do {                        \
        WiFi.mode(WIFI_OFF);    \
        WiFi.forceSleepBegin(); \
    } while (0)
// low latency responses, less chance for missing packets
#define gbsWifiLowLatency(dBm)              \
    do {                                    \
        WiFi.setSleepMode(WIFI_NONE_SLEEP); \
        WiFi.setOutputPower(dBm);           \
    } while (0)

// callback(const String &name, size_t size) for every file in the root directory
template <typename F>
void gbsForEachFile(F callback)
{
    Dir dir = GBS_FS.openDir("/");
    while (dir.next()) {
        callback(dir.fileName(), dir.fileSize());
    }
}

#endif
