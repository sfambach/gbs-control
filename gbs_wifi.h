#pragma once

#include <Arduino.h>

void updateWebSocketData();
void handleWiFi(boolean instant);
void handleType2Command(char argument);
void startWebserver();
void initUpdateOTA();
