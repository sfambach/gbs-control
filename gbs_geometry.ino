// picture position, size and video timings
// part of the gbs-control sketch: all .ino files are concatenated into one translation unit
// (gbs-control.ino first), so globals and types from gbs-control.ino are visible here.

void shiftHorizontal(uint16_t amountToShift, bool subtracting)
{
    uint16_t hrst = GBS::VDS_HSYNC_RST::read();
    uint16_t hbst = GBS::VDS_HB_ST::read();
    uint16_t hbsp = GBS::VDS_HB_SP::read();

    // Perform the addition/subtraction
    if (subtracting) {
        if ((int16_t)hbst - amountToShift >= 0) {
            hbst -= amountToShift;
        } else {
            hbst = hrst - (amountToShift - hbst);
        }
        if ((int16_t)hbsp - amountToShift >= 0) {
            hbsp -= amountToShift;
        } else {
            hbsp = hrst - (amountToShift - hbsp);
        }
    } else {
        if ((int16_t)hbst + amountToShift <= hrst) {
            hbst += amountToShift;
            // also extend hbst_d to maximum hrst-1
            if (hbst > GBS::VDS_DIS_HB_ST::read()) {
                GBS::VDS_DIS_HB_ST::write(hbst);
            }
        } else {
            hbst = 0 + (amountToShift - (hrst - hbst));
        }
        if ((int16_t)hbsp + amountToShift <= hrst) {
            hbsp += amountToShift;
        } else {
            hbsp = 0 + (amountToShift - (hrst - hbsp));
        }
    }

    GBS::VDS_HB_ST::write(hbst);
    GBS::VDS_HB_SP::write(hbsp);
    //Serial.print("hbst: "); Serial.println(hbst);
    //Serial.print("hbsp: "); Serial.println(hbsp);
}

void shiftHorizontalLeft()
{
    shiftHorizontal(4, true);
}

void shiftHorizontalRight()
{
    shiftHorizontal(4, false);
}

// unused but may become useful
void shiftHorizontalLeftIF(uint8_t amount)
{
    uint16_t IF_HB_ST2 = GBS::IF_HB_ST2::read() + amount;
    uint16_t IF_HB_SP2 = GBS::IF_HB_SP2::read() + amount;
    uint16_t PLLAD_MD = GBS::PLLAD_MD::read();

    if (rto->videoStandardInput <= 2) {
        GBS::IF_HSYNC_RST::write(PLLAD_MD / 2); // input line length from pll div
    } else if (rto->videoStandardInput <= 7) {
        GBS::IF_HSYNC_RST::write(PLLAD_MD);
    }
    uint16_t IF_HSYNC_RST = GBS::IF_HSYNC_RST::read();

    GBS::IF_LINE_SP::write(IF_HSYNC_RST + 1);

    // start
    if (IF_HB_ST2 < IF_HSYNC_RST) {
        GBS::IF_HB_ST2::write(IF_HB_ST2);
    } else {
        GBS::IF_HB_ST2::write(IF_HB_ST2 - IF_HSYNC_RST);
    }
    //SerialM.print("IF_HB_ST2:  "); SerialM.println(GBS::IF_HB_ST2::read());

    // stop
    if (IF_HB_SP2 < IF_HSYNC_RST) {
        GBS::IF_HB_SP2::write(IF_HB_SP2);
    } else {
        GBS::IF_HB_SP2::write((IF_HB_SP2 - IF_HSYNC_RST) + 1);
    }
    //SerialM.print("IF_HB_SP2:  "); SerialM.println(GBS::IF_HB_SP2::read());
}

