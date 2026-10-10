// ESP32 implementations for platform_esp32.h
#if defined(ESP32)
#include "platform.h"
#include <esp_system.h>

String gbsResetReason()
{
    switch (esp_reset_reason()) {
        case ESP_RST_POWERON: return "power on";
        case ESP_RST_EXT: return "external";
        case ESP_RST_SW: return "software";
        case ESP_RST_PANIC: return "exception / panic";
        case ESP_RST_INT_WDT: return "interrupt watchdog";
        case ESP_RST_TASK_WDT: return "task watchdog";
        case ESP_RST_WDT: return "other watchdog";
        case ESP_RST_DEEPSLEEP: return "deep sleep";
        case ESP_RST_BROWNOUT: return "brownout";
        case ESP_RST_SDIO: return "sdio";
        default: return "unknown";
    }
}

String gbsWifiStoredSsid()
{
    if (WiFi.getMode() == WIFI_MODE_NULL) {
        WiFi.mode(WIFI_STA); // the stored config can only be read once the WiFi driver runs
    }
    wifi_config_t conf;
    if (esp_wifi_get_config(WIFI_IF_STA, &conf) != ESP_OK) {
        return String();
    }
    return String(reinterpret_cast<const char *>(conf.sta.ssid));
}

void gbsWifiPreferStrongestAp()
{
    WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);
    WiFi.setSortMethod(WIFI_CONNECT_AP_BY_SIGNAL);
    // WiFi.begin() without arguments uses the stored config as it is, so fix that one as well
    wifi_config_t conf;
    if (esp_wifi_get_config(WIFI_IF_STA, &conf) == ESP_OK && conf.sta.ssid[0] &&
        (conf.sta.scan_method != WIFI_ALL_CHANNEL_SCAN || conf.sta.sort_method != WIFI_CONNECT_AP_BY_SIGNAL)) {
        conf.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
        conf.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;
        esp_wifi_set_config(WIFI_IF_STA, &conf);
    }
}

static void (*disconnectedCallback)(int) = nullptr;

void gbsWifiOnDisconnected(void (*callback)(int reason))
{
    disconnectedCallback = callback;
    WiFi.onEvent([](WiFiEvent_t, WiFiEventInfo_t info) {
        if (disconnectedCallback) {
            disconnectedCallback(info.wifi_sta_disconnected.reason);
        }
    }, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
}
#endif
