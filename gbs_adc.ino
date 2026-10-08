// ADC gain, offset and sampling phase
// part of the gbs-control sketch: all .ino files are concatenated into one translation unit
// (gbs-control.ino first), so globals and types from gbs-control.ino are visible here.

/// Write ADC gain registers, and save in adco->r_gain to properly transfer it
/// across loading presets or passthrough.
void setAdcGain(uint8_t gain) {
    // gain is actually range, increasing it dims the image.
    GBS::ADC_RGCTRL::write(gain);
    GBS::ADC_GGCTRL::write(gain);
    GBS::ADC_BGCTRL::write(gain);

    // Save gain for applying preset. (Gain affects passthrough presets, and
    // loading a passthrough preset loads from adco but doesn't save to it.)
    adco->r_gain = gain;
    adco->g_gain = gain;
    adco->b_gain = gain;
}

void setAdcParametersGainAndOffset()
{
    GBS::ADC_ROFCTRL::write(0x40);
    GBS::ADC_GOFCTRL::write(0x40);
    GBS::ADC_BOFCTRL::write(0x40);

    // Do not call setAdcGain() and overwrite adco->r_gain. This function should
    // only be called during startup, or during doPostPresetLoadSteps(), and if
    // `uopt->enableAutoGain == 1` then doPostPresetLoadSteps() will revert
    // these writes with `adco->r_gain`.
    GBS::ADC_RGCTRL::write(0x7B);
    GBS::ADC_GGCTRL::write(0x7B);
    GBS::ADC_BGCTRL::write(0x7B);
}

void setOverSampleRatio(uint8_t newRatio, boolean prepareOnly)
{
    uint8_t ks = GBS::PLLAD_KS::read();

    bool hi_res = rto->videoStandardInput == 8 || rto->videoStandardInput == 4 || rto->videoStandardInput == 3;
    bool bypass = rto->presetID == PresetHdBypass;

    switch (newRatio) {
        case 1:
            if (ks == 0)
                GBS::PLLAD_CKOS::write(0);
            if (ks == 1)
                GBS::PLLAD_CKOS::write(1);
            if (ks == 2)
                GBS::PLLAD_CKOS::write(2);
            if (ks == 3)
                GBS::PLLAD_CKOS::write(3);
            GBS::ADC_CLK_ICLK2X::write(0);
            GBS::ADC_CLK_ICLK1X::write(0);
            GBS::DEC1_BYPS::write(1); // dec1 couples to ADC_CLK_ICLK2X
            GBS::DEC2_BYPS::write(1);

            // Necessary to avoid a 2x-scaled image for some reason.
            if (hi_res && !bypass) {
                GBS::ADC_CLK_ICLK1X::write(1);
                //GBS::DEC2_BYPS::write(0);
            }

            rto->osr = 1;
            //if (!prepareOnly) SerialM.println("OSR 1x");

            break;
        case 2:
            if (ks == 0) {
                setOverSampleRatio(1, false);
                return;
            } // 2x impossible
            if (ks == 1)
                GBS::PLLAD_CKOS::write(0);
            if (ks == 2)
                GBS::PLLAD_CKOS::write(1);
            if (ks == 3)
                GBS::PLLAD_CKOS::write(2);
            GBS::ADC_CLK_ICLK2X::write(0);
            GBS::ADC_CLK_ICLK1X::write(1);
            GBS::DEC2_BYPS::write(0);
            GBS::DEC1_BYPS::write(1); // dec1 couples to ADC_CLK_ICLK2X

            if (hi_res && !bypass) {
                //GBS::ADC_CLK_ICLK2X::write(1);
                //GBS::DEC1_BYPS::write(0);
                // instead increase CKOS by 1 step
                GBS::PLLAD_CKOS::write(GBS::PLLAD_CKOS::read() + 1);
            }

            rto->osr = 2;
            //if (!prepareOnly) SerialM.println("OSR 2x");

            break;
        case 4:
            if (ks == 0) {
                setOverSampleRatio(1, false);
                return;
            } // 4x impossible
            if (ks == 1) {
                setOverSampleRatio(1, false);
                return;
            } // 4x impossible
            if (ks == 2)
                GBS::PLLAD_CKOS::write(0);
            if (ks == 3)
                GBS::PLLAD_CKOS::write(1);
            GBS::ADC_CLK_ICLK2X::write(1);
            GBS::ADC_CLK_ICLK1X::write(1);
            GBS::DEC1_BYPS::write(0); // dec1 couples to ADC_CLK_ICLK2X
            GBS::DEC2_BYPS::write(0);

            rto->osr = 4;
            //if (!prepareOnly) SerialM.println("OSR 4x");

            break;
        default:
            break;
    }

    if (!prepareOnly)
        latchPLLAD();
}

