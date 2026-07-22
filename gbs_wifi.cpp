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
#include "lib/WebSockets.h"
#include "lib/WebSocketsServer.h"

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


#if GBS_ENABLE_WEB_GUI
void updateWebSocketData()
{
    if (rto->webServerEnabled && rto->webServerStarted) {
        if (webSocket.connectedClients() > 0) {

            constexpr size_t MESSAGE_LEN = 6;
            char toSend[MESSAGE_LEN] = {0};
            toSend[0] = '#'; // makeshift ping in slot 0

            if (rto->isCustomPreset) {
                toSend[1] = '9';
            } else switch (rto->presetID) {
                case 0x01:
                case 0x11:
                    toSend[1] = '1';
                    break;
                case 0x02:
                case 0x12:
                    toSend[1] = '2';
                    break;
                case 0x03:
                case 0x13:
                    toSend[1] = '3';
                    break;
                case 0x04:
                case 0x14:
                    toSend[1] = '4';
                    break;
                case 0x05:
                case 0x15:
                    toSend[1] = '5';
                    break;
                case 0x06:
                case 0x16:
                    toSend[1] = '6';
                    break;
                case PresetHdBypass: // bypass 1
                case PresetBypassRGBHV: // bypass 2
                    toSend[1] = '8';
                    break;
                default:
                    toSend[1] = '0';
                    break;
            }

            toSend[2] = (char)uopt->presetSlot;

            // '@' = 0x40, used for "byte is present" detection; 0x80 not in ascii table
            toSend[3] = '@';
            toSend[4] = '@';
            toSend[5] = '@';

            if (uopt->enableAutoGain) {
                toSend[3] |= (1 << 0);
            }
            if (uopt->wantScanlines) {
                toSend[3] |= (1 << 1);
            }
            if (uopt->wantVdsLineFilter) {
                toSend[3] |= (1 << 2);
            }
            if (uopt->wantPeaking) {
                toSend[3] |= (1 << 3);
            }
            if (uopt->PalForce60) {
                toSend[3] |= (1 << 4);
            }
            if (uopt->wantOutputComponent) {
                toSend[3] |= (1 << 5);
            }

            if (uopt->matchPresetSource) {
                toSend[4] |= (1 << 0);
            }
            if (uopt->enableFrameTimeLock) {
                toSend[4] |= (1 << 1);
            }
            if (uopt->deintMode) {
                toSend[4] |= (1 << 2);
            }
            if (uopt->wantTap6) {
                toSend[4] |= (1 << 3);
            }
            if (uopt->wantStepResponse) {
                toSend[4] |= (1 << 4);
            }
            if (uopt->wantFullHeight) {
                toSend[4] |= (1 << 5);
            }

            if (uopt->enableCalibrationADC) {
                toSend[5] |= (1 << 0);
            }
            if (uopt->preferScalingRgbhv) {
                toSend[5] |= (1 << 1);
            }
            if (uopt->disableExternalClockGenerator) {
                toSend[5] |= (1 << 2);
            }

            // send ping and stats
            if (ESP.getFreeHeap() > 14000) {
                webSocket.broadcastTXT(toSend, MESSAGE_LEN);
            } else {
                webSocket.disconnect();
            }
        }
    }
}

void handleWiFi(boolean instant)
{
    static unsigned long lastTimePing = millis();
    if (rto->webServerEnabled && rto->webServerStarted) {
        gbs_mdns_update();
        persWM.handleWiFi(); // if connected, returns instantly. otherwise it reconnects or opens AP
        dnsServer.processNextRequest();

        if ((millis() - lastTimePing) > 953) { // slightly odd value so not everything happens at once
            webSocket.broadcastPing();
        }
        if (((millis() - lastTimePing) > 973) || instant) {
            if ((webSocket.connectedClients(false) > 0) || instant) { // true = with compliant ping
                updateWebSocketData();
            }
            lastTimePing = millis();
        }
    }

#if GBS_ENABLE_OTA
    if (rto->allowUpdatesOTA) {
        ArduinoOTA.handle();
    }
#endif
    yield();
}
#endif // GBS_ENABLE_WEB_GUI
#include "generated/webui_html.h"

#if GBS_ENABLE_WEB_GUI
#include "generated/webui_html.h"
// Regenerate: cd public && npm run build  (see public/README.md)