// unused but may become useful
void shiftHorizontalRightIF(uint8_t amount)
{
    int16_t IF_HB_ST2 = GBS::IF_HB_ST2::read() - amount;
    int16_t IF_HB_SP2 = GBS::IF_HB_SP2::read() - amount;
    uint16_t PLLAD_MD = GBS::PLLAD_MD::read();

    if (rto->videoStandardInput <= 2) {
        GBS::IF_HSYNC_RST::write(PLLAD_MD / 2); // input line length from pll div
    } else if (rto->videoStandardInput <= 7) {
        GBS::IF_HSYNC_RST::write(PLLAD_MD);
    }
    int16_t IF_HSYNC_RST = GBS::IF_HSYNC_RST::read();

    GBS::IF_LINE_SP::write(IF_HSYNC_RST + 1);

    if (IF_HB_ST2 > 0) {
        GBS::IF_HB_ST2::write(IF_HB_ST2);
    } else {
        GBS::IF_HB_ST2::write(IF_HSYNC_RST - 1);
    }
    //SerialM.print("IF_HB_ST2:  "); SerialM.println(GBS::IF_HB_ST2::read());

    if (IF_HB_SP2 > 0) {
        GBS::IF_HB_SP2::write(IF_HB_SP2);
    } else {
        GBS::IF_HB_SP2::write(IF_HSYNC_RST - 1);
        //GBS::IF_LINE_SP::write(GBS::IF_LINE_SP::read() - 2);
    }
    //SerialM.print("IF_HB_SP2:  "); SerialM.println(GBS::IF_HB_SP2::read());
}

void scaleHorizontal(uint16_t amountToScale, bool subtracting)
{
    uint16_t hscale = GBS::VDS_HSCALE::read();

    // smooth out areas of interest
    if (subtracting && (hscale == 513 || hscale == 512))
        amountToScale = 1;
    if (!subtracting && (hscale == 511 || hscale == 512))
        amountToScale = 1;

    if (subtracting && (((int)hscale - amountToScale) <= 256)) {
        hscale = 256;
        GBS::VDS_HSCALE::write(hscale);
        SerialM.println("limit");
        return;
    }

    if (subtracting && (hscale - amountToScale > 255)) {
        hscale -= amountToScale;
    } else if (hscale + amountToScale < 1023) {
        hscale += amountToScale;
    } else if (hscale + amountToScale == 1023) { // exact max > bypass but apply VDS fetch changes
        hscale = 1023;
        GBS::VDS_HSCALE::write(hscale);
        GBS::VDS_HSCALE_BYPS::write(1);
    } else if (hscale + amountToScale > 1023) { // max + overshoot > bypass and no VDS fetch adjust
        hscale = 1023;
        GBS::VDS_HSCALE::write(hscale);
        GBS::VDS_HSCALE_BYPS::write(1);
        SerialM.println("limit");
        return;
    }

    // will be scaling
    GBS::VDS_HSCALE_BYPS::write(0);

    // move within VDS VB fetch area (within reason)
    uint16_t htotal = GBS::VDS_HSYNC_RST::read();
    uint16_t toShift = 0;
    if (hscale < 540)
        toShift = 4;
    else if (hscale < 640)
        toShift = 3;
    else
        toShift = 2;

    if (subtracting) {
        shiftHorizontal(toShift, true);
        if ((GBS::VDS_HB_ST::read() + 5) < GBS::VDS_DIS_HB_ST::read()) {
            GBS::VDS_HB_ST::write(GBS::VDS_HB_ST::read() + 5);
        } else if ((GBS::VDS_DIS_HB_ST::read() + 5) < htotal) {
            GBS::VDS_DIS_HB_ST::write(GBS::VDS_DIS_HB_ST::read() + 5);
            GBS::VDS_HB_ST::write(GBS::VDS_DIS_HB_ST::read()); // dis_hbst = hbst
        }

        // fix HB_ST > HB_SP conditions
        if (GBS::VDS_HB_SP::read() < (GBS::VDS_HB_ST::read() + 16)) { // HB_SP < HB_ST
            if ((GBS::VDS_HB_SP::read()) > (htotal / 2)) {            // but HB_SP > some small value
                GBS::VDS_HB_ST::write(GBS::VDS_HB_SP::read() - 16);
            }
        }
    }

    // !subtracting check just for readability
    if (!subtracting) {
        shiftHorizontal(toShift, false);
        if ((GBS::VDS_HB_ST::read() - 5) > 0) {
            GBS::VDS_HB_ST::write(GBS::VDS_HB_ST::read() - 5);
        }
    }

    // fix scaling < 512 glitch: factor even, htotal even: hbst / hbsp should be even, etc
    if (hscale < 512) {
        if (hscale % 2 == 0) { // hscale 512 / even
            if (GBS::VDS_HB_ST::read() % 2 == 1) {
                GBS::VDS_HB_ST::write(GBS::VDS_HB_ST::read() + 1);
            }
            if (htotal % 2 == 1) {
                if (GBS::VDS_HB_SP::read() % 2 == 0) {
                    GBS::VDS_HB_SP::write(GBS::VDS_HB_SP::read() - 1);
                }
            } else {
                if (GBS::VDS_HB_SP::read() % 2 == 1) {
                    GBS::VDS_HB_SP::write(GBS::VDS_HB_SP::read() - 1);
                }
            }
        } else { // hscale 499 / uneven
            if (GBS::VDS_HB_ST::read() % 2 == 1) {
                GBS::VDS_HB_ST::write(GBS::VDS_HB_ST::read() + 1);
            }
            if (htotal % 2 == 0) {
                if (GBS::VDS_HB_SP::read() % 2 == 1) {
                    GBS::VDS_HB_SP::write(GBS::VDS_HB_SP::read() - 1);
                }
            } else {
                if (GBS::VDS_HB_SP::read() % 2 == 0) {
                    GBS::VDS_HB_SP::write(GBS::VDS_HB_SP::read() - 1);
                }
            }
        }
        // if scaling was < 512 and HB_ST moved, align with VDS_DIS_HB_ST
        if (GBS::VDS_DIS_HB_ST::read() < GBS::VDS_HB_ST::read()) {
            GBS::VDS_DIS_HB_ST::write(GBS::VDS_HB_ST::read());
        }
    }

    //SerialM.print("HB_ST: "); SerialM.println(GBS::VDS_HB_ST::read());
    //SerialM.print("HB_SP: "); SerialM.println(GBS::VDS_HB_SP::read());
    SerialM.print("HScale: ");
    SerialM.println(hscale);
    GBS::VDS_HSCALE::write(hscale);
}

