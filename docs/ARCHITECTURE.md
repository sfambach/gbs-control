# Architecture

GBS-Control is an Arduino/PlatformIO firmware for the GBS8200/GBS8220 scan converter. The codebase is organized for **Arduino IDE compatibility** (sketch + `.cpp` at repo root) while grouping data and headers into subdirectories.

## Directory layout

| Path | Purpose |
|---|---|
| `gbs-control.ino` | Sketch entry: globals, ISRs, `setup()`, `loop()`, serial mirror |
| `gbs_video.cpp` | GBS register I/O, presets, sync, scaling, input detection |
| `gbs_wifi.cpp` | Web UI, WebSocket, WiFi manager, OTA (`#if GBS_ENABLE_WEB_GUI`) |
| `gbs_prefs.cpp` | SPIFFS user preferences and custom preset slots |
| `gbs_oled_legacy.cpp` | Legacy rotary-encoder OLED menu (`#if GBS_ENABLE_OLED && !USE_NEW_OLED_MENU`) |
| `OLEDMenu*.cpp` | New OLED menu system (`USE_NEW_OLED_MENU`) |
| `presets/` | NTSC/PAL preset register arrays and shared preset sections |
| `ofw/` | Optimized firmware (OFW) blobs |
| `include/` | Shared headers (`options.h`, `slot.h`, `osd.h`, …) |
| `generated/` | Build artifacts embedded in firmware (`webui_html.h`) |
| `lib/` | Vendored, modified libraries (WebSockets, Si5351mcu) |
| `3rdparty/` | Git submodule reference trees + PlatformIO `lib_deps` sources |
| `public/` | Web UI source; `npm run build` → `webui.html` → `generated/webui_html.h` |
| `scripts/` | Maintenance scripts (`generate_translations.py`, `split_ino.py`) |
| `config.h` | Feature toggles, pins, compile-time options |
| `platform_gbs.h` | ESP8266/ESP32 portability (WiFi stubs, I2C, PCNT) |

## Module headers

- `gbs_globals.h` — shared types (`GBS`, `FrameSync`, `Menu`), runtime structs (`rto`, `uopt`)
- `gbs_video.h` — video/sync API used by sketch, WiFi, prefs, and OLED code
- `gbs_wifi.h` / `gbs_prefs.h` / `gbs_oled_legacy.h` — subsystem entry points

## Build systems

**PlatformIO** (`platformio.ini`):

- `src_dir = ./` — sketch and module `.cpp` files at repo root
- `lib_dir = ./lib/` — vendored libraries
- `build_src_filter` excludes `3rdparty/*`

**Arduino IDE:** open `gbs-control.ino`. All `.cpp` modules must remain beside the sketch. Headers use paths such as `presets/ntsc_240p.h` and `include/options.h`.

## Feature toggles

See [`config.h`](../config.h) and [`docs/LIBRARIES.md`](LIBRARIES.md). Subsystems compile out entirely when disabled (`GBS_ENABLE_OLED`, `GBS_ENABLE_WEB_GUI`, `GBS_ENABLE_OTA`).

## Web UI pipeline

```
public/ (TypeScript + templates)
  → npm run build
  → webui.html (gitignored)
  → public/scripts/html2h.sh
  → generated/webui_html.h (committed)
```

## Third-party libraries

AsyncPersWiFiManager, ESPAsyncWebServer, SSD1306, ESPping, and async TCP stacks are pulled via PlatformIO `lib_deps` and/or `3rdparty/` submodules. Patched WebSockets and Si5351 live in `lib/` and are compiled with the sketch.
