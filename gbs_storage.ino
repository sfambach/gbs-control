// presets, slots and user preferences on SPIFFS
// part of the gbs-control sketch: all .ino files are concatenated into one translation unit
// (gbs-control.ino first), so globals and types from gbs-control.ino are visible here.

// preset files are stored per slot and detected video mode: "/preset_<name>.<slot>"
// (order = order used when removing / shifting slots)
struct PresetFile
{
    uint8_t videoMode;
    const char *name;
};
static const PresetFile presetFiles[] = {
    {1, "ntsc"}, {2, "pal"}, {3, "ntsc_480p"}, {4, "pal_576p"}, {5, "ntsc_720p"},
    {6, "ntsc_1080p"}, {8, "medium_res"}, {14, "vga_upscale"}, {0, "unknown"},
};
static_assert(PRESET_FILE_COUNT == sizeof(presetFiles) / sizeof(presetFiles[0]), "update PRESET_FILE_COUNT");

String presetFileNameAt(size_t index, Ascii8 slot)
{
    return String("/preset_") + presetFiles[index].name + "." + String((char)slot);
}

// file name for a video mode, empty if this video mode has no preset file
String presetFileName(uint8_t videoMode, Ascii8 slot)
{
    for (size_t i = 0; i < PRESET_FILE_COUNT; i++) {
        if (presetFiles[i].videoMode == videoMode) {
            return presetFileNameAt(i, slot);
        }
    }
    return String();
}

// slot metadata in SLOTS_FILE

void initEmptySlots(SlotMetaArray &slots)
{
    for (int i = 0; i < SLOTS_TOTAL; i++) {
        slots.slot[i].slot = i;
        slots.slot[i].presetID = 0;
        slots.slot[i].scanlines = 0;
        slots.slot[i].scanlinesStrength = 0;
        slots.slot[i].wantVdsLineFilter = false;
        slots.slot[i].wantStepResponse = true;
        slots.slot[i].wantPeaking = true;
        char emptySlotName[25] = EMPTY_SLOT_NAME;
        strncpy(slots.slot[i].name, emptySlotName, 25);
    }
}

void writeSlots(const SlotMetaArray &slots)
{
    File f = GBS_FS.open(SLOTS_FILE, "w");
    f.write((const byte *)&slots, sizeof(slots));
    f.close();
}

// false if there is no slots file yet
bool readSlots(SlotMetaArray &slots)
{
    File f = GBS_FS.open(SLOTS_FILE, "r");
    if (!f) {
        return false;
    }
    f.read((byte *)&slots, sizeof(slots));
    f.close();
    return true;
}

// read the slots file, create it with empty slots if it doesn't exist yet
void readOrCreateSlots(SlotMetaArray &slots)
{
    if (!readSlots(slots)) {
        initEmptySlots(slots);
        writeSlots(slots);
    }
}

// create the slots file with empty slots if it doesn't exist yet
void ensureSlotsFile()
{
    File f = GBS_FS.open(SLOTS_FILE, "r");
    if (f) {
        f.close();
        return;
    }
    SlotMetaArray slots;
    initEmptySlots(slots);
    writeSlots(slots);
}

#if defined(ESP8266)
const uint8_t *loadPresetFromSPIFFS(byte forVideoMode)
{
    static uint8_t preset[432];
    String s = "";
    Ascii8 slot = 0;
    File f;

    f = GBS_FS.open("/preferencesv2.txt", "r");
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

    String fileName = presetFileName(forVideoMode, slot);
    if (fileName.length()) {
        f = GBS_FS.open(fileName, "r");
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
    f = GBS_FS.open("/preferencesv2.txt", "r");
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

    String fileName = presetFileName(rto->videoStandardInput, slot);
    if (fileName.length()) {
        f = GBS_FS.open(fileName, "w");
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
    File f = GBS_FS.open("/preferencesv2.txt", "w");
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

#endif