void handleType2Command(char argument)
{
    myLog("user", argument);
    switch (argument) {
        case '0':
            SerialM.print(F("pal force 60hz "));
            if (uopt->PalForce60 == 0) {
                uopt->PalForce60 = 1;
                SerialM.println("on");
            } else {
                uopt->PalForce60 = 0;
                SerialM.println("off");
            }
            saveUserPrefs();
            break;
        case '1':
            // reset to defaults button
            webSocket.close();
            loadDefaultUserOptions();
            saveUserPrefs();
            Serial.println(F("options set to defaults, restarting"));
            delay(60);
            ESP.reset(); // don't use restart(), messes up websocket reconnects
            //
            break;
        case '2':
            //
            break;
        case '3': // load custom preset
        {
            uopt->presetPreference = OutputCustomized; // custom
            if (rto->videoStandardInput == 14) {
                // vga upscale path: let synwatcher handle it
                rto->videoStandardInput = 15;
            } else {
                // normal path
                applyPresets(rto->videoStandardInput);
            }
            saveUserPrefs();
        } break;
        case '4': // save custom preset
            savePresetToSPIFFS();
            uopt->presetPreference = OutputCustomized; // custom
            saveUserPrefs();
            break;
        case '5':
            //Frame Time Lock toggle
            uopt->enableFrameTimeLock = !uopt->enableFrameTimeLock;
            saveUserPrefs();
            if (uopt->enableFrameTimeLock) {
                SerialM.println(F("FTL on"));
            } else {
                SerialM.println(F("FTL off"));
            }
            if (!rto->extClockGenDetected) {
                FrameSync::reset(uopt->frameTimeLockMethod);
            }
            if (uopt->enableFrameTimeLock) {
                activeFrameTimeLockInitialSteps();
            }
            break;
        case '6':
            //
            break;
        case '7':
            uopt->wantScanlines = !uopt->wantScanlines;
            SerialM.print(F("scanlines: "));
            if (uopt->wantScanlines) {
                SerialM.println(F("on (Line Filter recommended)"));
            } else {
                disableScanlines();
                SerialM.println("off");
            }
            saveUserPrefs();
            break;
        case '9':
            //
            break;
        case 'a':
            webSocket.close();
            Serial.println(F("restart"));
            delay(60);
            ESP.reset(); // don't use restart(), messes up websocket reconnects
            break;
        case 'e': // print files on spiffs
        {
            Dir dir = SPIFFS.openDir("/");
            while (dir.next()) {
                SerialM.print(dir.fileName());
                SerialM.print(" ");
                SerialM.println(dir.fileSize());
                delay(1); // wifi stack
            }
            ////
            File f = SPIFFS.open("/preferencesv2.txt", "r");
            if (!f) {
                SerialM.println(F("failed opening preferences file"));
            } else {
                SerialM.print(F("preset preference = "));
                SerialM.println((uint8_t)(f.read() - '0'));
                SerialM.print(F("frame time lock = "));
                SerialM.println((uint8_t)(f.read() - '0'));
                SerialM.print(F("preset slot = "));
                SerialM.println((uint8_t)(f.read()));
                SerialM.print(F("frame lock method = "));
                SerialM.println((uint8_t)(f.read() - '0'));
                SerialM.print(F("auto gain = "));
                SerialM.println((uint8_t)(f.read() - '0'));
                SerialM.print(F("scanlines = "));
                SerialM.println((uint8_t)(f.read() - '0'));
                SerialM.print(F("component output = "));
                SerialM.println((uint8_t)(f.read() - '0'));
                SerialM.print(F("deinterlacer mode = "));
                SerialM.println((uint8_t)(f.read() - '0'));
                SerialM.print(F("line filter = "));
                SerialM.println((uint8_t)(f.read() - '0'));
                SerialM.print(F("peaking = "));
                SerialM.println((uint8_t)(f.read() - '0'));
                SerialM.print(F("preferScalingRgbhv = "));
                SerialM.println((uint8_t)(f.read() - '0'));
                SerialM.print(F("6-tap = "));
                SerialM.println((uint8_t)(f.read() - '0'));
                SerialM.print(F("pal force60 = "));
                SerialM.println((uint8_t)(f.read() - '0'));
                SerialM.print(F("matched = "));
                SerialM.println((uint8_t)(f.read() - '0'));
                SerialM.print(F("step response = "));
                SerialM.println((uint8_t)(f.read() - '0'));
                SerialM.print(F("disable external clock generator = "));
                SerialM.println((uint8_t)(f.read() - '0'));

                f.close();
            }
        } break;
        case 'f':
        case 'g':
        case 'h':
        case 'p':
        case 's':
        case 'L': {
            // load preset via webui
            uint8_t videoMode = getVideoMode();
            if (videoMode == 0 && GBS::STATUS_SYNC_PROC_HSACT::read())
                videoMode = rto->videoStandardInput; // last known good as fallback
            //else videoMode stays 0 and we'll apply via some assumptions

            if (argument == 'f')
                uopt->presetPreference = Output960P; // 1280x960
            if (argument == 'g')
                uopt->presetPreference = Output720P; // 1280x720
            if (argument == 'h')
                uopt->presetPreference = Output480P; // 720x480/768x576
            if (argument == 'p')
                uopt->presetPreference = Output1024P; // 1280x1024
            if (argument == 's')
                uopt->presetPreference = Output1080P; // 1920x1080
            if (argument == 'L')
                uopt->presetPreference = OutputDownscale; // downscale

            rto->useHdmiSyncFix = 1; // disables sync out when programming preset
            if (rto->videoStandardInput == 14) {
                // vga upscale path: let synwatcher handle it
                rto->videoStandardInput = 15;
            } else {
                // normal path
                applyPresets(videoMode);
            }
            saveUserPrefs();
        } break;
        case 'i':
            // toggle active frametime lock method
            if (!rto->extClockGenDetected) {
                FrameSync::reset(uopt->frameTimeLockMethod);
            }
            if (uopt->frameTimeLockMethod == 0) {
                uopt->frameTimeLockMethod = 1;
            } else if (uopt->frameTimeLockMethod == 1) {
                uopt->frameTimeLockMethod = 0;
            }
            saveUserPrefs();
            activeFrameTimeLockInitialSteps();
            break;
        case 'l':
            // cycle through available SDRAM clocks
            {
                uint8_t PLL_MS = GBS::PLL_MS::read();
                uint8_t memClock = 0;

                if (PLL_MS == 0)
                    PLL_MS = 2;
                else if (PLL_MS == 2)
                    PLL_MS = 7;
                else if (PLL_MS == 7)
                    PLL_MS = 4;
                else if (PLL_MS == 4)
                    PLL_MS = 3;
                else if (PLL_MS == 3)
                    PLL_MS = 5;
                else if (PLL_MS == 5)
                    PLL_MS = 0;

                switch (PLL_MS) {
                    case 0:
                        memClock = 108;
                        break;
                    case 1:
                        memClock = 81;
                        break; // goes well with 4_2C = 0x14, 4_2D = 0x27
                    case 2:
                        memClock = 10;
                        break; // feedback clock
                    case 3:
                        memClock = 162;
                        break;
                    case 4:
                        memClock = 144;
                        break;
                    case 5:
                        memClock = 185;
                        break; // slight OC
                    case 6:
                        memClock = 216;
                        break; // !OC!
                    case 7:
                        memClock = 129;
                        break;
                    default:
                        break;
                }
                GBS::PLL_MS::write(PLL_MS);
                ResetSDRAM();
                if (memClock != 10) {
                    SerialM.print(F("SDRAM clock: "));
                    SerialM.print(memClock);
                    SerialM.println("Mhz");
                } else {
                    SerialM.print(F("SDRAM clock: "));
                    SerialM.println(F("Feedback clock"));
                }
            }
            break;
        case 'm':
            SerialM.print(F("Line Filter: "));
            if (uopt->wantVdsLineFilter) {
                uopt->wantVdsLineFilter = 0;
                GBS::VDS_D_RAM_BYPS::write(1);
                SerialM.println("off");
            } else {
                uopt->wantVdsLineFilter = 1;
                GBS::VDS_D_RAM_BYPS::write(0);
                SerialM.println("on");
            }
            saveUserPrefs();
            break;
        case 'n':
            SerialM.print(F("ADC gain++ : "));
            uopt->enableAutoGain = 0;
            setAdcGain(GBS::ADC_RGCTRL::read() - 1);
            SerialM.println(GBS::ADC_RGCTRL::read(), HEX);
            break;
        case 'o':
            SerialM.print(F("ADC gain-- : "));
            uopt->enableAutoGain = 0;
            setAdcGain(GBS::ADC_RGCTRL::read() + 1);
            SerialM.println(GBS::ADC_RGCTRL::read(), HEX);
            break;
        case 'A': {
            uint16_t htotal = GBS::VDS_HSYNC_RST::read();
            uint16_t hbstd = GBS::VDS_DIS_HB_ST::read();
            uint16_t hbspd = GBS::VDS_DIS_HB_SP::read();
            if ((hbstd > 4) && (hbspd < (htotal - 4))) {
                GBS::VDS_DIS_HB_ST::write(GBS::VDS_DIS_HB_ST::read() - 4);
                GBS::VDS_DIS_HB_SP::write(GBS::VDS_DIS_HB_SP::read() + 4);
            } else {
                SerialM.println("limit");
            }
        } break;
        case 'B': {
            uint16_t htotal = GBS::VDS_HSYNC_RST::read();
            uint16_t hbstd = GBS::VDS_DIS_HB_ST::read();
            uint16_t hbspd = GBS::VDS_DIS_HB_SP::read();
            if ((hbstd < (htotal - 4)) && (hbspd > 4)) {
                GBS::VDS_DIS_HB_ST::write(GBS::VDS_DIS_HB_ST::read() + 4);
                GBS::VDS_DIS_HB_SP::write(GBS::VDS_DIS_HB_SP::read() - 4);
            } else {
                SerialM.println("limit");
            }
        } break;
        case 'C': {
            // vert mask +
            uint16_t vtotal = GBS::VDS_VSYNC_RST::read();
            uint16_t vbstd = GBS::VDS_DIS_VB_ST::read();
            uint16_t vbspd = GBS::VDS_DIS_VB_SP::read();
            if ((vbstd > 6) && (vbspd < (vtotal - 4))) {
                GBS::VDS_DIS_VB_ST::write(vbstd - 2);
                GBS::VDS_DIS_VB_SP::write(vbspd + 2);
            } else {
                SerialM.println("limit");
            }
        } break;
        case 'D': {
            // vert mask -
            uint16_t vtotal = GBS::VDS_VSYNC_RST::read();
            uint16_t vbstd = GBS::VDS_DIS_VB_ST::read();
            uint16_t vbspd = GBS::VDS_DIS_VB_SP::read();
            if ((vbstd < (vtotal - 4)) && (vbspd > 6)) {
                GBS::VDS_DIS_VB_ST::write(vbstd + 2);
                GBS::VDS_DIS_VB_SP::write(vbspd - 2);
            } else {
                SerialM.println("limit");
            }
        } break;
        case 'q':
            if (uopt->deintMode != 1) {
                uopt->deintMode = 1;
                disableMotionAdaptDeinterlace();
                if (GBS::GBS_OPTION_SCANLINES_ENABLED::read()) {
                    disableScanlines();
                }
                saveUserPrefs();
            }
            SerialM.println(F("Deinterlacer: Bob"));
            break;
        case 'r':
            if (uopt->deintMode != 0) {
                uopt->deintMode = 0;
                saveUserPrefs();
                // will enable next loop()
            }
            SerialM.println(F("Deinterlacer: Motion Adaptive"));
            break;
        case 't':
            // unused now
            SerialM.print(F("6-tap: "));
            if (uopt->wantTap6 == 0) {
                uopt->wantTap6 = 1;
                GBS::VDS_TAP6_BYPS::write(0);
                GBS::MADPT_Y_DELAY_UV_DELAY::write(GBS::MADPT_Y_DELAY_UV_DELAY::read() - 1);
                SerialM.println("on");
            } else {
                uopt->wantTap6 = 0;
                GBS::VDS_TAP6_BYPS::write(1);
                GBS::MADPT_Y_DELAY_UV_DELAY::write(GBS::MADPT_Y_DELAY_UV_DELAY::read() + 1);
                SerialM.println("off");
            }
            saveUserPrefs();
            break;
        case 'u':
            // restart to attempt wifi station mode connect
            delay(30);
            WiFi.mode(WIFI_STA);
            gbs_wifi_set_hostname(device_hostname_partial); // _full
            delay(30);
            ESP.reset();
            break;
        case 'v': {
            uopt->wantFullHeight = !uopt->wantFullHeight;
            saveUserPrefs();
            uint8_t vidMode = getVideoMode();
            if (uopt->presetPreference == 5) {
                if (GBS::GBS_PRESET_ID::read() == 0x05 || GBS::GBS_PRESET_ID::read() == 0x15) {
                    applyPresets(vidMode);
                }
            }
        } break;
        case 'w':
            uopt->enableCalibrationADC = !uopt->enableCalibrationADC;
            saveUserPrefs();
            break;
        case 'x':
            uopt->preferScalingRgbhv = !uopt->preferScalingRgbhv;
            SerialM.print(F("preferScalingRgbhv: "));
            if (uopt->preferScalingRgbhv) {
                SerialM.println("on");
            } else {
                SerialM.println("off");
            }
            saveUserPrefs();
            break;
        case 'X':
            SerialM.print(F("ExternalClockGenerator "));
            if (uopt->disableExternalClockGenerator == 0) {
                uopt->disableExternalClockGenerator = 1;
                SerialM.println("disabled");
            } else {
                uopt->disableExternalClockGenerator = 0;
                SerialM.println("enabled");
            }
            saveUserPrefs();
            break;
        case 'z':
            // sog slicer level
            if (rto->currentLevelSOG > 0) {
                rto->currentLevelSOG -= 1;
            } else {
                rto->currentLevelSOG = 16;
            }
            setAndUpdateSogLevel(rto->currentLevelSOG);
            optimizePhaseSP();
            SerialM.print("Phase: ");
            SerialM.print(rto->phaseSP);
            SerialM.print(" SOG: ");
            SerialM.print(rto->currentLevelSOG);
            SerialM.println();
            break;
        case 'E':
            // test option for now
            SerialM.print(F("IF Auto Offset: "));
            toggleIfAutoOffset();
            if (GBS::IF_AUTO_OFST_EN::read()) {
                SerialM.println("on");
            } else {
                SerialM.println("off");
            }
            break;
        case 'F':
            // freeze pic
            if (GBS::CAPTURE_ENABLE::read()) {
                GBS::CAPTURE_ENABLE::write(0);
            } else {
                GBS::CAPTURE_ENABLE::write(1);
            }
            break;
        case 'K':
            // scanline strength
            if (uopt->scanlineStrength >= 0x10) {
                uopt->scanlineStrength -= 0x10;
            } else {
                uopt->scanlineStrength = 0x50;
            }
            if (rto->scanlinesEnabled) {
                GBS::MADPT_Y_MI_OFFSET::write(uopt->scanlineStrength);
                GBS::MADPT_UV_MI_OFFSET::write(uopt->scanlineStrength);
            }
            saveUserPrefs();
            SerialM.print(F("Scanline Brightness: "));
            SerialM.println(uopt->scanlineStrength, HEX);
            break;
        case 'Z':
            // brightness++
            GBS::VDS_Y_OFST::write(GBS::VDS_Y_OFST::read() + 1);
            SerialM.print(F("Brightness++ : "));
            SerialM.println(GBS::VDS_Y_OFST::read(), DEC);
            break;
        case 'T':
            // brightness--
            GBS::VDS_Y_OFST::write(GBS::VDS_Y_OFST::read() - 1);
            SerialM.print(F("Brightness-- : "));
            SerialM.println(GBS::VDS_Y_OFST::read(), DEC);
        break;
        case 'N':
            // contrast++
            GBS::VDS_Y_GAIN::write(GBS::VDS_Y_GAIN::read() + 1);
            SerialM.print(F("Contrast++ : "));
            SerialM.println(GBS::VDS_Y_GAIN::read(), DEC);
        break;
        case 'M':
            // contrast--
            GBS::VDS_Y_GAIN::write(GBS::VDS_Y_GAIN::read() - 1);
            SerialM.print(F("Contrast-- : "));
            SerialM.println(GBS::VDS_Y_GAIN::read(), DEC);
        break;
        case 'Q':
             // pb/u gain++
            GBS::VDS_UCOS_GAIN::write(GBS::VDS_UCOS_GAIN::read() + 1);
            SerialM.print(F("Pb/U gain++ : "));
            SerialM.println(GBS::VDS_UCOS_GAIN::read(), DEC);
            break;
        case 'H':
             // pb/u gain--
            GBS::VDS_UCOS_GAIN::write(GBS::VDS_UCOS_GAIN::read() - 1);
            SerialM.print(F("Pb/U gain-- : "));
            SerialM.println(GBS::VDS_UCOS_GAIN::read(), DEC);
            break;
        break;
        case 'P':
            // pr/v gain++
            GBS::VDS_VCOS_GAIN::write(GBS::VDS_VCOS_GAIN::read() + 1);
            SerialM.print(F("Pr/V gain++ : "));
            SerialM.println(GBS::VDS_VCOS_GAIN::read(), DEC);
            break;
        case 'S':
            // pr/v gain--
            GBS::VDS_VCOS_GAIN::write(GBS::VDS_VCOS_GAIN::read() - 1);
            SerialM.print(F("Pr/V gain-- : "));
            SerialM.println(GBS::VDS_VCOS_GAIN::read(), DEC);
            break;
        case 'O':
            // info
            if (GBS::ADC_INPUT_SEL::read() == 1)
            {
                SerialM.println("RGB reg");
                SerialM.println(F("------------ "));
                SerialM.print(F("Y_OFFSET: "));
                SerialM.println(GBS::VDS_Y_OFST::read(), DEC);
                SerialM.print(F("U_OFFSET: "));
                SerialM.println( GBS::VDS_U_OFST::read(), DEC);
                SerialM.print(F("V_OFFSET: "));
                SerialM.println(GBS::VDS_V_OFST::read(), DEC);
                SerialM.print(F("Y_GAIN: "));
                SerialM.println(GBS::VDS_Y_GAIN::read(), DEC);
                SerialM.print(F("USIN_GAIN: "));
                SerialM.println(GBS::VDS_USIN_GAIN::read(), DEC);
                SerialM.print(F("UCOS_GAIN: "));
                SerialM.println(GBS::VDS_UCOS_GAIN::read(), DEC);
            }
            else
            {
                SerialM.println("YPbPr reg");
                SerialM.println(F("------------ "));
                SerialM.print(F("Y_OFFSET: "));
                SerialM.println(GBS::VDS_Y_OFST::read(), DEC);
                SerialM.print(F("U_OFFSET: "));
                SerialM.println( GBS::VDS_U_OFST::read(), DEC);
                SerialM.print(F("V_OFFSET: "));
                SerialM.println(GBS::VDS_V_OFST::read(), DEC);
                SerialM.print(F("Y_GAIN: "));
                SerialM.println(GBS::VDS_Y_GAIN::read(), DEC);
                SerialM.print(F("USIN_GAIN: "));
                SerialM.println(GBS::VDS_USIN_GAIN::read(), DEC);
                SerialM.print(F("UCOS_GAIN: "));
                SerialM.println(GBS::VDS_UCOS_GAIN::read(), DEC);
            }
            break;
        case 'U':
            // default
            if (GBS::ADC_INPUT_SEL::read() == 1)
            {
                GBS::VDS_Y_GAIN::write(128);
                GBS::VDS_UCOS_GAIN::write(28);
                GBS::VDS_VCOS_GAIN::write(41);
                GBS::VDS_Y_OFST::write(0);
                GBS::VDS_U_OFST::write(0);
                GBS::VDS_V_OFST::write(0);
                GBS::ADC_ROFCTRL::write(adco->r_off);
                GBS::ADC_GOFCTRL::write(adco->g_off);
                GBS::ADC_BOFCTRL::write(adco->b_off);
                SerialM.println("RGB:defauit");
            }
            else
            {
                GBS::VDS_Y_GAIN::write(128);
                GBS::VDS_UCOS_GAIN::write(28);
                GBS::VDS_VCOS_GAIN::write(41);
                GBS::VDS_Y_OFST::write(254);
                GBS::VDS_U_OFST::write(3);
                GBS::VDS_V_OFST::write(3);
                GBS::ADC_ROFCTRL::write(adco->r_off);
                GBS::ADC_GOFCTRL::write(adco->g_off);
                GBS::ADC_BOFCTRL::write(adco->b_off);
                SerialM.println("YPbPr:defauit");
            }
            break;
        default:
            break;
    }
}

