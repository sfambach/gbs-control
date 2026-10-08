// frame rate measurement, best htotal, frame time lock
// part of the gbs-control sketch: all .ino files are concatenated into one translation unit
// (gbs-control.ino first), so globals and types from gbs-control.ino are visible here.

void activeFrameTimeLockInitialSteps()
{
    // skip if using external clock gen
    if (rto->extClockGenDetected) {
        SerialM.println(F("Active FrameTime Lock enabled, adjusting external clock gen frequency"));
        return;
    }
    // skip when out mode = bypass
    if (rto->presetID != PresetHdBypass && rto->presetID != PresetBypassRGBHV) {
        SerialM.print(F("Active FrameTime Lock enabled, disable if display unstable or stays blank! Method: "));
        if (uopt->frameTimeLockMethod == 0) {
            SerialM.println(F("0 (vtotal + VSST)"));
        }
        if (uopt->frameTimeLockMethod == 1) {
            SerialM.println(F("1 (vtotal only)"));
        }
        if (GBS::VDS_VS_ST::read() == 0) {
            // VS_ST needs to be at least 1, so method 1 can decrease it when needed (but currently only increases VS_ST)
            // don't force this here, instead make sure to have all presets follow the rule (easier dev)
            SerialM.println(F("Warning: Check VDS_VS_ST!"));
        }
    }
}

void fastGetBestHtotal()
{
    uint32_t inStart, inStop;
    signed long inPeriod = 1;
    double inHz = 1.0;
    GBS::TEST_BUS_SEL::write(0xa);
    if (FrameSync::vsyncInputSample(&inStart, &inStop)) {
        inPeriod = (inStop - inStart) >> 1;
        if (inPeriod > 1) {
            inHz = (double)1000000 / (double)inPeriod;
        }
        SerialM.print("inPeriod: ");
        SerialM.println(inPeriod);
        SerialM.print("in hz: ");
        SerialM.println(inHz);
    } else {
        SerialM.println("error");
    }

    uint16_t newVtotal = GBS::VDS_VSYNC_RST::read();
    double bestHtotal = 108000000 / ((double)newVtotal * inHz); // 107840000
    double bestHtotal50 = 108000000 / ((double)newVtotal * 50);
    double bestHtotal60 = 108000000 / ((double)newVtotal * 60);
    SerialM.print("newVtotal: ");
    SerialM.println(newVtotal);
    // display clock probably not exact 108mhz
    SerialM.print("bestHtotal: ");
    SerialM.println(bestHtotal);
    SerialM.print("bestHtotal50: ");
    SerialM.println(bestHtotal50);
    SerialM.print("bestHtotal60: ");
    SerialM.println(bestHtotal60);
    if (bestHtotal > 800 && bestHtotal < 3200) {
        //applyBestHTotal((uint16_t)bestHtotal);
        //FrameSync::resetWithoutRecalculation(); // was single use of this function, function has changed since
    }
}

