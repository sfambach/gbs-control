#pragma once

#include "config.h"

#if GBS_ENABLE_WEB_GUI && defined(ESP8266)

#include <Arduino.h>
#include <Stream.h>
#include "lib/WebSocketsServer.h"

extern WebSocketsServer webSocket;

class SerialMirror : public Stream
{
public:
    size_t write(uint8_t data) override;
    size_t write(const uint8_t *data, size_t size) override;
    int available() override;
    int read() override;
    int peek() override;
    void flush() override;
};

extern SerialMirror SerialM;

#else

#define SerialM Serial

#endif
