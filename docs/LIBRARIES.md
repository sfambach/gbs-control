# Third-party libraries

This firmware mixes **PlatformIO / Arduino registry libraries**, **vendored modified copies** (compiled with the project), and **upstream reference trees** in `3rdparty/` (git submodules, not compiled).

Edit hardware, feature, and pin settings in [`config.h`](../config.h), not in this document.

### Feature toggles (`config.h`)

| Define | Default | Effect |
|---|---|---|
| `GBS_ENABLE_OLED` | `1` | SSD1306 display, rotary encoder menu, boot splash |
| `GBS_ENABLE_WEB_GUI` | `1` | WiFi, web UI, WebSocket control, PersWiFiManager |
| `GBS_ENABLE_OTA` | `1` | Arduino OTA (requires web GUI; enabled at runtime via UI / serial `c`) |
| `GBS_DEVICE_MASTER` | defined | Master vs slave WiFi hostname / AP SSID |

Set any define to `0` to exclude that subsystem at compile time (saves flash/RAM).

### Pin overrides

All `GBS_*_PIN*` values can be overridden per board via PlatformIO `build_flags`, for example:

```ini
build_flags =
    -DGBS_DEBUG_IN_PIN=4
    -DGBS_OLED_PIN_SDA=8
    -DGBS_OLED_PIN_SCL=9
```

On **ESP32**, frame-sync measurement on `GBS_DEBUG_IN_PIN` uses the **PCNT** hardware glitch filter ([`platform_gbs.h`](../platform_gbs.h)). Pulse timing uses the CPU cycle counter on Xtensa ESP32 (classic, S2, S3) and microseconds on RISC-V ESP32 (C3, C6, H2).

PlatformIO environments: `d1_mini` (ESP8266), `esp32dev`, `esp32-s3-devkitc-1`, `esp32-c3-devkitm-1`, `esp32-c6-devkitc-1` — see [`platformio.ini`](../platformio.ini).

## Quick reference