boolean runAutoBestHTotal()
{
    if (!FrameSync::ready() && rto->autoBestHtotalEnabled == true && rto->videoStandardInput > 0 && rto->videoStandardInput < 15) {

        //Serial.println("running");
        //unsigned long startTime = millis();

        boolean stableNow = 1;

        for (uint8_t i = 0; i < 64; i++) {
            if (!getStatus16SpHsStable()) {
                stableNow = 0;
                //Serial.println("prevented: !getStatus16SpHsStable");
                break;
            }
        }

        if (stableNow) {
            if (GBS::STATUS_INT_SOG_BAD::read()) {
                //Serial.println("prevented_2!");
                resetInterruptSogBadBit();
                delay(40);
                stableNow = false;
            }
            resetInterruptSogBadBit();

            if (stableNow && (getVideoMode() == rto->videoStandardInput)) {
                uint8_t testBusSelBackup = GBS::TEST_BUS_SEL::read();
                uint8_t vdsBusSelBackup = GBS::VDS_TEST_BUS_SEL::read();
                uint8_t ifBusSelBackup = GBS::IF_TEST_SEL::read();

                if (testBusSelBackup != 0)
                    GBS::TEST_BUS_SEL::write(0); // needs decimation + if
                if (vdsBusSelBackup != 0)
                    GBS::VDS_TEST_BUS_SEL::write(0); // VDS test # 0 = VBlank
                if (ifBusSelBackup != 3)
                    GBS::IF_TEST_SEL::write(3); // IF averaged frame time

                yield();
                uint16_t bestHTotal = FrameSync::init(); // critical task
                yield();

                GBS::TEST_BUS_SEL::write(testBusSelBackup); // always restore from backup (TB has changed)
                if (vdsBusSelBackup != 0)
                    GBS::VDS_TEST_BUS_SEL::write(vdsBusSelBackup);
                if (ifBusSelBackup != 3)
                    GBS::IF_TEST_SEL::write(ifBusSelBackup);

                if (GBS::STATUS_INT_SOG_BAD::read()) {
                    //Serial.println("prevented_5 INT_SOG_BAD!");
                    stableNow = false;
                }
                for (uint8_t i = 0; i < 16; i++) {
                    if (!getStatus16SpHsStable()) {
                        stableNow = 0;
                        //Serial.println("prevented_5: !getStatus16SpHsStable");
                        break;
                    }
                }
                resetInterruptSogBadBit();

                if (bestHTotal > 4095) {
                    if (!rto->forceRetime) {
                        stableNow = false;
                    } else {
                        // roll with it
                        bestHTotal = 4095;
                    }
                }

                if (stableNow) {
                    for (uint8_t i = 0; i < 24; i++) {
                        delay(1);
                        if (!getStatus16SpHsStable()) {
                            stableNow = false;
                            //Serial.println("prevented_3!");
                            break;
                        }
                    }
                }

                if (bestHTotal > 0 && stableNow) {
                    boolean success = applyBestHTotal(bestHTotal);
                    if (success) {
                        rto->syncLockFailIgnore = 16;
                        //Serial.print("ok, took: ");
                        //Serial.println(millis() - startTime);
                        return true; // success
                    }
                }
            }
        }

        // reaching here can happen even if stableNow == 1
        if (!stableNow) {
            FrameSync::reset(uopt->frameTimeLockMethod);

            if (rto->syncLockFailIgnore > 0) {
                rto->syncLockFailIgnore--;
                if (rto->syncLockFailIgnore == 0) {
                    GBS::DAC_RGBS_PWDNZ::write(1); // xth chance
                    if (!uopt->wantOutputComponent) {
                        GBS::PAD_SYNC_OUT_ENZ::write(0); // enable sync out // xth chance
                    }
                    rto->autoBestHtotalEnabled = false;
                }
            }
            Serial.print(F("bestHtotal retry ("));
            Serial.print(rto->syncLockFailIgnore);
            Serial.println(")");
        }
    } else if (FrameSync::ready()) {
        // FS ready but mode is 0 or 15 or autoBestHtotal is off
        return true;
    }

    if (rto->continousStableCounter != 0 && rto->continousStableCounter != 255) {
        rto->continousStableCounter++; // stop repetitions
    }

    return false;
}

