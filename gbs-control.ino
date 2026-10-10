#include "config.h"

#include "ntsc_240p.h"
#include "pal_240p.h"
#include "ntsc_720x480.h"
#include "pal_768x576.h"
#include "ntsc_1280x720.h"
#include "ntsc_1280x1024.h"
#include "ntsc_1920x1080.h"
#include "ntsc_downscale.h"
#include "pal_1280x720.h"
#include "pal_1280x1024.h"
#include "pal_1920x1080.h"
#include "pal_downscale.h"
#include "presetMdSection.h"
#include "presetDeinterlacerSection.h"
#include "presetHdBypassSection.h"
#include "ofw_RGBS.h"
#include "options.h"
#include "slot.h"

#include <Wire.h>
#include "tv5725.h"
#include "osd.h"
#include "SSD1306Wire.h"
#include "images.h"

static inline void writeBytes(uint8_t slaveRegister, uint8_t *values, uint8_t numValues);
const uint8_t *loadPresetFromSPIFFS(byte forVideoMode);

// pins and addresses: see config.h
SSD1306Wire display(OLED_I2C_ADDRESS, OLED_PIN_SDA, OLED_PIN_SCL);
const int pin_clk = ENCODER_PIN_CLK;
const int pin_data = ENCODER_PIN_DATA;
const int pin_switch = ENCODER_PIN_SWITCH;

#if USE_NEW_OLED_MENU
#include "OLEDMenuImplementation.h"
#include "OSDManager.h"
OLEDMenuManager oledMenu(&display);
OSDManager osdManager;
volatile OLEDMenuNav oledNav = OLEDMenuNav::IDLE;
volatile uint8_t rotaryIsrID = 0;
#else
String oled_menu[4] = {"Resolutions", "Presets", "Misc.", "Current Settings"};
String oled_Resolutions[7] = {"1280x960", "1280x1024", "1280x720", "1920x1080", "480/576", "Downscale", "Pass-Through"};
String oled_Presets[8] = {"1", "2", "3", "4", "5", "6", "7", "Back"};
String oled_Misc[4] = {"Reset GBS", "Restore Factory", "-----Back"};

int oled_menuItem = 1;
int oled_subsetFrame = 0;
int oled_selectOption = 0;
int oled_page = 0;

int oled_lastCount = 0;
volatile int oled_encoder_pos = 0;
volatile int oled_main_pointer = 0; // volatile vars change done with ISR
volatile int oled_pointer_count = 0;
volatile int oled_sub_pointer = 0;
#endif
// chip specific includes and helpers (wifi, async tcp / web server, mdns, file system): see platform.h
// ESPAsyncTCP / AsyncTCP and ESPAsyncWebServer: maintained versions by ESP32Async (see platformio.ini)
#include "platform.h"
#if ENABLE_WIFI
#include <DNSServer.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

// PersWiFiManager library by Ryan Downing
// https://github.com/r-downing/PersWiFiManager
// included in project root folder to allow modifications within limitations of the Arduino framework
// See 3rdparty/PersWiFiManager for unmodified source and license
#include "PersWiFiManager.h"

// WebSockets library by Markus Sattler
// https://github.com/Links2004/arduinoWebSockets
// included in src folder to allow header modifications within limitations of the Arduino framework
// See 3rdparty/WebSockets for unmodified source and license
#include "src/WebSockets.h"
#include "src/WebSocketsServer.h"
#endif

// Optional:
// ESP8266-ping library to aid debugging WiFi issues, install via Arduino library manager
//#define HAVE_PINGER_LIBRARY
#ifdef HAVE_PINGER_LIBRARY
#include <Pinger.h>
#include <PingerResponse.h>
unsigned long pingLastTime;
Pinger pinger; // pinger global object to aid debugging WiFi issues
#endif

#if !ENABLE_WIFI && !USE_NEW_OLED_MENU
#error "ENABLE_WIFI=0 requires USE_NEW_OLED_MENU=1 (the legacy OLED menu uses the web socket)."
#endif
#if !ENABLE_WIFI && defined(HAVE_PINGER_LIBRARY)
#error "ENABLE_WIFI=0 is incompatible with HAVE_PINGER_LIBRARY (ping needs WiFi)."
#endif

typedef TV5725<GBS_ADDR> GBS;

static unsigned long lastVsyncLock = millis();

// Si5351mcu library by Pavel Milanes
// https://github.com/pavelmc/Si5351mcu
// included in project root folder to allow modifications within limitations of the Arduino framework
// See 3rdparty/Si5351mcu for unmodified source and license
#include "src/si5351mcu.h"
Si5351mcu Si;

// device name and access point: see config.h (DEVICE_HOSTNAME, AP_SSID, AP_PASSWORD)
#define GBS_STR_(x) #x
#define GBS_STR(x) GBS_STR_(x)
const char *ap_ssid = AP_SSID;
const char *ap_password = AP_PASSWORD;
// only use _partial throughout, comply to standards
const char *device_hostname_full = DEVICE_HOSTNAME ".local";
const char *device_hostname_partial = DEVICE_HOSTNAME; // for MDNS
//
static const char ap_info_string[] PROGMEM =
    "(WiFi): AP mode (SSID: " AP_SSID ", pass '" AP_PASSWORD "'): Access '" DEVICE_HOSTNAME ".local' in your browser";
static const char st_info_string[] PROGMEM =
    "(WiFi): Access 'http://" DEVICE_HOSTNAME ":" GBS_STR(WEB_SERVER_PORT) "' or 'http://" DEVICE_HOSTNAME ".local' (or device IP) in your browser";

#if ENABLE_WIFI
AsyncWebServer server(WEB_SERVER_PORT);
DNSServer dnsServer;
WebSocketsServer webSocket(WEBSOCKET_PORT);
//AsyncWebSocket webSocket("/ws");
PersWiFiManager persWM(server, dnsServer);
#endif

// DEBUG_IN_PIN: see config.h
// SCL = D1 (Lolin), D15 (Wemos D1) // ESP8266 Arduino default map: SCL
// SDA = D2 (Lolin), D14 (Wemos D1) // ESP8266 Arduino default map: SDA
#define LEDON                     \
    pinMode(LED_BUILTIN, OUTPUT); \
    digitalWrite(LED_BUILTIN, LOW)
#define LEDOFF                       \
    digitalWrite(LED_BUILTIN, HIGH); \
    pinMode(LED_BUILTIN, INPUT)

// fast register based digitalRead (see platform.h); defined here so library headers above keep the normal one
#define digitalRead(x) GBS_FAST_DIGITAL_READ(x)

struct MenuAttrs
{
    static const int8_t shiftDelta = 4;
    static const int8_t scaleDelta = 4;
    static const int16_t vertShiftRange = 300;
    static const int16_t horizShiftRange = 400;
    static const int16_t vertScaleRange = 100;
    static const int16_t horizScaleRange = 130;
    static const int16_t barLength = 100;
};
typedef MenuManager<GBS, MenuAttrs> Menu;