//void webSocketEvent(uint8_t num, uint8_t type, uint8_t * payload, size_t length) {
//  switch (type) {
//  case WStype_DISCONNECTED:
//    //Serial.print("WS: #"); Serial.print(num); Serial.print(" disconnected,");
//    //Serial.print(" remaining: "); Serial.println(webSocket.connectedClients());
//  break;
//  case WStype_CONNECTED:
//    //Serial.print("WS: #"); Serial.print(num); Serial.print(" connected, ");
//    //Serial.print(" total: "); Serial.println(webSocket.connectedClients());
//    updateWebSocketData();
//  break;
//  case WStype_PONG:
//    //Serial.print("p");
//    updateWebSocketData();
//  break;
//  }
//}

#if defined(ESP8266)
WiFiEventHandler disconnectedEventHandler;
#endif

void startWebserver()
{
    persWM.setApCredentials(ap_ssid, ap_password);
    persWM.setStaHostname(device_hostname_partial);
    persWM.setConnectTimeout(GBS_WIFI_CONNECT_TIMEOUT_SEC);
    persWM.onConnect([]() {
        SerialM.print(F("(WiFi): STA mode connected; IP: "));
        SerialM.println(WiFi.localIP().toString());
        if (gbs_mdns_begin(device_hostname_partial)) { // MDNS request for gbscontrol.local
            //Serial.println("MDNS started");
            gbs_mdns_add_http_service();
        }
        SerialM.println(FPSTR(st_info_string));
    });
    persWM.onAp([]() {
        SerialM.println(FPSTR(ap_info_string));
        // add mdns announce here as well?
    });

#if defined(ESP8266)
    disconnectedEventHandler = WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected &event) {
        Serial.print("Station disconnected, reason: ");
        Serial.println(event.reason);
    });