boolean applyBestHTotal(uint16_t bestHTotal)
{
    if (rto->outModeHdBypass) {
        return true; // false? doesn't matter atm
    }

    uint16_t orig_htotal = GBS::VDS_HSYNC_RST::read();
    int diffHTotal = bestHTotal - orig_htotal;
    uint16_t diffHTotalUnsigned = abs(diffHTotal);

    if (((diffHTotalUnsigned == 0) || (rto->extClockGenDetected && diffHTotalUnsigned == 1)) && // all this
        !rto->forceRetime)                                                                      // and that
    {
        if (!uopt->enableFrameTimeLock) { // FTL can double throw this when it resets to adjust
            SerialM.print(F("HTotal Adjust (skipped)"));

            if (!rto->extClockGenDetected) {
                float sfr = getSourceFieldRate(0);
                yield(); // wifi
                float ofr = getOutputFrameRate();
                if (sfr < 1.0f) {
                    sfr = getSourceFieldRate(0); // retry
                }
                if (ofr < 1.0f) {
                    ofr = getOutputFrameRate(); // retry
                }
                SerialM.print(F(", source Hz: "));
                SerialM.print(sfr, 3); // prec. 3
                SerialM.print(F(", output Hz: "));
                SerialM.println(ofr, 3); // prec. 3
            } else {
                SerialM.println();
            }
        }
        return true; // nothing to do
    }

    if (GBS::GBS_OPTION_PALFORCED60_ENABLED::read() == 1) {
        // source is 50Hz, preset has to stay at 60Hz: return
        return true;
    }

    if (uopt->PalForce60 && GBS::GBS_OPTION_SCALING_RGBHV::read() == 1) {
        // same option for upscaled RGBHV sources (e.g. DOS 70Hz): many displays and capture devices
        // accept the upscaled resolutions only at 60Hz, so keep the preset frame rate
        SerialM.println(F("HTotal Adjust skipped (force 60Hz)"));
        return true;
    }

    boolean isLargeDiff = (diffHTotalUnsigned > (orig_htotal * 0.06f)) ? true : false; // typical diff: 1802 to 1794 (=8)

    if (isLargeDiff && (getVideoMode() == 8 || rto->videoStandardInput == 14)) {
        // arcade stuff syncs down from 60 to 52 Hz..
        isLargeDiff = (diffHTotalUnsigned > (orig_htotal * 0.16f)) ? true : false;
    }

    if (isLargeDiff) {
        SerialM.println(F("ABHT: large diff"));
    }

    // rto->forceRetime = true means the correction should be forced (command '.')
    if (isLargeDiff && (rto->forceRetime == false)) {
        if (rto->videoStandardInput != 14) {
            rto->failRetryAttempts++;
            if (rto->failRetryAttempts < 8) {
                SerialM.println(F("retry"));
                FrameSync::reset(uopt->frameTimeLockMethod);
                delay(60);
            } else {
                SerialM.println(F("give up"));
                rto->autoBestHtotalEnabled = false;
            }
        }
        return false; // large diff, no forced
    }

    // bestHTotal 0? could be an invald manual retime
    if (bestHTotal == 0) {
        Serial.println(F("bestHTotal 0"));
        return false;
    }

    if (rto->forceRetime == false) {
        if (GBS::STATUS_INT_SOG_BAD::read() == 1) {
            //Serial.println("prevented in apply");
            return false;
        }
    }

    rto->failRetryAttempts = 0; // else all okay!, reset to 0

    // move blanking (display)
    uint16_t h_blank_display_start_position = GBS::VDS_DIS_HB_ST::read();
    uint16_t h_blank_display_stop_position = GBS::VDS_DIS_HB_SP::read();
    uint16_t h_blank_memory_start_position = GBS::VDS_HB_ST::read();
    uint16_t h_blank_memory_stop_position = GBS::VDS_HB_SP::read();

    // new 25.10.2019
    //uint16_t blankingAreaTotal = bestHTotal * 0.233f;
    //h_blank_display_start_position += (diffHTotal / 2);
    //h_blank_display_stop_position += (diffHTotal / 2);
    //h_blank_memory_start_position = bestHTotal - (blankingAreaTotal * 0.212f);
    //h_blank_memory_stop_position = blankingAreaTotal * 0.788f;

    // h_blank_memory_start_position usually is == h_blank_display_start_position
    if (h_blank_memory_start_position == h_blank_display_start_position) {
        h_blank_display_start_position += (diffHTotal / 2);
        h_blank_display_stop_position += (diffHTotal / 2);
        h_blank_memory_start_position = h_blank_display_start_position; // normal case
        h_blank_memory_stop_position += (diffHTotal / 2);
    } else {
        h_blank_display_start_position += (diffHTotal / 2);
        h_blank_display_stop_position += (diffHTotal / 2);
        h_blank_memory_start_position += (diffHTotal / 2); // the exception (currently 1280x1024)
        h_blank_memory_stop_position += (diffHTotal / 2);
    }

    if (diffHTotal < 0) {
        h_blank_display_start_position &= 0xfffe;
        h_blank_display_stop_position &= 0xfffe;
        h_blank_memory_start_position &= 0xfffe;
        h_blank_memory_stop_position &= 0xfffe;
    } else if (diffHTotal > 0) {
        h_blank_display_start_position += 1;
        h_blank_display_start_position &= 0xfffe;
        h_blank_display_stop_position += 1;
        h_blank_display_stop_position &= 0xfffe;
        h_blank_memory_start_position += 1;
        h_blank_memory_start_position &= 0xfffe;
        h_blank_memory_stop_position += 1;
        h_blank_memory_stop_position &= 0xfffe;
    }

    // don't move HSync with small diffs
    uint16_t h_sync_start_position = GBS::VDS_HS_ST::read();
    uint16_t h_sync_stop_position = GBS::VDS_HS_SP::read();

    // fix over / underflows
    if (h_blank_display_start_position > (bestHTotal - 8) || isLargeDiff) {
        // typically happens when scaling Hz up (60 to 70)
        //Serial.println("overflow h_blank_display_start_position");
        h_blank_display_start_position = bestHTotal * 0.936f;
    }
    if (h_blank_display_stop_position > bestHTotal || isLargeDiff) {
        //Serial.println("overflow h_blank_display_stop_position");
        h_blank_display_stop_position = bestHTotal * 0.178f;
    }
    if ((h_blank_memory_start_position > bestHTotal) || (h_blank_memory_start_position > h_blank_display_start_position) || isLargeDiff) {
        //Serial.println("overflow h_blank_memory_start_position");
        h_blank_memory_start_position = h_blank_display_start_position * 0.971f;
    }
    if (h_blank_memory_stop_position > bestHTotal || isLargeDiff) {
        //Serial.println("overflow h_blank_memory_stop_position");
        h_blank_memory_stop_position = h_blank_display_stop_position * 0.64f;
    }

    // check whether HS spills over HBSPD
    if (h_sync_start_position > h_sync_stop_position && (h_sync_start_position < (bestHTotal / 2))) { // is neg HSync
        if (h_sync_start_position >= h_blank_display_stop_position) {
            h_sync_start_position = h_blank_display_stop_position * 0.8f;
            h_sync_stop_position = 4; // good idea to move this close to 0 as well
        }
    } else {
        if (h_sync_stop_position >= h_blank_display_stop_position) {
            h_sync_stop_position = h_blank_display_stop_position * 0.8f;
            h_sync_start_position = 4; //
        }
    }

    // just fix HS
    if (isLargeDiff) {
        if (h_sync_start_position > h_sync_stop_position && (h_sync_start_position < (bestHTotal / 2))) { // is neg HSync
            h_sync_stop_position = 4;
            // stop = at least start, then a bit outwards
            h_sync_start_position = 16 + (h_blank_display_stop_position * 0.3f);
        } else {
            h_sync_start_position = 4;
            h_sync_stop_position = 16 + (h_blank_display_stop_position * 0.3f);
        }
    }

    // finally, fix forced timings with large diff
    // update: doesn't seem necessary anymore
    //if (isLargeDiff) {
    //  h_blank_display_start_position = bestHTotal * 0.946f;
    //  h_blank_display_stop_position = bestHTotal * 0.22f;
    //  h_blank_memory_start_position = h_blank_display_start_position * 0.971f;
    //  h_blank_memory_stop_position = h_blank_display_stop_position * 0.64f;

    //  if (h_sync_start_position > h_sync_stop_position && (h_sync_start_position < (bestHTotal / 2))) { // is neg HSync
    //    h_sync_stop_position = 0;
    //    // stop = at least start, then a bit outwards
    //    h_sync_start_position = 16 + (h_blank_display_stop_position * 0.4f);
    //  }
    //  else {
    //    h_sync_start_position = 0;
    //    h_sync_stop_position = 16 + (h_blank_display_stop_position * 0.4f);
    //  }
    //}

    if (diffHTotal != 0) { // apply
        // delay the change to field start, a bit more compatible
        uint16_t timeout = 0;
        while ((GBS::STATUS_VDS_FIELD::read() == 1) && (++timeout < 400))
            ;
        while ((GBS::STATUS_VDS_FIELD::read() == 0) && (++timeout < 800))
            ;

        GBS::VDS_HSYNC_RST::write(bestHTotal);
        GBS::VDS_DIS_HB_ST::write(h_blank_display_start_position);
        GBS::VDS_DIS_HB_SP::write(h_blank_display_stop_position);
        GBS::VDS_HB_ST::write(h_blank_memory_start_position);
        GBS::VDS_HB_SP::write(h_blank_memory_stop_position);
        GBS::VDS_HS_ST::write(h_sync_start_position);
        GBS::VDS_HS_SP::write(h_sync_stop_position);
    }

    boolean print = 1;
    if (uopt->enableFrameTimeLock) {
        if ((GBS::GBS_RUNTIME_FTL_ADJUSTED::read() == 1) && !rto->forceRetime) {
            // FTL enabled and regular update, so don't print
            print = 0;
        }
        GBS::GBS_RUNTIME_FTL_ADJUSTED::write(0);
    }

    rto->forceRetime = false;

    if (print) {
        SerialM.print(F("HTotal Adjust: "));
        if (diffHTotal >= 0) {
            SerialM.print(" "); // formatting to align with negative value readouts
        }
        SerialM.print(diffHTotal);

        if (!rto->extClockGenDetected) {
            float sfr = getSourceFieldRate(0);
            delay(0);
            float ofr = getOutputFrameRate();
            if (sfr < 1.0f) {
                sfr = getSourceFieldRate(0); // retry
            }
            if (ofr < 1.0f) {
                ofr = getOutputFrameRate(); // retry
            }
            SerialM.print(F(", source Hz: "));
            SerialM.print(sfr, 3); // prec. 3
            SerialM.print(F(", output Hz: "));
            SerialM.println(ofr, 3); // prec. 3
        } else {
            SerialM.println();
        }
    }

    return true;
}

