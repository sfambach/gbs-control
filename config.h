#ifndef GBS_CONFIG_H_
#define GBS_CONFIG_H_

// =============================================================================
// Feature toggles — set to 0 to disable and strip related init at compile time
// =============================================================================

// SSD1306 OLED + rotary encoder menu
#define GBS_ENABLE_OLED 1

// WiFi web UI, WebSocket control, PersWiFiManager, SPIFFS preset HTTP API
#define GBS_ENABLE_WEB_GUI 1

// Over-the-air firmware updates (requires GBS_ENABLE_WEB_GUI; enabled at runtime via web/serial)
#define GBS_ENABLE_OTA 1

#define HAVE_BUTTONS 0

#if GBS_ENABLE_OLED
#define USE_NEW_OLED_MENU 1
#else
#define USE_NEW_OLED_MENU 0
#endif

// Uncomment to enable WiFi ping debugging helpers (ESPping — ESP8266 and ESP32).
// #define HAVE_PINGER_LIBRARY

// Master device serves the web UI; comment out for a secondary/slave unit.
#define GBS_DEVICE_MASTER

// Frame sync debug output (serial or LED). Usually leave commented out.
// #define FS_DEBUG
// #define FS_DEBUG_LED
// #define FRAMESYNC_DEBUG

#if GBS_ENABLE_OTA && !GBS_ENABLE_WEB_GUI
#error "GBS_ENABLE_OTA requires GBS_ENABLE_WEB_GUI (WiFi stack)"
#endif

// =============================================================================
// Platform pins
// Override any pin with -DGBS_* on the compiler command line or in
// platformio.ini build_flags for your board (see docs/LIBRARIES.md).
// =============================================================================

#if defined(ESP8266)

#ifndef GBS_PIN_ENCODER_CLK
#define GBS_PIN_ENCODER_CLK 14 // D5 on Wemos D1 mini
#endif
#ifndef GBS_PIN_ENCODER_DATA
#define GBS_PIN_ENCODER_DATA 13 // D7
#endif
#ifndef GBS_PIN_ENCODER_SWITCH
#define GBS_PIN_ENCODER_SWITCH 0 // D3, must stay HIGH at boot
#endif

#ifndef GBS_OLED_I2C_ADDR
#define GBS_OLED_I2C_ADDR 0x3c
#endif
#ifndef GBS_OLED_PIN_SDA
#define GBS_OLED_PIN_SDA 4 // D2 / GPIO4 on Wemos D1 mini
#endif
#ifndef GBS_OLED_PIN_SCL
#define GBS_OLED_PIN_SCL 5 // D1 / GPIO5 on Wemos D1 mini
#endif

// Sync measurement input: D6 on Wemos D1 mini / Lolin NodeMCU (D12/MISO)
#ifndef GBS_DEBUG_IN_PIN
#define GBS_DEBUG_IN_PIN 12 // GPIO12 / D6
#endif

#define GBS_LED_ON                      \
    pinMode(LED_BUILTIN, OUTPUT);       \
    digitalWrite(LED_BUILTIN, LOW)
#define GBS_LED_OFF                         \
    digitalWrite(LED_BUILTIN, HIGH);        \
    pinMode(LED_BUILTIN, INPUT)

#define GBS_FAST_DIGITAL_READ(pin) ((GPIO_REG_READ(GPIO_IN_ADDRESS) >> (pin)) & 1)

#elif defined(ESP32)

#ifndef GBS_PIN_ENCODER_CLK
#define GBS_PIN_ENCODER_CLK 18
#endif
#ifndef GBS_PIN_ENCODER_DATA
#define GBS_PIN_ENCODER_DATA 19
#endif
#ifndef GBS_PIN_ENCODER_SWITCH
#define GBS_PIN_ENCODER_SWITCH 0
#endif

