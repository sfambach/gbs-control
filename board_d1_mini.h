#ifndef BOARD_D1_MINI_H_
#define BOARD_D1_MINI_H_

// Wemos / Lolin D1 mini (ESP8266), also NodeMCU
// Board layer: pins and wiring. To add a board, write a board_<name>.h and select it in config.h
// or with -DGBS_BOARD_HEADER=\"board_<name>.h\" in platformio.ini. Every value can still be
// overridden with -D...

// debug / measurement input from the GBS debug pin
// marked "D12/MISO/D6" (Wemos D1) or D6 (Lolin NodeMCU)
#ifndef DEBUG_IN_PIN
#define DEBUG_IN_PIN D6
#endif

// OLED display (SSD1306): I2C address and pins (same I2C bus as the GBS: SDA = D2, SCL = D1)
#ifndef OLED_I2C_ADDRESS
#define OLED_I2C_ADDRESS 0x3c
#endif
#ifndef OLED_PIN_SDA
#define OLED_PIN_SDA D2
#endif
#ifndef OLED_PIN_SCL
#define OLED_PIN_SCL D1
#endif

// rotary encoder
#ifndef ENCODER_PIN_CLK
#define ENCODER_PIN_CLK 14 // D5 = GPIO14 (input of one direction for encoder)
#endif
#ifndef ENCODER_PIN_DATA
#define ENCODER_PIN_DATA 13 // D7 = GPIO13 (input of one direction for encoder)
#endif
#ifndef ENCODER_PIN_SWITCH
#define ENCODER_PIN_SWITCH 0 // D3 = GPIO0 pulled HIGH, else boot fail (middle push button for encoder)
#endif

#endif
