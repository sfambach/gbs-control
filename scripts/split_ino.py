#!/usr/bin/env python3
"""Split gbs-control.ino into modular .cpp files (Phase B)."""

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
INO = ROOT / "gbs-control.ino"

VIDEO_PREAMBLE = '''\
#include "config.h"
#include "platform_gbs.h"
#include "gbs_globals.h"
#include "gbs_video.h"
#include "presets/ntsc_240p.h"
#include "presets/pal_240p.h"
#include "presets/ntsc_720x480.h"
#include "presets/pal_768x576.h"
#include "presets/ntsc_1280x720.h"
#include "presets/ntsc_1280x1024.h"
#include "presets/ntsc_1920x1080.h"
#include "presets/ntsc_downscale.h"
#include "presets/pal_1280x720.h"
#include "presets/pal_1280x1024.h"
#include "presets/pal_1920x1080.h"
#include "presets/pal_downscale.h"
#include "presets/presetMdSection.h"
#include "presets/presetDeinterlacerSection.h"
#include "presets/presetHdBypassSection.h"
#include "ofw/ofw_RGBS.h"

static uint8_t lastSegment = 0xFF;

'''

PREFS_PREAMBLE = '''\
#include "config.h"
#include "platform_gbs.h"
#include "gbs_globals.h"
#include "gbs_prefs.h"
#include "gbs_video.h"
#include "FS.h"
#include "presets/ntsc_240p.h"
#include "presets/pal_240p.h"

'''

WIFI_HELPERS = '''\
#if defined(ESP8266)
static inline void gbs_wifi_set_hostname(const char *name) { WiFi.hostname(name); }
static inline void gbs_wifi_disable_sleep() { WiFi.setSleepMode(WIFI_NONE_SLEEP); }
static inline void gbs_wifi_set_output_power(float dbm) { WiFi.setOutputPower(dbm); }
static inline void gbs_wifi_force_off() { WiFi.mode(WIFI_OFF); WiFi.forceSleepBegin(); }
static inline void gbs_mdns_update() { MDNS.update(); }
static inline bool gbs_mdns_begin(const char *host)
{
    return MDNS.begin(host, WiFi.localIP());
}
static inline void gbs_mdns_add_http_service()
{
    MDNS.addService("http", "tcp", 80);
    MDNS.announce();
}
#elif defined(ESP32)
static inline void gbs_wifi_set_hostname(const char *name) { WiFi.setHostname(name); }
static inline void gbs_wifi_disable_sleep() { WiFi.setSleep(false); }
static inline void gbs_wifi_set_output_power(float dbm)
{
    (void)dbm;
    WiFi.setTxPower(WIFI_POWER_17dBm);
}
static inline void gbs_wifi_force_off() { WiFi.mode(WIFI_OFF); }
static inline void gbs_mdns_update() {}
static inline bool gbs_mdns_begin(const char *host) { return MDNS.begin(host); }
static inline void gbs_mdns_add_http_service() { MDNS.addService("http", "tcp", 80); }
#endif

extern const char *ap_ssid;
extern const char *ap_password;
extern const char *device_hostname_partial;
extern AsyncWebServer server;
extern DNSServer dnsServer;
extern WebSocketsServer webSocket;
extern PersWiFiManager persWM;

'''

WIFI_PREAMBLE = '''\
#include "config.h"
#include "platform_gbs.h"
#include "gbs_globals.h"
#include "gbs_wifi.h"
#include "gbs_prefs.h"
#include "gbs_video.h"

#if GBS_ENABLE_WEB_GUI
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#elif defined(ESP32)
#include <WiFi.h>
#include <AsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>
#include "FS.h"
#include <DNSServer.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#if defined(ESP8266)
#include <ESP8266mDNS.h>
#elif defined(ESP32)
#include <ESPmDNS.h>
#endif
#include <PersWiFiManager.h>
#include "src/WebSockets.h"
#include "src/WebSocketsServer.h"

''' + WIFI_HELPERS + '''
'''

OLED_PREAMBLE = '''\
#include "config.h"
#include "platform_gbs.h"
#include "gbs_globals.h"
#include "gbs_oled_legacy.h"
#include "gbs_prefs.h"
#include "gbs_video.h"
#if GBS_ENABLE_OLED
#include "SSD1306Wire.h"
extern SSD1306Wire display;
extern const int pin_switch;
extern volatile int oled_encoder_pos;
extern volatile int oled_main_pointer;
extern volatile int oled_pointer_count;
extern volatile int oled_sub_pointer;
extern String oled_menu[4];
extern String oled_Resolutions[7];
extern String oled_Presets[8];
extern String oled_Misc[4];
extern int oled_menuItem;
extern int oled_subsetFrame;
extern int oled_selectOption;
extern int oled_page;
#endif

'''