#ifndef GBS_OLED_I2C_ADDR
#define GBS_OLED_I2C_ADDR 0x3c
#endif
#ifndef GBS_OLED_PIN_SDA
#define GBS_OLED_PIN_SDA 21
#endif
#ifndef GBS_OLED_PIN_SCL
#define GBS_OLED_PIN_SCL 22
#endif

// Frame-sync debug tap — assign to the pin wired on your ESP32 GBS board.
// PCNT hardware filter is applied on ESP32 (see platform_gbs.h).
#ifndef GBS_DEBUG_IN_PIN
#define GBS_DEBUG_IN_PIN 4
#endif

#define GBS_LED_ON digitalWrite(LED_BUILTIN, HIGH)
#define GBS_LED_OFF digitalWrite(LED_BUILTIN, LOW)

#define GBS_FAST_DIGITAL_READ(pin) digitalRead(pin)

#else
#error "gbs-control requires ESP8266 or ESP32"
#endif

// I2C bus for GBS8200/8220 and Si5351 (defaults follow OLED wiring)
#ifndef GBS_I2C_PIN_SDA
#define GBS_I2C_PIN_SDA GBS_OLED_PIN_SDA
#endif
#ifndef GBS_I2C_PIN_SCL
#define GBS_I2C_PIN_SCL GBS_OLED_PIN_SCL
#endif

#ifndef DEBUG_IN_PIN
#define DEBUG_IN_PIN GBS_DEBUG_IN_PIN
#endif

// =============================================================================
// WiFi / device identity
// =============================================================================

#if defined(GBS_DEVICE_MASTER)

#define GBS_WIFI_AP_SSID "gbscontrol"
#define GBS_WIFI_AP_PASSWORD "qqqqqqqq"
#define GBS_DEVICE_HOSTNAME "gbscontrol"
#define GBS_DEVICE_HOSTNAME_FULL "gbscontrol.local"
#define GBS_WIFI_AP_INFO "(WiFi): AP mode (SSID: gbscontrol, pass 'qqqqqqqq'): Access 'gbscontrol.local' in your browser"
#define GBS_WIFI_STA_INFO "(WiFi): Access 'http://gbscontrol:80' or 'http://gbscontrol.local' (or device IP) in your browser"

#else

#define GBS_WIFI_AP_SSID "gbsslave"
#define GBS_WIFI_AP_PASSWORD "qqqqqqqq"
#define GBS_DEVICE_HOSTNAME "gbsslave"
#define GBS_DEVICE_HOSTNAME_FULL "gbsslave.local"
#define GBS_WIFI_AP_INFO "(WiFi): AP mode (SSID: gbsslave, pass 'qqqqqqqq'): Access 'gbsslave.local' in your browser"
#define GBS_WIFI_STA_INFO "(WiFi): Access 'http://gbsslave:80' or 'http://gbsslave.local' (or device IP) in your browser"

#endif

// =============================================================================
// Network services
// =============================================================================

#define GBS_WEB_SERVER_PORT 80
#define GBS_WEBSOCKET_PORT 81
#define GBS_WIFI_CONNECT_TIMEOUT_SEC 45
#define WEBSOCKETS_SERVER_CLIENT_MAX 2

#ifndef WIFI_CONNECT_TIMEOUT
#define WIFI_CONNECT_TIMEOUT GBS_WIFI_CONNECT_TIMEOUT_SEC
#endif

// =============================================================================
// GBS hardware / video processing
// =============================================================================

#define GBS_I2C_ADDR 0x17 // 7-bit GBS8200/8220 I2C address
#ifndef GBS_ADDR
#define GBS_ADDR GBS_I2C_ADDR
#endif

#define GBS_AUTO_GAIN_INIT 0x48
#ifndef AUTO_GAIN_INIT
#define AUTO_GAIN_INIT GBS_AUTO_GAIN_INIT
#endif

// =============================================================================
// Preset slots (SPIFFS)
// =============================================================================

#define GBS_SLOTS_FILE "/slots.bin"
#define GBS_SLOTS_TOTAL 72
#define GBS_EMPTY_SLOT_NAME "Empty                   "

