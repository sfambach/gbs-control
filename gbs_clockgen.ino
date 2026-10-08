// external Si5351 clock generator
// part of the gbs-control sketch: all .ino files are concatenated into one translation unit
// (gbs-control.ino first), so globals and types from gbs-control.ino are visible here.

void externalClockGenResetClock()
{
    if (!rto->extClockGenDetected) {
        return;
    }
    fsDebugPrintf("externalClockGenResetClock()\n");

    uint8_t activeDisplayClock = GBS::PLL648_CONTROL_01::read();

    if (activeDisplayClock == 0x25)
        rto->freqExtClockGen = 40500000;
    else if (activeDisplayClock == 0x45)
        rto->freqExtClockGen = 54000000;
    else if (activeDisplayClock == 0x55)
        rto->freqExtClockGen = 64800000;
    else if (activeDisplayClock == 0x65)
        rto->freqExtClockGen = 81000000;
    else if (activeDisplayClock == 0x85)
        rto->freqExtClockGen = 108000000;
    else if (activeDisplayClock == 0x95)
        rto->freqExtClockGen = 129600000;
    else if (activeDisplayClock == 0xa5)
        rto->freqExtClockGen = 162000000;
    else if (activeDisplayClock == 0x35)
        rto->freqExtClockGen = 81000000; // clock unused
    else if (activeDisplayClock == 0)
        rto->freqExtClockGen = 81000000; // no preset loaded
    else if (!rto->outModeHdBypass) {
        SerialM.print(F("preset display clock: 0x"));
        SerialM.println(activeDisplayClock, HEX);
    }

    // problem: around 108MHz the library seems to double the clock
    // maybe there are regs to check for this and resetPLL
    if (rto->freqExtClockGen == 108000000) {
        Si.setFreq(0, 87000000);
        delay(1); // quick fix
    }
    // same thing it seems at 40500000
    if (rto->freqExtClockGen == 40500000) {
        Si.setFreq(0, 48500000);
        delay(1); // quick fix
    }

    Si.setFreq(0, rto->freqExtClockGen);
    GBS::PAD_CKIN_ENZ::write(0); // 0 = clock input enable (pin40)
    FrameSync::clearFrequency();

    SerialM.print(F("clock gen reset: "));
    SerialM.println(rto->freqExtClockGen);
}

void externalClockGenSyncInOutRate()
{
    fsDebugPrintf("externalClockGenSyncInOutRate()\n");

    if (!rto->extClockGenDetected) {
        return;
    }
    if (GBS::PAD_CKIN_ENZ::read() != 0) {
        return;
    }
    if (rto->outModeHdBypass) {
        return;
    }
    if (GBS::PLL648_CONTROL_01::read() != 0x75) {
        return;
    }

    float sfr = getSourceFieldRate(0);
    if (sfr < 47.0f || sfr > 86.0f) {
        SerialM.print(F("sync skipped sfr wrong: "));
        SerialM.println(sfr);
        return;
    }

    float ofr = getOutputFrameRate();
    if (ofr < 47.0f || ofr > 86.0f) {
        SerialM.print(F("sync skipped ofr wrong: "));
        SerialM.println(ofr);
        return;
    }

    uint32_t old = rto->freqExtClockGen;
    FrameSync::initFrequency(ofr, old);

    setExternalClockGenFrequencySmooth((sfr / ofr) * rto->freqExtClockGen);

    int32_t diff = rto->freqExtClockGen - old;

    SerialM.print(F("source Hz: "));
    SerialM.print(sfr, 5);
    SerialM.print(F(" new out: "));
    SerialM.print(getOutputFrameRate(), 5);
    SerialM.print(F(" clock: "));
    SerialM.print(rto->freqExtClockGen);
    SerialM.print(F(" ("));
    SerialM.print(diff >= 0 ? "+" : "");
    SerialM.print(diff);
    SerialM.println(F(")"));
    delay(1);
}

void externalClockGenDetectAndInitialize()
{
    const uint8_t xtal_cl = 0xD2; // 10pF, other choices are 8pF (0x92) and 6pF (0x52) NOTE: Per AN619, the low bytes should be written 0b010010

    // MHz: 27, 32.4, 40.5, 54, 64.8, 81, 108, 129.6, 162
    rto->freqExtClockGen = 81000000;
    rto->extClockGenDetected = 0;

    if (uopt->disableExternalClockGenerator) {
        SerialM.println(F("ExternalClockGenerator disabled, skipping detection"));
        return;
    }

    uint8_t retVal = 0;
    Wire.beginTransmission(SIADDR);
    retVal = Wire.endTransmission();

    if (retVal != 0) {
        return;
    }

    Wire.beginTransmission(SIADDR);
    Wire.write(0); // Device Status
    Wire.endTransmission();
    size_t bytes_read = Wire.requestFrom((uint8_t)SIADDR, (size_t)1, false);

    if (bytes_read == 1) {
        retVal = Wire.read();
        if ((retVal & 0x80) == 0) {
            // SYS_INIT indicates device is ready.
            rto->extClockGenDetected = 1;
        } else {
            return;
        }
    } else {
        return;
    }

    Si.init(25000000L); // many Si5351 boards come with 25MHz crystal; 27000000L for one with 27MHz
    Wire.beginTransmission(SIADDR);
    Wire.write(183);    // XTAL_CL
    Wire.write(xtal_cl);
    Wire.endTransmission();
    Si.setPower(0, SIOUT_6mA);
    Si.setFreq(0, rto->freqExtClockGen);
    Si.disable(0);
}