// moves horizontal sync (VDS or HDB as needed)
void moveHS(uint16_t amountToAdd, bool subtracting)
{
    if (rto->outModeHdBypass) {
        uint16_t SP_CS_HS_ST = GBS::SP_CS_HS_ST::read();
        uint16_t SP_CS_HS_SP = GBS::SP_CS_HS_SP::read();
        uint16_t htotal = GBS::HD_HSYNC_RST::read();

        if (videoStandardInputIsPalNtscSd()) {
            htotal -= 8; // account for the way hdbypass is setup in setOutModeHdBypass()
            htotal *= 2;
        }

        if (htotal == 0)
            return; // safety
        int16_t amount = subtracting ? (0 - amountToAdd) : amountToAdd;

        if (SP_CS_HS_ST + amount < 0) {
            SP_CS_HS_ST = htotal + SP_CS_HS_ST; // yep, this works :p
        }
        if (SP_CS_HS_SP + amount < 0) {
            SP_CS_HS_SP = htotal + SP_CS_HS_SP;
        }

        GBS::SP_CS_HS_ST::write((SP_CS_HS_ST + amount) % htotal);
        GBS::SP_CS_HS_SP::write((SP_CS_HS_SP + amount) % htotal);

        SerialM.print("HSST: ");
        SerialM.print(GBS::SP_CS_HS_ST::read());
        SerialM.print(" HSSP: ");
        SerialM.println(GBS::SP_CS_HS_SP::read());
    } else {
        uint16_t VDS_HS_ST = GBS::VDS_HS_ST::read();
        uint16_t VDS_HS_SP = GBS::VDS_HS_SP::read();
        uint16_t htotal = GBS::VDS_HSYNC_RST::read();

        if (htotal == 0)
            return; // safety
        int16_t amount = subtracting ? (0 - amountToAdd) : amountToAdd;

        if (VDS_HS_ST + amount < 0) {
            VDS_HS_ST = htotal + VDS_HS_ST; // yep, this works :p
        }
        if (VDS_HS_SP + amount < 0) {
            VDS_HS_SP = htotal + VDS_HS_SP;
        }

        GBS::VDS_HS_ST::write((VDS_HS_ST + amount) % htotal);
        GBS::VDS_HS_SP::write((VDS_HS_SP + amount) % htotal);

        //SerialM.print("HSST: "); SerialM.print(GBS::VDS_HS_ST::read());
        //SerialM.print(" HSSP: "); SerialM.println(GBS::VDS_HS_SP::read());
    }
    printVideoTimings();
}

