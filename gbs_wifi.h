#pragma once

#include <Arduino.h>

void updateWebSocketData();
void handleWiFi(boolean instant);
void handleType2Command(char argument);
void startWebserver();
void initUpdateOTA();

#if GBS_ENABLE_WEB_GUI
extern const char ap_info_string[] PROGMEM;
extern const char st_info_string[] PROGMEM;
#endif
