// declarations of the functions in the gbs_*.ino parts of the sketch
// (included by gbs-control.ino before framesync.h, which already needs some of them)
#pragma once

// gbs_clockgen.ino
void externalClockGenResetClock();
void externalClockGenSyncInOutRate();
void externalClockGenDetectAndInitialize();

// gbs_registers.ino
static inline void writeOneByte(uint8_t slaveRegister, uint8_t value);
static inline void writeBytes(uint8_t slaveRegister, uint8_t *values, uint8_t numValues);
void copyBank(uint8_t *bank, const uint8_t *programArray, uint16_t *index);
uint8_t getSingleByteFromPreset(const uint8_t *programArray, unsigned int offset);
static inline void readFromRegister(uint8_t reg, int bytesToRead, uint8_t *output);
void printReg(uint8_t seg, uint8_t reg);
void dumpRegisters(byte segment);
void resetDebugPort();
void readEeprom();
void stopWire();
void startWire();

// gbs_presets.ino
boolean videoStandardInputIsPalNtscSd();
void zeroAll();
void loadHdBypassSection();
void loadPresetDeinterlacerSection();
void loadPresetMdSection();
void writeProgramArrayNew(const uint8_t *programArray, boolean skipMDSection);
void setResetParameters();
void OutputComponentOrVGA();
void applyComponentColorMixing();
void toggleIfAutoOffset();
void applyYuvPatches();
void applyRGBPatches();
void doPostPresetLoadSteps();
void applyPresets(uint8_t result);
void setOutModeHdBypass(bool regsInitialized);
void bypassModeSwitch_RGBHV();
void loadDefaultUserOptions();

// gbs_storage.ino
static constexpr size_t PRESET_FILE_COUNT = 9; // entries in presetFiles[]
String presetFileNameAt(size_t index, Ascii8 slot);
String presetFileName(uint8_t videoMode, Ascii8 slot);
void initEmptySlots(SlotMetaArray &slots);
void writeSlots(const SlotMetaArray &slots);
bool readSlots(SlotMetaArray &slots);
void readOrCreateSlots(SlotMetaArray &slots);
void ensureSlotsFile();
#if defined(ESP8266) || defined(ESP32)
const uint8_t *loadPresetFromSPIFFS(byte forVideoMode);
void savePresetToSPIFFS();
void saveUserPrefs();
#endif

// gbs_sync.ino
void resetInterruptSogSwitchBit();
void resetInterruptSogBadBit();
void resetInterruptNoHsyncBadBit();
void updateHVSyncEdge();
void prepareSyncProcessor();
void setAndUpdateSogLevel(uint8_t level);
void goLowPowerWithInputDetection();
boolean optimizePhaseSP();
void optimizeSogLevel();
uint8_t inputAndSyncDetect();
void resetPLLAD();
void latchPLLAD();
void resetPLL();
void ResetSDRAM();
void resetDigital();
void resetSyncProcessor();
void resetModeDetect();
void unfreezeVideo();
void freezeVideo();
uint8_t getVideoMode();
boolean getSyncPresent();
boolean getStatus00IfHsVsStable();
boolean getStatus16SpHsStable();
void updateSpDynamic(boolean withCurrentVideoModeCheck);
void updateCoastPosition(boolean autoCoast);
void updateClampPosition();
void fastSogAdjust();
void runSyncWatcher();
boolean checkBoardPower();

// gbs_adc.ino
void setAdcGain(uint8_t gain);
void setAdcParametersGainAndOffset();
void setOverSampleRatio(uint8_t newRatio, boolean prepareOnly);
void togglePhaseAdjustUnits();
void advancePhase();
void movePhaseThroughRange();
void setAndLatchPhaseSP();
void setAndLatchPhaseADC();
void nudgeMD();
void runAutoGain();
void calibrateAdcOffset();

// gbs_geometry.ino
void shiftHorizontal(uint16_t amountToShift, bool subtracting);
void shiftHorizontalLeft();
void shiftHorizontalRight();
void shiftHorizontalLeftIF(uint8_t amount);
void shiftHorizontalRightIF(uint8_t amount);
void scaleHorizontal(uint16_t amountToScale, bool subtracting);
void moveHS(uint16_t amountToAdd, bool subtracting);
void moveVS(uint16_t amountToAdd, bool subtracting);
void invertHS();
void invertVS();
void scaleVertical(uint16_t amountToScale, bool subtracting);
void shiftVertical(uint16_t amountToAdd, bool subtracting);
void shiftVerticalUp();
void shiftVerticalDown();
void shiftVerticalUpIF();
void shiftVerticalDownIF();
void setHSyncStartPosition(uint16_t value);
void setHSyncStopPosition(uint16_t value);
void setMemoryHblankStartPosition(uint16_t value);
void setMemoryHblankStopPosition(uint16_t value);
void setDisplayHblankStartPosition(uint16_t value);
void setDisplayHblankStopPosition(uint16_t value);
void setVSyncStartPosition(uint16_t value);
void setVSyncStopPosition(uint16_t value);
void setMemoryVblankStartPosition(uint16_t value);
void setMemoryVblankStopPosition(uint16_t value);
void setDisplayVblankStartPosition(uint16_t value);
void setDisplayVblankStopPosition(uint16_t value);
uint16_t getCsVsStart();
uint16_t getCsVsStop();
void setCsVsStart(uint16_t start);
void setCsVsStop(uint16_t stop);
void printVideoTimings();
void set_htotal(uint16_t htotal);
void set_vtotal(uint16_t vtotal);
void setIfHblankParameters();

// gbs_frametiming.ino
void activeFrameTimeLockInitialSteps();
void fastGetBestHtotal();
boolean runAutoBestHTotal();
boolean applyBestHTotal(uint16_t bestHTotal);
float getSourceFieldRate(boolean useSPBus);
float getOutputFrameRate();
uint32_t getPllRate();
boolean snapToIntegralFrameRate(void);

// gbs_features.ino
uint8_t getMovingAverage(uint8_t item);
void enableScanlines();
void disableScanlines();
void enableMotionAdaptDeinterlace();
void disableMotionAdaptDeinterlace();
void printInfo();

// gbs_api.ino
#if ENABLE_WIFI
void startApi();
#endif

// gbs_web.ino
void updateWebSocketData();
void handleWiFi(boolean instant);
#if defined(ESP8266) || defined(ESP32)
#if ENABLE_WIFI
void startWebserver();
void initUpdateOTA();
#endif
#endif

// gbs_commands.ino
void handleSerialCommand();
void discardSerialRxData();
void myLog(char const* type, char command);
#if defined(ESP8266) || defined(ESP32)
void handleType2Command(char argument);
#endif

// gbs_ui.ino
#if !USE_NEW_OLED_MENU
void GBS_IRAM_ATTR isrRotaryEncoder();
#endif
#if USE_NEW_OLED_MENU
void GBS_IRAM_ATTR isrRotaryEncoderRotateForNewMenu();
void GBS_IRAM_ATTR isrRotaryEncoderPushForNewMenu();
#endif
#if HAVE_BUTTONS
uint8_t readButtons(void);
void debounceButtons(void);
bool buttonDown(uint8_t pos);
void handleButtons(void);
#endif
#if defined(ESP8266) || defined(ESP32)
void StrClear(char *str, uint16_t length);
#endif
#if !USE_NEW_OLED_MENU
void settingsMenuOLED();
void pointerfunction();
void subpointerfunction();
#endif