void moveVS(uint16_t amountToAdd, bool subtracting)
{
    uint16_t vtotal = GBS::VDS_VSYNC_RST::read();
    if (vtotal == 0)
        return; // safety
    uint16_t VDS_DIS_VB_ST = GBS::VDS_DIS_VB_ST::read();
    uint16_t newVDS_VS_ST = GBS::VDS_VS_ST::read();
    uint16_t newVDS_VS_SP = GBS::VDS_VS_SP::read();

    if (subtracting) {
        if ((newVDS_VS_ST - amountToAdd) > VDS_DIS_VB_ST) {
            newVDS_VS_ST -= amountToAdd;
            newVDS_VS_SP -= amountToAdd;
        } else
            SerialM.println("limit");
    } else {
        if ((newVDS_VS_SP + amountToAdd) < vtotal) {
            newVDS_VS_ST += amountToAdd;
            newVDS_VS_SP += amountToAdd;
        } else
            SerialM.println("limit");
    }
    //SerialM.print("VSST: "); SerialM.print(newVDS_VS_ST);
    //SerialM.print(" VSSP: "); SerialM.println(newVDS_VS_SP);

    GBS::VDS_VS_ST::write(newVDS_VS_ST);
    GBS::VDS_VS_SP::write(newVDS_VS_SP);
}

void invertHS()
{
    uint8_t high, low;
    uint16_t newST, newSP;

    writeOneByte(0xf0, 3);
    readFromRegister(0x0a, 1, &low);
    readFromRegister(0x0b, 1, &high);
    newST = ((((uint16_t)high) & 0x000f) << 8) | (uint16_t)low;
    readFromRegister(0x0b, 1, &low);
    readFromRegister(0x0c, 1, &high);
    newSP = ((((uint16_t)high) & 0x00ff) << 4) | ((((uint16_t)low) & 0x00f0) >> 4);

    uint16_t temp = newST;
    newST = newSP;
    newSP = temp;

    writeOneByte(0x0a, (uint8_t)(newST & 0x00ff));
    writeOneByte(0x0b, ((uint8_t)(newSP & 0x000f) << 4) | ((uint8_t)((newST & 0x0f00) >> 8)));
    writeOneByte(0x0c, (uint8_t)((newSP & 0x0ff0) >> 4));
}

void invertVS()
{
    uint8_t high, low;
    uint16_t newST, newSP;

    writeOneByte(0xf0, 3);
    readFromRegister(0x0d, 1, &low);
    readFromRegister(0x0e, 1, &high);
    newST = ((((uint16_t)high) & 0x000f) << 8) | (uint16_t)low;
    readFromRegister(0x0e, 1, &low);
    readFromRegister(0x0f, 1, &high);
    newSP = ((((uint16_t)high) & 0x00ff) << 4) | ((((uint16_t)low) & 0x00f0) >> 4);

    uint16_t temp = newST;
    newST = newSP;
    newSP = temp;

    writeOneByte(0x0d, (uint8_t)(newST & 0x00ff));
    writeOneByte(0x0e, ((uint8_t)(newSP & 0x000f) << 4) | ((uint8_t)((newST & 0x0f00) >> 8)));
    writeOneByte(0x0f, (uint8_t)((newSP & 0x0ff0) >> 4));
}

void scaleVertical(uint16_t amountToScale, bool subtracting)
{
    uint16_t vscale = GBS::VDS_VSCALE::read();

    // least invasive "is vscaling enabled" check
    if (vscale == 1023) {
        GBS::VDS_VSCALE_BYPS::write(0);
    }

    // smooth out areas of interest
    if (subtracting && (vscale == 513 || vscale == 512))
        amountToScale = 1;
    if (subtracting && (vscale == 684 || vscale == 683))
        amountToScale = 1;
    if (!subtracting && (vscale == 511 || vscale == 512))
        amountToScale = 1;
    if (!subtracting && (vscale == 682 || vscale == 683))
        amountToScale = 1;

    if (subtracting && (vscale - amountToScale > 128)) {
        vscale -= amountToScale;
    } else if (subtracting) {
        vscale = 128;
    } else if (vscale + amountToScale <= 1023) {
        vscale += amountToScale;
    } else if (vscale + amountToScale > 1023) {
        vscale = 1023;
        // don't enable vscale bypass here, since that disables ie line filter
    }

    SerialM.print("VScale: ");
    SerialM.println(vscale);
    GBS::VDS_VSCALE::write(vscale);
}

