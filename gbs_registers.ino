// I2C access to the TV5725 registers
// part of the gbs-control sketch: all .ino files are concatenated into one translation unit
// (gbs-control.ino first), so globals and types from gbs-control.ino are visible here.

static inline void writeOneByte(uint8_t slaveRegister, uint8_t value)
{
    writeBytes(slaveRegister, &value, 1);
}

static inline void writeBytes(uint8_t slaveRegister, uint8_t *values, uint8_t numValues)
{
    if (slaveRegister == 0xF0 && numValues == 1) {
        lastSegment = *values;
    } else
        GBS::write(lastSegment, slaveRegister, values, numValues);
}

void copyBank(uint8_t *bank, const uint8_t *programArray, uint16_t *index)
{
    for (uint8_t x = 0; x < 16; ++x) {
        bank[x] = pgm_read_byte(programArray + *index);
        (*index)++;
    }
}

uint8_t getSingleByteFromPreset(const uint8_t *programArray, unsigned int offset)
{
    return pgm_read_byte(programArray + offset);
}

static inline void readFromRegister(uint8_t reg, int bytesToRead, uint8_t *output)
{
    return GBS::read(lastSegment, reg, output, bytesToRead);
}

void printReg(uint8_t seg, uint8_t reg)
{
    uint8_t readout;
    readFromRegister(reg, 1, &readout);
    // didn't think this HEX trick would work, but it does! (?)
    SerialM.print("0x");
    SerialM.print(readout, HEX);
    SerialM.print(", // s");
    SerialM.print(seg);
    SerialM.print("_");
    SerialM.println(reg, HEX);
    // old:
    //SerialM.print(readout); SerialM.print(", // s"); SerialM.print(seg); SerialM.print("_"); SerialM.println(reg, HEX);
}

// dumps the current chip configuration in a format that's ready to use as new preset :)
void dumpRegisters(byte segment)
{
    if (segment > 5)
        return;
    writeOneByte(0xF0, segment);

    switch (segment) {
        case 0:
            for (int x = 0x40; x <= 0x5F; x++) {
                printReg(0, x);
            }
            for (int x = 0x90; x <= 0x9F; x++) {
                printReg(0, x);
            }
            break;
        case 1:
            for (int x = 0x0; x <= 0x2F; x++) {
                printReg(1, x);
            }
            break;
        case 2:
            for (int x = 0x0; x <= 0x3F; x++) {
                printReg(2, x);
            }
            break;
        case 3:
            for (int x = 0x0; x <= 0x7F; x++) {
                printReg(3, x);
            }
            break;
        case 4:
            for (int x = 0x0; x <= 0x5F; x++) {
                printReg(4, x);
            }
            break;
        case 5:
            for (int x = 0x0; x <= 0x6F; x++) {
                printReg(5, x);
            }
            break;
    }
}

void resetDebugPort()
{
    GBS::PAD_BOUT_EN::write(1); // output to pad enabled
    GBS::IF_TEST_EN::write(1);
    GBS::IF_TEST_SEL::write(3);    // IF vertical period signal
    GBS::TEST_BUS_SEL::write(0xa); // test bus to SP
    GBS::TEST_BUS_EN::write(1);
    GBS::TEST_BUS_SP_SEL::write(0x0f); // SP test signal select (vsync in, after SOG separation)
    GBS::MEM_FF_TOP_FF_SEL::write(1);  // g0g13/14 shows FIFO stats (capture, rff, wff, etc)
    // SP test bus enable bit is included in TEST_BUS_SP_SEL
    GBS::VDS_TEST_EN::write(1); // VDS test enable
}

void readEeprom()
{
    int addr = 0;
    const uint8_t eepromAddr = 0x50;
    Wire.beginTransmission(eepromAddr);
    //if (addr >= 0x1000) { addr = 0; }
    Wire.write(addr >> 8);     // high addr byte, 4 bits +
    Wire.write((uint8_t)addr); // low addr byte, 8 bits = 12 bits (0xfff max)
    Wire.endTransmission();
    Wire.requestFrom(eepromAddr, (uint8_t)128);
    uint8_t readData = 0;
    uint8_t i = 0;
    while (Wire.available()) {
        Serial.print("addr 0x");
        Serial.print(i, HEX);
        Serial.print(": 0x");
        readData = Wire.read();
        Serial.println(readData, HEX);
        //addr++;
        i++;
    }
}

void stopWire()
{
    pinMode(SCL, INPUT);
    pinMode(SDA, INPUT);
    delayMicroseconds(80);
}

void startWire()
{
    Wire.begin();
    // The i2c wire library sets pullup resistors on by default.
    // Disable these to detect/work with GBS onboard pullups
    pinMode(SCL, OUTPUT_OPEN_DRAIN);
    pinMode(SDA, OUTPUT_OPEN_DRAIN);
    // no issues even at 700k, requires ESP8266 160Mhz CPU clock, else (80Mhz) uses 400k in library
    // no problem with Si5351 at 700k either
    Wire.setClock(400000);
    //Wire.setClock(700000);
}