EXTRACTS = [
    ("gbs_video.cpp", 299, 7098, VIDEO_PREAMBLE, False),
    ("gbs_prefs.cpp", 7100, 7124, PREFS_PREAMBLE, False),
    ("gbs_wifi.cpp", 7627, 7764, WIFI_PREAMBLE, False),
    ("gbs_wifi.cpp", 8956, 9988, '#include "generated/webui_html.h"\n\n', True),
    ("gbs_prefs.cpp", 9991, 10218, "", True),
    ("gbs_oled_legacy.cpp", 10220, 10830, OLED_PREAMBLE, False),
]

INSERT_AFTER = '#include "framesync.h"'
INSERT_LINES = [
    '#include "gbs_globals.h"',
    '#include "gbs_video.h"',
    '#include "gbs_prefs.h"',
    '#include "gbs_wifi.h"',
    '#include "gbs_oled_legacy.h"',
    '',
]

REMOVE_PATTERNS = [
    r'static inline void writeBytes\(uint8_t slaveRegister, uint8_t \*values, uint8_t numValues\);\n',
    r'const uint8_t \*loadPresetFromSPIFFS\(byte forVideoMode\);\n\n',
    r'struct MenuAttrs\n\{[\s\S]*?typedef MenuManager<GBS, MenuAttrs> Menu;\n\n',
    r'/// Video processing mode[\s\S]*?enum PresetID : uint8_t \{[\s\S]*?\};\n',
    r'struct FrameSyncAttrs\n\{[\s\S]*?typedef FrameSyncManager<GBS, FrameSyncAttrs> FrameSync;\n\n',
    r'static uint8_t lastSegment = 0xFF;[^\n]*\n',
]

SIG_RE = re.compile(
    r"^(?:static inline )?(?:void|bool|uint8_t|int|float|byte) \w+\([^;\n]*\)\s*\{?",
    re.MULTILINE,
)


def demote_static_inline(text: str) -> str:
    text = text.replace(
        "static inline void writeOneByte(", "void writeOneByte("
    )
    text = text.replace(
        "static inline void writeBytes(", "void writeBytes("
    )
    text = text.replace(
        "static inline void readFromRegister(", "void readFromRegister("
    )
    return text


def build_video_header(video_body: str) -> str:
    seen = set()
    decls = ["#pragma once", "", "#include <Arduino.h>", ""]
    for match in SIG_RE.finditer(video_body):
        sig = match.group(0).rstrip("{").strip()
        if sig.startswith("static"):
            continue
        if sig in seen:
            continue
        seen.add(sig)
        decls.append(sig + ";")
    decls.append("")
    return "\n".join(decls)


def main():
    original = INO.read_text(encoding="utf-8")
    lines = original.splitlines(keepends=True)
    remove = set()
    files: dict[str, list[str]] = {}

    for name, start, end, header, append in EXTRACTS:
        chunk = lines[start - 1 : end]
        if not append:
            files[name] = [header] + chunk
        else:
            files.setdefault(name, [])
            if header:
                files[name].extend([header])
            files[name].extend(chunk)
        for i in range(start, end + 1):
            remove.add(i)

    new_lines = []
    for i, line in enumerate(lines, start=1):
        if i not in remove:
            new_lines.append(line)

    new_text = "".join(new_lines)
    for pat in REMOVE_PATTERNS:
        new_text, n = re.subn(pat, "", new_text, count=1)
        if n:
            print(f"Removed pattern: {n}")

    if INSERT_AFTER in new_text:
        new_text = new_text.replace(
            INSERT_AFTER + "\n",
            INSERT_AFTER + "\n" + "\n".join(INSERT_LINES) + "\n",
            1,
        )

    video_text = demote_static_inline("".join(files["gbs_video.cpp"]))
    files["gbs_video.cpp"] = [video_text]

    (ROOT / "gbs_video.h").write_text(
        build_video_header(video_text), encoding="utf-8"
    )
    print("Wrote gbs_video.h")

    for name, content in files.items():
        text = content if isinstance(content, str) else "".join(content)
        if not text.endswith("\n"):
            text += "\n"
        (ROOT / name).write_text(text, encoding="utf-8")
        line_count = text.count("\n")
        print(f"Wrote {name} (~{line_count} lines)")

    INO.write_text(new_text, encoding="utf-8")
    print(f"Updated gbs-control.ino (~{new_text.count(chr(10))} lines)")


if __name__ == "__main__":
    main()
