#ifndef BOARD_D1_MINI32_H_
#define BOARD_D1_MINI32_H_

// Wemos / Lolin "D1 mini ESP32" (ESP32-WROOM in D1 mini form factor)
// Same wiring as the ESP8266 D1 mini by pin label (D1, D2, D6, ...); the GPIO numbers differ:
//   D1 = GPIO22 (SCL), D2 = GPIO21 (SDA), D6 = GPIO19, D5 = GPIO18, D7 = GPIO23, D3 = GPIO17
// See board_d1_mini.h for how to add a board.

#ifndef DEBUG_IN_PIN
#define DEBUG_IN_PIN D6 // GPIO19
#endif

#ifndef OLED_I2C_ADDRESS
#define OLED_I2C_ADDRESS 0x3c
#endif
#ifndef OLED_PIN_SDA
#define OLED_PIN_SDA D2 // GPIO21
#endif
#ifndef OLED_PIN_SCL
#define OLED_PIN_SCL D1 // GPIO22
#endif

// rotary encoder (same labels as on the ESP8266 D1 mini)
#ifndef ENCODER_PIN_CLK
#define ENCODER_PIN_CLK D5 // GPIO18
#endif
#ifndef ENCODER_PIN_DATA
#define ENCODER_PIN_DATA D7 // GPIO23
#endif
#ifndef ENCODER_PIN_SWITCH
#define ENCODER_PIN_SWITCH D3 // GPIO17
#endif

#endif
