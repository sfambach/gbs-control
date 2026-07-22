#include "gbs_serial.h"

#if GBS_ENABLE_WEB_GUI && defined(ESP8266)

size_t SerialMirror::write(uint8_t data)
{
    if (ESP.getFreeHeap() > 20000) {
        webSocket.broadcastTXT(&data, 1);
    } else {
        webSocket.disconnect();
    }
    return Serial.write(data);
}

size_t SerialMirror::write(const uint8_t *data, size_t size)
{
    if (ESP.getFreeHeap() > 20000) {
        webSocket.broadcastTXT(data, size);
    } else {
        webSocket.disconnect();
    }
    return Serial.write(data, size);
}

int SerialMirror::available()
{
    return 0;
}

int SerialMirror::read()
{
    return -1;
}

int SerialMirror::peek()
{
    return -1;
}

void SerialMirror::flush() {}

SerialMirror SerialM;

#endif
