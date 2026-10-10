# Changelog (sfambach/gbs-control)

Versions follow [semantic versioning](https://semver.org): major = incompatible changes (e.g. stored
presets / slots), minor = new features, patch = fixes. The firmware version comes from the git tag
(see README, "Version numbers").

## [Unreleased]

### Added
- ESP32 support (D1 mini ESP32): platform layer (`platform_esp8266.h`, `platform_esp32.h`) and board
  files (`board_d1_mini.h`, `board_d1_mini32.h`), host name `gbscontrol32`
- JSON API: `/api/status`, `/api/set`, `/api/output`
- `config.h`: central build configuration
- WiFi setup without restart: the device joins the new network right away and offers to switch the
  access point off; the page then opens the device in the new network
- access point fallback retries the stored network in the background (AP + STA)
- firmware version (from git) at boot, in `/api/status` and in the web UI; release workflow
- serial command `I`: WiFi diagnostics

### Changed
- web UI: commands over the WebSocket, much faster page loads (larger TCP segments)
- maintained ESP32Async libraries (ESPAsyncTCP / AsyncTCP, ESPAsyncWebServer)
- serial port 921600 baud
- code split into modules (`gbs_*.ino`)
- ESP32: connects to the strongest access point of a network

### Fixed
- UTF-8 slot names (umlauts)
- crash in `/wifi/list` while a scan was running
- coast position retry hogging the main loop
- sluggish web UI on the ESP8266
