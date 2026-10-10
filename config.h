#ifndef CONFIG_H_
#define CONFIG_H_

// Central build configuration of gbs-control.
// Every value can be overridden from the build system, e.g. in platformio.ini:
//   build_flags = -DENABLE_WIFI=0 -DDEVICE_HOSTNAME=\"gbsslave\"

// --- firmware ----------------------------------------------------------------

// set by tools/pio_version.py from git describe (PlatformIO builds)
#ifndef GBS_FW_VERSION
#define GBS_FW_VERSION "dev"
#endif

// --- features --------------------------------------------------------------

// 0: compile without WiFi / web UI / OTA (OLED menu only)
#ifndef ENABLE_WIFI
#define ENABLE_WIFI 1
#endif

// 1: OLED menu with rotary encoder (OLEDMenuManager), 0: legacy OLED menu
#ifndef USE_NEW_OLED_MENU
#define USE_NEW_OLED_MENU 1
#endif

// 1: separate push buttons instead of / in addition to the rotary encoder
#ifndef HAVE_BUTTONS
#define HAVE_BUTTONS 0
#endif

// --- device / network ------------------------------------------------------

// host name for mDNS (<name>.local) and DHCP; change it to run 2 or more devices on one network
#ifndef DEVICE_HOSTNAME
#define DEVICE_HOSTNAME "gbscontrol"
#endif

// access point used when no network is configured or the configured one is not reachable
#ifndef AP_SSID
#define AP_SSID DEVICE_HOSTNAME
#endif
#ifndef AP_PASSWORD
#define AP_PASSWORD "qqqqqqqq"
#endif

#ifndef WEB_SERVER_PORT
#define WEB_SERVER_PORT 80
#endif
#ifndef WEBSOCKET_PORT
#define WEBSOCKET_PORT 81
#endif

// WiFi transmit power in dBm (0.0 - 20.5)
#ifndef WIFI_OUTPUT_POWER
#define WIFI_OUTPUT_POWER 16.0f
#endif

// seconds to wait for the configured network at startup before opening the access point
#ifndef WIFI_CONNECT_TIMEOUT
#define WIFI_CONNECT_TIMEOUT 45
#endif
// in access point fallback with a configured network: retry it every .._INTERVAL for .._DURATION seconds
#ifndef WIFI_AP_RETRY_INTERVAL
#define WIFI_AP_RETRY_INTERVAL 60
#endif
#ifndef WIFI_AP_RETRY_DURATION
#define WIFI_AP_RETRY_DURATION 15
#endif

// --- serial / web terminal -------------------------------------------------

// serial monitor speed (platformio.ini: monitor_speed must match)
#ifndef SERIAL_BAUD
#define SERIAL_BAUD 921600
#endif

// log output for the web ui terminal is collected and sent as one websocket frame
#ifndef WEB_LOG_BUFFER_SIZE
#define WEB_LOG_BUFFER_SIZE 512
#endif
#ifndef WEB_LOG_FLUSH_INTERVAL_MS
#define WEB_LOG_FLUSH_INTERVAL_MS 40
#endif

// web ui commands waiting for loop()
#ifndef WEB_CMD_QUEUE_LEN
#define WEB_CMD_QUEUE_LEN 16
#endif

// --- board: pins and wiring ----------------------------------------------

#if defined(GBS_BOARD_HEADER)
#include GBS_BOARD_HEADER
#elif defined(ESP32)
#include "board_d1_mini32.h"
#else
#include "board_d1_mini.h"
#endif
#ifndef GBS_BOARD_NAME
#define GBS_BOARD_NAME "unknown board"
#endif

// --- measurement -----------------------------------------------------------

// vsync / pulse sampling on DEBUG_IN_PIN gives up when the first edge does not arrive in time
// (no signal on the debug pin). Measured: up to ~65 ms right after input detection.
#ifndef FIRST_EDGE_TIMEOUT_US
#define FIRST_EDGE_TIMEOUT_US 200000
#endif

#endif