/// Video processing mode, loaded into register GBS_PRESET_ID by applyPresets()
/// and read to rto->presetID by doPostPresetLoadSteps(). Shown on web UI.
enum PresetID : uint8_t {
    PresetHdBypass = 0x21,
    PresetBypassRGBHV = 0x22,
};
struct runTimeOptions rtos;
struct runTimeOptions *rto = &rtos;
struct userOptions uopts;
struct userOptions *uopt = &uopts;
struct adcOptions adcopts;
struct adcOptions *adco = &adcopts;

String slotIndexMap = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~()!*:,";

char serialCommand;               // Serial / Web Server commands
char userCommand;               // Serial / Web Server commands
static uint8_t lastSegment = 0xFF; // GBS segment for direct access

#if ENABLE_WIFI
// Commands from the web ui (http /sc, /uc or websocket) are queued here, so a command that
// arrives while loop() is busy (preset change, sync watcher, ...) is no longer overwritten or lost.
// Producer: wifi / async tcp callbacks, consumer: loop(). Both run in the same (non preemptive)
// context on ESP8266, so a plain ring buffer is enough.
struct WebCommand
{
    char type; // 's' = serialCommand, 'u' = userCommand
    char cmd;
};
static WebCommand webCmdQueue[WEB_CMD_QUEUE_LEN];
static volatile uint8_t webCmdHead = 0;
static volatile uint8_t webCmdTail = 0;

static void queueWebCommand(char type, char cmd)
{
    uint8_t next = (webCmdHead + 1) % WEB_CMD_QUEUE_LEN;
    if (next == webCmdTail) {
        return; // full, drop (same as the old behaviour of a lost command)
    }
    webCmdQueue[webCmdHead].type = type;
    webCmdQueue[webCmdHead].cmd = cmd;
    webCmdHead = next;
}

// number of identical commands still waiting in the queue (used by /api/set for toggles)
static uint8_t countQueuedWebCommand(char type, char cmd)
{
    uint8_t n = 0;
    for (uint8_t i = webCmdTail; i != webCmdHead; i = (i + 1) % WEB_CMD_QUEUE_LEN) {
        if (webCmdQueue[i].type == type && webCmdQueue[i].cmd == cmd) {
            n++;
        }
    }
    return n;
}

// hand the next queued command to loop(), only when no other command is pending
static void dequeueWebCommand()
{
    if (serialCommand != '@' || userCommand != '@' || webCmdTail == webCmdHead) {
        return;
    }
    WebCommand c = webCmdQueue[webCmdTail];
    webCmdTail = (webCmdTail + 1) % WEB_CMD_QUEUE_LEN;
    if (c.type == 's') {
        serialCommand = c.cmd;
    } else {
        userCommand = c.cmd;
    }
}
#endif
//uint8_t globalDelay; // used for dev / debug

#if (defined(ESP8266) || defined(ESP32)) && ENABLE_WIFI
// serial mirror class for websocket logs
// Output for the web ui is collected in a small buffer and sent as one websocket frame
// from handleWiFi() (or when the buffer is full), instead of one frame per print() call.
// Sending happens only at points where the wifi stack was serviced before, so no new
// work is added inside timing critical code.
class SerialMirror : public Stream
{
public:
    static constexpr size_t BUF_LEN = WEB_LOG_BUFFER_SIZE;
    static constexpr unsigned long FLUSH_INTERVAL_MS = WEB_LOG_FLUSH_INTERVAL_MS;

    size_t write(const uint8_t *data, size_t size)
    {
        bufferForWeb(data, size);
        Serial.write(data, size);
        return size;
    }

    size_t write(const char *data, size_t size)
    {
        return write((const uint8_t *)data, size);
    }

    size_t write(uint8_t data)
    {
        bufferForWeb(&data, 1);
        Serial.write(data);
        return 1;
    }

    size_t write(char data)
    {
        return write((uint8_t)data);
    }

    int available()
    {
        return 0;
    }
    int read()
    {
        return -1;
    }
    int peek()
    {
        return -1;
    }
    void flush()
    {
        sendBuffered();
    }

    // called regularly from handleWiFi()
    void flushIfDue()
    {
        if (len > 0 && (millis() - lastFlush) >= FLUSH_INTERVAL_MS) {
            sendBuffered();
        }
    }

private:
    uint8_t buf[BUF_LEN];
    size_t len = 0;
    unsigned long lastFlush = 0;

    void bufferForWeb(const uint8_t *data, size_t size)
    {
        if (webSocket.connectedClients(false) == 0) {
            len = 0; // nobody listening, don't spend time on it
            return;
        }
        while (size > 0) {
            size_t chunk = BUF_LEN - len;
            if (chunk > size) {
                chunk = size;
            }
            memcpy(buf + len, data, chunk);
            len += chunk;
            data += chunk;
            size -= chunk;
            if (len == BUF_LEN) {
                sendBuffered();
            }
        }
    }

    void sendBuffered()
    {
        if (len > 0) {
            if (ESP.getFreeHeap() > 20000) {
                webSocket.broadcastTXT(buf, len);
            } else {
                webSocket.disconnect();
            }
            len = 0;
        }
        lastFlush = millis();
    }
};

SerialMirror SerialM;
#else
#define SerialM Serial
#endif

#include "gbs_prototypes.h"
#include "framesync.h"

//
// Sync locking tunables/magic numbers
//
struct FrameSyncAttrs
{
    static const uint8_t debugInPin = DEBUG_IN_PIN;
    static const uint32_t lockInterval = 100 * 16.70; // every 100 frames
    static const int16_t syncCorrection = 2;          // Sync correction in scanlines to apply when phase lags target
    static const int32_t syncTargetPhase = 90;        // Target vsync phase offset (output trails input) in degrees
                                                      // to debug: syncTargetPhase = 343 lockInterval = 15 * 16
};
typedef FrameSyncManager<GBS, FrameSyncAttrs> FrameSync;

// in operation: t5t04t1 for 10% lower power on ADC
// also: s0s40s1c for 5% (lower memclock of 108mhz)
// for some reason: t0t45t2 t0t45t4 (enable SDAC, output max voltage) 5% lower  done in presets
// t0t4ft4 clock out should be off
// s4s01s20 (was 30) faster latency // unstable at 108mhz
// both phase controls off saves some power 506ma > 493ma
// oversample ratio can save 10% at 1x
// t3t24t3 VDS_TAP6_BYPS on can save 2%