#ifndef SLOTS_FILE
#define SLOTS_FILE GBS_SLOTS_FILE
#endif
#ifndef SLOTS_TOTAL
#define SLOTS_TOTAL GBS_SLOTS_TOTAL
#endif
#ifndef EMPTY_SLOT_NAME
#define EMPTY_SLOT_NAME GBS_EMPTY_SLOT_NAME
#endif

// =============================================================================
// OLED menu
// =============================================================================

#define OLED_MENU_WIDTH 128
#define OLED_MENU_HEIGHT 64
#define OLED_MENU_MAX_SUBITEMS_NUM 16
#define OLED_MENU_MAX_ITEMS_NUM 64
#define OLED_MENU_MAX_DEPTH 8
#define OLED_MENU_REFRESH_INTERVAL_IN_MS 50
#define OLED_MENU_SCREEN_SAVER_REFRESH_INTERVAL_IN_MS 5000
#define OLED_MENU_SCROLL_LEAD_IN_TIME_IN_MS 600
#define OLED_MENU_SCREEN_SAVER_KICK_IN_SECONDS 180
#define OLED_MENU_OVER_DRAW 0
#define OLED_MENU_RESET_ALWAYS_SCROLL_ON_SELECTION 0
#define OLED_MENU_WRAPPING_SPACE (OLED_MENU_WIDTH / 3)
#define REVERSE_ROTARY_ENCODER_FOR_OLED_MENU 0
#define REVERSE_ROTARY_ENCODER_FOR_OSD 0
#define OSD_TIMEOUT 8000

#define OLED_MENU_STATUS_BAR_HEIGHT (OLED_MENU_HEIGHT / 4)
#define OLED_MENU_USABLE_AREA_HEIGHT (OLED_MENU_HEIGHT - OLED_MENU_STATUS_BAR_HEIGHT)
#define OLED_MENU_SCROLL_LEAD_IN_FRAMES (OLED_MENU_SCROLL_LEAD_IN_TIME_IN_MS / OLED_MENU_REFRESH_INTERVAL_IN_MS)

// =============================================================================
// On-screen display (OSD) overlay
// =============================================================================

#define GBS_OSD_MENU_WIDTH 131
#define GBS_OSD_MENU_HEIGHT 19

#ifndef MENU_WIDTH
#define MENU_WIDTH GBS_OSD_MENU_WIDTH
#endif
#ifndef MENU_HEIGHT
#define MENU_HEIGHT GBS_OSD_MENU_HEIGHT
#endif

// =============================================================================
// Frame sync tuning
// =============================================================================

#define GBS_FRAMESYNC_LOCK_INTERVAL 1670 // roughly every 100 frames at ~16.7 kHz line rate
#define GBS_FRAMESYNC_CORRECTION 2                 // scanlines when phase lags target
#define GBS_FRAMESYNC_TARGET_PHASE 90              // degrees; output trails input

// =============================================================================
// OSD menu widget tuning (shift/scale controls)
// =============================================================================

#define GBS_MENU_SHIFT_DELTA 4
#define GBS_MENU_SCALE_DELTA 4
#define GBS_MENU_VERT_SHIFT_RANGE 300
#define GBS_MENU_HORIZ_SHIFT_RANGE 400
#define GBS_MENU_VERT_SCALE_RANGE 100
#define GBS_MENU_HORIZ_SCALE_RANGE 130
#define GBS_MENU_BAR_LENGTH 100

// =============================================================================
// Optional front-panel buttons (when HAVE_BUTTONS == 1)
// =============================================================================

#define GBS_BUTTON_INPUT_SHIFT 0
#define GBS_BUTTON_DOWN_SHIFT 1
#define GBS_BUTTON_UP_SHIFT 2
#define GBS_BUTTON_MENU_SHIFT 3
#define GBS_BUTTON_BACK_SHIFT 4

#endif