#elif defined(ESP32)
    WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
        if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
            Serial.print("Station disconnected, reason: ");
            Serial.println(info.wifi_sta_disconnected.reason);
        }
    }, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
#endif

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        //Serial.println("sending web page");
        if (ESP.getFreeHeap() > 10000) {
            AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", webui_html, webui_html_len);
            response->addHeader("Content-Encoding", "gzip");
            request->send(response);
        }
    });

    server.on("/sc", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (ESP.getFreeHeap() > 10000) {
            int params = request->params();
            //Serial.print("got serial request params: ");
            //Serial.println(params);
            if (params > 0) {
                const AsyncWebParameter *p = request->getParam(0);
                //Serial.println(p->name());
                serialCommand = p->name().charAt(0);

                // hack, problem with '+' command received via url param
                if (serialCommand == ' ') {
                    serialCommand = '+';
                }
            }
            request->send(200); // reply
        }
    });

    server.on("/uc", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (ESP.getFreeHeap() > 10000) {
            int params = request->params();
            //Serial.print("got user request params: ");
            //Serial.println(params);
            if (params > 0) {
                const AsyncWebParameter *p = request->getParam(0);
                //Serial.println(p->name());
                userCommand = p->name().charAt(0);
            }
            request->send(200); // reply
        }
    });

    server.on("/wifi/connect", HTTP_POST, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response =
            request->beginResponse(200, "application/json", "true");
        request->send(response);

        if (request->arg("n").length()) {     // n holds ssid
            if (request->arg("p").length()) { // p holds password
                // false = only save credentials, don't connect
                WiFi.begin(request->arg("n").c_str(), request->arg("p").c_str(), 0, 0, false);
            } else {
                WiFi.begin(request->arg("n").c_str(), emptyString, 0, 0, false);
            }
        } else {
            WiFi.begin();
        }

        userCommand = 'u'; // next loop, set wifi station mode and restart device
    });

    server.on("/bin/slots.bin", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (ESP.getFreeHeap() > 10000) {
            SlotMetaArray slotsObject;
            File slotsBinaryFileRead = SPIFFS.open(SLOTS_FILE, "r");

            if (!slotsBinaryFileRead) {
                File slotsBinaryFileWrite = SPIFFS.open(SLOTS_FILE, "w");
                for (int i = 0; i < SLOTS_TOTAL; i++) {
                    slotsObject.slot[i].slot = i;
                    slotsObject.slot[i].presetID = 0;
                    slotsObject.slot[i].scanlines = 0;
                    slotsObject.slot[i].scanlinesStrength = 0;
                    slotsObject.slot[i].wantVdsLineFilter = false;
                    slotsObject.slot[i].wantStepResponse = true;
                    slotsObject.slot[i].wantPeaking = true;
                    char emptySlotName[25] = "Empty                   ";
                    strncpy(slotsObject.slot[i].name, emptySlotName, 25);
                }
                slotsBinaryFileWrite.write((byte *)&slotsObject, sizeof(slotsObject));
                slotsBinaryFileWrite.close();
            } else {
                slotsBinaryFileRead.close();
            }

            request->send(SPIFFS, "/slots.bin", "application/octet-stream");
        }
    });

    server.on("/slot/set", HTTP_GET, [](AsyncWebServerRequest *request) {
        bool result = false;

        if (ESP.getFreeHeap() > 10000) {
            int params = request->params();

            if (params > 0) {
                const AsyncWebParameter *slotParam = request->getParam(0);
                String slotParamValue = slotParam->value();
                char slotValue[2];
                slotParamValue.toCharArray(slotValue, sizeof(slotValue));
                uopt->presetSlot = (uint8_t)slotValue[0];
                uopt->presetPreference = OutputCustomized;
                saveUserPrefs();
                result = true;
            }
        }

        request->send(200, "application/json", result ? "true" : "false");
    });

    server.on("/slot/save", HTTP_GET, [](AsyncWebServerRequest *request) {
        bool result = false;

        if (ESP.getFreeHeap() > 10000) {
            int params = request->params();

            if (params > 0) {
                SlotMetaArray slotsObject;
                File slotsBinaryFileRead = SPIFFS.open(SLOTS_FILE, "r");

                if (slotsBinaryFileRead) {
                    slotsBinaryFileRead.read((byte *)&slotsObject, sizeof(slotsObject));
                    slotsBinaryFileRead.close();
                } else {
                    File slotsBinaryFileWrite = SPIFFS.open(SLOTS_FILE, "w");

                    for (int i = 0; i < SLOTS_TOTAL; i++) {
                        slotsObject.slot[i].slot = i;
                        slotsObject.slot[i].presetID = 0;
                        slotsObject.slot[i].scanlines = 0;
                        slotsObject.slot[i].scanlinesStrength = 0;
                        slotsObject.slot[i].wantVdsLineFilter = false;
                        slotsObject.slot[i].wantStepResponse = true;
                        slotsObject.slot[i].wantPeaking = true;
                        char emptySlotName[25] = "Empty                   ";
                        strncpy(slotsObject.slot[i].name, emptySlotName, 25);
                    }

                    slotsBinaryFileWrite.write((byte *)&slotsObject, sizeof(slotsObject));
                    slotsBinaryFileWrite.close();
                }

                // index param
                const AsyncWebParameter *slotIndexParam = request->getParam(0);
                String slotIndexString = slotIndexParam->value();
                uint8_t slotIndex = lowByte(slotIndexString.toInt());
                if (slotIndex >= SLOTS_TOTAL) {
                    goto fail;
                }

                // name param
                const AsyncWebParameter *slotNameParam = request->getParam(1);
                String slotName = slotNameParam->value();

                char emptySlotName[25] = "                        ";
                strncpy(slotsObject.slot[slotIndex].name, emptySlotName, 25);

                slotsObject.slot[slotIndex].slot = slotIndex;
                slotName.toCharArray(slotsObject.slot[slotIndex].name, sizeof(slotsObject.slot[slotIndex].name));
                slotsObject.slot[slotIndex].presetID = rto->presetID;
                slotsObject.slot[slotIndex].scanlines = uopt->wantScanlines;
                slotsObject.slot[slotIndex].scanlinesStrength = uopt->scanlineStrength;
                slotsObject.slot[slotIndex].wantVdsLineFilter = uopt->wantVdsLineFilter;
                slotsObject.slot[slotIndex].wantStepResponse = uopt->wantStepResponse;
                slotsObject.slot[slotIndex].wantPeaking = uopt->wantPeaking;

                File slotsBinaryOutputFile = SPIFFS.open(SLOTS_FILE, "w");
                slotsBinaryOutputFile.write((byte *)&slotsObject, sizeof(slotsObject));
                slotsBinaryOutputFile.close();

                result = true;
            }
        }

        fail:
        request->send(200, "application/json", result ? "true" : "false");
    });

    server.on("/slot/remove", HTTP_GET, [](AsyncWebServerRequest *request) {
        bool result = false;
        int params = request->params();
        const AsyncWebParameter *p = request->getParam(0);
        char param = p->name().charAt(0);
        if (params > 0)
        {
            if (param == '0')
            {
                SerialM.println("Wait...");
                result = true;
            }
            else
            {
                Ascii8 slot = uopt->presetSlot;
                Ascii8 nextSlot;
                auto currentSlot = slotIndexMap.indexOf(slot);

                SlotMetaArray slotsObject;
                File slotsBinaryFileRead = SPIFFS.open(SLOTS_FILE, "r");
                slotsBinaryFileRead.read((byte *)&slotsObject, sizeof(slotsObject));
                slotsBinaryFileRead.close();
                String slotName = slotsObject.slot[currentSlot].name;

                // remove preset files
                SPIFFS.remove("/preset_ntsc." + String((char)slot));
                SPIFFS.remove("/preset_pal." + String((char)slot));
                SPIFFS.remove("/preset_ntsc_480p." + String((char)slot));
                SPIFFS.remove("/preset_pal_576p." + String((char)slot));
                SPIFFS.remove("/preset_ntsc_720p." + String((char)slot));
                SPIFFS.remove("/preset_ntsc_1080p." + String((char)slot));
                SPIFFS.remove("/preset_medium_res." + String((char)slot));
                SPIFFS.remove("/preset_vga_upscale." + String((char)slot));
                SPIFFS.remove("/preset_unknown." + String((char)slot));

                uint8_t loopCount = 0;
                uint8_t flag = 1;
                while (flag != 0)
                {
                    slot = slotIndexMap[currentSlot + loopCount];
                    nextSlot = slotIndexMap[currentSlot + loopCount + 1];
                    flag = 0;
                    flag += SPIFFS.rename("/preset_ntsc." + String((char)(nextSlot)), "/preset_ntsc." + String((char)slot));
                    flag += SPIFFS.rename("/preset_pal." + String((char)(nextSlot)), "/preset_pal." + String((char)slot));
                    flag += SPIFFS.rename("/preset_ntsc_480p." + String((char)(nextSlot)), "/preset_ntsc_480p." + String((char)slot));
                    flag += SPIFFS.rename("/preset_pal_576p." + String((char)(nextSlot)), "/preset_pal_576p." + String((char)slot));
                    flag += SPIFFS.rename("/preset_ntsc_720p." + String((char)(nextSlot)), "/preset_ntsc_720p." + String((char)slot));
                    flag += SPIFFS.rename("/preset_ntsc_1080p." + String((char)(nextSlot)), "/preset_ntsc_1080p." + String((char)slot));
                    flag += SPIFFS.rename("/preset_medium_res." + String((char)(nextSlot)), "/preset_medium_res." + String((char)slot));
                    flag += SPIFFS.rename("/preset_vga_upscale." + String((char)(nextSlot)), "/preset_vga_upscale." + String((char)slot));
                    flag += SPIFFS.rename("/preset_unknown." + String((char)(nextSlot)), "/preset_unknown." + String((char)slot));

                    slotsObject.slot[currentSlot + loopCount].slot = slotsObject.slot[currentSlot + loopCount + 1].slot;
                    slotsObject.slot[currentSlot + loopCount].presetID = slotsObject.slot[currentSlot + loopCount + 1].presetID;
                    slotsObject.slot[currentSlot + loopCount].scanlines = slotsObject.slot[currentSlot + loopCount + 1].scanlines;
                    slotsObject.slot[currentSlot + loopCount].scanlinesStrength = slotsObject.slot[currentSlot + loopCount + 1].scanlinesStrength;
                    slotsObject.slot[currentSlot + loopCount].wantVdsLineFilter = slotsObject.slot[currentSlot + loopCount + 1].wantVdsLineFilter;
                    slotsObject.slot[currentSlot + loopCount].wantStepResponse = slotsObject.slot[currentSlot + loopCount + 1].wantStepResponse;
                    slotsObject.slot[currentSlot + loopCount].wantPeaking = slotsObject.slot[currentSlot + loopCount + 1].wantPeaking;
                    // slotsObject.slot[currentSlot + loopCount].name = slotsObject.slot[currentSlot + loopCount + 1].name;
                    strncpy(slotsObject.slot[currentSlot + loopCount].name, slotsObject.slot[currentSlot + loopCount + 1].name, 25);
                    loopCount++;
                }

                File slotsBinaryFileWrite = SPIFFS.open(SLOTS_FILE, "w");
                slotsBinaryFileWrite.write((byte *)&slotsObject, sizeof(slotsObject));
                slotsBinaryFileWrite.close();
                SerialM.println("Preset \"" + slotName + "\" removed");
                result = true;
            }
        }

        request->send(200, "application/json", result ? "true" : "false");
    });

    server.on("/spiffs/upload", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "application/json", "true");
    });

    server.on(
        "/spiffs/upload", HTTP_POST,
        [](AsyncWebServerRequest *request) { request->send(200, "application/json", "true"); },
        [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
            if (!index) {
                request->_tempFile = SPIFFS.open("/" + filename, "w");
            }
            if (len) {
                request->_tempFile.write(data, len);
            }
            if (final) {
                request->_tempFile.close();
            }
        });

    server.on("/spiffs/download", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (ESP.getFreeHeap() > 10000) {
            int params = request->params();
            if (params > 0) {
                request->send(SPIFFS, request->getParam(0)->value(), String(), true);
            } else {
                request->send(200, "application/json", "false");
            }
        } else {
            request->send(200, "application/json", "false");
        }
    });

    server.on("/spiffs/dir", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (ESP.getFreeHeap() > 10000) {
            Dir dir = SPIFFS.openDir("/");
            String output = "[";

            while (dir.next()) {
                output += "\"";
                output += dir.fileName();
                output += "\",";
                delay(1); // wifi stack
            }

            output += "]";

            output.replace(",]", "]");

            request->send(200, "application/json", output);
            return;
        }
        request->send(200, "application/json", "false");
    });

    server.on("/spiffs/format", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "application/json", SPIFFS.format() ? "true" : "false");
    });

    server.on("/wifi/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        WiFiMode_t wifiMode = WiFi.getMode();
        request->send(200, "application/json", wifiMode == WIFI_AP ? "{\"mode\":\"ap\"}" : "{\"mode\":\"sta\",\"ssid\":\"" + WiFi.SSID() + "\"}");
    });

    server.on("/gbs/restore-filters", HTTP_GET, [](AsyncWebServerRequest *request) {
        SlotMetaArray slotsObject;
        File slotsBinaryFileRead = SPIFFS.open(SLOTS_FILE, "r");
        bool result = false;
        if (slotsBinaryFileRead) {
            slotsBinaryFileRead.read((byte *)&slotsObject, sizeof(slotsObject));
            slotsBinaryFileRead.close();
            auto currentSlot = slotIndexMap.indexOf(uopt->presetSlot);
            if (currentSlot == -1) {
                goto fail;
            }

            uopt->wantScanlines = slotsObject.slot[currentSlot].scanlines;

            SerialM.print(F("slot: "));
            SerialM.println(uopt->presetSlot);
            SerialM.print(F("scanlines: "));
            if (uopt->wantScanlines) {
                SerialM.println(F("on (Line Filter recommended)"));
            } else {
                disableScanlines();
                SerialM.println("off");
            }
            saveUserPrefs();

            uopt->scanlineStrength = slotsObject.slot[currentSlot].scanlinesStrength;
            uopt->wantVdsLineFilter = slotsObject.slot[currentSlot].wantVdsLineFilter;
            uopt->wantStepResponse = slotsObject.slot[currentSlot].wantStepResponse;
            uopt->wantPeaking = slotsObject.slot[currentSlot].wantPeaking;
            result = true;
        }

        fail:
        request->send(200, "application/json", result ? "true" : "false");
    });

    //webSocket.onEvent(webSocketEvent);

    persWM.setConnectNonBlock(true);
    if (WiFi.SSID().length() == 0) {
        // no stored network to connect to > start AP mode right away
        persWM.setupWiFiHandlers();
        persWM.startApMode();
    } else {
        persWM.begin(); // first try connecting to stored network, go AP mode after timeout
    }

    server.begin();    // Webserver for the site
    webSocket.begin(); // Websocket for interaction
    yield();
}

void initUpdateOTA()
{
#if GBS_ENABLE_OTA
    ArduinoOTA.setHostname("GBS OTA");

    // ArduinoOTA.setPassword("admin");
    // Password can be set with it's md5 value as well
    // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
    //ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

    // update: no password is as (in)secure as this publicly stated hash..
    // rely on the user having to enable the OTA feature on the web ui

    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
            type = "sketch";
        else // U_SPIFFS
            type = "filesystem";

        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        SPIFFS.end();
        SerialM.println("Start updating " + type);
    });
    ArduinoOTA.onEnd([]() {
        SerialM.println("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        SerialM.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        SerialM.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR)
            SerialM.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR)
            SerialM.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR)
            SerialM.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR)
            SerialM.println("Receive Failed");
        else if (error == OTA_END_ERROR)
            SerialM.println("End Failed");
    });
    ArduinoOTA.begin();
    yield();
#endif // GBS_ENABLE_OTA
}
#endif // GBS_ENABLE_WEB_GUI