// modified to move VBSP, set VBST to VBSP-2
void shiftVertical(uint16_t amountToAdd, bool subtracting)
{
    typedef GBS::Tie<GBS::VDS_VB_ST, GBS::VDS_VB_SP> Regs;
    uint16_t vrst = GBS::VDS_VSYNC_RST::read() - FrameSync::getSyncLastCorrection();
    uint16_t vbst = 0, vbsp = 0;
    int16_t newVbst = 0, newVbsp = 0;

    Regs::read(vbst, vbsp);
    newVbst = vbst;
    newVbsp = vbsp;

    if (subtracting) {
        //newVbst -= amountToAdd;
        newVbsp -= amountToAdd;
    } else {
        //newVbst += amountToAdd;
        newVbsp += amountToAdd;
    }

    // handle the case where hbst or hbsp have been decremented below 0
    if (newVbst < 0) {
        newVbst = vrst + newVbst;
    }
    if (newVbsp < 0) {
        newVbsp = vrst + newVbsp;
    }

    // handle the case where vbst or vbsp have been incremented above vrstValue
    if (newVbst > (int16_t)vrst) {
        newVbst = newVbst - vrst;
    }
    if (newVbsp > (int16_t)vrst) {
        newVbsp = newVbsp - vrst;
    }

    // mod: newVbsp needs to be at least newVbst+2
    if (newVbsp < (newVbst + 2)) {
        newVbsp = newVbst + 2;
    }
    // mod: -= 2
    newVbst = newVbsp - 2;

    Regs::write(newVbst, newVbsp);
    //SerialM.print("VSST: "); SerialM.print(newVbst); SerialM.print(" VSSP: "); SerialM.println(newVbsp);
}

void shiftVerticalUp()
{
    shiftVertical(1, true);
}

void shiftVerticalDown()
{
    shiftVertical(1, false);
}

void shiftVerticalUpIF()
{
    // -4 to allow variance in source line count
    uint8_t offset = rto->videoStandardInput == 2 ? 4 : 1;
    uint16_t sourceLines = GBS::VPERIOD_IF::read() - offset;
    // add an override for sourceLines, in case where the IF data is not available
    if ((GBS::GBS_OPTION_SCALING_RGBHV::read() == 1) && rto->videoStandardInput == 14) {
        sourceLines = GBS::STATUS_SYNC_PROC_VTOTAL::read();
    }
    int16_t stop = GBS::IF_VB_SP::read();
    int16_t start = GBS::IF_VB_ST::read();

    if (stop < (sourceLines - 1) && start < (sourceLines - 1)) {
        stop += 2;
        start += 2;
    } else {
        start = 0;
        stop = 2;
    }
    GBS::IF_VB_SP::write(stop);
    GBS::IF_VB_ST::write(start);
}

void shiftVerticalDownIF()
{
    uint8_t offset = rto->videoStandardInput == 2 ? 4 : 1;
    uint16_t sourceLines = GBS::VPERIOD_IF::read() - offset;
    // add an override for sourceLines, in case where the IF data is not available
    if ((GBS::GBS_OPTION_SCALING_RGBHV::read() == 1) && rto->videoStandardInput == 14) {
        sourceLines = GBS::STATUS_SYNC_PROC_VTOTAL::read();
    }

    int16_t stop = GBS::IF_VB_SP::read();
    int16_t start = GBS::IF_VB_ST::read();

    if (stop > 1 && start > 1) {
        stop -= 2;
        start -= 2;
    } else {
        start = sourceLines - 2;
        stop = sourceLines;
    }
    GBS::IF_VB_SP::write(stop);
    GBS::IF_VB_ST::write(start);
}

void setHSyncStartPosition(uint16_t value)
{
    if (rto->outModeHdBypass) {
        //GBS::HD_HS_ST::write(value);
        GBS::SP_CS_HS_ST::write(value);
    } else {
        GBS::VDS_HS_ST::write(value);
    }
}