// GBS boards have 2 potential sync sources:
// - RCA connectors
// - VGA input / 5 pin RGBS header / 8 pin VGA header (all 3 are shared electrically)
// This routine looks for sync on the currently active input. If it finds it, the input is returned.
// If it doesn't find sync, it switches the input and returns 0, so that an active input will be found eventually.
uint8_t detectAndSwitchToActiveInput()
{ // if any
    uint8_t currentInput = GBS::ADC_INPUT_SEL::read();
    unsigned long timeout = millis();
    while (millis() - timeout < 450) {
        delay(10);
        handleWiFi(0);

        boolean stable = getStatus16SpHsStable();
        if (stable) {
            currentInput = GBS::ADC_INPUT_SEL::read();
            SerialM.print(F("Activity detected, input: "));
            if (currentInput == 1)
                SerialM.println("RGB");
            else
                SerialM.println(F("Component"));

            if (currentInput == 1) { // RGBS or RGBHV
                boolean vsyncActive = 0;
                rto->inputIsYpBpR = false; // declare for MD
                rto->currentLevelSOG = 13; // test startup with MD and MS separately!
                setAndUpdateSogLevel(rto->currentLevelSOG);

                unsigned long timeOutStart = millis();
                // vsync test
                // 360ms good up to 5_34 SP_V_TIMER_VAL = 0x0b
                while (!vsyncActive && ((millis() - timeOutStart) < 360)) {
                    vsyncActive = GBS::STATUS_SYNC_PROC_VSACT::read();
                    handleWiFi(0); // wifi stack
                    delay(1);
                }

                // if VSync is active, it's RGBHV or RGBHV with CSync on HS pin
                if (vsyncActive) {
                    SerialM.println(F("VSync: present"));
                    GBS::MD_SEL_VGA60::write(1); // VGA 640x480 more likely than EDTV
                    boolean hsyncActive = 0;

                    timeOutStart = millis();
                    while (!hsyncActive && millis() - timeOutStart < 400) {
                        hsyncActive = GBS::STATUS_SYNC_PROC_HSACT::read();
                        handleWiFi(0); // wifi stack
                        delay(1);
                    }

                    if (hsyncActive) {
                        SerialM.print(F("HSync: present"));
                        // The HSync and SOG pins are setup to detect CSync, if present
                        // (SOG mode on, coasting setup, debug bus setup, etc)
                        // SP_H_PROTECT is needed for CSync with a VS source present as well
                        GBS::SP_H_PROTECT::write(1);
                        delay(120);

                        short decodeSuccess = 0;
                        for (int i = 0; i < 3; i++) {
                            // no success if: no signal at all (returns 0.0f), no embedded VSync (returns ~18.5f)
                            // todo: this takes a while with no csync present
                            rto->syncTypeCsync = 1; // temporary for test
                            float sfr = getSourceFieldRate(1);
                            rto->syncTypeCsync = 0; // undo
                            if (sfr > 40.0f)
                                decodeSuccess++; // properly decoded vsync from 40 to xx Hz
                        }

                        if (decodeSuccess >= 2) {
                            SerialM.println(F(" (with CSync)"));
                            GBS::SP_PRE_COAST::write(0x10); // increase from 9 to 16 (EGA 364)
                            delay(40);
                            rto->syncTypeCsync = true;
                        } else {
                            SerialM.println();
                            rto->syncTypeCsync = false;
                        }

                        // check for 25khz, all regular SOG modes first // update: only check for mode 8
                        // MD reg for medium res starts at 0x2C and needs 16 loops to ramp to max of 0x3C (vt 360 .. 496)
                        // if source is HS+VS, can't detect via MD unit, need to set 5_11=0x92 and look at vt: counter
                        for (uint8_t i = 0; i < 16; i++) {
                            //printInfo();
                            uint8_t innerVideoMode = getVideoMode();
                            if (innerVideoMode == 8) {
                                setAndUpdateSogLevel(rto->currentLevelSOG);
                                rto->medResLineCount = GBS::MD_HD1250P_CNTRL::read();
                                SerialM.println(F("med res"));

                                return 1;
                            }
                            // update 25khz detection
                            GBS::MD_HD1250P_CNTRL::write(GBS::MD_HD1250P_CNTRL::read() + 1);
                            //Serial.println(GBS::MD_HD1250P_CNTRL::read(), HEX);
                            delay(30);
                        }

                        rto->videoStandardInput = 15;
                        // exception: apply preset here, not later in syncwatcher
                        applyPresets(rto->videoStandardInput);
                        delay(100);

                        return 3;
                    } else {
                        // need to continue looking
                        SerialM.println(F("but no HSync!"));
                    }
                }

                if (!vsyncActive) { // then do RGBS check
                    rto->syncTypeCsync = true;
                    GBS::MD_SEL_VGA60::write(0); // EDTV60 more likely than VGA60
                    uint16_t testCycle = 0;
                    timeOutStart = millis();
                    while ((millis() - timeOutStart) < 6000) {
                        delay(2);
                        if (getVideoMode() > 0) {
                            if (getVideoMode() != 8) { // if it's mode 8, need to set stuff first
                                return 1;
                            }
                        }
                        testCycle++;
                        // post coast 18 can mislead occasionally (SNES 239 mode)
                        // but even then it still detects the video mode pretty well
                        if ((testCycle % 150) == 0) {
                            if (rto->currentLevelSOG == 1) {
                                rto->currentLevelSOG = 2;
                            } else {
                                rto->currentLevelSOG += 2;
                            }
                            if (rto->currentLevelSOG >= 15) {
                                rto->currentLevelSOG = 1;
                            }
                            setAndUpdateSogLevel(rto->currentLevelSOG);
                        }

                        // new: check for 25khz, use regular scaling route for those
                        if (getVideoMode() == 8) {
                            rto->currentLevelSOG = rto->thisSourceMaxLevelSOG = 13;
                            setAndUpdateSogLevel(rto->currentLevelSOG);
                            rto->medResLineCount = GBS::MD_HD1250P_CNTRL::read();
                            SerialM.println(F("med res"));
                            return 1;
                        }

                        uint8_t currentMedResLineCount = GBS::MD_HD1250P_CNTRL::read();
                        if (currentMedResLineCount < 0x3c) {
                            GBS::MD_HD1250P_CNTRL::write(currentMedResLineCount + 1);
                        } else {
                            GBS::MD_HD1250P_CNTRL::write(0x33);
                        }
                        //Serial.println(GBS::MD_HD1250P_CNTRL::read(), HEX);
                    }

                    //rto->currentLevelSOG = rto->thisSourceMaxLevelSOG = 13;
                    //setAndUpdateSogLevel(rto->currentLevelSOG);

                    return 1; //anyway, let later stage deal with it
                }

                GBS::SP_SOG_MODE::write(1);
                resetSyncProcessor();
                resetModeDetect(); // there was some signal but we lost it. MD is stuck anyway, so reset
                delay(40);
            } else if (currentInput == 0) { // YUV
                uint16_t testCycle = 0;
                rto->inputIsYpBpR = true;    // declare for MD
                GBS::MD_SEL_VGA60::write(0); // EDTV more likely than VGA 640x480

                unsigned long timeOutStart = millis();
                while ((millis() - timeOutStart) < 6000) {
                    delay(2);
                    if (getVideoMode() > 0) {
                        return 2;
                    }

                    testCycle++;
                    if ((testCycle % 180) == 0) {
                        if (rto->currentLevelSOG == 1) {
                            rto->currentLevelSOG = 2;
                        } else {
                            rto->currentLevelSOG += 2;
                        }
                        if (rto->currentLevelSOG >= 16) {
                            rto->currentLevelSOG = 1;
                        }
                        setAndUpdateSogLevel(rto->currentLevelSOG);
                        rto->thisSourceMaxLevelSOG = rto->currentLevelSOG;
                    }
                }

                rto->currentLevelSOG = rto->thisSourceMaxLevelSOG = 14;
                setAndUpdateSogLevel(rto->currentLevelSOG);

                return 2; //anyway, let later stage deal with it
            }

            SerialM.println(" lost..");
            rto->currentLevelSOG = 2;
            setAndUpdateSogLevel(rto->currentLevelSOG);
        }

        GBS::ADC_INPUT_SEL::write(!currentInput); // can only be 1 or 0
        delay(200);

        return 0; // don't do the check on the new input here, wait till next run
    }

    return 0;
}

