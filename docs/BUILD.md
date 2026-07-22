# Local build guide

Quick reference for building GBS-Control on your machine. CI uses the same PlatformIO environments — see [`.github/workflows/build.yml`](../.github/workflows/build.yml).

## Prerequisites

| Tool | Purpose | Install |
|---|---|---|
| **PlatformIO** | Firmware build | `py -3 -m pip install -U platformio` |
| **Git** | Submodules | [git-scm.com](https://git-scm.com/) |
| **Node.js** *(optional)* | Web UI rebuild | [nodejs.org](https://nodejs.org/) |
| **Git Bash** *(optional)* | `html2h.sh` for web UI | Comes with Git for Windows |

After pip install, add Python Scripts to PATH (Windows), e.g.:

`%LOCALAPPDATA%\Programs\Python\Python313\Scripts`

Then `pio --version` should work. Alternatively always use `py -3 -m platformio`.

## One-time setup

```powershell
cd C:\devel\esp\gbs-control
git submodule update --init --recursive
py -3 -m pip install -U platformio
```

## Build firmware (ESP8266 — default)

The committed `generated/webui_html.h` is enough for firmware builds; you do **not** need npm unless you change the web UI.

### Windows (PowerShell)

```powershell
.\build.ps1                      # build d1_mini
.\build.ps1 -Target firmware-only
.\build.ps1 -Board esp32dev
.\build.ps1 -Target upload -Port COM5
.\build.ps1 -Help
```

Output: **`.pio\build\d1_mini\firmware.bin`**

### PlatformIO directly

```powershell
py -3 -m platformio run -e d1_mini
```

### Linux / macOS / Git Bash (Makefile)

```bash
make firmware-only               # no web UI
make upload BOARD=d1_mini
make help
```

## PlatformIO environments

| `-Board` / `-e` | Target |
|---|---|
| `d1_mini` | **ESP8266** (Wemos D1 mini) — primary GBS-Control hardware |
| `esp32dev` | ESP32 classic (port in progress) |
| `esp32-s3-devkitc-1` | ESP32-S3 |
| `esp32-c3-devkitm-1` | ESP32-C3 |
| `esp32-c6-devkitc-1` | ESP32-C6 |

See [`platformio.ini`](../platformio.ini) and [`config.h`](../config.h) for pins and feature toggles.

## Flash / serial monitor

```powershell
.\build.ps1 -Target upload -Port COM5
.\build.ps1 -Target monitor -Port COM5
```

Or PlatformIO:

```powershell
py -3 -m platformio run -e d1_mini -t upload --upload-port COM5
py -3 -m platformio device monitor -e d1_mini --port COM5
```

## Web UI (optional)

Only needed when editing files under `public/`:

```powershell
cd public
npm install
npm run build
```

Requires **Git Bash** tools (`bash`, `gzip`, `xxd`) for `scripts/html2h.sh`. Commit `generated/webui_html.h` after changes.

## Arduino IDE

Open `gbs-control.ino` in the repo root. All `.cpp` modules must stay beside the sketch. External libraries: see [LIBRARIES.md](LIBRARIES.md).

## Troubleshooting

| Problem | Fix |
|---|---|
| `pio` not found | Use `py -3 -m platformio` or add Python Scripts to PATH |
| Submodule / `3rdparty` errors | `git submodule update --init --recursive` |
| First build slow | PlatformIO downloads toolchains (~hundreds of MB) |
| Upload fails | Check COM port in Device Manager; close serial monitors |
| Web UI build fails | `public/assets/` may be missing locally — use committed `generated/webui_html.h` |

## Without local build

Download `.bin` artifacts from [GitHub Actions](https://github.com/sfambach/gbs-control/actions) (ESP8266: `firmware-d1_mini`).