void setHSyncStopPosition(uint16_t value)
{
    if (rto->outModeHdBypass) {
        //GBS::HD_HS_SP::write(value);
        GBS::SP_CS_HS_SP::write(value);
    } else {
        GBS::VDS_HS_SP::write(value);
    }
}

void setMemoryHblankStartPosition(uint16_t value)
{
    GBS::VDS_HB_ST::write(value);
    GBS::HD_HB_ST::write(value);
}

void setMemoryHblankStopPosition(uint16_t value)
{
    GBS::VDS_HB_SP::write(value);
    GBS::HD_HB_SP::write(value);
}

void setDisplayHblankStartPosition(uint16_t value)
{
    GBS::VDS_DIS_HB_ST::write(value);
}

void setDisplayHblankStopPosition(uint16_t value)
{
    GBS::VDS_DIS_HB_SP::write(value);
}

void setVSyncStartPosition(uint16_t value)
{
    GBS::VDS_VS_ST::write(value);
    GBS::HD_VS_ST::write(value);
}

void setVSyncStopPosition(uint16_t value)
{
    GBS::VDS_VS_SP::write(value);
    GBS::HD_VS_SP::write(value);
}

void setMemoryVblankStartPosition(uint16_t value)
{
    GBS::VDS_VB_ST::write(value);
    GBS::HD_VB_ST::write(value);
}

void setMemoryVblankStopPosition(uint16_t value)
{
    GBS::VDS_VB_SP::write(value);
    GBS::HD_VB_SP::write(value);
}

void setDisplayVblankStartPosition(uint16_t value)
{
    GBS::VDS_DIS_VB_ST::write(value);
}

void setDisplayVblankStopPosition(uint16_t value)
{
    GBS::VDS_DIS_VB_SP::write(value);
}

uint16_t getCsVsStart()
{
    return (GBS::SP_SDCS_VSST_REG_H::read() << 8) + GBS::SP_SDCS_VSST_REG_L::read();
}

uint16_t getCsVsStop()
{
    return (GBS::SP_SDCS_VSSP_REG_H::read() << 8) + GBS::SP_SDCS_VSSP_REG_L::read();
}

void setCsVsStart(uint16_t start)
{
    GBS::SP_SDCS_VSST_REG_H::write(start >> 8);
    GBS::SP_SDCS_VSST_REG_L::write(start & 0xff);
}

void setCsVsStop(uint16_t stop)
{
    GBS::SP_SDCS_VSSP_REG_H::write(stop >> 8);
    GBS::SP_SDCS_VSSP_REG_L::write(stop & 0xff);
}