void togglePhaseAdjustUnits()
{
    GBS::PA_SP_BYPSZ::write(0); // yes, 0 means bypass on here
    GBS::PA_SP_BYPSZ::write(1);
    delay(2);
    GBS::PA_ADC_BYPSZ::write(0);
    GBS::PA_ADC_BYPSZ::write(1);
    delay(2);
}

void advancePhase()
{
    rto->phaseADC = (rto->phaseADC + 1) & 0x1f;
    setAndLatchPhaseADC();
}

void movePhaseThroughRange()
{
    for (uint8_t i = 0; i < 128; i++) { // 4x for 4x oversampling?
        advancePhase();
    }
}

void setAndLatchPhaseSP()
{
    GBS::PA_SP_LAT::write(0); // latch off
    GBS::PA_SP_S::write(rto->phaseSP);
    GBS::PA_SP_LAT::write(1); // latch on
}

void setAndLatchPhaseADC()
{
    GBS::PA_ADC_LAT::write(0);
    GBS::PA_ADC_S::write(rto->phaseADC);
    GBS::PA_ADC_LAT::write(1);
}

void nudgeMD()
{
    GBS::MD_VS_FLIP::write(!GBS::MD_VS_FLIP::read());
    GBS::MD_VS_FLIP::write(!GBS::MD_VS_FLIP::read());
}

void runAutoGain()
{
    static unsigned long lastTimeAutoGain = millis();
    uint8_t limit_found = 0, greenValue = 0;
    uint8_t loopCeiling = 0;
    uint8_t status00reg = GBS::STATUS_00::read(); // confirm no mode changes happened

    //GBS::DEC_TEST_SEL::write(5);

    //for (uint8_t i = 0; i < 14; i++) {
    //  uint8_t greenValue = GBS::TEST_BUS_2E::read();
    //  if (greenValue >= 0x28 && greenValue <= 0x2f) {  // 0x2c seems to be "highest" (haven't seen 0x2b yet)
    //    if (getStatus16SpHsStable() && (GBS::STATUS_00::read() == status00reg)) {
    //      limit_found++;
    //    }
    //    else return;
    //  }
    //}

    if ((millis() - lastTimeAutoGain) < 30000) {
        loopCeiling = 61;
    } else {
        loopCeiling = 8;
    }

    for (uint8_t i = 0; i < loopCeiling; i++) {
        if (i % 20 == 0) {
            handleWiFi(0);
            limit_found = 0;
        }
        greenValue = GBS::TEST_BUS_2F::read();

        if (greenValue == 0x7f) {
            if (getStatus16SpHsStable() && (GBS::STATUS_00::read() == status00reg)) {
                limit_found++;
                // 240p test suite (SNES ver): display vertical lines (hor. line test)
                //Serial.print("g: "); Serial.println(greenValue, HEX);
                //Serial.print("--"); Serial.println();
            } else
                return;

            if (limit_found == 2) {
                limit_found = 0;
                uint8_t level = GBS::ADC_GGCTRL::read();
                if (level < 0xfe) {
                    setAdcGain(level + 2);

                    // remember these gain settings
                    adco->r_gain = GBS::ADC_RGCTRL::read();
                    adco->g_gain = GBS::ADC_GGCTRL::read();
                    adco->b_gain = GBS::ADC_BGCTRL::read();

                    printInfo();
                    delay(2); // let it settle a little
                    lastTimeAutoGain = millis();
                }
            }
        }
    }
}

