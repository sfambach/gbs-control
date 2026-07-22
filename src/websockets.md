arduinoWebSockets **2.7.2** ([8d0744e](https://github.com/Links2004/arduinoWebSockets/commit/8d0744eb5e916ec646d83bd1ffed5f643aab04d8)), vendored server subset in `src/`.

gbs-control patches vs upstream:

- `WebSockets.h` — ESP8266 uses `NETWORK_ESP8266_ASYNC` (matches ESPAsyncTCP / no `webSocket.loop()`).
- `WebSocketsServer.h` — includes `config.h`; `WEBSOCKETS_SERVER_CLIENT_MAX` from project config.

Reference submodule: `3rdparty/WebSockets` @ tag **2.7.2**.
