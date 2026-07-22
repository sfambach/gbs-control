#include "config.h"
#include "platform_gbs.h"
#include "gbs_globals.h"
#include "gbs_oled_legacy.h"
#include "gbs_prefs.h"
#include "gbs_video.h"
#if GBS_ENABLE_OLED
#include "SSD1306Wire.h"
extern SSD1306Wire display;
extern const int pin_switch;
extern volatile int oled_encoder_pos;
extern volatile int oled_main_pointer;
extern volatile int oled_pointer_count;
extern volatile int oled_sub_pointer;
extern String oled_menu[4];
extern String oled_Resolutions[7];
extern String oled_Presets[8];
extern String oled_Misc[4];
extern int oled_menuItem;
extern int oled_subsetFrame;
extern int oled_selectOption;
extern int oled_page;
#endif

#if GBS_ENABLE_OLED && !USE_NEW_OLED_MENU
//OLED Functionality
void settingsMenuOLED()
{
    uint8_t videoMode = getVideoMode();
    byte button_nav = digitalRead(pin_switch);
    if (button_nav == LOW) {
        delay(350);         //TODO
        oled_subsetFrame++; //this button counter for navigating menu
        oled_selectOption++;
    }
    //main menu
    if (oled_page == 0) {
        pointerfunction();
        display.clear();
        display.setTextAlignment(TEXT_ALIGN_LEFT);
        display.setFont(ArialMT_Plain_16);
        display.drawString(0, oled_main_pointer, ">");
        display.drawString(14, 0, String(oled_menu[0]));
        display.drawString(14, 15, String(oled_menu[1]));
        display.drawString(14, 30, String(oled_menu[2]));
        display.drawString(14, 45, String(oled_menu[3]));
        display.display();
    }
    //cursor location on main menu
    if (oled_main_pointer == 0 && oled_subsetFrame == 0) {
        oled_pointer_count = 0;
        oled_menuItem = 1;
    }
    if (oled_main_pointer == 15 && oled_subsetFrame == 0) {
        oled_pointer_count = 0;
        oled_menuItem = 2;
    }
    if (oled_main_pointer == 30 && oled_subsetFrame == 0) {
        oled_pointer_count = 0;
        oled_sub_pointer = 0;
        oled_menuItem = 3;
    }
    if (oled_main_pointer == 45 && oled_subsetFrame == 0) {
        oled_pointer_count = 0;
        oled_menuItem = 4;
    }


    //resolution pages
    if (oled_menuItem == 1 && oled_subsetFrame == 1) {
        oled_page = 1;
        oled_main_pointer = 0;
        subpointerfunction();
        display.clear();
        display.drawString(0, oled_sub_pointer, ">");
        display.drawString(14, 0, String(oled_Resolutions[0]));
        display.drawString(14, 15, String(oled_Resolutions[1]));
        display.drawString(14, 30, String(oled_Resolutions[2]));
        display.drawString(14, 45, String(oled_Resolutions[3]));
        display.display();
    } else if (oled_menuItem == 1 && oled_subsetFrame == 2) {
        subpointerfunction();
        oled_page = 2;
        display.clear();
        display.drawString(0, oled_sub_pointer, ">");
        display.drawString(14, 0, String(oled_Resolutions[4]));
        display.drawString(14, 15, String(oled_Resolutions[5]));
        display.drawString(14, 30, String(oled_Resolutions[6]));
        display.drawString(14, 45, "-----Back");
        display.display();
        if (oled_sub_pointer <= -15) {
            oled_page = 1;
            oled_subsetFrame = 1;
            oled_sub_pointer = 45;
            display.clear();
        } else if (oled_sub_pointer > 45) {
            oled_page = 2;
            oled_subsetFrame = 2;
            oled_sub_pointer = 45;
        }
    }
    //selection
    //1280x960
    if (oled_menuItem == 1) {
        if (oled_pointer_count == 0 && oled_selectOption == 2) {
            oled_subsetFrame = 3;
            for (int i = 0; i <= 800; i++) {
                display.clear();
                display.drawString(0, 10, "1280x960");
                display.drawString(0, 30, "Loaded!");
                display.display();
            }
            if (videoMode == 0 && GBS::STATUS_SYNC_PROC_HSACT::read())
                videoMode = rto->videoStandardInput;
            uopt->presetPreference = Output960P;
            rto->useHdmiSyncFix = 1;
            if (rto->videoStandardInput == 14) {
                rto->videoStandardInput = 15;
            } else {
                applyPresets(videoMode);
            }
            saveUserPrefs();
            oled_selectOption = 1;
            oled_subsetFrame = 1;
        }
        //1280x1024
        if (oled_pointer_count == 1 && oled_selectOption == 2) {
            oled_subsetFrame = 3;
            for (int i = 0; i <= 800; i++) {
                display.clear();
                display.drawString(0, 10, "1280x1024");
                display.drawString(0, 30, "Loaded!");
                display.display();
            }
            if (videoMode == 0 && GBS::STATUS_SYNC_PROC_HSACT::read())
                videoMode = rto->videoStandardInput;
            uopt->presetPreference = Output1024P;
            rto->useHdmiSyncFix = 1;
            if (rto->videoStandardInput == 14) {
                rto->videoStandardInput = 15;
            } else {
                applyPresets(videoMode);
            }
            saveUserPrefs();
            oled_selectOption = 1;
            oled_subsetFrame = 1;
        }
        //1280x720
        if (oled_pointer_count == 2 && oled_selectOption == 2) {
            oled_subsetFrame = 3;
            for (int i = 0; i <= 800; i++) {
                display.clear();
                display.drawString(0, 10, "1280x720");
                display.drawString(0, 30, "Loaded!");
                display.display();
            }
            if (videoMode == 0 && GBS::STATUS_SYNC_PROC_HSACT::read())
                videoMode = rto->videoStandardInput;
            uopt->presetPreference = Output720P;
            rto->useHdmiSyncFix = 1;
            if (rto->videoStandardInput == 14) {
                rto->videoStandardInput = 15;
            } else {
                applyPresets(videoMode);
            }
            saveUserPrefs();
            oled_selectOption = 1;
            oled_subsetFrame = 1;
        }
        //1920x1080
        if (oled_pointer_count == 3 && oled_selectOption == 2) {
            oled_subsetFrame = 3;
            for (int i = 0; i <= 800; i++) {
                display.clear();
                display.drawString(0, 10, "1920x1080");
                display.drawString(0, 30, "Loaded!");
                display.display();
            }
            if (videoMode == 0 && GBS::STATUS_SYNC_PROC_HSACT::read())
                videoMode = rto->videoStandardInput;
            uopt->presetPreference = Output1080P;
            rto->useHdmiSyncFix = 1;
            if (rto->videoStandardInput == 14) {
                rto->videoStandardInput = 15;
            } else {
                applyPresets(videoMode);
            }
            saveUserPrefs();
            oled_selectOption = 1;
            oled_subsetFrame = 1;
        }
        //720x480
        if (oled_pointer_count == 4 && oled_selectOption == 2) {
            oled_subsetFrame = 3;
            for (int i = 0; i <= 800; i++) {
                display.clear();
                display.drawString(0, 10, "480/576");
                display.drawString(0, 30, "Loaded!");
                display.display();
            }
            if (videoMode == 0 && GBS::STATUS_SYNC_PROC_HSACT::read())
                videoMode = rto->videoStandardInput;
            uopt->presetPreference = Output480P;
            rto->useHdmiSyncFix = 1;
            if (rto->videoStandardInput == 14) {
                rto->videoStandardInput = 15;
            } else {
                applyPresets(videoMode);
            }
            saveUserPrefs();
            oled_selectOption = 1;
            oled_subsetFrame = 2;
        }
        //downscale
        if (oled_pointer_count == 5 && oled_selectOption == 2) {
            oled_subsetFrame = 3;
            for (int i = 0; i <= 800; i++) {
                display.clear();
                display.drawString(0, 10, "Downscale");
                display.drawString(0, 30, "Loaded!");
                display.display();
            }
            if (videoMode == 0 && GBS::STATUS_SYNC_PROC_HSACT::read())
                videoMode = rto->videoStandardInput;
            uopt->presetPreference = OutputDownscale;
            rto->useHdmiSyncFix = 1;
            if (rto->videoStandardInput == 14) {
                rto->videoStandardInput = 15;
            } else {
                applyPresets(videoMode);
            }
            saveUserPrefs();
            oled_selectOption = 1;
            oled_subsetFrame = 2;
        }
        //passthrough/bypass
        if (oled_pointer_count == 6 && oled_selectOption == 2) {
            oled_subsetFrame = 3;
            for (int i = 0; i <= 800; i++) {
                display.clear();
                display.drawString(0, 10, "Pass-Through");
                display.drawString(0, 30, "Loaded!");
                display.display();
            }
            setOutModeHdBypass(false);
            uopt->presetPreference = OutputBypass;
            if (uopt->presetPreference == 10 && rto->videoStandardInput != 15) {
                rto->autoBestHtotalEnabled = 0;
                if (rto->applyPresetDoneStage == 11) {
                    rto->applyPresetDoneStage = 1;
                } else {
                    rto->applyPresetDoneStage = 10;
                }
            } else {
                rto->applyPresetDoneStage = 1;
            }
            saveUserPrefs();
            oled_selectOption = 1;
            oled_subsetFrame = 2;
        }
        //go back
        if (oled_pointer_count == 7 && oled_selectOption == 2) {
            oled_page = 0;
            oled_subsetFrame = 0;
            oled_main_pointer = 0;
            oled_sub_pointer = 0;
            oled_selectOption = 0;
        }
    }
    //Presets pages
    if (oled_menuItem == 2 && oled_subsetFrame == 1) {
        oled_page = 1;
        oled_main_pointer = 0;
        subpointerfunction();
        display.clear();
        display.setTextAlignment(TEXT_ALIGN_CENTER);
        display.setFont(Open_Sans_Regular_20);
        display.drawString(64, -8, "Preset Slot:");
        display.setFont(Open_Sans_Regular_35);
        display.drawString(64, 15, String(oled_Presets[oled_pointer_count]));
        display.display();
    } else if (oled_menuItem == 2 && oled_subsetFrame == 2) {
        oled_page = 2;
        subpointerfunction();
        display.clear();
        display.setTextAlignment(TEXT_ALIGN_CENTER);
        display.setFont(Open_Sans_Regular_20);
        display.drawString(64, -8, "Preset Slot:");
        display.setFont(Open_Sans_Regular_35);
        display.drawString(64, 15, String(oled_Presets[oled_pointer_count]));
        display.display();
    }
    //Preset selection
    if (oled_menuItem == 2) {
        if (oled_pointer_count == 0 && oled_selectOption == 2) {
            oled_subsetFrame = 3;
            uopt->presetSlot = 'A';
            uopt->presetPreference = OutputCustomized;
            saveUserPrefs();
            for (int i = 0; i <= 280; i++) {
                display.clear();
                display.setFont(Open_Sans_Regular_20);                            //first array element selected
                display.drawString(64, -8, "Preset #" + String(oled_Presets[0])); //set to frame that "doesnt exist"
                display.setFont(Open_Sans_Regular_35);
                display.drawString(64, 15, "Loaded!");
                display.display(); //display loading conf
            }
            uopt->presetPreference = OutputCustomized;
            if (rto->videoStandardInput == 14) {
                rto->videoStandardInput = 15;
            } else {
                applyPresets(rto->videoStandardInput);
            }
            saveUserPrefs();
            delay(50);             //allowing "catchup"
            oled_selectOption = 1; //reset select container
            oled_subsetFrame = 1;  //switch back to prev frame (prev screen)
        }
        if (oled_pointer_count == 1 && oled_selectOption == 2) {
            oled_subsetFrame = 3;
            uopt->presetSlot = 'B';
            uopt->presetPreference = OutputCustomized;
            saveUserPrefs();
            for (int i = 0; i <= 280; i++) {
                display.clear();
                display.setFont(Open_Sans_Regular_20);
                display.drawString(64, -8, "Preset #" + String(oled_Presets[1]));
                display.setFont(Open_Sans_Regular_35);
                display.drawString(64, 15, "Loaded!");
                display.display();
            }
            uopt->presetPreference = OutputCustomized;
            if (rto->videoStandardInput == 14) {
                rto->videoStandardInput = 15;
            } else {
                applyPresets(rto->videoStandardInput);
            }
            saveUserPrefs();
            delay(50);
            oled_selectOption = 1;
            oled_subsetFrame = 1;
        }
        if (oled_pointer_count == 2 && oled_selectOption == 2) {
            oled_subsetFrame = 3;
            uopt->presetSlot = 'C';
            uopt->presetPreference = OutputCustomized;
            saveUserPrefs();
            for (int i = 0; i <= 280; i++) {
                display.clear();
                display.setFont(Open_Sans_Regular_20);
                display.drawString(64, -8, "Preset #" + String(oled_Presets[2]));
                display.setFont(Open_Sans_Regular_35);
                display.drawString(64, 15, "Loaded!");
                display.display();
            }
            uopt->presetPreference = OutputCustomized;
            if (rto->videoStandardInput == 14) {
                rto->videoStandardInput = 15;
            } else {
                applyPresets(rto->videoStandardInput);
            }
            saveUserPrefs();
            delay(50);
            oled_selectOption = 1;
            oled_subsetFrame = 1;
        }
        if (oled_pointer_count == 3 && oled_selectOption == 2) {
            oled_subsetFrame = 3;
            uopt->presetSlot = 'D';
            uopt->presetPreference = OutputCustomized;
            saveUserPrefs();
            for (int i = 0; i <= 280; i++) {
                display.clear();
                display.setFont(Open_Sans_Regular_20);
                display.drawString(64, -8, "Preset #" + String(oled_Presets[3]));
                display.setFont(Open_Sans_Regular_35);
                display.drawString(64, 15, "Loaded!");
                display.display();
            }
            uopt->presetPreference = OutputCustomized;
            if (rto->videoStandardInput == 14) {
                rto->videoStandardInput = 15;
            } else {
                applyPresets(rto->videoStandardInput);
            }
            saveUserPrefs();
            delay(50);
            oled_selectOption = 1;
            oled_subsetFrame = 1;
        }
        if (oled_pointer_count == 4 && oled_selectOption == 2) {
            oled_subsetFrame = 3;
            uopt->presetSlot = 'E';
            uopt->presetPreference = OutputCustomized;
            saveUserPrefs();
            for (int i = 0; i <= 280; i++) {
                display.clear();
                display.setFont(Open_Sans_Regular_20);
                display.drawString(64, -8, "Preset #" + String(oled_Presets[4]));
                display.setFont(Open_Sans_Regular_35);
                display.drawString(64, 15, "Loaded!");
                display.display();
            }
            uopt->presetPreference = OutputCustomized;
            if (rto->videoStandardInput == 14) {
                rto->videoStandardInput = 15;
            } else {
                applyPresets(rto->videoStandardInput);
            }
            saveUserPrefs();
            delay(50);
            oled_selectOption = 1;
            oled_subsetFrame = 2;
        }
        if (oled_pointer_count == 5 && oled_selectOption == 2) {
            oled_subsetFrame = 3;
            uopt->presetSlot = 'F';
            uopt->presetPreference = OutputCustomized;
            saveUserPrefs();
            for (int i = 0; i <= 280; i++) {
                display.clear();
                display.setFont(Open_Sans_Regular_20);
                display.drawString(64, -8, "Preset #" + String(oled_Presets[5]));
                display.setFont(Open_Sans_Regular_35);
                display.drawString(64, 15, "Loaded!");
                display.display();
            }
            uopt->presetPreference = OutputCustomized;
            if (rto->videoStandardInput == 14) {
                rto->videoStandardInput = 15;
            } else {
                applyPresets(rto->videoStandardInput);
            }
            saveUserPrefs();
            delay(50);
            oled_selectOption = 1;
            oled_subsetFrame = 2;
        }
        if (oled_pointer_count == 6 && oled_selectOption == 2) {
            oled_subsetFrame = 3;
            uopt->presetSlot = 'G';
            uopt->presetPreference = OutputCustomized;
            saveUserPrefs();
            for (int i = 0; i <= 280; i++) {
                display.clear();
                display.setFont(Open_Sans_Regular_20);
                display.drawString(64, -8, "Preset #" + String(oled_Presets[6]));
                display.setFont(Open_Sans_Regular_35);
                display.drawString(64, 15, "Loaded!");
                display.display();
            }
            uopt->presetPreference = OutputCustomized;
            if (rto->videoStandardInput == 14) {
                rto->videoStandardInput = 15;
            } else {
                applyPresets(rto->videoStandardInput);
            }
            saveUserPrefs();
            delay(50);
            oled_selectOption = 1;
            oled_subsetFrame = 2;
        }
        if (oled_pointer_count == 7 && oled_selectOption == 2) {
            oled_page = 0;
            oled_subsetFrame = 0;
            oled_main_pointer = 0;
            oled_sub_pointer = 0;
            oled_selectOption = 0;
        }
    }
    //Misc pages
    if (oled_menuItem == 3 && oled_subsetFrame == 1) {
        oled_page = 1;
        oled_main_pointer = 0;
        subpointerfunction();
        display.clear();
        display.drawString(0, oled_sub_pointer, ">");
        display.drawString(14, 0, String(oled_Misc[0]));
        display.drawString(14, 15, String(oled_Misc[1]));
        display.drawString(14, 45, String(oled_Misc[2]));
        display.display();
        if (oled_sub_pointer <= 0) {
            oled_sub_pointer = 0;
        }
        if (oled_sub_pointer >= 45) {
            oled_sub_pointer = 45;
        }
    }
    //Misc selection
    if (oled_menuItem == 3) {
        if (oled_pointer_count == 0 && oled_selectOption == 2) {
            oled_subsetFrame = 3;
            for (int i = 0; i <= 800; i++) {
                display.clear();
                display.drawString(0, 10, "Resetting GBS");
                display.drawString(0, 30, "Please Wait...");
                display.display();
            }
            webSocket.close();
            delay(60);
            ESP.reset();
            oled_selectOption = 0;
            oled_subsetFrame = 0;
        }

        if (oled_pointer_count == 1 && oled_selectOption == 2) {
            oled_subsetFrame = 3;
            for (int i = 0; i <= 800; i++) {
                display.clear();
                display.drawString(0, 10, "Defaults Loading");
                display.drawString(0, 30, "Please Wait...");
                display.display();
            }
            webSocket.close();
            loadDefaultUserOptions();
            saveUserPrefs();
            delay(60);
            ESP.reset();
            oled_selectOption = 1;
            oled_subsetFrame = 1;
        }

        if (oled_pointer_count == 3 && oled_selectOption == 2) {
            oled_page = 0;
            oled_subsetFrame = 0;
            oled_main_pointer = 0;
            oled_sub_pointer = 0;
            oled_selectOption = 0;
        }
    }
    //Current Settings pages
    if (oled_menuItem == 4 && oled_subsetFrame == 1) {
        boolean vsyncActive = 0;
        boolean hsyncActive = 0;
        float ofr = getOutputFrameRate();
        uint8_t currentInput = GBS::ADC_INPUT_SEL::read();
        rto->presetID = GBS::GBS_PRESET_ID::read();

        oled_page = 1;
        oled_pointer_count = 0;
        oled_main_pointer = 0;

        subpointerfunction();
        display.clear();
        display.setFont(URW_Gothic_L_Book_20);
        display.setTextAlignment(TEXT_ALIGN_LEFT);

        if (rto->presetID == 0x01 || rto->presetID == 0x11) {
            display.drawString(0, 0, "1280x960");
        } else if (rto->presetID == 0x02 || rto->presetID == 0x12) {
            display.drawString(0, 0, "1280x1024");
        } else if (rto->presetID == 0x03 || rto->presetID == 0x13) {
            display.drawString(0, 0, "1280x720");
        } else if (rto->presetID == 0x05 || rto->presetID == 0x15) {
            display.drawString(0, 0, "1920x1080");
        } else if (rto->presetID == 0x06 || rto->presetID == 0x16) {
            display.drawString(0, 0, "Downscale");
        } else if (rto->presetID == 0x04) {
            display.drawString(0, 0, "720x480");
        } else if (rto->presetID == 0x14) {
            display.drawString(0, 0, "768x576");
        } else {
            display.drawString(0, 0, "bypass");
        }

        display.drawString(0, 20, String(ofr, 5) + "Hz");

        if (currentInput == 1) {
            display.drawString(0, 41, "RGB");
        } else {
            display.drawString(0, 41, "YpBpR");
        }

        if (currentInput == 1) {
            vsyncActive = GBS::STATUS_SYNC_PROC_VSACT::read();
            if (vsyncActive) {
                display.drawString(70, 41, "V");
                hsyncActive = GBS::STATUS_SYNC_PROC_HSACT::read();
                if (hsyncActive) {
                    display.drawString(53, 41, "H");
                }
            }
        }
        display.display();
    }
    //current setting Selection
    if (oled_menuItem == 4) {
        if (oled_pointer_count >= 0 && oled_selectOption == 2) {
            oled_page = 0;
            oled_subsetFrame = 0;
            oled_main_pointer = 0;
            oled_sub_pointer = 0;
            oled_selectOption = 0;
        }
    }
}

void pointerfunction()
{
    if (oled_main_pointer <= 0) {
        oled_main_pointer = 0;
    }
    if (oled_main_pointer >= 45) { //limits
        oled_main_pointer = 45;
    }

    if (oled_pointer_count <= 0) {
        oled_pointer_count = 0;
    } else if (oled_pointer_count >= 3) {
        oled_pointer_count = 3;
    }
}
void subpointerfunction()
{
    if (oled_sub_pointer < 0) {
        oled_sub_pointer = 0;
        oled_subsetFrame = 1;
        oled_page = 1;
    }
    if (oled_sub_pointer > 45) { //limits to switch between the two pages
        oled_sub_pointer = 0;    //TODO
        oled_subsetFrame = 2;
        oled_page = 2;
    }
    // }   if (sub_pointer <= -15){ //TODO: replace/take out
    //   sub_pointer = 0;
    //   page = 1;
    //   subsetFrame = 1;
    // }
    if (oled_pointer_count <= 0) {
        oled_pointer_count = 0;
    } else if (oled_pointer_count >= 7) {
        oled_pointer_count = 7;
    }
}
#endif