void calibrateAdcOffset()
{
    GBS::PAD_BOUT_EN::write(0);          // disable output to pin for test
    GBS::PLL648_CONTROL_01::write(0xA5); // display clock to adc = 162mhz
    GBS::ADC_INPUT_SEL::write(2);        // 10 > R2/G2/B2 as input (not connected, so to isolate ADC)
    GBS::DEC_MATRIX_BYPS::write(1);
    GBS::DEC_TEST_ENABLE::write(1);
    GBS::ADC_5_03::write(0x31);    // bottom clamps, filter max (40mhz)
    GBS::ADC_TEST_04::write(0x00); // disable bit 1
    GBS::SP_CS_CLP_ST::write(0x00);
    GBS::SP_CS_CLP_SP::write(0x00);
    GBS::SP_5_56::write(0x05); // SP_SOG_MODE needs to be 1
    GBS::SP_5_57::write(0x80);
    GBS::ADC_5_00::write(0x02);
    GBS::TEST_BUS_SEL::write(0x0b); // 0x2b
    GBS::TEST_BUS_EN::write(1);
    resetDigital();

    uint16_t hitTargetCounter = 0;
    uint16_t readout16 = 0;
    uint8_t missTargetCounter = 0;
    uint8_t readout = 0;

    GBS::ADC_RGCTRL::write(0x7F);
    GBS::ADC_GGCTRL::write(0x7F);
    GBS::ADC_BGCTRL::write(0x7F);
    GBS::ADC_ROFCTRL::write(0x7F);
    GBS::ADC_GOFCTRL::write(0x3D); // start
    GBS::ADC_BOFCTRL::write(0x7F);
    GBS::DEC_TEST_SEL::write(1); // 5_1f = 0x1c

    //unsigned long overallTimer = millis();
    unsigned long startTimer = 0;
    for (uint8_t i = 0; i < 3; i++) {
        missTargetCounter = 0;
        hitTargetCounter = 0;
        delay(20);
        startTimer = millis();

        // loop breaks either when the timer runs out, or hitTargetCounter reaches target
        while ((millis() - startTimer) < 800) {
            readout16 = GBS::TEST_BUS::read() & 0x7fff;
            //Serial.println(readout16, HEX);
            // readout16 is unsigned, always >= 0
            if (readout16 < 7) {
                hitTargetCounter++;
                missTargetCounter = 0;
            } else if (missTargetCounter++ > 2) {
                if (i == 0) {
                    GBS::ADC_GOFCTRL::write(GBS::ADC_GOFCTRL::read() + 1); // incr. offset
                    readout = GBS::ADC_GOFCTRL::read();
                    Serial.print(" G: ");
                } else if (i == 1) {
                    GBS::ADC_ROFCTRL::write(GBS::ADC_ROFCTRL::read() + 1);
                    readout = GBS::ADC_ROFCTRL::read();
                    Serial.print(" R: ");
                } else if (i == 2) {
                    GBS::ADC_BOFCTRL::write(GBS::ADC_BOFCTRL::read() + 1);
                    readout = GBS::ADC_BOFCTRL::read();
                    Serial.print(" B: ");
                }
                Serial.print(readout, HEX);

                if (readout >= 0x52) {
                    // some kind of failure
                    break;
                }

                delay(10);
                hitTargetCounter = 0;
                missTargetCounter = 0;
                startTimer = millis(); // extend timer
            }
            if (hitTargetCounter > 1500) {
                break;
            }
        }
        if (i == 0) {
            // G done, prep R
            adco->g_off = GBS::ADC_GOFCTRL::read();
            GBS::ADC_GOFCTRL::write(0x7F);
            GBS::ADC_ROFCTRL::write(0x3D);
            GBS::DEC_TEST_SEL::write(2); // 5_1f = 0x2c
        }
        if (i == 1) {
            adco->r_off = GBS::ADC_ROFCTRL::read();
            GBS::ADC_ROFCTRL::write(0x7F);
            GBS::ADC_BOFCTRL::write(0x3D);
            GBS::DEC_TEST_SEL::write(3); // 5_1f = 0x3c
        }
        if (i == 2) {
            adco->b_off = GBS::ADC_BOFCTRL::read();
        }
        Serial.println("");
    }

    if (readout >= 0x52) {
        // there was a problem; revert
        adco->r_off = adco->g_off = adco->b_off = 0x40;
    }

    GBS::ADC_GOFCTRL::write(adco->g_off);
    GBS::ADC_ROFCTRL::write(adco->r_off);
    GBS::ADC_BOFCTRL::write(adco->b_off);

    //Serial.println(millis() - overallTimer);
}
