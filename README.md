# gbs-control

Documentation: https://ramapcsx2.github.io/gbs-control/

Gbscontrol is an alternative firmware for Tvia Trueview5725 based upscalers / video converter boards.  
Its growing list of features includes:   
- very low lag
- sharp and defined upscaling, comparing well to other -expensive- units
- no synchronization loss switching 240p/480i (output runs independent from input, sync to display never drops)
- on demand motion adaptive deinterlacer that engages automatically and only when needed
- works with almost anything: 8 bit consoles, 16/32 bit consoles, 2000s consoles, home computers, etc
- little compromise, eventhough the hardware is very affordable (less than $30 typically)
- lots of useful features and image enhancements
- optional control interface via web browser, utilizing the ESP8266 WiFi capabilities
- good color reproduction with auto gain and auto offset for the tripple 8 bit @ 160MHz ADC
- optional bypass capability to, for example, transcode Component to RGB/HV in high quality
 
Supported standards are NTSC / PAL, the EDTV and HD formats, as well as VGA from 192p to 1600x1200 (earliest DOS, home computers, PC).
Sources can be connected via RGB/HV (VGA), RGBS (game consoles, SCART) or Component Video (YUV).
Various variations are supported, such as the PlayStation 2's VGA modes that run over Component cables.

Gbscontrol is a continuation of previous work by dooklink, mybook4, Ian Stedman and others.  

Bob from RetroRGB did an overview video on the project. This is a highly recommended watch!   
https://www.youtube.com/watch?v=fmfR0XI5czI

Development threads:  
https://shmups.system11.org/viewtopic.php?f=6&t=52172   
https://circuit-board.de/forum/index.php/Thread/15601-GBS-8220-Custom-Firmware-in-Arbeit/

## Configuration

Hardware pins, feature toggles (`GBS_ENABLE_OLED`, `GBS_ENABLE_WEB_GUI`, `GBS_ENABLE_OTA`), WiFi credentials, OLED menu options, and tuning constants are centralized in **`config.h`**. Edit that file when adapting the firmware to your board (including ESP32-C3/S3/C6 and other variants — see `platformio.ini` and `docs/LIBRARIES.md`).

## Building

Third-party libraries, submodule setup, **PlatformIO**, and **Arduino IDE** instructions are documented in **[docs/LIBRARIES.md](docs/LIBRARIES.md)**.

Clone with submodules (pinned reference copies under `3rdparty/` — AsyncPersWiFiManager, WebSockets, ESP32Async stack, ESPping, SSD1306):

```bash
git clone --recursive https://github.com/sfambach/gbs-control.git
# or, after a plain clone:
git submodule update --init --recursive
```

PlatformIO builds still download libraries via `lib_deps`; submodules are for version pinning and offline diff. See **[docs/LIBRARIES.md](docs/LIBRARIES.md)**.

**PlatformIO:** `pio run -e d1_mini` (ESP8266) · `pio run -e esp32dev` (ESP32 library target, firmware port in progress)

Or use the root **Makefile** (requires `make`, `pio`, and npm):

```bash
make                  # web UI + firmware (ESP8266 / d1_mini)
make firmware-only    # skip web UI rebuild
make upload BOARD=esp32dev
make help
```

**CI / nightly builds:** GitHub Actions builds when firmware or web-UI sources change (pure doc edits are skipped). Nightly runs only after recent commits on `main`. Download `.bin` files from *Artifacts* in the [Actions tab](https://github.com/sfambach/gbs-control/actions). Manual rebuild: *Actions → Build → Run workflow*.

**Arduino IDE:** open `gbs-control.ino`. Install board support and external libraries per platform — see **[docs/LIBRARIES.md](docs/LIBRARIES.md)**. WebSockets and Si5351mcu are vendored in `lib/`; WiFi management uses **[AsyncPersWiFiManager](https://github.com/sfambach/AsyncPersWiFiManager)**.

**Web UI:** sources in [`public/`](public/). Run `npm run build` there (or `make webui`) to regenerate [`generated/webui_html.h`](generated/webui_html.h) — see [`public/README.md`](public/README.md).

**Author:** [www.fambach.net](https://www.fambach.net)   
