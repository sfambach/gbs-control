// JSON interface for web clients (/api/...), next to the old single char commands
// part of the gbs-control sketch: all .ino files are concatenated into one translation unit
// (gbs-control.ino first), so globals and types from gbs-control.ino are visible here.
//
//   GET /api/status                      everything a client needs to show the current state
//   GET /api/set?<option>=<0|1>[&...]    set options by name (deinterlacer=bob|motionAdaptive)
//   GET /api/output?preset=<name>        output resolution: 1280x960 1280x1024 1280x720 1920x1080
//                                        480p downscale passthrough
//
// The handlers only read RAM state (rto / uopt), never GBS registers, so they can't interfere with
// register sequences in loop(). Changes are executed by queueing the same commands the old web ui
// sends, so the behaviour is identical.

#if ENABLE_WIFI

// options that the old web ui toggles with a single command
struct ApiToggle
{
    const char *name;
    char type; // 's' = serialCommand, 'u' = userCommand
    char cmd;
    uint8_t (*get)();
};

static const ApiToggle apiToggles[] = {
    {"adcAutoGain", 's', 'T', []() -> uint8_t { return uopt->enableAutoGain; }},
    {"scanlines", 'u', '7', []() -> uint8_t { return uopt->wantScanlines; }},
    {"lineFilter", 'u', 'm', []() -> uint8_t { return uopt->wantVdsLineFilter; }},
    {"peaking", 's', 'f', []() -> uint8_t { return uopt->wantPeaking; }},
    {"stepResponse", 's', 'V', []() -> uint8_t { return uopt->wantStepResponse; }},
    {"matchPresetSource", 's', 'Z', []() -> uint8_t { return uopt->matchPresetSource; }},
    {"fullHeight", 'u', 'v', []() -> uint8_t { return uopt->wantFullHeight; }},
    {"upscaleLowResVga", 'u', 'x', []() -> uint8_t { return uopt->preferScalingRgbhv; }},
    {"outputComponent", 's', 'L', []() -> uint8_t { return uopt->wantOutputComponent; }},
    {"force60Hz", 'u', '0', []() -> uint8_t { return uopt->PalForce60; }},
    {"disableExternalClockGenerator", 'u', 'X', []() -> uint8_t { return uopt->disableExternalClockGenerator; }},
    {"adcCalibration", 'u', 'w', []() -> uint8_t { return uopt->enableCalibrationADC; }},
    {"frameTimeLock", 'u', '5', []() -> uint8_t { return uopt->enableFrameTimeLock; }},
};

struct ApiOutputPreset
{
    const char *name;
    char type;
    char cmd;
};

static const ApiOutputPreset apiOutputPresets[] = {
    {"1280x960", 'u', 'f'}, {"1280x1024", 'u', 'p'}, {"1280x720", 'u', 'g'}, {"1920x1080", 'u', 's'},
    {"480p", 'u', 'h'}, {"downscale", 'u', 'L'}, {"passthrough", 's', 'K'},
};

static const char *apiOutputName()
{
    if (rto->isCustomPreset) {
        return "custom";
    }
    switch (rto->presetID) {
        case 0x01: case 0x11: return "1280x960";
        case 0x02: case 0x12: return "1280x1024";
        case 0x03: case 0x13: return "1280x720";
        case 0x04: case 0x14: return "480p";
        case 0x05: case 0x15: return "1920x1080";
        case 0x06: case 0x16: return "downscale";
        case PresetHdBypass: case PresetBypassRGBHV: return "passthrough";
        default: return "none";
    }
}

static String apiJsonBool(bool b)
{
    return b ? "true" : "false";
}

static void apiSendJson(AsyncWebServerRequest *request, int code, const String &json)
{
    AsyncWebServerResponse *response = request->beginResponse(code, "application/json", json);
    response->addHeader("Cache-Control", "no-store");
    request->send(response);
}

