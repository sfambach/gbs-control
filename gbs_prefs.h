#pragma once

#include <Arduino.h>

void StrClear(char *str, uint16_t length);
const uint8_t *loadPresetFromSPIFFS(byte forVideoMode);
void savePresetToSPIFFS();
void saveUserPrefs();
void loadDefaultUserOptions();
