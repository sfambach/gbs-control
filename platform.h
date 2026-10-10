#ifndef PLATFORM_H_
#define PLATFORM_H_

// Platform layer: everything that depends on the microcontroller family.
// To add a new chip, write a platform_<chip>.h with the same set of definitions and select it here.
//
// Each platform header provides:
//   includes for wifi / async tcp / web server / mdns / file system
//   GBS_PLATFORM_NAME                       name for logs
//   GBS_IRAM_ATTR                           attribute for interrupt handlers
//   GBS_FS                                  file system object (presets, slots, preferences)
//   GBS_FAST_DIGITAL_READ(pin)              fast register based digitalRead()
//   gbsCycleCount()                         CPU cycle counter (vsync measurement)
//   GBS_MEASURE_BEGIN() / _ARMED() / _END() hooks around a vsync measurement
//   gbsRestart(), gbsResetReason()
//   gbsWifiSetHostname(), gbsWifiOff(), gbsWifiLowLatency(dBm)
//   gbsForEachFile(callback(name, size))

#include "config.h"

#if defined(ESP8266)
#include "platform_esp8266.h"
#elif defined(ESP32)
#include "platform_esp32.h"
#else
#error "gbs-control: unsupported platform, add a platform_<chip>.h (see platform.h)"
#endif

#endif
