#include "config.h"
#include "platform_gbs.h"
#include "gbs_globals.h"
#include "gbs_prefs.h"
#include "gbs_video.h"
#include "FS.h"
#include "presets/ntsc_240p.h"
#include "presets/pal_240p.h"

void loadDefaultUserOptions()
{
    uopt->presetPreference = Output960P;    // #1
    uopt->enableFrameTimeLock = 0; // permanently adjust frame timing to avoid glitch vertical bar. does not work on all displays!
    uopt->presetSlot = 'A';          //
    uopt->frameTimeLockMethod = 0; // compatibility with more displays
    uopt->enableAutoGain = 0;
    uopt->wantScanlines = 0;
    uopt->wantOutputComponent = 0;
    uopt->deintMode = 0;
    uopt->wantVdsLineFilter = 0;
    uopt->wantPeaking = 1;
    uopt->preferScalingRgbhv = 1;
    uopt->wantTap6 = 1;
    uopt->PalForce60 = 0;
    uopt->matchPresetSource = 1;             // #14
    uopt->wantStepResponse = 1;              // #15
    uopt->wantFullHeight = 1;                // #16
    uopt->enableCalibrationADC = 1;          // #17
    uopt->scanlineStrength = 0x30;           // #18
    uopt->disableExternalClockGenerator = 0; // #19
}

// sets every element of str to 0 (clears array)
void StrClear(char *str, uint16_t length)
{
    for (int i = 0; i < length; i++) {
        str[i] = 0;
    }
}

const uint8_t *loadPresetFromSPIFFS(byte forVideoMode)
{
    static uint8_t preset[432];
    String s = "";
    Ascii8 slot = 0;
    File f;

    f = SPIFFS.open("/preferencesv2.txt", "r");
    if (f) {
        SerialM.println(F("preferencesv2.txt opened"));
        uint8_t result[3];
        result[0] = f.read(); // todo: move file cursor manually
        result[1] = f.read();
        result[2] = f.read();

        f.close();
        slot = result[2];
    } else {
        // file not found, we don't know what preset to load
        SerialM.println(F("please select a preset slot first!")); // say "slot" here to make people save usersettings
        if (forVideoMode == 2 || forVideoMode == 4)
            return pal_240p;
        else
            return ntsc_240p;
    }

    SerialM.print(F("loading from preset slot "));
    SerialM.print((char)slot);
    SerialM.print(": ");

    if (forVideoMode == 1) {
        f = SPIFFS.open("/preset_ntsc." + String((char)slot), "r");
    } else if (forVideoMode == 2) {
        f = SPIFFS.open("/preset_pal." + String((char)slot), "r");
    } else if (forVideoMode == 3) {
        f = SPIFFS.open("/preset_ntsc_480p." + String((char)slot), "r");
    } else if (forVideoMode == 4) {
        f = SPIFFS.open("/preset_pal_576p." + String((char)slot), "r");
    } else if (forVideoMode == 5) {
        f = SPIFFS.open("/preset_ntsc_720p." + String((char)slot), "r");
    } else if (forVideoMode == 6) {
        f = SPIFFS.open("/preset_ntsc_1080p." + String((char)slot), "r");
    } else if (forVideoMode == 8) {
        f = SPIFFS.open("/preset_medium_res." + String((char)slot), "r");
    } else if (forVideoMode == 14) {
        f = SPIFFS.open("/preset_vga_upscale." + String((char)slot), "r");
    } else if (forVideoMode == 0) {
        f = SPIFFS.open("/preset_unknown." + String((char)slot), "r");
    }

    if (!f) {
        SerialM.println(F("no preset file for this slot and source"));
        if (forVideoMode == 2 || forVideoMode == 4)
            return pal_240p;
        else
            return ntsc_240p;
    } else {
        SerialM.println(f.name());
        s = f.readStringUntil('}');
        f.close();
    }

    char *tmp;
    uint16_t i = 0;
    tmp = strtok(&s[0], ",");
    while (tmp) {
        preset[i++] = (uint8_t)atoi(tmp);
        tmp = strtok(NULL, ",");
        yield(); // wifi stack
    }

    return preset;
}