float getSourceFieldRate(boolean useSPBus)
{
    double esp8266_clock_freq = ESP.getCpuFreqMHz() * 1000000;
    uint8_t testBusSelBackup = GBS::TEST_BUS_SEL::read();
    uint8_t spBusSelBackup = GBS::TEST_BUS_SP_SEL::read();
    uint8_t ifBusSelBackup = GBS::IF_TEST_SEL::read();
    uint8_t debugPinBackup = GBS::PAD_BOUT_EN::read();

    if (debugPinBackup != 1)
        GBS::PAD_BOUT_EN::write(1); // enable output to pin for test

    if (ifBusSelBackup != 3)
        GBS::IF_TEST_SEL::write(3); // IF averaged frame time

    if (useSPBus) {
        if (rto->syncTypeCsync) {
            //Serial.println("TestBus for csync");
            if (testBusSelBackup != 0xa)
                GBS::TEST_BUS_SEL::write(0xa);
        } else {
            //Serial.println("TestBus for HV Sync");
            if (testBusSelBackup != 0x0)
                GBS::TEST_BUS_SEL::write(0x0); // RGBHV: TB 0x20 has vsync
        }
        if (spBusSelBackup != 0x0f)
            GBS::TEST_BUS_SP_SEL::write(0x0f);
    } else {
        if (testBusSelBackup != 0)
            GBS::TEST_BUS_SEL::write(0); // needs decimation + if
    }

    float retVal = 0;

    uint32_t fieldTimeTicks = FrameSync::getPulseTicks();
    if (fieldTimeTicks == 0) {
        // try again
        fieldTimeTicks = FrameSync::getPulseTicks();
    }

    if (fieldTimeTicks > 0) {
        retVal = esp8266_clock_freq / (double)fieldTimeTicks;
        if (retVal < 47.0f || retVal > 86.0f) {
            // try again
            fieldTimeTicks = FrameSync::getPulseTicks();
            if (fieldTimeTicks > 0) {
                retVal = esp8266_clock_freq / (double)fieldTimeTicks;
            }
        }
    }

    GBS::TEST_BUS_SEL::write(testBusSelBackup);
    GBS::PAD_BOUT_EN::write(debugPinBackup);
    if (spBusSelBackup != 0x0f)
        GBS::TEST_BUS_SP_SEL::write(spBusSelBackup);
    if (ifBusSelBackup != 3)
        GBS::IF_TEST_SEL::write(ifBusSelBackup);

    return retVal;
}

