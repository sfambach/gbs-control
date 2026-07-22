#pragma once

#include "config.h"
#include "platform_gbs.h"
#include "tv5725.h"
#include "include/options.h"
#include "include/osd.h"
#include "framesync.h"
#include "lib/si5351mcu.h"

typedef TV5725<GBS_ADDR> GBS;

/// Video processing mode, loaded into register GBS_PRESET_ID by applyPresets()
/// and read to rto->presetID by doPostPresetLoadSteps(). Shown on web UI.
enum PresetID : uint8_t {
    PresetHdBypass = 0x21,
    PresetBypassRGBHV = 0x22,
};

struct MenuAttrs
{
    static const int8_t shiftDelta = GBS_MENU_SHIFT_DELTA;
    static const int8_t scaleDelta = GBS_MENU_SCALE_DELTA;
    static const int16_t vertShiftRange = GBS_MENU_VERT_SHIFT_RANGE;
    static const int16_t horizShiftRange = GBS_MENU_HORIZ_SHIFT_RANGE;
    static const int16_t vertScaleRange = GBS_MENU_VERT_SCALE_RANGE;
    static const int16_t horizScaleRange = GBS_MENU_HORIZ_SCALE_RANGE;
    static const int16_t barLength = GBS_MENU_BAR_LENGTH;
};
typedef MenuManager<GBS, MenuAttrs> Menu;

struct FrameSyncAttrs
{
    static const uint8_t debugInPin = DEBUG_IN_PIN;
    static const uint32_t lockInterval = GBS_FRAMESYNC_LOCK_INTERVAL;
    static const int16_t syncCorrection = GBS_FRAMESYNC_CORRECTION;
    static const int32_t syncTargetPhase = GBS_FRAMESYNC_TARGET_PHASE;
};
typedef FrameSyncManager<GBS, FrameSyncAttrs> FrameSync;

extern struct runTimeOptions rtos;
extern struct runTimeOptions *rto;
extern struct userOptions uopts;
extern struct userOptions *uopt;
extern struct adcOptions adcopts;
extern struct adcOptions *adco;
extern Si5351mcu Si;

extern char serialCommand;
extern char userCommand;
extern String slotIndexMap;
extern unsigned long lastVsyncLock;

#define LEDON GBS_LED_ON
#define LEDOFF GBS_LED_OFF

#include "gbs_serial.h"

void myLog(char const *type, char command);