void printVideoTimings()
{
    if (rto->presetID < 0x20) {
        SerialM.println("");
        SerialM.print(F("HT / scale   : "));
        SerialM.print(GBS::VDS_HSYNC_RST::read());
        SerialM.print(" ");
        SerialM.println(GBS::VDS_HSCALE::read());
        SerialM.print(F("HS ST/SP     : "));
        SerialM.print(GBS::VDS_HS_ST::read());
        SerialM.print(" ");
        SerialM.println(GBS::VDS_HS_SP::read());
        SerialM.print(F("HB ST/SP(d)  : "));
        SerialM.print(GBS::VDS_DIS_HB_ST::read());
        SerialM.print(" ");
        SerialM.println(GBS::VDS_DIS_HB_SP::read());
        SerialM.print(F("HB ST/SP     : "));
        SerialM.print(GBS::VDS_HB_ST::read());
        SerialM.print(" ");
        SerialM.println(GBS::VDS_HB_SP::read());
        SerialM.println(F("------"));
        // vertical
        SerialM.print(F("VT / scale   : "));
        SerialM.print(GBS::VDS_VSYNC_RST::read());
        SerialM.print(" ");
        SerialM.println(GBS::VDS_VSCALE::read());
        SerialM.print(F("VS ST/SP     : "));
        SerialM.print(GBS::VDS_VS_ST::read());
        SerialM.print(" ");
        SerialM.println(GBS::VDS_VS_SP::read());
        SerialM.print(F("VB ST/SP(d)  : "));
        SerialM.print(GBS::VDS_DIS_VB_ST::read());
        SerialM.print(" ");
        SerialM.println(GBS::VDS_DIS_VB_SP::read());
        SerialM.print(F("VB ST/SP     : "));
        SerialM.print(GBS::VDS_VB_ST::read());
        SerialM.print(" ");
        SerialM.println(GBS::VDS_VB_SP::read());
        // IF V offset
        SerialM.print(F("IF VB ST/SP  : "));
        SerialM.print(GBS::IF_VB_ST::read());
        SerialM.print(" ");
        SerialM.println(GBS::IF_VB_SP::read());
    } else {
        SerialM.println("");
        SerialM.print(F("HD_HSYNC_RST : "));
        SerialM.println(GBS::HD_HSYNC_RST::read());
        SerialM.print(F("HD_INI_ST    : "));
        SerialM.println(GBS::HD_INI_ST::read());
        SerialM.print(F("HS ST/SP     : "));
        SerialM.print(GBS::SP_CS_HS_ST::read());
        SerialM.print(" ");
        SerialM.println(GBS::SP_CS_HS_SP::read());
        SerialM.print(F("HB ST/SP     : "));
        SerialM.print(GBS::HD_HB_ST::read());
        SerialM.print(" ");
        SerialM.println(GBS::HD_HB_SP::read());
        SerialM.println(F("------"));
        // vertical
        SerialM.print(F("VS ST/SP     : "));
        SerialM.print(GBS::HD_VS_ST::read());
        SerialM.print(" ");
        SerialM.println(GBS::HD_VS_SP::read());
        SerialM.print(F("VB ST/SP     : "));
        SerialM.print(GBS::HD_VB_ST::read());
        SerialM.print(" ");
        SerialM.println(GBS::HD_VB_SP::read());
    }

    SerialM.print(F("CsVT         : "));
    SerialM.println(GBS::STATUS_SYNC_PROC_VTOTAL::read());
    SerialM.print(F("CsVS_ST/SP   : "));
    SerialM.print(getCsVsStart());
    SerialM.print(F(" "));
    SerialM.println(getCsVsStop());
}

void set_htotal(uint16_t htotal)
{
    // ModeLine "1280x960" 108.00 1280 1376 1488 1800 960 961 964 1000 +HSync +VSync
    // front porch: H2 - H1: 1376 - 1280
    // back porch : H4 - H3: 1800 - 1488
    // sync pulse : H3 - H2: 1488 - 1376

    uint16_t h_blank_display_start_position = htotal - 1;
    uint16_t h_blank_display_stop_position = htotal - ((htotal * 3) / 4);
    uint16_t center_blank = ((h_blank_display_stop_position / 2) * 3) / 4; // a bit to the left
    uint16_t h_sync_start_position = center_blank - (center_blank / 2);
    uint16_t h_sync_stop_position = center_blank + (center_blank / 2);
    uint16_t h_blank_memory_start_position = h_blank_display_start_position - 1;
    uint16_t h_blank_memory_stop_position = h_blank_display_stop_position - (h_blank_display_stop_position / 50);

    GBS::VDS_HSYNC_RST::write(htotal);
    GBS::VDS_HS_ST::write(h_sync_start_position);
    GBS::VDS_HS_SP::write(h_sync_stop_position);
    GBS::VDS_DIS_HB_ST::write(h_blank_display_start_position);
    GBS::VDS_DIS_HB_SP::write(h_blank_display_stop_position);
    GBS::VDS_HB_ST::write(h_blank_memory_start_position);
    GBS::VDS_HB_SP::write(h_blank_memory_stop_position);
}