| Library | Upstream | Version / commit | Built from | ESP32 | Modified |
|---|---|---|---|---|---|
| AsyncPersWiFiManager | [sfambach/AsyncPersWiFiManager](https://github.com/sfambach/AsyncPersWiFiManager) | **v1.0.0** | [`3rdparty/AsyncPersWiFiManager/`](../3rdparty/AsyncPersWiFiManager/) submodule + PlatformIO `lib_deps` | Yes | Async fork |
| arduinoWebSockets | [Links2004/arduinoWebSockets](https://github.com/Links2004/arduinoWebSockets) | tag **2.7.2** | [`src/WebSockets*`](../src/) — see [`src/websockets.md`](../src/websockets.md) | Yes | Yes |
| Si5351mcu | [pavelmc/Si5351mcu](https://github.com/pavelmc/Si5351mcu) | **0.7.1** ([cbbd806](https://github.com/pavelmc/Si5351mcu/commit/cbbd8067e9c8e35ca2b9c886c2c97d8d553c97ed)) | [`src/si5351mcu.*`](../src/si5351mcu.h) | Wire-based | Yes |
| OLED menu UI | [PSHarold/OLED-SSD1306-Menu](https://github.com/PSHarold/OLED-SSD1306-Menu) | adapted | [`OLEDMenu*.h` / `.cpp`](../OLEDMenuManager.h) | Yes | Yes |
| ESPAsyncTCP | [ESP32Async/ESPAsyncTCP](https://github.com/ESP32Async/ESPAsyncTCP) | **v2.0.0** | [`3rdparty/ESPAsyncTCP/`](../3rdparty/ESPAsyncTCP/) ref + PlatformIO `lib_deps` | — | No |
| AsyncTCP | [ESP32Async/AsyncTCP](https://github.com/ESP32Async/AsyncTCP) | **v3.5.0** | [`3rdparty/AsyncTCP/`](../3rdparty/AsyncTCP/) ref + PlatformIO `lib_deps` | Yes | No |
| ESP Async WebServer | [ESP32Async/ESPAsyncWebServer](https://github.com/ESP32Async/ESPAsyncWebServer) | **v3.11.2** | [`3rdparty/ESPAsyncWebServer/`](../3rdparty/ESPAsyncWebServer/) ref + PlatformIO `lib_deps` | Yes | No |
| SSD1306 OLED driver | [ThingPulse ESP8266/ESP32 OLED](https://github.com/ThingPulse/esp8266-oled-ssd1306) | **4.6.2** | [`3rdparty/esp8266-oled-ssd1306/`](../3rdparty/esp8266-oled-ssd1306/) submodule + PlatformIO `lib_deps` | Yes | No |
| ESPping *(optional)* | [dvarrel/ESPping](https://github.com/dvarrel/ESPping) | **1.0.5** | [`3rdparty/ESPping/`](../3rdparty/ESPping/) ref + PlatformIO `lib_deps` | Yes | No |

### Platform-specific async TCP stack

Community-maintained forks under **[ESP32Async](https://github.com/ESP32Async)** replace the archived me-no-dev originals.

| Platform | TCP library for ESPAsyncWebServer + WebSockets (async on ESP8266) |
|---|---|
| ESP8266 | **[ESPAsyncTCP](https://github.com/ESP32Async/ESPAsyncTCP)** |
| ESP32 | **[AsyncTCP](https://github.com/ESP32Async/AsyncTCP)** |

Both use **[ESPAsyncWebServer](https://github.com/ESP32Async/ESPAsyncWebServer)** (same header/API family as the old me-no-dev 1.x line, now at 3.x).

Do not install ESPAsyncTCP on ESP32 or AsyncTCP on ESP8266 — they are platform-specific companions to ESP Async WebServer.

### Reference copies (submodules, not compiled)

| Path | Tag | Purpose |
|---|---|---|
| [`3rdparty/AsyncPersWiFiManager/`](../3rdparty/AsyncPersWiFiManager/) | v1.0.0 | Async ESPAsyncWebServer WiFi manager ([sfambach fork](https://github.com/sfambach/AsyncPersWiFiManager)) |
| [`3rdparty/WebSockets/`](../3rdparty/WebSockets/) | 2.7.2 | Upstream — diff against patched `src/` copy |
| [`3rdparty/ESPAsyncTCP/`](../3rdparty/ESPAsyncTCP/) | v2.0.0 | Pinned ESP8266 async TCP reference |
| [`3rdparty/AsyncTCP/`](../3rdparty/AsyncTCP/) | v3.5.0 | Pinned ESP32 async TCP reference |
| [`3rdparty/ESPAsyncWebServer/`](../3rdparty/ESPAsyncWebServer/) | v3.11.2 | Pinned web server reference |
| [`3rdparty/ESPping/`](../3rdparty/ESPping/) | 1.0.5 | Pinned ping library reference |
| [`3rdparty/esp8266-oled-ssd1306/`](../3rdparty/esp8266-oled-ssd1306/) | 4.6.2 | Pinned SSD1306 OLED driver reference |

These trees are **not compiled** (except AsyncPersWiFiManager and SSD1306, which PlatformIO builds from `file://3rdparty/...` paths). PlatformIO still fetches the same versions via `lib_deps` in [`platformio.ini`](../platformio.ini) (see comments there). Arduino IDE users install matching tags into `Arduino/libraries` — submodules are optional for browsing/diff offline.

Excluded from the firmware build via `build_src_filter` in `platformio.ini`.

### gbs-control WebSockets notes

- **ESP8266:** vendored `src/WebSockets.h` forces `NETWORK_ESP8266_ASYNC` (no `webSocket.loop()` needed).
- **ESP32:** upstream default `NETWORK_ESP32` (sync). When the main firmware is ported, call `webSocket.loop()` from the main loop unless you switch to an async network type.

### Project code (not external libraries)

Preset register blobs (`ntsc_*.h`, `pal_*.h`), [`webui_html.h`](../webui_html.h), [`options.h`](../options.h), and GBS driver headers are part of this firmware, not third-party libraries.

---

## Cloning the repository

`3rdparty/` uses **git submodules**. After clone:

```bash
git clone --recursive https://github.com/sfambach/gbs-control.git
```

If you already cloned without `--recursive`:

```bash
git submodule update --init --recursive
```

Submodules under `3rdparty/` pin upstream versions for **reference and diffing**. Builds use:

- **Modified copies** in the project root and `src/` (WebSockets, Si5351mcu, OLED menu)
Clone with submodules (reference copies under `3rdparty/` — not all are compiled directly):

- **PlatformIO `lib_deps`** for unmodified libs, including **`file://3rdparty/AsyncPersWiFiManager`** and **`file://3rdparty/esp8266-oled-ssd1306`**

Arduino IDE does not use submodules for compilation; install the tagged releases listed in the sections below.

---

## PlatformIO (recommended)

Requirements: [PlatformIO](https://platformio.org/) (CLI or VS Code extension).

**ESP8266 (production target today):**

```bash
cd gbs-control
pio run -e d1_mini
pio run -e d1_mini -t upload
```

**ESP32 (library bring-up env — full firmware port still in progress):**

```bash
pio run -e esp32dev
```

Dependencies are declared in [`platformio.ini`](../platformio.ini) (`lib_deps` git URLs match submodule tags in `3rdparty/`):

- **Downloaded by PlatformIO:** AsyncPersWiFiManager and SSD1306 (from submodule paths), platform-specific async TCP, ESP Async WebServer, ESPping
- **Vendored in-repo:** WebSockets + Si5351mcu (`src/` via `lib_dir = ./src/`)
- **Submodule reference:** everything under `3rdparty/` including [AsyncPersWiFiManager](https://github.com/sfambach/AsyncPersWiFiManager)

---

## Arduino IDE — ESP8266

The sketch is the **project folder** — open [`gbs-control.ino`](../gbs-control.ino) (File → Open). Arduino treats the containing directory as the sketch and compiles all `.cpp` files there plus `#include` paths relative to that folder.

### 1. Board support

1. **File → Preferences → Additional Board Manager URLs** — add:
   ```
   https://arduino.esp8266.com/stable/package_esp8266com_index.json
   ```
2. **Tools → Board → Boards Manager** — install **esp8266** by ESP8266 Community.
3. **Tools → Board** — select **LOLIN(WEMOS) D1 R2 & mini** (or equivalent D1 mini).
4. Suggested settings (match [`platformio.ini`](../platformio.ini) `[env:d1_mini]`):
   - **CPU Frequency:** 160 MHz
   - **Flash Size:** 4MB (FS:1MB OTA:~1019KB) or similar 4MB layout
   - **Upload Speed:** 921600 *(optional)*

### 2. Install external libraries (ESP8266)

Use **Sketch → Include Library → Manage Libraries…** or install manually into your Arduino **libraries** folder:

| Install as | Library name / source | Required |
|---|---|---|
| Library Manager or GitHub | **AsyncPersWiFiManager** — [sfambach/AsyncPersWiFiManager](https://github.com/sfambach/AsyncPersWiFiManager) **v1.0.0** | Yes (provides `PersWiFiManager.h`) |
| Library Manager or GitHub | **ESP Async TCP** — [ESP32Async/ESPAsyncTCP](https://github.com/ESP32Async/ESPAsyncTCP) **v2.0.0** | Yes |
| Library Manager or GitHub | **ESP Async WebServer** — [ESP32Async/ESPAsyncWebServer](https://github.com/ESP32Async/ESPAsyncWebServer) **v3.11.2** | Yes |
| Library Manager or GitHub | **ESP8266 and ESP32 OLED driver for SSD1306 displays** (ThingPulse) — tag **4.6.2** | Yes |
| Library Manager or GitHub | **ESPping** (dvarrel) — tag **1.0.5** | Only if `HAVE_PINGER_LIBRARY` in [`config.h`](../config.h) |

**Windows libraries folder (typical):**

```
C:\Users\<you>\Documents\Arduino\libraries
```

**Manual install:** download each repo (green **Code → Download ZIP**), extract, and rename the folder to match the library name (no `-master` suffix).

Upstream links:

- https://github.com/sfambach/AsyncPersWiFiManager
- https://github.com/ESP32Async/ESPAsyncTCP
- https://github.com/ESP32Async/ESPAsyncWebServer
- https://github.com/ThingPulse/esp8266-oled-ssd1306
- https://github.com/dvarrel/ESPping *(optional)*

### 3. Libraries already in this repo (do not install separately)

| Component | Location | Include style |
|---|---|---|
| WebSockets (modified) | `src/WebSockets.h`, `src/WebSocketsServer.h`, … | `#include "src/WebSockets.h"` |
| Si5351mcu (modified) | `src/si5351mcu.h`, `src/si5351mcu.cpp` | `#include "src/si5351mcu.h"` |
| OLED menu (adapted) | `OLEDMenu*.h`, `OLEDMenu*.cpp` | project-local headers |

**AsyncPersWiFiManager** is installed as a library (PlatformIO / Arduino `libraries` folder). Do **not** keep a separate copy in the sketch folder.

Do **not** add a separate WebSockets folder under `Arduino/libraries` for this build — the sketch uses the in-tree `src/` copy.

### 4. Web UI assets

If you change files under [`public/`](../public/), rebuild the embedded HTML before compiling:

```bash
cd public
npm install
npm run build
```

That regenerates [`webui_html.h`](../webui_html.h). See [`public/README.md`](../public/README.md).

### 5. Compile and upload

1. Open `gbs-control.ino`.
2. Select the D1 mini board and serial port.
3. **Sketch → Upload**.

---

## Arduino IDE — ESP32 (planned)

Use this when building for ESP32. The main sketch still contains ESP8266-specific code; library versions below are what this repo targets for the port.

### 1. Board support

1. **File → Preferences → Additional Board Manager URLs** — add (if not already present):
   ```
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
2. **Tools → Board → Boards Manager** — install **esp32** by Espressif Systems.
3. **Tools → Board** — select your ESP32 module (e.g. **ESP32 Dev Module**).

### 2. Install external libraries (ESP32)

| Install as | Library name / source | Required |
|---|---|---|
| Library Manager or GitHub | **AsyncPersWiFiManager** — [sfambach/AsyncPersWiFiManager](https://github.com/sfambach/AsyncPersWiFiManager) **v1.0.0** | Yes (provides `PersWiFiManager.h`) |
| Library Manager or GitHub | **Async TCP** — [ESP32Async/AsyncTCP](https://github.com/ESP32Async/AsyncTCP) **v3.5.0** | Yes — **not** ESPAsyncTCP |
| Library Manager or GitHub | **ESP Async WebServer** — [ESP32Async/ESPAsyncWebServer](https://github.com/ESP32Async/ESPAsyncWebServer) **v3.11.2** | Yes |
| Library Manager or GitHub | **ESP8266 and ESP32 OLED driver for SSD1306 displays** (ThingPulse) — tag **4.6.2** | Yes |
| Library Manager or GitHub | **ESPping** (dvarrel) — tag **1.0.5** | Only if `HAVE_PINGER_LIBRARY` in [`config.h`](../config.h) |

Upstream links:

- https://github.com/sfambach/AsyncPersWiFiManager
- https://github.com/ESP32Async/AsyncTCP
- https://github.com/ESP32Async/ESPAsyncWebServer
- https://github.com/ThingPulse/esp8266-oled-ssd1306
- https://github.com/dvarrel/ESPping *(optional)*

**Do not install on ESP32:** ESPAsyncTCP (use AsyncTCP instead).

Same in-repo vendored copies as ESP8266 (`src/WebSockets*`, `src/si5351mcu*`, OLED menu). Install **AsyncPersWiFiManager** from the fork. Pins for ESP32 are placeholders in [`config.h`](../config.h) until hardware is assigned.

---

## Updating upstream reference submodules

To refresh read-only trees under `3rdparty/` (for diffing; PlatformIO builds keep using `lib_deps` unless you change those URLs too):

```bash
cd 3rdparty/AsyncPersWiFiManager && git fetch --tags && git checkout v1.0.0
cd ../WebSockets && git fetch --tags && git checkout 2.7.2
cd ../ESPAsyncTCP && git fetch --tags && git checkout v2.0.0
cd ../AsyncTCP && git fetch --tags && git checkout v3.5.0
cd ../ESPAsyncWebServer && git fetch --tags && git checkout v3.11.2
cd ../ESPping && git fetch --tags && git checkout 1.0.5
cd ../esp8266-oled-ssd1306 && git fetch --tags && git checkout 4.6.2
cd ../..
git add 3rdparty/
```

When bumping a library version, update **both** the submodule checkout and the matching `lib_deps` entry in `platformio.ini`, then this document.

After changing vendored **build** copies, update sidecars: [`src/si5351mcu.md`](../src/si5351mcu.md), [`src/websockets.md`](../src/websockets.md).

---

## Licenses

- Project: see [`LICENSE`](../LICENSE) in the repository root.
- Submodule upstream licenses: see each tree under `3rdparty/` or the upstream repository.
- WebSockets (vendored copy): LGPL-2.1 — see [`src/LICENSE`](../src/LICENSE).