static void apiStatus(AsyncWebServerRequest *request)
{
    String j;
    j.reserve(900);
    j += "{\"output\":{\"preset\":\"";
    j += apiOutputName();
    j += "\",\"presetId\":";
    j += rto->presetID;
    j += ",\"custom\":" + apiJsonBool(rto->isCustomPreset);
    j += ",\"slot\":\"";
    j += (char)uopt->presetSlot;
    j += "\",\"preference\":";
    j += (uint8_t)uopt->presetPreference;
    j += "},\"input\":{\"connected\":" + apiJsonBool(!rto->sourceDisconnected);
    j += ",\"videoMode\":";
    j += rto->videoStandardInput;
    j += ",\"sync\":\"";
    j += rto->syncTypeCsync ? "csync" : "hv";
    j += "\",\"ypbpr\":" + apiJsonBool(rto->inputIsYpBpR);
    j += ",\"stable\":";
    j += rto->continousStableCounter;
    j += "},\"options\":{";
    for (size_t i = 0; i < sizeof(apiToggles) / sizeof(apiToggles[0]); i++) {
        if (i) j += ",";
        j += "\"";
        j += apiToggles[i].name;
        j += "\":" + apiJsonBool(apiToggles[i].get());
    }
    j += ",\"deinterlacer\":\"";
    j += uopt->deintMode ? "bob" : "motionAdaptive";
    j += "\"},\"wifi\":{";
    if ((WiFi.getMode() & WIFI_AP) && WiFi.status() != WL_CONNECTED) {
        j += "\"mode\":\"ap\"";
    } else {
        j += "\"mode\":\"sta\",\"ssid\":\"" + WiFi.SSID() + "\",\"bssid\":\"" + WiFi.BSSIDstr() +
             "\",\"channel\":" + String(WiFi.channel()) + ",\"rssi\":" + String(WiFi.RSSI());
    }
    j += "},\"freeHeap\":";
    j += ESP.getFreeHeap();
    j += "}";
    apiSendJson(request, 200, j);
}

static void apiSet(AsyncWebServerRequest *request)
{
    String unknown;
    uint8_t queued = 0;
    for (size_t p = 0; p < request->params(); p++) {
        const AsyncWebParameter *param = request->getParam(p);
        const String &name = param->name();
        const String &value = param->value();
        bool handled = false;
        if (name == "deinterlacer") {
            if (value == "bob" || value == "motionAdaptive") {
                queueWebCommand('u', value == "bob" ? 'q' : 'r');
                queued++;
                handled = true;
            }
        } else {
            for (size_t i = 0; i < sizeof(apiToggles) / sizeof(apiToggles[0]); i++) {
                const ApiToggle &t = apiToggles[i];
                if (name != t.name) {
                    continue;
                }
                if (value == "0" || value == "1" || value == "true" || value == "false") {
                    bool want = (value == "1" || value == "true");
                    // toggles still waiting in the queue flip the current value
                    bool current = (t.get() != 0) ^ (countQueuedWebCommand(t.type, t.cmd) & 1);
                    if (want != current) {
                        queueWebCommand(t.type, t.cmd);
                        queued++;
                    }
                    handled = true;
                }
                break;
            }
        }
        if (!handled) {
            if (unknown.length()) unknown += ",";
            unknown += "\"" + name + "\"";
        }
    }
    String j = "{\"ok\":" + apiJsonBool(unknown.length() == 0) + ",\"queued\":" + String(queued);
    if (unknown.length()) {
        j += ",\"invalid\":[" + unknown + "]";
    }
    j += "}";
    apiSendJson(request, unknown.length() ? 400 : 200, j);
}

static void apiOutput(AsyncWebServerRequest *request)
{
    if (request->hasParam("preset")) {
        const String &preset = request->getParam("preset")->value();
        for (size_t i = 0; i < sizeof(apiOutputPresets) / sizeof(apiOutputPresets[0]); i++) {
            if (preset == apiOutputPresets[i].name) {
                queueWebCommand(apiOutputPresets[i].type, apiOutputPresets[i].cmd);
                apiSendJson(request, 200, "{\"ok\":true}");
                return;
            }
        }
    }
    apiSendJson(request, 400, "{\"ok\":false,\"error\":\"preset: 1280x960 1280x1024 1280x720 1920x1080 480p downscale passthrough\"}");
}

void startApi()
{
    server.on("/api/status", HTTP_GET, apiStatus);
    server.on("/api/set", HTTP_GET, apiSet);
    server.on("/api/output", HTTP_GET, apiOutput);
}

#endif
