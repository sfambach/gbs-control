// scanlines, deinterlacer and info output
// part of the gbs-control sketch: all .ino files are concatenated into one translation unit
// (gbs-control.ino first), so globals and types from gbs-control.ino are visible here.

// feed the current measurement, get back the moving average
uint8_t getMovingAverage(uint8_t item)
{
    static const uint8_t sz = 16;
    static uint8_t arr[sz] = {0};
    static uint8_t pos = 0;

    arr[pos] = item;
    if (pos < (sz - 1)) {
        pos++;
    } else {
        pos = 0;
    }

    uint16_t sum = 0;
    for (uint8_t i = 0; i < sz; i++) {
        sum += arr[i];
    }
    return sum >> 4; // for array size 16
}

void enableScanlines()
{
    if (GBS::GBS_OPTION_SCANLINES_ENABLED::read() == 0) {
        //SerialM.println("enableScanlines())");

        //GBS::RFF_ADR_ADD_2::write(0);
        //GBS::RFF_REQ_SEL::write(1);
        //GBS::RFF_MASTER_FLAG::write(0x3f);
        //GBS::RFF_WFF_OFFSET::write(0); // scanline fix
        //GBS::RFF_FETCH_NUM::write(0);
        //GBS::RFF_ENABLE::write(1); //GBS::WFF_ENABLE::write(1);
        //delay(10);
        //GBS::RFF_ENABLE::write(0); //GBS::WFF_ENABLE::write(0);

        // following lines set up UV deinterlacer (on top of normal Y)
        GBS::MADPT_UVDLY_PD_SP::write(0);     // 2_39 0..3
        GBS::MADPT_UVDLY_PD_ST::write(0);     // 2_39 4..7
        GBS::MADPT_EN_UV_DEINT::write(1);     // 2_3a 0
        GBS::MADPT_UV_MI_DET_BYPS::write(1);  // 2_3a 7 enables 2_3b adjust
        GBS::MADPT_UV_MI_OFFSET::write(uopt->scanlineStrength); // 2_3b offset (mixing factor here)
        GBS::MADPT_MO_ADP_UV_EN::write(1);    // 2_16 5 (try to do this some other way?)

        GBS::DIAG_BOB_PLDY_RAM_BYPS::write(0); // 2_00 7 enabled, looks better
        GBS::MADPT_PD_RAM_BYPS::write(0);      // 2_24 2
        GBS::RFF_YUV_DEINTERLACE::write(1);    // scanline fix 2
        GBS::MADPT_Y_MI_DET_BYPS::write(1);    // make sure, so that mixing works
        //GBS::VDS_Y_GAIN::write(GBS::VDS_Y_GAIN::read() + 0x30); // more luma gain
        //GBS::VDS_Y_OFST::write(GBS::VDS_Y_OFST::read() + 4);
        GBS::VDS_WLEV_GAIN::write(0x08);                       // 3_58
        GBS::VDS_W_LEV_BYPS::write(0);                         // brightness
        GBS::MADPT_VIIR_COEF::write(0x08);                     // 2_27 VIIR filter strength
        GBS::MADPT_Y_MI_OFFSET::write(uopt->scanlineStrength); // 2_0b offset (mixing factor here)
        GBS::MADPT_VIIR_BYPS::write(0);                        // 2_26 6 VIIR line fifo // 1 = off
        GBS::RFF_LINE_FLIP::write(1);                          // clears potential garbage in rff buffer

        GBS::MAPDT_VT_SEL_PRGV::write(0);
        GBS::GBS_OPTION_SCANLINES_ENABLED::write(1);
    }
    rto->scanlinesEnabled = 1;
}

