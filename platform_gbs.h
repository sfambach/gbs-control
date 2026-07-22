#ifndef PLATFORM_GBS_H_
#define PLATFORM_GBS_H_

#include "config.h"

// ---------------------------------------------------------------------------
// Compile-time platform detection (ESP8266 + all ESP32 Arduino targets)
// ---------------------------------------------------------------------------

#if defined(ESP8266)
#define GBS_PLATFORM_ESP8266 1
#elif defined(ESP32)
#define GBS_PLATFORM_ESP32 1
#if defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C6) || defined(CONFIG_IDF_TARGET_ESP32H2)
#define GBS_ARCH_RISCV 1
#else
#define GBS_ARCH_XTENSA 1
#endif
#else
#error "gbs-control requires ESP8266 or ESP32 (any variant)"
#endif

// ISR placement
#if GBS_PLATFORM_ESP8266
#define GBS_IRAM_ATTR ICACHE_RAM_ATTR
#elif GBS_PLATFORM_ESP32
#define GBS_IRAM_ATTR IRAM_ATTR
#include "esp_timer.h"
#endif

// Cycle counter / time base for frame-sync pulse measurement
#if GBS_PLATFORM_ESP8266 || (GBS_PLATFORM_ESP32 && defined(GBS_ARCH_XTENSA))
#define GBS_MEASURE_POLL_ITERATIONS 3000000UL
static inline uint32_t gbs_cycle_count()
{
    return ESP.getCycleCount();
}
#else
// ESP32-C3/C6/H2 (RISC-V): no CCOUNT — use microseconds
#define GBS_MEASURE_POLL_ITERATIONS 200000UL
static inline uint32_t gbs_cycle_count()
{
    return (uint32_t)esp_timer_get_time();
}
#endif

// I2C clock derived from CPU speed on ESP8266; fixed 400 kHz default elsewhere
#ifndef GBS_I2C_CLOCK_HZ
#if GBS_PLATFORM_ESP8266
#if F_CPU >= 160000000L
#define GBS_I2C_CLOCK_HZ 700000
#else
#define GBS_I2C_CLOCK_HZ 400000
#endif
#else
#define GBS_I2C_CLOCK_HZ 400000
#endif
#endif

#if !GBS_ENABLE_WEB_GUI
static inline void handleWiFi(bool instant = false)
{
    (void)instant;
}
#endif

#if GBS_PLATFORM_ESP32
#include "driver/pcnt.h"

// PCNT glitch filter on the debug input — hardware debounce for vsync measurement
static inline void gbs_measure_period_init_hw()
{
    pcnt_config_t cfg = {};
    cfg.pulse_gpio_num = DEBUG_IN_PIN;
    cfg.ctrl_gpio_num = PCNT_PIN_NOT_USED;
    cfg.unit = PCNT_UNIT_0;
    cfg.channel = PCNT_CHANNEL_0;
    cfg.pos_mode = PCNT_COUNT_INC;
    cfg.neg_mode = PCNT_COUNT_DIS;
    cfg.lctrl_mode = PCNT_MODE_KEEP;
    cfg.hctrl_mode = PCNT_MODE_KEEP;
    cfg.counter_h_lim = 32767;
    cfg.counter_l_lim = -32768;
    pcnt_unit_config(&cfg);
    pcnt_set_filter_value(PCNT_UNIT_0, 100);
    pcnt_filter_enable(PCNT_UNIT_0);
    pcnt_counter_pause(PCNT_UNIT_0);
    pcnt_counter_clear(PCNT_UNIT_0);
    pcnt_counter_resume(PCNT_UNIT_0);
}
#else
static inline void gbs_measure_period_init_hw() {}
#endif

#endif