#define AUTO_GAIN_INIT 0x48

#if !ENABLE_WIFI
// preinit() runs before setup() and before the SDK starts WiFi, so keep the radio
// off here. Using the raw SDK keeps ESP8266WiFi.h excluded (stray WiFi.* then fails
// to compile). Only static/C calls are allowed in preinit().
extern "C" {
#include "user_interface.h"
}
void preinit()
{
    wifi_set_opmode_current(NULL_MODE); // radio off for this boot; not persisted
}
#endif

//RF_PRE_INIT() {
//  system_phy_set_powerup_option(3);  // full RFCAL at boot
//}

//void preinit() {
//  //system_phy_set_powerup_option(3); // 0 = default, use init byte; 3 = full calibr. each boot, extra 200ms
//  system_phy_set_powerup_option(0);
//}

#if !USE_NEW_OLED_MENU
#endif

#if USE_NEW_OLED_MENU
#endif

void setup()
{
    display.init();                 //inits OLED on I2C bus
    display.flipScreenVertically(); //orientation fix for OLED

    pinMode(pin_clk, INPUT_PULLUP);
    pinMode(pin_data, INPUT_PULLUP);
    pinMode(pin_switch, INPUT_PULLUP);

#if USE_NEW_OLED_MENU
    attachInterrupt(digitalPinToInterrupt(pin_clk), isrRotaryEncoderRotateForNewMenu, FALLING);
    attachInterrupt(digitalPinToInterrupt(pin_switch), isrRotaryEncoderPushForNewMenu, FALLING);
    initOLEDMenu();
    initOSD();
#else
    // ISR TO PIN
    attachInterrupt(digitalPinToInterrupt(pin_clk), isrRotaryEncoder, FALLING);
#endif

#if ENABLE_WIFI
    rto->webServerEnabled = true;
#else
    rto->webServerEnabled = false;
#endif
    rto->webServerStarted = false; // make sure this is set

    Serial.begin(SERIAL_BAUD); // serial monitor must use the same speed (platformio.ini: monitor_speed)
    Serial.setTimeout(10);

    // millis() at this point: typically 65ms
    // start web services as early in boot as possible
#if ENABLE_WIFI
    gbsWifiSetHostname(device_hostname_partial); // was _full
#endif

    startWire();
    // run some dummy commands to init I2C to GBS and cached segments
    GBS::SP_SOG_MODE::read();
    writeOneByte(0xF0, 0);
    writeOneByte(0x00, 0);
    GBS::STATUS_00::read();

#if ENABLE_WIFI
    if (rto->webServerEnabled) {
        rto->allowUpdatesOTA = false;       // need to initialize for handleWiFi()
        gbsWifiLowLatency(WIFI_OUTPUT_POWER); // low latency responses; float dBm: min 0.0f, max 20.5f
        startWebserver();
        rto->webServerStarted = true;
    } else {
        //WiFi.disconnect(); // deletes credentials
        gbsWifiOff();
    }
#endif
#ifdef HAVE_PINGER_LIBRARY
    pingLastTime = millis();
#endif

    SerialM.println(F("\nstartup"));
    SerialM.print(F("reset reason: "));
    SerialM.println(gbsResetReason()); // e.g. after a crash or watchdog reset

    loadDefaultUserOptions();
    //globalDelay = 0;

    rto->allowUpdatesOTA = false; // ESP over the air updates. default to off, enable via web interface
    rto->enableDebugPings = false;
    rto->autoBestHtotalEnabled = true; // automatically find the best horizontal total pixel value for a given input timing
    rto->syncLockFailIgnore = 16;      // allow syncLock to fail x-1 times in a row before giving up (sync glitch immunity)
    rto->forceRetime = false;
    rto->syncWatcherEnabled = true; // continously checks the current sync status. required for normal operation
    rto->phaseADC = 16;
    rto->phaseSP = 16;
    rto->failRetryAttempts = 0;
    rto->presetID = 0;
    rto->isCustomPreset = false;
    rto->HPLLState = 0;
    rto->motionAdaptiveDeinterlaceActive = false;
    rto->deinterlaceAutoEnabled = true;
    rto->scanlinesEnabled = false;
    rto->boardHasPower = true;
    rto->presetIsPalForce60 = false;
    rto->syncTypeCsync = false;
    rto->isValidForScalingRGBHV = false;
    rto->medResLineCount = 0x33; // 51*8=408
    rto->osr = 0;
    rto->useHdmiSyncFix = 0;
    rto->notRecognizedCounter = 0;

    // more run time variables
    rto->inputIsYpBpR = false;
    rto->videoStandardInput = 0;
    rto->outModeHdBypass = false;
    rto->videoIsFrozen = false;
    if (!rto->webServerEnabled)
        rto->webServerStarted = false;
    rto->printInfos = false;
    rto->sourceDisconnected = true;
    rto->isInLowPowerMode = false;
    rto->applyPresetDoneStage = 0;
    rto->presetVlineShift = 0;
    rto->clampPositionIsSet = 0;
    rto->coastPositionIsSet = 0;
    rto->continousStableCounter = 0;
    rto->currentLevelSOG = 5;
    rto->thisSourceMaxLevelSOG = 31; // 31 = auto sog has not (yet) run

    adco->r_gain = 0;
    adco->g_gain = 0;
    adco->b_gain = 0;
    adco->r_off = 0;
    adco->g_off = 0;
    adco->b_off = 0;

    serialCommand = '@'; // ASCII @ = 0
    userCommand = '@';

    pinMode(DEBUG_IN_PIN, INPUT);
    pinMode(LED_BUILTIN, OUTPUT);
    LEDON; // enable the LED, lets users know the board is starting up

    //Serial.setDebugOutput(true); // if you want simple wifi debug info

    // delay 1 of 2
    unsigned long initDelay = millis();
    // upped from < 500 to < 1500, allows more time for wifi and GBS startup
    while (millis() - initDelay < 1500) {
        display.drawXbm(2, 2, gbsicon_width, gbsicon_height, gbsicon_bits);
        display.display();
        handleWiFi(0);
        delay(1);
    }
    display.clear();
    // if i2c established and chip running, issue software reset now
    GBS::RESET_CONTROL_0x46::write(0);
    GBS::RESET_CONTROL_0x47::write(0);
    GBS::PLLAD_VCORST::write(1);
    GBS::PLLAD_PDZ::write(0); // AD PLL off

    // file system (web page, custom presets, ect)
    if (!gbsFsBegin()) {
        SerialM.println(F("SPIFFS mount failed! ((1M SPIFFS) selected?)"));
    } else {
        // load user preferences file
        File f = GBS_FS.open("/preferencesv2.txt", "r");
        if (!f) {
            SerialM.println(F("no preferences file yet, create new"));
            loadDefaultUserOptions();
            saveUserPrefs(); // if this fails, there must be a spiffs problem
        } else {
            //on a fresh / spiffs not formatted yet MCU:  userprefs.txt open ok //result = 207
            uopt->presetPreference = (PresetPreference)(f.read() - '0'); // #1
            if (uopt->presetPreference > 10)
                uopt->presetPreference = Output960P; // fresh spiffs ?

            uopt->enableFrameTimeLock = (uint8_t)(f.read() - '0');
            if (uopt->enableFrameTimeLock > 1)
                uopt->enableFrameTimeLock = 0;

            uopt->presetSlot = lowByte(f.read());

            uopt->frameTimeLockMethod = (uint8_t)(f.read() - '0');
            if (uopt->frameTimeLockMethod > 1)
                uopt->frameTimeLockMethod = 0;

            uopt->enableAutoGain = (uint8_t)(f.read() - '0');
            if (uopt->enableAutoGain > 1)
                uopt->enableAutoGain = 0;

            uopt->wantScanlines = (uint8_t)(f.read() - '0');
            if (uopt->wantScanlines > 1)
                uopt->wantScanlines = 0;

            uopt->wantOutputComponent = (uint8_t)(f.read() - '0');
            if (uopt->wantOutputComponent > 1)
                uopt->wantOutputComponent = 0;

            uopt->deintMode = (uint8_t)(f.read() - '0');
            if (uopt->deintMode > 2)
                uopt->deintMode = 0;

            uopt->wantVdsLineFilter = (uint8_t)(f.read() - '0');
            if (uopt->wantVdsLineFilter > 1)
                uopt->wantVdsLineFilter = 0;

            uopt->wantPeaking = (uint8_t)(f.read() - '0');
            if (uopt->wantPeaking > 1)
                uopt->wantPeaking = 1;

            uopt->preferScalingRgbhv = (uint8_t)(f.read() - '0');
            if (uopt->preferScalingRgbhv > 1)
                uopt->preferScalingRgbhv = 1;

            uopt->wantTap6 = (uint8_t)(f.read() - '0');
            if (uopt->wantTap6 > 1)
                uopt->wantTap6 = 1;

            uopt->PalForce60 = (uint8_t)(f.read() - '0');
            if (uopt->PalForce60 > 1)
                uopt->PalForce60 = 0;

            uopt->matchPresetSource = (uint8_t)(f.read() - '0'); // #14
            if (uopt->matchPresetSource > 1)
                uopt->matchPresetSource = 1;

            uopt->wantStepResponse = (uint8_t)(f.read() - '0'); // #15
            if (uopt->wantStepResponse > 1)
                uopt->wantStepResponse = 1;

            uopt->wantFullHeight = (uint8_t)(f.read() - '0'); // #16
            if (uopt->wantFullHeight > 1)
                uopt->wantFullHeight = 1;

            uopt->enableCalibrationADC = (uint8_t)(f.read() - '0'); // #17
            if (uopt->enableCalibrationADC > 1)
                uopt->enableCalibrationADC = 1;

            uopt->scanlineStrength = (uint8_t)(f.read() - '0'); // #18
            if (uopt->scanlineStrength > 0x60)
                uopt->enableCalibrationADC = 0x30;

            uopt->disableExternalClockGenerator = (uint8_t)(f.read() - '0'); // #19
            if (uopt->disableExternalClockGenerator > 1)
                uopt->disableExternalClockGenerator = 0;

            f.close();
        }
    }

    GBS::PAD_CKIN_ENZ::write(1); // disable to prevent startup spike damage
    externalClockGenDetectAndInitialize();
    // library may change i2c clock or pins, so restart
    startWire();
    GBS::STATUS_00::read();
    GBS::STATUS_00::read();
    GBS::STATUS_00::read();

    // delay 2 of 2
    initDelay = millis();
    while (millis() - initDelay < 1000) {
        handleWiFi(0);
        delay(1);
    }

#if ENABLE_WIFI
    if (WiFi.status() == WL_CONNECTED) {
        // nothing
    } else if (gbsWifiStoredSsid().length() == 0) {
        SerialM.println(FPSTR(ap_info_string));
    } else {
        SerialM.println(F("(WiFi): still connecting.."));
        WiFi.reconnect(); // only valid for station class (ok here)
    }
#endif

    // dummy commands
    GBS::STATUS_00::read();
    GBS::STATUS_00::read();
    GBS::STATUS_00::read();

    boolean powerOrWireIssue = 0;
    if (!checkBoardPower()) {
        stopWire(); // sets pinmodes SDA, SCL to INPUT
        for (int i = 0; i < 40; i++) {
            // I2C SDA probably stuck, attempt recovery (max attempts in tests was around 10)
            startWire();
            GBS::STATUS_00::read();
            digitalWrite(SCL, 0);
            delayMicroseconds(12);
            stopWire();
            if (digitalRead(SDA) == 1) {
                break;
            } // unstuck
            if ((i % 7) == 0) {
                delay(1);
            }
        }

        // restart and dummy
        startWire();
        delay(1);
        GBS::STATUS_00::read();
        delay(1);

        if (!checkBoardPower()) {
            stopWire();
            powerOrWireIssue = 1; // fail
            rto->boardHasPower = false;
            rto->syncWatcherEnabled = false;
        } else { // recover success
            rto->syncWatcherEnabled = true;
            rto->boardHasPower = true;
            SerialM.println(F("recovered"));
        }
    }

    if (powerOrWireIssue == 0) {
        // second init, in cases where i2c got stuck earlier but has recovered
        // *if* ext clock gen is installed and *if* i2c got stuck, then clockgen must be already running
        if (!rto->extClockGenDetected) {
            externalClockGenDetectAndInitialize();
        }
        if (rto->extClockGenDetected == 1) {
            Serial.println(F("ext clockgen detected"));
        } else {
            Serial.println(F("no ext clockgen"));
        }

        zeroAll();
        setResetParameters();
        prepareSyncProcessor();

        uint8_t productId = GBS::CHIP_ID_PRODUCT::read();
        uint8_t revisionId = GBS::CHIP_ID_REVISION::read();
        SerialM.print(F("Chip ID: "));
        SerialM.print(productId, HEX);
        SerialM.print(" ");
        SerialM.println(revisionId, HEX);

        if (uopt->enableCalibrationADC) {
            // enabled by default
            calibrateAdcOffset();
        }
        setResetParameters();

        delay(4); // help wifi (presets are unloaded now)
        handleWiFi(1);
        delay(4);

        // startup reliability test routine
        /*rto->videoStandardInput = 1;
    writeProgramArrayNew(ntsc_240p, 0);
    doPostPresetLoadSteps();
    int i = 100000;
    while (i-- > 0) loop();
    ESP.restart();*/

        //rto->syncWatcherEnabled = false; // allows passive operation by disabling syncwatcher here
        //inputAndSyncDetect();
        //if (rto->syncWatcherEnabled == true) {
        //  rto->isInLowPowerMode = true; // just for initial detection; simplifies code later
        //  for (uint8_t i = 0; i < 3; i++) {
        //    if (inputAndSyncDetect()) {
        //      break;
        //    }
        //  }
        //  rto->isInLowPowerMode = false;
        //}
    } else {
        SerialM.println(F("Please check board power and cabling or restart!"));
    }

    LEDOFF; // LED behaviour: only light LED on active sync

    // some debug tools leave garbage in the serial rx buffer
    if (Serial.available()) {
        discardSerialRxData();
    }
}