void disableScanlines()
{
    if (GBS::GBS_OPTION_SCANLINES_ENABLED::read() == 1) {
        //SerialM.println("disableScanlines())");
        GBS::MAPDT_VT_SEL_PRGV::write(1);

        // following lines set up UV deinterlacer (on top of normal Y)
        GBS::MADPT_UVDLY_PD_SP::write(4); // 2_39 0..3
        GBS::MADPT_UVDLY_PD_ST::write(4); // 2_39 4..77
        GBS::MADPT_EN_UV_DEINT::write(0);     // 2_3a 0
        GBS::MADPT_UV_MI_DET_BYPS::write(0);  // 2_3a 7 enables 2_3b adjust
        GBS::MADPT_UV_MI_OFFSET::write(4);    // 2_3b
        GBS::MADPT_MO_ADP_UV_EN::write(0);    // 2_16 5

        GBS::DIAG_BOB_PLDY_RAM_BYPS::write(1); // 2_00 7
        GBS::VDS_W_LEV_BYPS::write(1);         // brightness
        //GBS::VDS_Y_GAIN::write(GBS::VDS_Y_GAIN::read() - 0x30);
        //GBS::VDS_Y_OFST::write(GBS::VDS_Y_OFST::read() - 4);
        GBS::MADPT_Y_MI_OFFSET::write(0xff); // 2_0b offset 0xff disables mixing
        GBS::MADPT_VIIR_BYPS::write(1);      // 2_26 6 disable VIIR
        GBS::MADPT_PD_RAM_BYPS::write(1);
        GBS::RFF_LINE_FLIP::write(0); // back to default

        GBS::GBS_OPTION_SCANLINES_ENABLED::write(0);
    }
    rto->scanlinesEnabled = 0;
}

void enableMotionAdaptDeinterlace()
{
    freezeVideo();
    GBS::DEINT_00::write(0x19);          // 2_00 // bypass angular (else 0x00)
    GBS::MADPT_Y_MI_OFFSET::write(0x00); // 2_0b  // also used for scanline mixing
    //GBS::MADPT_STILL_NOISE_EST_EN::write(1); // 2_0A 5 (was 0 before)
    GBS::MADPT_Y_MI_DET_BYPS::write(0); //2_0a_7  // switch to automatic motion indexing
    //GBS::MADPT_UVDLY_PD_BYPS::write(0); // 2_35_5 // UVDLY
    //GBS::MADPT_EN_UV_DEINT::write(0);   // 2_3a 0
    //GBS::MADPT_EN_STILL_FOR_NRD::write(1); // 2_3a 3 (new)

    if (rto->videoStandardInput == 1)
        GBS::MADPT_VTAP2_COEFF::write(6); // 2_19 vertical filter
    if (rto->videoStandardInput == 2)
        GBS::MADPT_VTAP2_COEFF::write(4);

    //GBS::RFF_WFF_STA_ADDR_A::write(0);
    //GBS::RFF_WFF_STA_ADDR_B::write(1);
    GBS::RFF_ADR_ADD_2::write(1);
    GBS::RFF_REQ_SEL::write(3);
    //GBS::RFF_MASTER_FLAG::write(0x24);  // use preset's value
    //GBS::WFF_SAFE_GUARD::write(0); // 4_42 3
    GBS::RFF_FETCH_NUM::write(0x80);    // part of RFF disable fix, could leave 0x80 always otherwise
    GBS::RFF_WFF_OFFSET::write(0x100);  // scanline fix
    GBS::RFF_YUV_DEINTERLACE::write(0); // scanline fix 2
    GBS::WFF_FF_STA_INV::write(0);      // 4_42_2 // 22.03.19 : turned off // update: only required in PAL?
    //GBS::WFF_LINE_FLIP::write(0); // 4_4a_4 // 22.03.19 : turned off // update: only required in PAL?
    GBS::WFF_ENABLE::write(1); // 4_42 0 // enable before RFF
    GBS::RFF_ENABLE::write(1); // 4_4d 7
    //delay(60); // 55 first good
    unfreezeVideo();
    delay(60);
    GBS::MAPDT_VT_SEL_PRGV::write(0); // 2_16_7
    rto->motionAdaptiveDeinterlaceActive = true;
}

