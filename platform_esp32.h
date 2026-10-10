#ifndef PLATFORM_ESP32_H_
#define PLATFORM_ESP32_H_

// ESP32 (Xtensa: ESP32, ESP32-S2/S3; RISC-V variants need a different cycle counter), see platform.h
// Arduino core 2.x (ESP-IDF 4.4)

#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <hal/cpu_hal.h>
#if ENABLE_WIFI
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#endif

#define GBS_PLATFORM_NAME "ESP32"

// types the ESP8266 SDK provides and the code uses
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;

#define GBS_IRAM_ATTR IRAM_ATTR

// LittleFS: more robust than SPIFFS (deprecated); a new device starts with an empty file system
#define GBS_FS LittleFS
#define gbsFsBegin() LittleFS.begin(true) // format on first use

// fast register based digitalRead
#if SOC_GPIO_PIN_COUNT > 32
#define GBS_FAST_DIGITAL_READ(x) \
    ((x) < 32 ? ((REG_READ(GPIO_IN_REG) >> (x)) & 1) : ((REG_READ(GPIO_IN1_REG) >> ((x) - 32)) & 1))
#else
#define GBS_FAST_DIGITAL_READ(x) ((REG_READ(GPIO_IN_REG) >> (x)) & 1)
#endif

// CPU cycle counter of the current core (start and stop are taken by the same ISR / core)
#define gbsCycleCount(var) ((var) = cpu_hal_get_cycle_count())

// attachInterrupt() / detachInterrupt() must not be called from an ISR on ESP32:
// framesync.h then uses one ISR with a small state machine
#define GBS_ISR_CAN_REATTACH 0

// the loop task is not watched by the task watchdog, wifi runs on the other core
#define GBS_MEASURE_BEGIN()
#define GBS_MEASURE_ARMED()
#define GBS_MEASURE_END() detachInterrupt(DEBUG_IN_PIN)

#define gbsRestart() ESP.restart()
String gbsResetReason();

#define gbsWifiSetHostname(name) WiFi.setHostname(name)
#define gbsWifiOff() WiFi.mode(WIFI_OFF)
#define gbsWifiLowLatency(dBm)                                \
    do {                                                      \
        WiFi.setSleep(false);                                 \
        WiFi.setTxPower((wifi_power_t)(int)((dBm) * 4.0f)); \
    } while (0)
// SSID stored in flash (WiFi.SSID() is only set while connected on ESP32)
String gbsWifiStoredSsid();
// called with the disconnect reason code
void gbsWifiOnDisconnected(void (*callback)(int reason));
#define gbsMdnsUpdate()
#define gbsMdnsBegin(name) MDNS.begin(name)
#define gbsMdnsAnnounce()
#define GBS_WIFI_ENC_OPEN WIFI_AUTH_OPEN
// the bundled WebSockets library runs in synchronous mode on ESP32 and needs loop() calls
#define GBS_WEBSOCKET_NEEDS_LOOP 1

// callback(const String &name, size_t size) for every file in the root directory (name with leading '/')
template <typename F>
void gbsForEachFile(F callback)
{
    File root = GBS_FS.open("/");
    if (!root) {
        return;
    }
    for (File f = root.openNextFile(); f; f = root.openNextFile()) {
        String name = f.name();
        if (!name.startsWith("/")) {
            name = "/" + name;
        }
        callback(name, f.size());
    }
}

#endif
