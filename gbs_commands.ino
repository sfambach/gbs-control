// serial terminal commands and web ui (user) commands
// part of the gbs-control sketch: all .ino files are concatenated into one translation unit
// (gbs-control.ino first), so globals and types from gbs-control.ino are visible here.

void discardSerialRxData()
{
    uint16_t maxThrowAway = 0x1fff;
    while (Serial.available() && maxThrowAway > 0) {
        Serial.read();
        maxThrowAway--;
    }
}

void myLog(char const* type, char command) {
    SerialM.printf("%s command %c at settings source %d, custom slot %d, status %x\n",
        type, command, uopt->presetPreference, uopt->presetSlot, rto->presetID);
}

#if defined(ESP8266) || defined(ESP32)
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
#if ENABLE_WIFI
            webSocket.close();
#endif
            loadDefaultUserOptions();
            saveUserPrefs();
            Serial.println(F("options set to defaults, restarting"));
            delay(60);
            gbsRestart(); // don't use restart(), messes up websocket reconnects
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
#if ENABLE_WIFI
            webSocket.close();
#endif
            Serial.println(F("restart"));
            delay(60);
            gbsRestart(); // don't use restart(), messes up websocket reconnects
            break;
        case 'e': // print files on spiffs
        {
            gbsForEachFile([](const String &name, size_t size) {
                SerialM.print(name);
                SerialM.print(" ");
                SerialM.println(size);
                delay(1); // wifi stack
            });
            ////
            File f = GBS_FS.open("/preferencesv2.txt", "r");
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
#if ENABLE_WIFI
            // restart to attempt wifi station mode connect
            delay(30);
            WiFi.mode(WIFI_STA);
            gbsWifiSetHostname(device_hostname_partial); // _full
            delay(30);
            gbsRestart();
#endif
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

#endif

// serial terminal / web ui "serial" commands (one char each, multi stage for s/t/g register access)
void handleSerialCommand()
{
    static uint8_t readout = 0;
    static uint8_t segmentCurrent = 255;
    static uint8_t registerCurrent = 255;
    static uint8_t inputToogleBit = 0;

    switch (serialCommand) {
        case ' ':
            // skip spaces
            inputStage = segmentCurrent = registerCurrent = 0; // and reset these
            break;
#if defined(ESP32) || defined(ESP8266)
        case 'I': {
            // WiFi diagnostics: stored network and what the scan sees (blocking scan, user triggered)
            String stored = gbsWifiStoredSsid();
            SerialM.printf("(WiFi): stored SSID '%s' (%u bytes), status %d, mode %d\n", stored.c_str(),
                           stored.length(), (int)WiFi.status(), (int)WiFi.getMode());
            int n = WiFi.scanNetworks(false, true);
            for (int i = 0; i < n; i++) {
                SerialM.printf("  '%s' %s ch %d %d dBm enc %d\n", WiFi.SSID(i).c_str(), WiFi.BSSIDstr(i).c_str(),
                               WiFi.channel(i), WiFi.RSSI(i), (int)WiFi.encryptionType(i));
            }
            SerialM.printf("(WiFi): %d networks\n", n);
            WiFi.scanDelete();
        } break;
#endif
        case 'd': {
            // don't store scanlines
            if (GBS::GBS_OPTION_SCANLINES_ENABLED::read() == 1) {
                disableScanlines();
            }
            // pal forced 60hz: no need to revert here. let the voffset function handle it

            if (uopt->enableFrameTimeLock && FrameSync::getSyncLastCorrection() != 0) {
                FrameSync::reset(uopt->frameTimeLockMethod);
            }
            // dump
            for (int segment = 0; segment <= 5; segment++) {
                dumpRegisters(segment);
            }
            SerialM.println("};");
        } break;
        case '+':
            SerialM.println("hor. +");
            shiftHorizontalRight();
            //shiftHorizontalRightIF(4);
            break;
        case '-':
            SerialM.println("hor. -");
            shiftHorizontalLeft();
            //shiftHorizontalLeftIF(4);
            break;
        case '*':
            shiftVerticalUpIF();
            break;
        case '/':
            shiftVerticalDownIF();
            break;
        case 'z':
            SerialM.println(F("scale+"));
            scaleHorizontal(2, true);
            break;
        case 'h':
            SerialM.println(F("scale-"));
            scaleHorizontal(2, false);
            break;
        case 'q':
            resetDigital();
            delay(2);
            ResetSDRAM();
            delay(2);
            togglePhaseAdjustUnits();
            //enableVDS();
            break;
        case 'D':
            SerialM.print(F("debug view: "));
            if (GBS::ADC_UNUSED_62::read() == 0x00) { // "remembers" debug view
                //if (uopt->wantPeaking == 0) { GBS::VDS_PK_Y_H_BYPS::write(0); } // 3_4e 0 // enable peaking but don't store
                GBS::VDS_PK_LB_GAIN::write(0x3f);                   // 3_45
                GBS::VDS_PK_LH_GAIN::write(0x3f);                   // 3_47
                GBS::ADC_UNUSED_60::write(GBS::VDS_Y_OFST::read()); // backup
                GBS::ADC_UNUSED_61::write(GBS::HD_Y_OFFSET::read());
                GBS::ADC_UNUSED_62::write(1); // remember to remove on restore
                GBS::VDS_Y_OFST::write(GBS::VDS_Y_OFST::read() + 0x24);
                GBS::HD_Y_OFFSET::write(GBS::HD_Y_OFFSET::read() + 0x24);
                if (!rto->inputIsYpBpR) {
                    // RGB input that has HD_DYN bypassed, use it now
                    GBS::HD_DYN_BYPS::write(0);
                    GBS::HD_U_OFFSET::write(GBS::HD_U_OFFSET::read() + 0x24);
                    GBS::HD_V_OFFSET::write(GBS::HD_V_OFFSET::read() + 0x24);
                }
                //GBS::IF_IN_DREG_BYPS::write(1); // enhances noise from not delaying IF processing properly
                SerialM.println("on");
            } else {
                //if (uopt->wantPeaking == 0) { GBS::VDS_PK_Y_H_BYPS::write(1); } // 3_4e 0
                if (rto->presetID == 0x05) {
                    GBS::VDS_PK_LB_GAIN::write(0x16); // 3_45
                    GBS::VDS_PK_LH_GAIN::write(0x0A); // 3_47
                } else {
                    GBS::VDS_PK_LB_GAIN::write(0x16); // 3_45
                    GBS::VDS_PK_LH_GAIN::write(0x18); // 3_47
                }
                GBS::VDS_Y_OFST::write(GBS::ADC_UNUSED_60::read()); // restore
                GBS::HD_Y_OFFSET::write(GBS::ADC_UNUSED_61::read());
                if (!rto->inputIsYpBpR) {
                    // RGB input, HD_DYN_BYPS again
                    GBS::HD_DYN_BYPS::write(1);
                    GBS::HD_U_OFFSET::write(0); // probably just 0 by default
                    GBS::HD_V_OFFSET::write(0); // probably just 0 by default
                }
                //GBS::IF_IN_DREG_BYPS::write(0);
                GBS::ADC_UNUSED_60::write(0); // .. and clear
                GBS::ADC_UNUSED_61::write(0);
                GBS::ADC_UNUSED_62::write(0);
                SerialM.println("off");
            }
            serialCommand = '@';
            break;
        case 'C':
            SerialM.println(F("PLL: ICLK"));
            // display clock in last test best at 0x85
            GBS::PLL648_CONTROL_01::write(0x85);
            GBS::PLL_CKIS::write(1); // PLL use ICLK (instead of oscillator)
            latchPLLAD();
            //GBS::VDS_HSCALE::write(512);
            rto->syncLockFailIgnore = 16;
            FrameSync::reset(uopt->frameTimeLockMethod); // adjust htotal to new display clock
            rto->forceRetime = true;
            //applyBestHTotal(FrameSync::init()); // adjust htotal to new display clock
            //applyBestHTotal(FrameSync::init()); // twice
            //GBS::VDS_FLOCK_EN::write(1); //risky
            delay(200);
            break;
        case 'Y':
            writeProgramArrayNew(ntsc_1280x720, false);
            doPostPresetLoadSteps();
            break;
        case 'y':
            writeProgramArrayNew(pal_1280x720, false);
            doPostPresetLoadSteps();
            break;
        case 'P':
            SerialM.print(F("auto deinterlace: "));
            rto->deinterlaceAutoEnabled = !rto->deinterlaceAutoEnabled;
            if (rto->deinterlaceAutoEnabled) {
                SerialM.println("on");
            } else {
                SerialM.println("off");
            }
            break;
        case 'p':
            if (!rto->motionAdaptiveDeinterlaceActive) {
                if (GBS::GBS_OPTION_SCANLINES_ENABLED::read() == 1) { // don't rely on rto->scanlinesEnabled
                    disableScanlines();
                }
                enableMotionAdaptDeinterlace();
            } else {
                disableMotionAdaptDeinterlace();
            }
            break;
        case 'k':
            bypassModeSwitch_RGBHV();
            break;
        case 'K':
            setOutModeHdBypass(false);
            uopt->presetPreference = OutputBypass;
            saveUserPrefs();
            break;
        case 'T':
            SerialM.print(F("auto gain "));
            if (uopt->enableAutoGain == 0) {
                uopt->enableAutoGain = 1;
                setAdcGain(AUTO_GAIN_INIT);
                GBS::DEC_TEST_ENABLE::write(1);
                SerialM.println("on");
            } else {
                uopt->enableAutoGain = 0;
                GBS::DEC_TEST_ENABLE::write(0);
                SerialM.println("off");
            }
            saveUserPrefs();
            break;
        case 'e':
            writeProgramArrayNew(ntsc_240p, false);
            doPostPresetLoadSteps();
            break;
        case 'r':
            writeProgramArrayNew(pal_240p, false);
            doPostPresetLoadSteps();
            break;
        case '.': {
            if (!rto->outModeHdBypass) {
                // bestHtotal recalc
                rto->autoBestHtotalEnabled = true;
                rto->syncLockFailIgnore = 16;
                rto->forceRetime = true;
                FrameSync::reset(uopt->frameTimeLockMethod);

                if (!rto->syncWatcherEnabled) {
                    boolean autoBestHtotalSuccess = 0;
                    delay(30);
                    autoBestHtotalSuccess = runAutoBestHTotal();
                    if (!autoBestHtotalSuccess) {
                        SerialM.println(F("(unchanged)"));
                    }
                }
            }
        } break;
        case '!':
            //fastGetBestHtotal();
            //readEeprom();
            Serial.print(F("sfr: "));
            Serial.println(getSourceFieldRate(1));
            Serial.print(F("pll: "));
            Serial.println(getPllRate());
            break;
        case '$': {
            // EEPROM write protect pin (7, next to Vcc) is under original MCU control
            // MCU drags to Vcc to write, EEPROM drags to Gnd normally
            // This routine only works with that "WP" pin disconnected
            // 0x17 = input selector? // 0x18 = input selector related?
            // 0x54 = coast start 0x55 = coast end
            uint16_t writeAddr = 0x54;
            const uint8_t eepromAddr = 0x50;
            for (; writeAddr < 0x56; writeAddr++) {
                Wire.beginTransmission(eepromAddr);
                Wire.write(writeAddr >> 8);     // high addr byte, 4 bits +
                Wire.write((uint8_t)writeAddr); // mlow addr byte, 8 bits = 12 bits (0xfff max)
                Wire.write(0x10);               // coast end value ?
                Wire.endTransmission();
                delay(5);
            }

            //Wire.beginTransmission(eepromAddr);
            //Wire.write((uint8_t)0); Wire.write((uint8_t)0);
            //Wire.write(0xff); // force eeprom clear probably
            //Wire.endTransmission();
            //delay(5);

            Serial.println("done");
        } break;
        case 'j':
            //resetPLL();
            latchPLLAD();
            break;
        case 'J':
            resetPLLAD();
            break;
        case 'v':
            rto->phaseSP += 1;
            rto->phaseSP &= 0x1f;
            SerialM.print("SP: ");
            SerialM.println(rto->phaseSP);
            setAndLatchPhaseSP();
            //setAndLatchPhaseADC();
            break;
        case 'b':
            advancePhase();
            latchPLLAD();
            SerialM.print("ADC: ");
            SerialM.println(rto->phaseADC);
            break;
        case '#':
            rto->videoStandardInput = 13;
            applyPresets(13);
            //Serial.println(getStatus00IfHsVsStable());
            //globalDelay++;
            //SerialM.println(globalDelay);
            break;
        case 'n': {
            uint16_t pll_divider = GBS::PLLAD_MD::read();
            pll_divider += 1;
            GBS::PLLAD_MD::write(pll_divider);
            GBS::IF_HSYNC_RST::write((pll_divider / 2));
            GBS::IF_LINE_SP::write(((pll_divider / 2) + 1) + 0x40);
            SerialM.print(F("PLL div: "));
            SerialM.print(pll_divider, HEX);
            SerialM.print(" ");
            SerialM.println(pll_divider);
            // set IF before latching
            //setIfHblankParameters();
            latchPLLAD();
            delay(1);
            //applyBestHTotal(GBS::VDS_HSYNC_RST::read());
            updateClampPosition();
            updateCoastPosition(0);
        } break;
        case 'N': {
            //if (GBS::RFF_ENABLE::read()) {
            //  disableMotionAdaptDeinterlace();
            //}
            //else {
            //  enableMotionAdaptDeinterlace();
            //}

            //GBS::RFF_ENABLE::write(!GBS::RFF_ENABLE::read());

            if (rto->scanlinesEnabled) {
                rto->scanlinesEnabled = false;
                disableScanlines();
            } else {
                rto->scanlinesEnabled = true;
                enableScanlines();
            }
        } break;
        case 'a':
            SerialM.print(F("HTotal++: "));
            SerialM.println(GBS::VDS_HSYNC_RST::read() + 1);
            if (GBS::VDS_HSYNC_RST::read() < 4095) {
                if (uopt->enableFrameTimeLock) {
                    // syncLastCorrection != 0 check is included
                    FrameSync::reset(uopt->frameTimeLockMethod);
                }
                rto->forceRetime = 1;
                applyBestHTotal(GBS::VDS_HSYNC_RST::read() + 1);
            }
            break;
        case 'A':
            SerialM.print(F("HTotal--: "));
            SerialM.println(GBS::VDS_HSYNC_RST::read() - 1);
            if (GBS::VDS_HSYNC_RST::read() > 0) {
                if (uopt->enableFrameTimeLock) {
                    // syncLastCorrection != 0 check is included
                    FrameSync::reset(uopt->frameTimeLockMethod);
                }
                rto->forceRetime = 1;
                applyBestHTotal(GBS::VDS_HSYNC_RST::read() - 1);
            }
            break;
        case 'M': {
        } break;
        case 'm':
            SerialM.print(F("syncwatcher "));
            if (rto->syncWatcherEnabled == true) {
                rto->syncWatcherEnabled = false;
                if (rto->videoIsFrozen) {
                    unfreezeVideo();
                }
                SerialM.println("off");
            } else {
                rto->syncWatcherEnabled = true;
                SerialM.println("on");
            }
            break;
        case ',':
            printVideoTimings();
            break;
        case 'i':
            rto->printInfos = !rto->printInfos;
            break;
        case 'c':
#if ENABLE_WIFI
            SerialM.println(F("OTA Updates on"));
            initUpdateOTA();
            rto->allowUpdatesOTA = true;
#endif
            break;
        case 'G':
            SerialM.print(F("Debug Pings "));
            if (!rto->enableDebugPings) {
                SerialM.println("on");
                rto->enableDebugPings = 1;
            } else {
                SerialM.println("off");
                rto->enableDebugPings = 0;
            }
            break;
        case 'u':
            ResetSDRAM();
            break;
        case 'f':
            SerialM.print(F("peaking "));
            if (uopt->wantPeaking == 0) {
                uopt->wantPeaking = 1;
                GBS::VDS_PK_Y_H_BYPS::write(0);
                SerialM.println("on");
            } else {
                uopt->wantPeaking = 0;
                GBS::VDS_PK_Y_H_BYPS::write(1);
                SerialM.println("off");
            }
            saveUserPrefs();
            break;
        case 'F':
            SerialM.print(F("ADC filter "));
            if (GBS::ADC_FLTR::read() > 0) {
                GBS::ADC_FLTR::write(0);
                SerialM.println("off");
            } else {
                GBS::ADC_FLTR::write(3);
                SerialM.println("on");
            }
            break;
        case 'L': {
            // Component / VGA Output
            uopt->wantOutputComponent = !uopt->wantOutputComponent;
            OutputComponentOrVGA();
            saveUserPrefs();
            // apply 1280x720 preset now, otherwise a reboot would be required
            uint8_t videoMode = getVideoMode();
            if (videoMode == 0)
                videoMode = rto->videoStandardInput;
            PresetPreference backup = uopt->presetPreference;
            uopt->presetPreference = Output720P;
            rto->videoStandardInput = 0; // force hard reset
            applyPresets(videoMode);
            uopt->presetPreference = backup;
        } break;
        case 'l':
            SerialM.println(F("resetSyncProcessor"));
            //freezeVideo();
            resetSyncProcessor();
            //delay(10);
            //unfreezeVideo();
            break;
        case 'Z': {
            uopt->matchPresetSource = !uopt->matchPresetSource;
            saveUserPrefs();
            uint8_t vidMode = getVideoMode();
            if (uopt->presetPreference == 0 && GBS::GBS_PRESET_ID::read() == 0x11) {
                applyPresets(vidMode);
            } else if (uopt->presetPreference == 4 && GBS::GBS_PRESET_ID::read() == 0x02) {
                applyPresets(vidMode);
            }
        } break;
        case 'W':
            uopt->enableFrameTimeLock = !uopt->enableFrameTimeLock;
            break;
        case 'E':
            writeProgramArrayNew(ntsc_1280x1024, false);
            doPostPresetLoadSteps();
            break;
        case 'R':
            writeProgramArrayNew(pal_1280x1024, false);
            doPostPresetLoadSteps();
            break;
        case '0':
            moveHS(4, true);
            break;
        case '1':
            moveHS(4, false);
            break;
        case '2':
            writeProgramArrayNew(pal_768x576, false); // ModeLine "720x576@50" 27 720 732 795 864 576 581 586 625 -hsync -vsync
            doPostPresetLoadSteps();
            break;
        case '3':
            //
            break;
        case '4': {
            // scale vertical +
            if (GBS::VDS_VSCALE::read() <= 256) {
                SerialM.println("limit");
                break;
            }
            scaleVertical(2, true);
            // actually requires full vertical mask + position offset calculation
        } break;
        case '5': {
            // scale vertical -
            if (GBS::VDS_VSCALE::read() == 1023) {
                SerialM.println("limit");
                break;
            }
            scaleVertical(2, false);
            // actually requires full vertical mask + position offset calculation
        } break;
        case '6':
            if (videoStandardInputIsPalNtscSd() && !rto->outModeHdBypass) {
                if (GBS::IF_HBIN_SP::read() >= 10) {                     // IF_HBIN_SP: min 2
                    GBS::IF_HBIN_SP::write(GBS::IF_HBIN_SP::read() - 8); // canvas move right
                    if ((GBS::IF_HSYNC_RST::read() - 4) > ((GBS::PLLAD_MD::read() >> 1) + 5)) {
                        GBS::IF_HSYNC_RST::write(GBS::IF_HSYNC_RST::read() - 4); // shrink 1_0e
                        GBS::IF_LINE_SP::write(GBS::IF_LINE_SP::read() - 4);     // and 1_22 to go with it
                    }
                } else {
                    SerialM.println("limit");
                }
            } else if (!rto->outModeHdBypass) {
                if (GBS::IF_HB_SP2::read() >= 4)
                    GBS::IF_HB_SP2::write(GBS::IF_HB_SP2::read() - 4); // 1_1a
                else
                    GBS::IF_HB_SP2::write(GBS::IF_HSYNC_RST::read() - 0x30);
                if (GBS::IF_HB_ST2::read() >= 4)
                    GBS::IF_HB_ST2::write(GBS::IF_HB_ST2::read() - 4); // 1_18
                else
                    GBS::IF_HB_ST2::write(GBS::IF_HSYNC_RST::read() - 0x30);
                SerialM.print(F("IF_HB_ST2: "));
                SerialM.print(GBS::IF_HB_ST2::read(), HEX);
                SerialM.print(F(" IF_HB_SP2: "));
                SerialM.println(GBS::IF_HB_SP2::read(), HEX);
            }
            break;
        case '7':
            if (videoStandardInputIsPalNtscSd() && !rto->outModeHdBypass) {
                if (GBS::IF_HBIN_SP::read() < 0x150) {                   // (arbitrary) max limit
                    GBS::IF_HBIN_SP::write(GBS::IF_HBIN_SP::read() + 8); // canvas move left
                } else {
                    SerialM.println("limit");
                }
            } else if (!rto->outModeHdBypass) {
                if (GBS::IF_HB_SP2::read() < (GBS::IF_HSYNC_RST::read() - 0x30))
                    GBS::IF_HB_SP2::write(GBS::IF_HB_SP2::read() + 4); // 1_1a
                else
                    GBS::IF_HB_SP2::write(0);
                if (GBS::IF_HB_ST2::read() < (GBS::IF_HSYNC_RST::read() - 0x30))
                    GBS::IF_HB_ST2::write(GBS::IF_HB_ST2::read() + 4); // 1_18
                else
                    GBS::IF_HB_ST2::write(0);
                SerialM.print(F("IF_HB_ST2: "));
                SerialM.print(GBS::IF_HB_ST2::read(), HEX);
                SerialM.print(F(" IF_HB_SP2: "));
                SerialM.println(GBS::IF_HB_SP2::read(), HEX);
            }
            break;
        case '8':
            //SerialM.println("invert sync");
            invertHS();
            invertVS();
            //optimizePhaseSP();
            break;
        case '9':
            writeProgramArrayNew(ntsc_720x480, false);
            doPostPresetLoadSteps();
            break;
        case 'o': {
            if (rto->osr == 1) {
                setOverSampleRatio(2, false);
            } else if (rto->osr == 2) {
                setOverSampleRatio(4, false);
            } else if (rto->osr == 4) {
                setOverSampleRatio(1, false);
            }
            delay(4);
            optimizePhaseSP();
            SerialM.print("OSR ");
            SerialM.print(rto->osr);
            SerialM.println("x");
            rto->phaseIsSet = 0; // do it again in modes applicable
        } break;
        case 'g':
            inputStage++;
            // we have a multibyte command
            if (inputStage > 0) {
                if (inputStage == 1) {
                    segmentCurrent = Serial.parseInt();
                    SerialM.print("G");
                    SerialM.print(segmentCurrent);
                } else if (inputStage == 2) {
                    char szNumbers[3];
                    szNumbers[0] = Serial.read();
                    szNumbers[1] = Serial.read();
                    szNumbers[2] = '\0';
                    //char * pEnd;
                    registerCurrent = strtol(szNumbers, NULL, 16);
                    SerialM.print("R");
                    SerialM.print(registerCurrent, HEX);
                    if (segmentCurrent <= 5) {
                        writeOneByte(0xF0, segmentCurrent);
                        readFromRegister(registerCurrent, 1, &readout);
                        SerialM.print(" value: 0x");
                        SerialM.println(readout, HEX);
                    } else {
                        discardSerialRxData();
                        SerialM.println("abort");
                    }
                    inputStage = 0;
                }
            }
            break;
        case 's':
            inputStage++;
            // we have a multibyte command
            if (inputStage > 0) {
                if (inputStage == 1) {
                    segmentCurrent = Serial.parseInt();
                    SerialM.print("S");
                    SerialM.print(segmentCurrent);
                } else if (inputStage == 2) {
                    char szNumbers[3];
                    for (uint8_t a = 0; a <= 1; a++) {
                        // ascii 0x30 to 0x39 for '0' to '9'
                        if ((Serial.peek() >= '0' && Serial.peek() <= '9') ||
                            (Serial.peek() >= 'a' && Serial.peek() <= 'f') ||
                            (Serial.peek() >= 'A' && Serial.peek() <= 'F')) {
                            szNumbers[a] = Serial.read();
                        } else {
                            szNumbers[a] = 0; // NUL char
                            Serial.read();    // but consume the char
                        }
                    }
                    szNumbers[2] = '\0';
                    //char * pEnd;
                    registerCurrent = strtol(szNumbers, NULL, 16);
                    SerialM.print("R");
                    SerialM.print(registerCurrent, HEX);
                } else if (inputStage == 3) {
                    char szNumbers[3];
                    for (uint8_t a = 0; a <= 1; a++) {
                        if ((Serial.peek() >= '0' && Serial.peek() <= '9') ||
                            (Serial.peek() >= 'a' && Serial.peek() <= 'f') ||
                            (Serial.peek() >= 'A' && Serial.peek() <= 'F')) {
                            szNumbers[a] = Serial.read();
                        } else {
                            szNumbers[a] = 0; // NUL char
                            Serial.read();    // but consume the char
                        }
                    }
                    szNumbers[2] = '\0';
                    //char * pEnd;
                    inputToogleBit = strtol(szNumbers, NULL, 16);
                    if (segmentCurrent <= 5) {
                        writeOneByte(0xF0, segmentCurrent);
                        readFromRegister(registerCurrent, 1, &readout);
                        SerialM.print(" (was 0x");
                        SerialM.print(readout, HEX);
                        SerialM.print(")");
                        writeOneByte(registerCurrent, inputToogleBit);
                        readFromRegister(registerCurrent, 1, &readout);
                        SerialM.print(" is now: 0x");
                        SerialM.println(readout, HEX);
                    } else {
                        discardSerialRxData();
                        SerialM.println("abort");
                    }
                    inputStage = 0;
                }
            }
            break;
        case 't':
            inputStage++;
            // we have a multibyte command
            if (inputStage > 0) {
                if (inputStage == 1) {
                    segmentCurrent = Serial.parseInt();
                    SerialM.print("T");
                    SerialM.print(segmentCurrent);
                } else if (inputStage == 2) {
                    char szNumbers[3];
                    for (uint8_t a = 0; a <= 1; a++) {
                        // ascii 0x30 to 0x39 for '0' to '9'
                        if ((Serial.peek() >= '0' && Serial.peek() <= '9') ||
                            (Serial.peek() >= 'a' && Serial.peek() <= 'f') ||
                            (Serial.peek() >= 'A' && Serial.peek() <= 'F')) {
                            szNumbers[a] = Serial.read();
                        } else {
                            szNumbers[a] = 0; // NUL char
                            Serial.read();    // but consume the char
                        }
                    }
                    szNumbers[2] = '\0';
                    //char * pEnd;
                    registerCurrent = strtol(szNumbers, NULL, 16);
                    SerialM.print("R");
                    SerialM.print(registerCurrent, HEX);
                } else if (inputStage == 3) {
                    if (Serial.peek() >= '0' && Serial.peek() <= '7') {
                        inputToogleBit = Serial.parseInt();
                    } else {
                        inputToogleBit = 255; // will get discarded next step
                    }
                    SerialM.print(" Bit: ");
                    SerialM.print(inputToogleBit);
                    inputStage = 0;
                    if ((segmentCurrent <= 5) && (inputToogleBit <= 7)) {
                        writeOneByte(0xF0, segmentCurrent);
                        readFromRegister(registerCurrent, 1, &readout);
                        SerialM.print(" (was 0x");
                        SerialM.print(readout, HEX);
                        SerialM.print(")");
                        writeOneByte(registerCurrent, readout ^ (1 << inputToogleBit));
                        readFromRegister(registerCurrent, 1, &readout);
                        SerialM.print(" is now: 0x");
                        SerialM.println(readout, HEX);
                    } else {
                        discardSerialRxData();
                        inputToogleBit = registerCurrent = 0;
                        SerialM.println("abort");
                    }
                }
            }
            break;
        case '<': {
            if (segmentCurrent != 255 && registerCurrent != 255) {
                writeOneByte(0xF0, segmentCurrent);
                readFromRegister(registerCurrent, 1, &readout);
                writeOneByte(registerCurrent, readout - 1); // also allow wrapping
                Serial.print("S");
                Serial.print(segmentCurrent);
                Serial.print("_");
                Serial.print(registerCurrent, HEX);
                readFromRegister(registerCurrent, 1, &readout);
                Serial.print(" : ");
                Serial.println(readout, HEX);
            }
        } break;
        case '>': {
            if (segmentCurrent != 255 && registerCurrent != 255) {
                writeOneByte(0xF0, segmentCurrent);
                readFromRegister(registerCurrent, 1, &readout);
                writeOneByte(registerCurrent, readout + 1);
                Serial.print("S");
                Serial.print(segmentCurrent);
                Serial.print("_");
                Serial.print(registerCurrent, HEX);
                readFromRegister(registerCurrent, 1, &readout);
                Serial.print(" : ");
                Serial.println(readout, HEX);
            }
        } break;
        case '_': {
            uint32_t ticks = FrameSync::getPulseTicks();
            Serial.println(ticks);
        } break;
        case '~':
            goLowPowerWithInputDetection(); // test reset + input detect
            break;
        case 'w': {
            //Serial.flush();
            uint16_t value = 0;
            String what = Serial.readStringUntil(' ');

            if (what.length() > 5) {
                SerialM.println(F("abort"));
                inputStage = 0;
                break;
            }
            if (what.equals("f")) {
                if (rto->extClockGenDetected) {
                    Serial.print(F("old freqExtClockGen: "));
                    Serial.println((uint32_t)rto->freqExtClockGen);
                    rto->freqExtClockGen = Serial.parseInt();
                    // safety range: 1 - 250 MHz
                    if (rto->freqExtClockGen >= 1000000 && rto->freqExtClockGen <= 250000000) {
                        Si.setFreq(0, rto->freqExtClockGen);
                        rto->clampPositionIsSet = 0;
                        rto->coastPositionIsSet = 0;
                    }
                    Serial.print(F("set freqExtClockGen: "));
                    Serial.println((uint32_t)rto->freqExtClockGen);
                }
                break;
            }

            value = Serial.parseInt();
            if (value < 4096) {
                SerialM.print("set ");
                SerialM.print(what);
                SerialM.print(" ");
                SerialM.println(value);
                if (what.equals("ht")) {
                    //set_htotal(value);
                    if (!rto->outModeHdBypass) {
                        rto->forceRetime = 1;
                        applyBestHTotal(value);
                    } else {
                        GBS::VDS_HSYNC_RST::write(value);
                    }
                } else if (what.equals("vt")) {
                    set_vtotal(value);
                } else if (what.equals("hsst")) {
                    setHSyncStartPosition(value);
                } else if (what.equals("hssp")) {
                    setHSyncStopPosition(value);
                } else if (what.equals("hbst")) {
                    setMemoryHblankStartPosition(value);
                } else if (what.equals("hbsp")) {
                    setMemoryHblankStopPosition(value);
                } else if (what.equals("hbstd")) {
                    setDisplayHblankStartPosition(value);
                } else if (what.equals("hbspd")) {
                    setDisplayHblankStopPosition(value);
                } else if (what.equals("vsst")) {
                    setVSyncStartPosition(value);
                } else if (what.equals("vssp")) {
                    setVSyncStopPosition(value);
                } else if (what.equals("vbst")) {
                    setMemoryVblankStartPosition(value);
                } else if (what.equals("vbsp")) {
                    setMemoryVblankStopPosition(value);
                } else if (what.equals("vbstd")) {
                    setDisplayVblankStartPosition(value);
                } else if (what.equals("vbspd")) {
                    setDisplayVblankStopPosition(value);
                } else if (what.equals("sog")) {
                    setAndUpdateSogLevel(value);
                } else if (what.equals("ifini")) {
                    GBS::IF_INI_ST::write(value);
                    //Serial.println(GBS::IF_INI_ST::read());
                } else if (what.equals("ifvst")) {
                    GBS::IF_VB_ST::write(value);
                    //Serial.println(GBS::IF_VB_ST::read());
                } else if (what.equals("ifvsp")) {
                    GBS::IF_VB_SP::write(value);
                    //Serial.println(GBS::IF_VB_ST::read());
                } else if (what.equals("vsstc")) {
                    setCsVsStart(value);
                } else if (what.equals("vsspc")) {
                    setCsVsStop(value);
                }
            } else {
                SerialM.println("abort");
            }
        } break;
        case 'x': {
            uint16_t if_hblank_scale_stop = GBS::IF_HBIN_SP::read();
            GBS::IF_HBIN_SP::write(if_hblank_scale_stop + 1);
            SerialM.print("1_26: ");
            SerialM.println((if_hblank_scale_stop + 1), HEX);
        } break;
        case 'X': {
            uint16_t if_hblank_scale_stop = GBS::IF_HBIN_SP::read();
            GBS::IF_HBIN_SP::write(if_hblank_scale_stop - 1);
            SerialM.print("1_26: ");
            SerialM.println((if_hblank_scale_stop - 1), HEX);
        } break;
        case '(': {
            writeProgramArrayNew(ntsc_1920x1080, false);
            doPostPresetLoadSteps();
        } break;
        case ')': {
            writeProgramArrayNew(pal_1920x1080, false);
            doPostPresetLoadSteps();
        } break;
        case 'V': {
            SerialM.print(F("step response "));
            uopt->wantStepResponse = !uopt->wantStepResponse;
            if (uopt->wantStepResponse) {
                GBS::VDS_UV_STEP_BYPS::write(0);
                SerialM.println("on");
            } else {
                GBS::VDS_UV_STEP_BYPS::write(1);
                SerialM.println("off");
            }
            saveUserPrefs();
        } break;
        case 'S': {
            snapToIntegralFrameRate();
            break;
        }
        case ':':
            externalClockGenSyncInOutRate();
            break;
        case ';':
            externalClockGenResetClock();
            if (rto->extClockGenDetected) {
                rto->extClockGenDetected = 0;
                Serial.println(F("ext clock gen bypass"));
            } else {
                rto->extClockGenDetected = 1;
                Serial.println(F("ext clock gen active"));
                externalClockGenSyncInOutRate();
            }
            //{
            //  float bla = 0;
            //  double esp8266_clock_freq = ESP.getCpuFreqMHz() * 1000000;
            //  bla = esp8266_clock_freq / (double)FrameSync::getPulseTicks();
            //  Serial.println(bla, 5);
            //}
            break;
        default:
            Serial.print(F("unknown command "));
            Serial.println(serialCommand, HEX);
            break;
    }
}
