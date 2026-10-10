# gbs-control

Documentation: https://ramapcsx2.github.io/gbs-control/

## This fork (sfambach/gbs-control)

Faster web UI, JSON API, cleaned up code base (modules, `config.h`) and an ESP32 port.
Write-ups (German): [Retro Projekt – GBS Control](https://fambach.net) and part 2 with the ESP32.

### Supported boards

| board | PlatformIO env | board file | host name |
|---|---|---|---|
| Wemos / Lolin D1 mini (ESP8266) | `d1_mini` | `board_d1_mini.h` | `gbscontrol` |
| D1 mini ESP32 (ESP32-WROOM) | `d1_mini32` | `board_d1_mini32.h` | `gbscontrol32` |

Same wiring by pin label on both: SDA = D2, SCL = D1, debug pin of the GBS board = D6.
A new board needs a `board_<name>.h` (pins, `GBS_BOARD_NAME`) and an env in `platformio.ini`;
a new chip family additionally a `platform_<family>.h` (see `platform.h`).

### Firmware binaries

Releases: https://github.com/sfambach/gbs-control/releases

| file | use |
|---|---|
| `gbscontrol-<version>-d1_mini-esp8266.bin` | ESP8266, flash at `0x0` (or OTA) |
| `gbscontrol-<version>-d1_mini32-esp32-full.bin` | ESP32, first install, flash at `0x0` |
| `gbscontrol-<version>-d1_mini32-esp32-update.bin` | ESP32, update (OTA, or at `0x10000`) |

```
esptool.py --chip esp32 write_flash 0x0 gbscontrol-<version>-d1_mini32-esp32-full.bin
esptool.py --chip esp8266 write_flash 0x0 gbscontrol-<version>-d1_mini-esp8266.bin
```

Presets, slots and settings live in the file system partition and survive a firmware update.

### Building

```
pio run -e d1_mini          # ESP8266
pio run -e d1_mini32        # ESP32 (also writes firmware_merged.bin)
pio run -e d1_mini32 -t upload
```

After changing `webui.html` regenerate `webui_html.h` with `cd public/scripts && ./html2h.sh`.

### Version numbers

The version comes from git (`tools/pio_version.py`): tag `v1.2.0` → `1.2.0`, later commits
`1.2.0-3-g<hash>`, uncommitted changes `-dirty`, no tag yet `dev-<hash>`. It is printed at boot,
reported in `/api/status` (`firmware.version`, `firmware.board`) and shown in the web UI (System).

Release: note the changes in `CHANGELOG.md`, then

```
git tag -a v1.2.0 -m "v1.2.0"
git push origin v1.2.0
```

The GitHub workflow (`.github/workflows/build.yml`) builds every push and, for a tag, publishes a
release with the binaries above.

## Original project

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