float getOutputFrameRate()
{
    double esp8266_clock_freq = ESP.getCpuFreqMHz() * 1000000;
    uint8_t testBusSelBackup = GBS::TEST_BUS_SEL::read();
    uint8_t debugPinBackup = GBS::PAD_BOUT_EN::read();

    if (debugPinBackup != 1)
        GBS::PAD_BOUT_EN::write(1); // enable output to pin for test

    if (testBusSelBackup != 2)
        GBS::TEST_BUS_SEL::write(2); // 0x4d = 0x22 VDS test

    float retVal = 0;

    uint32_t fieldTimeTicks = FrameSync::getPulseTicks();
    if (fieldTimeTicks == 0) {
        // try again
        fieldTimeTicks = FrameSync::getPulseTicks();
    }

    if (fieldTimeTicks > 0) {
        retVal = esp8266_clock_freq / (double)fieldTimeTicks;
        if (retVal < 47.0f || retVal > 86.0f) {
            // try again
            fieldTimeTicks = FrameSync::getPulseTicks();
            if (fieldTimeTicks > 0) {
                retVal = esp8266_clock_freq / (double)fieldTimeTicks;
            }
        }
    }

    GBS::TEST_BUS_SEL::write(testBusSelBackup);
    GBS::PAD_BOUT_EN::write(debugPinBackup);

    return retVal;
}