void disableMotionAdaptDeinterlace()
{
    GBS::MAPDT_VT_SEL_PRGV::write(1); // 2_16_7
    GBS::DEINT_00::write(0xff);       // 2_00

    GBS::RFF_FETCH_NUM::write(0x1);  // RFF disable fix
    GBS::RFF_WFF_OFFSET::write(0x1); // RFF disable fix
    delay(2);
    GBS::WFF_ENABLE::write(0);
    GBS::RFF_ENABLE::write(0); // can cause mem reset requirement, procedure above should fix it

    //GBS::WFF_ADR_ADD_2::write(0);
    GBS::WFF_FF_STA_INV::write(1); // 22.03.19 : turned off // update: only required in PAL?
    //GBS::WFF_LINE_FLIP::write(1); // 22.03.19 : turned off // update: only required in PAL?
    GBS::MADPT_Y_MI_OFFSET::write(0x7f);
    //GBS::MADPT_STILL_NOISE_EST_EN::write(0); // new
    GBS::MADPT_Y_MI_DET_BYPS::write(1);
    //GBS::MADPT_UVDLY_PD_BYPS::write(1); // 2_35_5
    //GBS::MADPT_EN_UV_DEINT::write(0); // 2_3a 0
    //GBS::MADPT_EN_STILL_FOR_NRD::write(0); // 2_3a 3 (new)
    rto->motionAdaptiveDeinterlaceActive = false;
}

void printInfo()
{
    static char print[121]; // Increase if compiler complains about sprintf
    static uint8_t clearIrqCounter = 0;
    static uint8_t lockCounterPrevious = 0;
    uint8_t lockCounter = 0;

    int32_t wifi = 0;
#if ENABLE_WIFI
    if ((WiFi.status() == WL_CONNECTED) || (WiFi.getMode() == WIFI_AP)) {
        wifi = WiFi.RSSI();
    }
#endif

    uint16_t hperiod = GBS::HPERIOD_IF::read();
    uint16_t vperiod = GBS::VPERIOD_IF::read();
    uint8_t stat0FIrq = GBS::STATUS_0F::read();
    char HSp = GBS::STATUS_SYNC_PROC_HSPOL::read() ? '+' : '-'; // 0 = neg, 1 = pos
    char VSp = GBS::STATUS_SYNC_PROC_VSPOL::read() ? '+' : '-'; // 0 = neg, 1 = pos
    char h = 'H', v = 'V';
    if (!GBS::STATUS_SYNC_PROC_HSACT::read()) {
        h = HSp = ' ';
    }
    if (!GBS::STATUS_SYNC_PROC_VSACT::read()) {
        v = VSp = ' ';
    }

    //int charsToPrint =
    sprintf(print, "h:%4u v:%4u PLL:%01u A:%02x%02x%02x S:%02x.%02x.%02x %c%c%c%c I:%02x D:%04x m:%hu ht:%4d vt:%4d hpw:%4d u:%3x s:%2x S:%2d W:%2d\n",
            hperiod, vperiod, lockCounterPrevious,
            GBS::ADC_RGCTRL::read(), GBS::ADC_GGCTRL::read(), GBS::ADC_BGCTRL::read(),
            GBS::STATUS_00::read(), GBS::STATUS_05::read(), GBS::SP_CS_0x3E::read(),
            h, HSp, v, VSp, stat0FIrq, GBS::TEST_BUS::read(), getVideoMode(),
            GBS::STATUS_SYNC_PROC_HTOTAL::read(), GBS::STATUS_SYNC_PROC_VTOTAL::read() /*+ 1*/, // emucrt: without +1 is correct line count
            GBS::STATUS_SYNC_PROC_HLOW_LEN::read(), rto->noSyncCounter, rto->continousStableCounter,
            rto->currentLevelSOG, wifi);

    //SerialM.print("charsToPrint: "); SerialM.println(charsToPrint);
    SerialM.print(print);

    if (stat0FIrq != 0x00) {
        // clear 0_0F interrupt bits regardless of syncwatcher status
        clearIrqCounter++;
        if (clearIrqCounter >= 50) {
            clearIrqCounter = 0;
            GBS::INTERRUPT_CONTROL_00::write(0xff); // reset irq status
            GBS::INTERRUPT_CONTROL_00::write(0x00);
        }
    }

    yield();
    if (GBS::STATUS_SYNC_PROC_HSACT::read()) { // else source might not be active
        for (uint8_t i = 0; i < 9; i++) {
            if (GBS::STATUS_MISC_PLLAD_LOCK::read() == 1) {
                lockCounter++;
            } else {
                for (int i = 0; i < 10; i++) {
                    if (GBS::STATUS_MISC_PLLAD_LOCK::read() == 1) {
                        lockCounter++;
                        break;
                    }
                }
            }
        }
    }
    lockCounterPrevious = getMovingAverage(lockCounter);
}