void savePresetToSPIFFS()
{
    uint8_t readout = 0;
    File f;
    Ascii8 slot = 0;

    // first figure out if the user has set a preferenced slot
    f = SPIFFS.open("/preferencesv2.txt", "r");
    if (f) {
        uint8_t result[3];
        result[0] = f.read(); // todo: move file cursor manually
        result[1] = f.read();
        result[2] = f.read();

        f.close();
        slot = result[2]; // got the slot to save to now
    } else {
        // file not found, we don't know where to save this preset
        SerialM.println(F("please select a preset slot first!"));
        return;
    }

    SerialM.print(F("saving to preset slot "));
    SerialM.println(String((char)slot));

    if (rto->videoStandardInput == 1) {
        f = SPIFFS.open("/preset_ntsc." + String((char)slot), "w");
    } else if (rto->videoStandardInput == 2) {
        f = SPIFFS.open("/preset_pal." + String((char)slot), "w");
    } else if (rto->videoStandardInput == 3) {
        f = SPIFFS.open("/preset_ntsc_480p." + String((char)slot), "w");
    } else if (rto->videoStandardInput == 4) {
        f = SPIFFS.open("/preset_pal_576p." + String((char)slot), "w");
    } else if (rto->videoStandardInput == 5) {
        f = SPIFFS.open("/preset_ntsc_720p." + String((char)slot), "w");
    } else if (rto->videoStandardInput == 6) {
        f = SPIFFS.open("/preset_ntsc_1080p." + String((char)slot), "w");
    } else if (rto->videoStandardInput == 8) {
        f = SPIFFS.open("/preset_medium_res." + String((char)slot), "w");
    } else if (rto->videoStandardInput == 14) {
        f = SPIFFS.open("/preset_vga_upscale." + String((char)slot), "w");
    } else if (rto->videoStandardInput == 0) {
        f = SPIFFS.open("/preset_unknown." + String((char)slot), "w");
    }

    if (!f) {
        SerialM.println(F("open save file failed!"));
    } else {
        SerialM.println(F("open save file ok"));

        GBS::GBS_PRESET_CUSTOM::write(1); // use one reserved bit to mark this as a custom preset
        // don't store scanlines
        if (GBS::GBS_OPTION_SCANLINES_ENABLED::read() == 1) {
            disableScanlines();
        }

        if (!rto->extClockGenDetected) {
            if (uopt->enableFrameTimeLock && FrameSync::getSyncLastCorrection() != 0) {
                FrameSync::reset(uopt->frameTimeLockMethod);
            }
        }

        for (int i = 0; i <= 5; i++) {
            writeOneByte(0xF0, i);
            switch (i) {
                case 0:
                    for (int x = 0x40; x <= 0x5F; x++) {
                        readFromRegister(x, 1, &readout);
                        f.print(readout);
                        f.println(",");
                    }
                    for (int x = 0x90; x <= 0x9F; x++) {
                        readFromRegister(x, 1, &readout);
                        f.print(readout);
                        f.println(",");
                    }
                    break;
                case 1:
                    for (int x = 0x0; x <= 0x2F; x++) {
                        readFromRegister(x, 1, &readout);
                        f.print(readout);
                        f.println(",");
                    }
                    break;
                case 2:
                    // not needed anymore
                    break;
                case 3:
                    for (int x = 0x0; x <= 0x7F; x++) {
                        readFromRegister(x, 1, &readout);
                        f.print(readout);
                        f.println(",");
                    }
                    break;
                case 4:
                    for (int x = 0x0; x <= 0x5F; x++) {
                        readFromRegister(x, 1, &readout);
                        f.print(readout);
                        f.println(",");
                    }
                    break;
                case 5:
                    for (int x = 0x0; x <= 0x6F; x++) {
                        readFromRegister(x, 1, &readout);
                        f.print(readout);
                        f.println(",");
                    }
                    break;
            }
        }
        f.println("};");
        SerialM.print(F("preset saved as: "));
        SerialM.println(f.name());
        f.close();
    }
}

void saveUserPrefs()
{
    File f = SPIFFS.open("/preferencesv2.txt", "w");
    if (!f) {
        SerialM.println(F("saveUserPrefs: open file failed"));
        return;
    }
    f.write(uopt->presetPreference + '0'); // #1
    f.write(uopt->enableFrameTimeLock + '0');
    f.write(uopt->presetSlot);
    f.write(uopt->frameTimeLockMethod + '0');
    f.write(uopt->enableAutoGain + '0');
    f.write(uopt->wantScanlines + '0');
    f.write(uopt->wantOutputComponent + '0');
    f.write(uopt->deintMode + '0');
    f.write(uopt->wantVdsLineFilter + '0');
    f.write(uopt->wantPeaking + '0');
    f.write(uopt->preferScalingRgbhv + '0');
    f.write(uopt->wantTap6 + '0');
    f.write(uopt->PalForce60 + '0');
    f.write(uopt->matchPresetSource + '0');             // #14
    f.write(uopt->wantStepResponse + '0');              // #15
    f.write(uopt->wantFullHeight + '0');                // #16
    f.write(uopt->enableCalibrationADC + '0');          // #17
    f.write(uopt->scanlineStrength + '0');              // #18
    f.write(uopt->disableExternalClockGenerator + '0'); // #19


    f.close();
}