#if HAVE_BUTTONS
#define INPUT_SHIFT 0
#define DOWN_SHIFT 1
#define UP_SHIFT 2
#define MENU_SHIFT 3
#define BACK_SHIFT 4

static const uint8_t historySize = 32;
static const uint16_t buttonPollInterval = 100; // microseconds
static uint8_t buttonHistory[historySize];
static uint8_t buttonIndex;
static uint8_t buttonState;
static uint8_t buttonChanged;

#endif

#if ENABLE_WIFI

#else
void updateWebSocketData() {}
void handleWiFi(boolean) {}
#endif

#ifdef LOOP_DIAG
// diagnostic only (env d1_mini_diag): measures loop / section durations, prints every 3 s
struct LoopDiag {
    uint32_t loops = 0, loopMax = 0, loopSum = 0, ftlCalls = 0, ftlMax = 0, swCalls = 0, swMax = 0, wifiMax = 0;
    uint32_t lastPrint = 0;
    uint32_t firstEdgeMaxUs = 0, firstEdgeOver60 = 0;
    uint32_t sampOk = 0, sampFail = 0, sampFailFirstEdge = 0, sampFailMaxUs = 0, sampFailToggles = 0, sampOkMaxUs = 0;
} loopDiag;
void diagSampleFail(uint32_t us, uint32_t toggles, bool gotFirstEdge)
{
    loopDiag.sampFail++;
    if (gotFirstEdge) loopDiag.sampFailFirstEdge++;
    if (us > loopDiag.sampFailMaxUs) loopDiag.sampFailMaxUs = us;
    loopDiag.sampFailToggles += toggles;
}
void diagSampleOk(uint32_t us, uint32_t firstEdgeUs)
{
    if (firstEdgeUs > loopDiag.firstEdgeMaxUs) loopDiag.firstEdgeMaxUs = firstEdgeUs;
    if (firstEdgeUs > 60000 && firstEdgeUs < 10000000) { // ignore ccount wrap artefacts
        loopDiag.firstEdgeOver60++;
        Serial.printf("[diag] !!! first edge only after %u ms (would fail with 60 ms abort)\n", firstEdgeUs / 1000);
    }
    loopDiag.sampOk++;
    if (us > loopDiag.sampOkMaxUs) loopDiag.sampOkMaxUs = us;
}
#define DIAG_T0(name) uint32_t name = micros()
#define DIAG_MAX(var, t0) { uint32_t d_ = micros() - (t0); if (d_ > loopDiag.var) loopDiag.var = d_; }
#else
#define DIAG_T0(name)
#define DIAG_MAX(var, t0)
#endif