void set_vtotal(uint16_t vtotal)
{
    uint16_t VDS_DIS_VB_ST = vtotal - 2;                         // just below vtotal
    uint16_t VDS_DIS_VB_SP = (vtotal >> 6) + 8;                  // positive, above new sync stop position
    uint16_t VDS_VB_ST = ((uint16_t)(vtotal * 0.016f)) & 0xfffe; // small fraction of vtotal
    uint16_t VDS_VB_SP = VDS_VB_ST + 2;                          // always VB_ST + 2
    uint16_t v_sync_start_position = 1;
    uint16_t v_sync_stop_position = 5;
    // most low line count formats have negative sync!
    // exception: 1024x768 (1344x806 total) has both sync neg. // also 1360x768 (1792x795 total)
    if ((vtotal < 530) || (vtotal >= 803 && vtotal <= 809) || (vtotal >= 793 && vtotal <= 798)) {
        uint16_t temp = v_sync_start_position;
        v_sync_start_position = v_sync_stop_position;
        v_sync_stop_position = temp;
    }

    GBS::VDS_VSYNC_RST::write(vtotal);
    GBS::VDS_VS_ST::write(v_sync_start_position);
    GBS::VDS_VS_SP::write(v_sync_stop_position);
    GBS::VDS_VB_ST::write(VDS_VB_ST);
    GBS::VDS_VB_SP::write(VDS_VB_SP);
    GBS::VDS_DIS_VB_ST::write(VDS_DIS_VB_ST);
    GBS::VDS_DIS_VB_SP::write(VDS_DIS_VB_SP);

    // VDS_VSYN_SIZE1 + VDS_VSYN_SIZE2 to VDS_VSYNC_RST + 2
    GBS::VDS_VSYN_SIZE1::write(GBS::VDS_VSYNC_RST::read() + 2);
    GBS::VDS_VSYN_SIZE2::write(GBS::VDS_VSYNC_RST::read() + 2);
}

void setIfHblankParameters()
{
    if (!rto->outModeHdBypass) {
        uint16_t pll_divider = GBS::PLLAD_MD::read();

        // if line doubling (PAL, NTSC), div 2 + a couple pixels
        GBS::IF_HSYNC_RST::write(((pll_divider >> 1) + 13) & 0xfffe); // 1_0e
        GBS::IF_LINE_SP::write(GBS::IF_HSYNC_RST::read() + 1);        // 1_22
        if (rto->presetID == 0x05) {
            // override for 1080p manually for now (pll_divider alone isn't correct :/)
            GBS::IF_HSYNC_RST::write(GBS::IF_HSYNC_RST::read() + 32);
            GBS::IF_LINE_SP::write(GBS::IF_LINE_SP::read() + 32);
        }
        if (rto->presetID == 0x15) {
            GBS::IF_HSYNC_RST::write(GBS::IF_HSYNC_RST::read() + 20);
            GBS::IF_LINE_SP::write(GBS::IF_LINE_SP::read() + 20);
        }

        if (GBS::IF_LD_RAM_BYPS::read()) {
            // no LD = EDTV or similar
            GBS::IF_HB_SP2::write((uint16_t)((float)pll_divider * 0.06512f) & 0xfffe); // 1_1a // 0.06512f
            // pll_divider / 2 - 3 is minimum IF_HB_ST2
            GBS::IF_HB_ST2::write((uint16_t)((float)pll_divider * 0.4912f) & 0xfffe); // 1_18
        } else {
            // LD mode (PAL, NTSC)
            GBS::IF_HB_SP2::write(4 + ((uint16_t)((float)pll_divider * 0.0224f) & 0xfffe)); // 1_1a
            GBS::IF_HB_ST2::write((uint16_t)((float)pll_divider * 0.4550f) & 0xfffe);       // 1_18

            if (GBS::IF_HB_ST2::read() >= 0x420) {
                // limit (fifo?) (0x420 = 1056) (might be 0x424 instead)
                // limit doesn't apply to EDTV modes, where 1_18 typically = 0x4B0
                GBS::IF_HB_ST2::write(0x420);
            }

            if (rto->presetID == 0x05 || rto->presetID == 0x15) {
                // override 1_1a for 1080p manually for now (pll_divider alone isn't correct :/)
                GBS::IF_HB_SP2::write(0x2A);
            }

            // position move via 1_26 and reserve for deinterlacer: add IF RST pixels
            // seems no extra pixels available at around PLLAD:84A or 2122px
            //uint16_t currentRst = GBS::IF_HSYNC_RST::read();
            //GBS::IF_HSYNC_RST::write((currentRst + (currentRst / 15)) & 0xfffe);  // 1_0e
            //GBS::IF_LINE_SP::write(GBS::IF_HSYNC_RST::read() + 1);                // 1_22
        }
    }
}