// used for RGBHV to determine the ADPLL speed "level" / can jitter with SOG Sync
uint32_t getPllRate()
{
    uint32_t esp8266_clock_freq = ESP.getCpuFreqMHz() * 1000000;
    uint8_t testBusSelBackup = GBS::TEST_BUS_SEL::read();
    uint8_t spBusSelBackup = GBS::TEST_BUS_SP_SEL::read();
    uint8_t debugPinBackup = GBS::PAD_BOUT_EN::read();

    if (testBusSelBackup != 0xa) {
        GBS::TEST_BUS_SEL::write(0xa);
    }
    if (rto->syncTypeCsync) {
        if (spBusSelBackup != 0x6b)
            GBS::TEST_BUS_SP_SEL::write(0x6b);
    } else {
        if (spBusSelBackup != 0x09)
            GBS::TEST_BUS_SP_SEL::write(0x09);
    }
    GBS::PAD_BOUT_EN::write(1); // enable output to pin for test
    yield();                    // BOUT signal and wifi
    delayMicroseconds(200);
    uint32_t ticks = FrameSync::getPulseTicks();

    // restore
    GBS::PAD_BOUT_EN::write(debugPinBackup);
    if (testBusSelBackup != 0xa) {
        GBS::TEST_BUS_SEL::write(testBusSelBackup);
    }
    GBS::TEST_BUS_SP_SEL::write(spBusSelBackup);

    uint32_t retVal = 0;
    if (ticks > 0) {
        retVal = esp8266_clock_freq / ticks;
    }

    return retVal;
}

boolean snapToIntegralFrameRate(void)
{

    // Fetch the current output frame rate
    float ofr = getOutputFrameRate();

    if (ofr < 1.0f) {
        delay(1);
        ofr = getOutputFrameRate();
    }

    float target;
    if (ofr > 56.5f && ofr < 64.5f) {
        target = 60.0f; // NTSC like
    } else if (ofr > 46.5f && ofr < 54.5f) {
        target = 50.0f; // PAL like
    } else {
        // too far out of spec for an auto adjust
        SerialM.println(F("out of bounds"));
        return false;
    }

    SerialM.print(F("Snap to "));
    SerialM.print(target, 1); // precission 1
    SerialM.println("Hz");

    // We'll be adjusting the htotal incrementally, so store current and best match.
    uint16_t currentHTotal = GBS::VDS_HSYNC_RST::read();
    uint16_t closestHTotal = currentHTotal;

    // What's the closest we've been to the frame rate?
    float closestDifference = fabs(target - ofr);

    // Repeatedly adjust htotals until we find the closest match.
    for (;;) {

        delay(0);

        // Try to move closer to the desired framerate.
        if (target > ofr) {
            if (currentHTotal > 0 && applyBestHTotal(currentHTotal - 1)) {
                --currentHTotal;
            } else {
                return false;
            }
        } else if (target < ofr) {
            if (currentHTotal < 4095 && applyBestHTotal(currentHTotal + 1)) {
                ++currentHTotal;
            } else {
                return false;
            }
        } else {
            return true;
        }

        // Are we closer?
        ofr = getOutputFrameRate();

        if (ofr < 1.0f) {
            delay(1);
            ofr = getOutputFrameRate();
        }
        if (ofr < 1.0f) {
            return false;
        }

        // If we're getting closer, continue trying, otherwise break out of the test loop.
        float newDifference = fabs(target - ofr);
        if (newDifference < closestDifference) {
            closestDifference = newDifference;
            closestHTotal = currentHTotal;
        } else {
            break;
        }
    }

    // Reapply the closest htotal if need be.
    if (closestHTotal != currentHTotal) {
        applyBestHTotal(closestHTotal);
    }

    return true;
}