// multi stage serial commands (s/t/g): shared by loop() and handleSerialCommand()
static uint8_t inputStage = 0;

void loop()
{
    static unsigned long lastTimeSyncWatcher = millis();
    static unsigned long lastTimeSourceCheck = 500; // 500 to start right away (after setup it will be 2790ms when we get here)
    static unsigned long lastTimeInterruptClear = millis();

#if HAVE_BUTTONS
    static unsigned long lastButton = micros();
    if (micros() - lastButton > buttonPollInterval) {
        lastButton = micros();
        handleButtons();
    }
#endif

#if USE_NEW_OLED_MENU
    uint8_t oldIsrID = rotaryIsrID;
    // make sure no rotary encoder isr happened while menu was updating.
    // skipping this check will make the rotary encoder not responsive randomly.
    // (oledNav change will be lost if isr happened during menu updating)
    oledMenu.tick(oledNav);
    if (oldIsrID == rotaryIsrID) {
        oledNav = OLEDMenuNav::IDLE;
    }
#else
    settingsMenuOLED();
    if (oled_encoder_pos != oled_lastCount) {
        oled_lastCount = oled_encoder_pos;
    }
#endif

    DIAG_T0(diagLoopStart);
    DIAG_T0(diagWifi);
    handleWiFi(0); // WiFi + OTA + WS + MDNS, checks for server enabled + started
    DIAG_MAX(wifiMax, diagWifi);

    // is there a command from Terminal or web ui?
    // Serial takes precedence
    if (Serial.available()) {
        serialCommand = Serial.read();
    } else if (inputStage > 0) {
        // multistage with no more data
        SerialM.println(F(" abort"));
        discardSerialRxData();
        serialCommand = ' ';
    }
#if ENABLE_WIFI
    if (inputStage == 0) {
        dequeueWebCommand(); // next web ui command, if nothing else is pending
    }
#endif
    if (serialCommand != '@') {
        // multistage with bad characters?
        if (inputStage > 0) {
            // need 's', 't' or 'g'
            if (serialCommand != 's' && serialCommand != 't' && serialCommand != 'g') {
                discardSerialRxData();
                SerialM.println(F(" abort"));
                serialCommand = ' ';
            }
        }
        myLog("serial", serialCommand);

        handleSerialCommand();

        delay(1); // give some time to read in eventual next chars

        // a web ui or terminal command has finished. good idea to reset sync lock timer
        // important if the command was to change presets, possibly others
        lastVsyncLock = millis();

        if (!Serial.available()) {
            // in case we handled a Serial or web server command and there's no more extra commands
            // but keep debug view command (resets once called)
            if (serialCommand != 'D') {
                serialCommand = '@';
            }
            handleWiFi(1);
        }
    }

    if (userCommand != '@') {
        handleType2Command(userCommand);
        userCommand = '@'; // in case we handled web server command
        lastVsyncLock = millis();
        handleWiFi(1);
    }

    DIAG_T0(diagFtl);
#ifdef LOOP_DIAG
    uint32_t diagFtlBefore = loopDiag.ftlCalls;
#endif
    // run FrameTimeLock if enabled
    if (uopt->enableFrameTimeLock && rto->sourceDisconnected == false && rto->autoBestHtotalEnabled &&
        rto->syncWatcherEnabled && FrameSync::ready() && millis() - lastVsyncLock > FrameSyncAttrs::lockInterval && rto->continousStableCounter > 20 && rto->noSyncCounter == 0)
    {
        uint16_t htotal = GBS::STATUS_SYNC_PROC_HTOTAL::read();
        uint16_t pllad = GBS::PLLAD_MD::read();

        if (((htotal > (pllad - 3)) && (htotal < (pllad + 3)))) {
            uint8_t debug_backup = GBS::TEST_BUS_SEL::read();
            if (debug_backup != 0x0) {
                GBS::TEST_BUS_SEL::write(0x0);
            }
            //unsigned long startTime = millis();
            fsDebugPrintf("running frame sync, clock gen enabled = %d\n", rto->extClockGenDetected);
            bool success = rto->extClockGenDetected
                ? FrameSync::runFrequency()
                : FrameSync::runVsync(uopt->frameTimeLockMethod);
            if (!success) {
                if (rto->syncLockFailIgnore-- == 0) {
                    FrameSync::reset(uopt->frameTimeLockMethod); // in case run() failed because we lost sync signal
                }
            } else if (rto->syncLockFailIgnore > 0) {
                rto->syncLockFailIgnore = 16;
            }
            //Serial.println(millis() - startTime);

            if (debug_backup != 0x0) {
                GBS::TEST_BUS_SEL::write(debug_backup);
            }
        }
#ifdef LOOP_DIAG
        loopDiag.ftlCalls++;
#endif
        lastVsyncLock = millis();
    }
#ifdef LOOP_DIAG
    if (loopDiag.ftlCalls != diagFtlBefore) DIAG_MAX(ftlMax, diagFtl);
#endif

    if (rto->syncWatcherEnabled && rto->boardHasPower) {
        if ((millis() - lastTimeInterruptClear) > 3000) {
            GBS::INTERRUPT_CONTROL_00::write(0xfe); // reset except for SOGBAD
            GBS::INTERRUPT_CONTROL_00::write(0x00);
            lastTimeInterruptClear = millis();
        }
    }

    // information mode
    if (rto->printInfos == true) {
        printInfo();
    }

    //uint16_t testbus = GBS::TEST_BUS::read() & 0x0fff;
    //if (testbus >= 0x0FFD){
    //  Serial.println(testbus,HEX);
    //}
    //if (rto->videoIsFrozen && (rto->continousStableCounter >= 2)) {
    //    unfreezeVideo();
    //}

    DIAG_T0(diagSw);
#ifdef LOOP_DIAG
    uint32_t diagSwBefore = loopDiag.swCalls;
#endif
    // syncwatcher polls SP status. when necessary, initiates adjustments or preset changes
    if (rto->sourceDisconnected == false && rto->syncWatcherEnabled == true && (millis() - lastTimeSyncWatcher) > 20) {
        runSyncWatcher();
        lastTimeSyncWatcher = millis();

        // auto adc gain
        if (uopt->enableAutoGain == 1 && !rto->sourceDisconnected && rto->videoStandardInput > 0 && rto->clampPositionIsSet && rto->noSyncCounter == 0 && rto->continousStableCounter > 90 && rto->boardHasPower) {
            uint16_t htotal = GBS::STATUS_SYNC_PROC_HTOTAL::read();
            uint16_t pllad = GBS::PLLAD_MD::read();
            if (((htotal > (pllad - 3)) && (htotal < (pllad + 3)))) {
                uint8_t debugRegBackup = 0, debugPinBackup = 0;
                debugPinBackup = GBS::PAD_BOUT_EN::read();
                debugRegBackup = GBS::TEST_BUS_SEL::read();
                GBS::PAD_BOUT_EN::write(0);    // disable output to pin for test
                GBS::DEC_TEST_SEL::write(1);   // luma and G channel
                GBS::TEST_BUS_SEL::write(0xb); // decimation
                if (GBS::STATUS_INT_SOG_BAD::read() == 0) {
                    runAutoGain();
                }
                GBS::TEST_BUS_SEL::write(debugRegBackup);
                GBS::PAD_BOUT_EN::write(debugPinBackup); // debug output pin back on
            }
        }
    }

#ifdef LOOP_DIAG
    if (lastTimeSyncWatcher == millis()) { // watcher ran in this pass
        loopDiag.swCalls++;
        DIAG_MAX(swMax, diagSw);
    }
    (void)diagSwBefore;
#endif

    // init frame sync + besthtotal routine
    if (rto->autoBestHtotalEnabled && !FrameSync::ready() && rto->syncWatcherEnabled) {
        if (rto->continousStableCounter >= 10 && rto->coastPositionIsSet &&
            ((millis() - lastVsyncLock) > 500)) {
            if ((rto->continousStableCounter % 5) == 0) { // 5, 10, 15, .., 255
                uint16_t htotal = GBS::STATUS_SYNC_PROC_HTOTAL::read();
                uint16_t pllad = GBS::PLLAD_MD::read();
                if (((htotal > (pllad - 3)) && (htotal < (pllad + 3)))) {
                    runAutoBestHTotal();
                }
            }
        }
    }

    // update clamp + coast positions after preset change // do it quickly
    // retry at most every 100 ms: updateCoastPosition() takes ~3 ms and can fail on every try
    // (seen with RGBHV upscaling, HPERIOD_IF too small), which blocked loop() and wifi
    static unsigned long lastCoastTry = 0;
    if ((rto->videoStandardInput <= 14 && rto->videoStandardInput != 0) &&
        rto->syncWatcherEnabled && !rto->coastPositionIsSet && (millis() - lastCoastTry) >= 100) {
        lastCoastTry = millis();
        if (rto->continousStableCounter >= 7) {
            if ((getStatus16SpHsStable() == 1) && (getVideoMode() == rto->videoStandardInput)) {
                updateCoastPosition(0);
                if (rto->coastPositionIsSet) {
                    if (videoStandardInputIsPalNtscSd()) {
                        // todo: verify for other csync formats
                        GBS::SP_DIS_SUB_COAST::write(0); // enable 5_3e 5
                        GBS::SP_H_PROTECT::write(0);     // no 5_3e 4
                    }
                }
            }
        }
    }

    // don't exclude modes 13 / 14 / 15 (rgbhv bypass)
    if ((rto->videoStandardInput != 0) && (rto->continousStableCounter >= 4) &&
        !rto->clampPositionIsSet && rto->syncWatcherEnabled) {
        updateClampPosition();
        if (rto->clampPositionIsSet) {
            if (GBS::SP_NO_CLAMP_REG::read() == 1) {
                GBS::SP_NO_CLAMP_REG::write(0);
            }
        }
    }

    // later stage post preset adjustments
    if ((rto->applyPresetDoneStage == 1) &&
        ((rto->continousStableCounter > 35 && rto->continousStableCounter < 45) || // this
         !rto->syncWatcherEnabled))                                                // or that
    {
        if (rto->applyPresetDoneStage == 1) {
            // 2nd chance
            GBS::DAC_RGBS_PWDNZ::write(1); // 2nd chance
            if (!uopt->wantOutputComponent) {
                GBS::PAD_SYNC_OUT_ENZ::write(0); // enable sync out // 2nd chance
            }
            if (!rto->syncWatcherEnabled) {
                updateClampPosition();
                GBS::SP_NO_CLAMP_REG::write(0); // 5_57 0
            }

            if (rto->extClockGenDetected && rto->videoStandardInput != 14) {
                // switch to ext clock
                if (!rto->outModeHdBypass) {
                    if (GBS::PLL648_CONTROL_01::read() != 0x35 && GBS::PLL648_CONTROL_01::read() != 0x75) {
                        // first store original in an option byte in 1_2D
                        GBS::GBS_PRESET_DISPLAY_CLOCK::write(GBS::PLL648_CONTROL_01::read());
                        // enable and switch input
                        Si.enable(0);
                        delayMicroseconds(800);
                        GBS::PLL648_CONTROL_01::write(0x75);
                    }
                }
                // sync clocks now
                externalClockGenSyncInOutRate();
            }
            rto->applyPresetDoneStage = 0;
        }
    } else if (rto->applyPresetDoneStage == 1 && (rto->continousStableCounter > 35)) {
        // 3rd chance
        GBS::DAC_RGBS_PWDNZ::write(1); // enable DAC // 3rd chance
        if (!uopt->wantOutputComponent) {
            GBS::PAD_SYNC_OUT_ENZ::write(0); // enable sync out // 3rd chance
        }

        // sync clocks now
        externalClockGenSyncInOutRate();
        rto->applyPresetDoneStage = 0; // timeout
    }

    if (rto->applyPresetDoneStage == 10) {
        rto->applyPresetDoneStage = 11; // set first, so we don't loop applying presets
        setOutModeHdBypass(false);
    }

    if (rto->syncWatcherEnabled == true && rto->sourceDisconnected == true && rto->boardHasPower) {
        if ((millis() - lastTimeSourceCheck) >= 500) {
            if (checkBoardPower()) {
                inputAndSyncDetect(); // source is off or just started; keep looking for new input
            } else {
                rto->boardHasPower = false;
                rto->continousStableCounter = 0;
                rto->syncWatcherEnabled = false;
            }
            lastTimeSourceCheck = millis();

            // vary SOG slicer level from 2 to 6
            uint8_t currentSOG = GBS::ADC_SOGCTRL::read();
            if (currentSOG >= 3) {
                rto->currentLevelSOG = currentSOG - 1;
                GBS::ADC_SOGCTRL::write(rto->currentLevelSOG);
            } else {
                rto->currentLevelSOG = 6;
                GBS::ADC_SOGCTRL::write(rto->currentLevelSOG);
            }
        }
    }

    // has the GBS board lost power? // check at 2 points, in case one doesn't register
    // low values chosen to not do this check on small sync issues
    if ((rto->noSyncCounter == 61 || rto->noSyncCounter == 62) && rto->boardHasPower) {
        if (!checkBoardPower()) {
            rto->noSyncCounter = 1; // some neutral "no sync" value
            rto->boardHasPower = false;
            rto->continousStableCounter = 0;
            //rto->syncWatcherEnabled = false;
            stopWire(); // sets pinmodes SDA, SCL to INPUT
        } else {
            rto->noSyncCounter = 63; // avoid checking twice
        }
    }

    // power good now? // added syncWatcherEnabled check to enable passive mode
    // (passive mode = watching OFW without interrupting)
    if (!rto->boardHasPower && rto->syncWatcherEnabled) { // then check if power has come on
        if (digitalRead(SCL) && digitalRead(SDA)) {
            delay(50);
            if (digitalRead(SCL) && digitalRead(SDA)) {
                Serial.println(F("power good"));
                delay(350); // i've seen the MTV230 go on briefly on GBS power cycle
                startWire();
                {
                    // run some dummy commands to init I2C
                    GBS::SP_SOG_MODE::read();
                    GBS::SP_SOG_MODE::read();
                    writeOneByte(0xF0, 0);
                    writeOneByte(0x00, 0); // update cached segment
                    GBS::STATUS_00::read();
                }
                rto->syncWatcherEnabled = true;
                rto->boardHasPower = true;
                delay(100);
                goLowPowerWithInputDetection();
            }
        }
    }

#ifdef LOOP_DIAG
    {
        uint32_t d = micros() - diagLoopStart;
        loopDiag.loops++;
        loopDiag.loopSum += d;
        if (d > loopDiag.loopMax) loopDiag.loopMax = d;
        if (millis() - loopDiag.lastPrint >= 3000) {
            Serial.printf("[diag] loops:%u avg:%uus max:%ums | wifi max:%ums | syncw n:%u max:%ums | ftl n:%u max:%ums | rssi:%d heap:%u\n",
                loopDiag.loops, loopDiag.loops ? loopDiag.loopSum / loopDiag.loops : 0, loopDiag.loopMax / 1000,
                loopDiag.wifiMax / 1000, loopDiag.swCalls, loopDiag.swMax / 1000, loopDiag.ftlCalls, loopDiag.ftlMax / 1000,
                WiFi.RSSI(), ESP.getFreeHeap());
            Serial.printf("[diag] first edge max:%ums, over 60ms:%u\n", loopDiag.firstEdgeMaxUs / 1000, loopDiag.firstEdgeOver60);
            Serial.printf("[diag] D6 samples ok:%u (max %ums) fail:%u (max %ums, first edge seen:%u, D6 level changes while waiting:%u)\n",
                loopDiag.sampOk, loopDiag.sampOkMaxUs / 1000, loopDiag.sampFail, loopDiag.sampFailMaxUs / 1000,
                loopDiag.sampFailFirstEdge, loopDiag.sampFailToggles);
            {
                // D6 level without / with internal pull-up, plus GBS debug output settings
                uint32_t lvlIn = digitalRead(DEBUG_IN_PIN);
                pinMode(DEBUG_IN_PIN, INPUT_PULLUP);
                delayMicroseconds(100);
                uint32_t lvlPu = digitalRead(DEBUG_IN_PIN);
                uint32_t togglesPu = 0, last = lvlPu, t0 = micros();
                while (micros() - t0 < 30000) { // 30 ms, more than one vsync period
                    uint32_t l = digitalRead(DEBUG_IN_PIN);
                    if (l != last) { last = l; togglesPu++; }
                }
                pinMode(DEBUG_IN_PIN, INPUT);
                Serial.printf("[diag] D6 level input:%u pullup:%u (changes in 30ms with pullup:%u) | PAD_BOUT_EN:%u TEST_BUS_SEL:0x%02x\n",
                    lvlIn, lvlPu, togglesPu, GBS::PAD_BOUT_EN::read(), GBS::TEST_BUS_SEL::read());
                // independent of the input: select the output vsync (VDS, always generated) on the debug pin
                uint8_t tbBackup = GBS::TEST_BUS_SEL::read();
                uint8_t boutBackup = GBS::PAD_BOUT_EN::read();
                GBS::PAD_BOUT_EN::write(1);
                GBS::TEST_BUS_SEL::write(0x2);
                delay(2);
                uint32_t togglesOut = 0;
                last = digitalRead(DEBUG_IN_PIN);
                t0 = micros();
                while (micros() - t0 < 60000) { // 60 ms, a few output frames
                    uint32_t l = digitalRead(DEBUG_IN_PIN);
                    if (l != last) { last = l; togglesOut++; }
                }
                GBS::TEST_BUS_SEL::write(tbBackup);
                GBS::PAD_BOUT_EN::write(boutBackup);
                Serial.printf("[diag] D6 with OUTPUT vsync selected (TEST_BUS_SEL=0x02): changes in 60ms:%u (expect ~6-10 at 50-85 Hz)\n", togglesOut);
            }
            loopDiag = LoopDiag();
            loopDiag.lastPrint = millis();
        }
    }
#endif

#ifdef HAVE_PINGER_LIBRARY
    // periodic pings for debugging WiFi issues
    if (WiFi.status() == WL_CONNECTED) {
        if (rto->enableDebugPings && millis() - pingLastTime > 1000) {
            // regular interval pings
            if (pinger.Ping(WiFi.gatewayIP(), 1, 750) == false) {
                Serial.println("Error during last ping command.");
            }
            pingLastTime = millis();
        }
    }
#endif
}

#if defined(ESP8266) || defined(ESP32)
#if ENABLE_WIFI
#include "webui_html.h"
// gzip -c9 webui.html > webui_html && xxd -i webui_html > webui_html.h && rm webui_html && sed -i -e 's/unsigned char webui_html\[]/const uint8_t webui_html[] PROGMEM/' webui_html.h && sed -i -e 's/unsigned int webui_html_len/const unsigned int webui_html_len/' webui_html.h
#endif

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


#endif

#if !USE_NEW_OLED_MENU

#endif
