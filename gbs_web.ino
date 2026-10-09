// web server, websocket and wifi
// part of the gbs-control sketch: all .ino files are concatenated into one translation unit
// (gbs-control.ino first), so globals and types from gbs-control.ino are visible here.

#if ENABLE_WIFI
void updateWebSocketData()
{
    if (rto->webServerEnabled && rto->webServerStarted) {
        if (webSocket.connectedClients() > 0) {

            constexpr size_t MESSAGE_LEN = 6;
            char toSend[MESSAGE_LEN] = {0};
            toSend[0] = '#'; // makeshift ping in slot 0

            if (rto->isCustomPreset) {
                toSend[1] = '9';
            } else switch (rto->presetID) {
                case 0x01:
                case 0x11:
                    toSend[1] = '1';
                    break;
                case 0x02:
                case 0x12:
                    toSend[1] = '2';
                    break;
                case 0x03:
                case 0x13:
                    toSend[1] = '3';
                    break;
                case 0x04:
                case 0x14:
                    toSend[1] = '4';
                    break;
                case 0x05:
                case 0x15:
                    toSend[1] = '5';
                    break;
                case 0x06:
                case 0x16:
                    toSend[1] = '6';
                    break;
                case PresetHdBypass: // bypass 1
                case PresetBypassRGBHV: // bypass 2
                    toSend[1] = '8';
                    break;
                default:
                    toSend[1] = '0';
                    break;
            }

            toSend[2] = (char)uopt->presetSlot;

            // '@' = 0x40, used for "byte is present" detection; 0x80 not in ascii table
            toSend[3] = '@';
            toSend[4] = '@';
            toSend[5] = '@';

            if (uopt->enableAutoGain) {
                toSend[3] |= (1 << 0);
            }
            if (uopt->wantScanlines) {
                toSend[3] |= (1 << 1);
            }
            if (uopt->wantVdsLineFilter) {
                toSend[3] |= (1 << 2);
            }
            if (uopt->wantPeaking) {
                toSend[3] |= (1 << 3);
            }
            if (uopt->PalForce60) {
                toSend[3] |= (1 << 4);
            }
            if (uopt->wantOutputComponent) {
                toSend[3] |= (1 << 5);
            }

            if (uopt->matchPresetSource) {
                toSend[4] |= (1 << 0);
            }
            if (uopt->enableFrameTimeLock) {
                toSend[4] |= (1 << 1);
            }
            if (uopt->deintMode) {
                toSend[4] |= (1 << 2);
            }
            if (uopt->wantTap6) {
                toSend[4] |= (1 << 3);
            }
            if (uopt->wantStepResponse) {
                toSend[4] |= (1 << 4);
            }
            if (uopt->wantFullHeight) {
                toSend[4] |= (1 << 5);
            }

            if (uopt->enableCalibrationADC) {
                toSend[5] |= (1 << 0);
            }
            if (uopt->preferScalingRgbhv) {
                toSend[5] |= (1 << 1);
            }
            if (uopt->disableExternalClockGenerator) {
                toSend[5] |= (1 << 2);
            }

            // send ping and stats
            if (ESP.getFreeHeap() > 14000) {
                webSocket.broadcastTXT(toSend, MESSAGE_LEN);
            } else {
                webSocket.disconnect();
            }
        }
    }
}

void handleWiFi(boolean instant)
{
    static unsigned long lastTimePing = millis();
    static bool pingSent = false;
    if (rto->webServerEnabled && rto->webServerStarted) {
        MDNS.update();
        persWM.handleWiFi(); // if connected, returns instantly. otherwise it reconnects or opens AP
        dnsServer.processNextRequest();
        // buffered log output for the web ui terminal; right away after a command
        if (instant) {
            SerialM.flush();
        } else {
            SerialM.flushIfDue();
        }

        // slightly odd value so not everything happens at once
        // (only once per cycle, this used to ping on every loop pass between 953 and 973 ms)
        if (!pingSent && (millis() - lastTimePing) > 953) {
            webSocket.broadcastPing();
            pingSent = true;
        }
        if (((millis() - lastTimePing) > 973) || instant) {
            if ((webSocket.connectedClients(false) > 0) || instant) { // true = with compliant ping
                updateWebSocketData();
            }
            lastTimePing = millis();
            pingSent = false;
        }
    }

    if (rto->allowUpdatesOTA) {
        ArduinoOTA.handle();
    }
    yield();
}

#endif

#if defined(ESP8266)
#if ENABLE_WIFI
void startWebserver()
{
    persWM.setApCredentials(ap_ssid, ap_password);
    persWM.onConnect([]() {
        SerialM.print(F("(WiFi): STA mode connected; IP: "));
        SerialM.println(WiFi.localIP().toString());
        if (MDNS.begin(device_hostname_partial, WiFi.localIP())) { // MDNS request for gbscontrol.local
            //Serial.println("MDNS started");
            MDNS.addService("http", "tcp", 80); // Add service to MDNS-SD
            MDNS.announce();
        }
        SerialM.println(FPSTR(st_info_string));
    });
    persWM.onAp([]() {
        SerialM.println(FPSTR(ap_info_string));
        // add mdns announce here as well?
    });

    disconnectedEventHandler = WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected &event) {
        Serial.print("Station disconnected, reason: ");
        Serial.println(event.reason);
    });

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        // the page is ~170kB, let the browser cache it and only revalidate with a tiny request.
        // the ETag changes with every firmware build, so an updated web ui is always loaded.
        static String etag;
        if (etag.length() == 0) {
            etag = String("\"") + webui_html_len + "-" + __DATE__ + "-" + __TIME__ + "\"";
            etag.replace(" ", "_");
        }
        if (request->hasHeader("If-None-Match") && request->header("If-None-Match") == etag) {
            AsyncWebServerResponse *response = request->beginResponse(304);
            response->addHeader("ETag", etag);
            response->addHeader("Cache-Control", "no-cache");
            request->send(response);
            return;
        }
        //Serial.println("sending web page");
        if (ESP.getFreeHeap() > 10000) {
            AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", webui_html, webui_html_len);
            response->addHeader("Content-Encoding", "gzip");
            response->addHeader("ETag", etag);
            response->addHeader("Cache-Control", "no-cache");
            request->send(response);
        }
    });

    server.on("/sc", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (ESP.getFreeHeap() > 10000) {
            int params = request->params();
            //Serial.print("got serial request params: ");
            //Serial.println(params);
            if (params > 0) {
                const AsyncWebParameter *p = request->getParam(0);
                //Serial.println(p->name());
                char cmd = p->name().charAt(0);

                // hack, problem with '+' command received via url param
                if (cmd == ' ') {
                    cmd = '+';
                }
                queueWebCommand('s', cmd);
            }
            request->send(200); // reply
        }
    });

    server.on("/uc", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (ESP.getFreeHeap() > 10000) {
            int params = request->params();
            //Serial.print("got user request params: ");
            //Serial.println(params);
            if (params > 0) {
                const AsyncWebParameter *p = request->getParam(0);
                //Serial.println(p->name());
                queueWebCommand('u', p->name().charAt(0));
            }
            request->send(200); // reply
        }
    });

    server.on("/wifi/connect", HTTP_POST, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response =
            request->beginResponse(200, "application/json", "true");
        request->send(response);

        if (request->arg("n").length()) {     // n holds ssid
            if (request->arg("p").length()) { // p holds password
                // false = only save credentials, don't connect
                WiFi.begin(request->arg("n").c_str(), request->arg("p").c_str(), 0, 0, false);
            } else {
                WiFi.begin(request->arg("n").c_str(), emptyString, 0, 0, false);
            }
        } else {
            WiFi.begin();
        }

        userCommand = 'u'; // next loop, set wifi station mode and restart device
    });

    server.on("/bin/slots.bin", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (ESP.getFreeHeap() > 10000) {
            ensureSlotsFile();
            request->send(SPIFFS, SLOTS_FILE, "application/octet-stream");
        }
    });

    server.on("/slot/set", HTTP_GET, [](AsyncWebServerRequest *request) {
        bool result = false;

        if (ESP.getFreeHeap() > 10000) {
            int params = request->params();

            if (params > 0) {
                const AsyncWebParameter *slotParam = request->getParam(0);
                String slotParamValue = slotParam->value();
                char slotValue[2];
                slotParamValue.toCharArray(slotValue, sizeof(slotValue));
                uopt->presetSlot = (uint8_t)slotValue[0];
                uopt->presetPreference = OutputCustomized;
                saveUserPrefs();
                result = true;
            }
        }

        request->send(200, "application/json", result ? "true" : "false");
    });

    server.on("/slot/save", HTTP_GET, [](AsyncWebServerRequest *request) {
        bool result = false;

        if (ESP.getFreeHeap() > 10000) {
            int params = request->params();

            if (params > 0) {
                SlotMetaArray slotsObject;
                readOrCreateSlots(slotsObject);

                // index param
                const AsyncWebParameter *slotIndexParam = request->getParam(0);
                String slotIndexString = slotIndexParam->value();
                uint8_t slotIndex = lowByte(slotIndexString.toInt());
                if (slotIndex >= SLOTS_TOTAL) {
                    goto fail;
                }

                // name param
                const AsyncWebParameter *slotNameParam = request->getParam(1);
                String slotName = slotNameParam->value();

                char emptySlotName[25] = "                        ";
                strncpy(slotsObject.slot[slotIndex].name, emptySlotName, 25);

                slotsObject.slot[slotIndex].slot = slotIndex;
                slotName.toCharArray(slotsObject.slot[slotIndex].name, sizeof(slotsObject.slot[slotIndex].name));
                slotsObject.slot[slotIndex].presetID = rto->presetID;
                slotsObject.slot[slotIndex].scanlines = uopt->wantScanlines;
                slotsObject.slot[slotIndex].scanlinesStrength = uopt->scanlineStrength;
                slotsObject.slot[slotIndex].wantVdsLineFilter = uopt->wantVdsLineFilter;
                slotsObject.slot[slotIndex].wantStepResponse = uopt->wantStepResponse;
                slotsObject.slot[slotIndex].wantPeaking = uopt->wantPeaking;

                writeSlots(slotsObject);

                result = true;
            }
        }

        fail:
        request->send(200, "application/json", result ? "true" : "false");
    });

    server.on("/slot/remove", HTTP_GET, [](AsyncWebServerRequest *request) {
        bool result = false;
        int params = request->params();
        if (params > 0)
        {
            // fix: read the parameter only if there is one (was a null pointer access)
            char param = request->getParam(0)->name().charAt(0);
            if (param == '0')
            {
                SerialM.println("Wait...");
                result = true;
            }
            else
            {
                Ascii8 slot = uopt->presetSlot;
                Ascii8 nextSlot;
                auto currentSlot = slotIndexMap.indexOf(slot);

                SlotMetaArray slotsObject;
                // fix: unknown slot or missing slots file would index outside of slotsObject
                if (currentSlot < 0 || currentSlot >= SLOTS_TOTAL || !readSlots(slotsObject)) {
                    request->send(200, "application/json", "false");
                    return;
                }
                String slotName = slotsObject.slot[currentSlot].name;

                // remove preset files
                for (size_t i = 0; i < PRESET_FILE_COUNT; i++) {
                    SPIFFS.remove(presetFileNameAt(i, slot));
                }

                uint8_t loopCount = 0;
                uint8_t flag = 1;
                // fix: stop at the last slot (was writing past the end of slotsObject)
                while (flag != 0 && (currentSlot + loopCount + 1) < SLOTS_TOTAL)
                {
                    slot = slotIndexMap[currentSlot + loopCount];
                    nextSlot = slotIndexMap[currentSlot + loopCount + 1];
                    flag = 0;
                    for (size_t i = 0; i < PRESET_FILE_COUNT; i++) {
                        flag += SPIFFS.rename(presetFileNameAt(i, nextSlot), presetFileNameAt(i, slot));
                    }

                    slotsObject.slot[currentSlot + loopCount].slot = slotsObject.slot[currentSlot + loopCount + 1].slot;
                    slotsObject.slot[currentSlot + loopCount].presetID = slotsObject.slot[currentSlot + loopCount + 1].presetID;
                    slotsObject.slot[currentSlot + loopCount].scanlines = slotsObject.slot[currentSlot + loopCount + 1].scanlines;
                    slotsObject.slot[currentSlot + loopCount].scanlinesStrength = slotsObject.slot[currentSlot + loopCount + 1].scanlinesStrength;
                    slotsObject.slot[currentSlot + loopCount].wantVdsLineFilter = slotsObject.slot[currentSlot + loopCount + 1].wantVdsLineFilter;
                    slotsObject.slot[currentSlot + loopCount].wantStepResponse = slotsObject.slot[currentSlot + loopCount + 1].wantStepResponse;
                    slotsObject.slot[currentSlot + loopCount].wantPeaking = slotsObject.slot[currentSlot + loopCount + 1].wantPeaking;
                    // slotsObject.slot[currentSlot + loopCount].name = slotsObject.slot[currentSlot + loopCount + 1].name;
                    strncpy(slotsObject.slot[currentSlot + loopCount].name, slotsObject.slot[currentSlot + loopCount + 1].name, 25);
                    loopCount++;
                }

                writeSlots(slotsObject);
                SerialM.println("Preset \"" + slotName + "\" removed");
                result = true;
            }
        }

        request->send(200, "application/json", result ? "true" : "false");
    });

    server.on("/spiffs/upload", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "application/json", "true");
    });

    server.on(
        "/spiffs/upload", HTTP_POST,
        [](AsyncWebServerRequest *request) { request->send(200, "application/json", "true"); },
        [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
            if (!index) {
                request->_tempFile = SPIFFS.open("/" + filename, "w");
            }
            if (len) {
                request->_tempFile.write(data, len);
            }
            if (final) {
                request->_tempFile.close();
            }
        });

    server.on("/spiffs/download", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (ESP.getFreeHeap() > 10000) {
            int params = request->params();
            if (params > 0) {
                request->send(SPIFFS, request->getParam(0)->value(), String(), true);
            } else {
                request->send(200, "application/json", "false");
            }
        } else {
            request->send(200, "application/json", "false");
        }
    });

    server.on("/spiffs/dir", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (ESP.getFreeHeap() > 10000) {
            Dir dir = SPIFFS.openDir("/");
            String output = "[";

            while (dir.next()) {
                output += "\"";
                output += dir.fileName();
                output += "\",";
                delay(1); // wifi stack
            }

            output += "]";

            output.replace(",]", "]");

            request->send(200, "application/json", output);
            return;
        }
        request->send(200, "application/json", "false");
    });

    server.on("/spiffs/format", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "application/json", SPIFFS.format() ? "true" : "false");
    });

    server.on("/wifi/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        WiFiMode_t wifiMode = WiFi.getMode();
        request->send(200, "application/json", wifiMode == WIFI_AP ? "{\"mode\":\"ap\"}" : "{\"mode\":\"sta\",\"ssid\":\"" + WiFi.SSID() + "\"}");
    });

    server.on("/gbs/restore-filters", HTTP_GET, [](AsyncWebServerRequest *request) {
        SlotMetaArray slotsObject;
        bool result = false;
        if (readSlots(slotsObject)) {
            auto currentSlot = slotIndexMap.indexOf(uopt->presetSlot);
            if (currentSlot == -1) {
                goto fail;
            }

            uopt->wantScanlines = slotsObject.slot[currentSlot].scanlines;

            SerialM.print(F("slot: "));
            SerialM.println(uopt->presetSlot);
            SerialM.print(F("scanlines: "));
            if (uopt->wantScanlines) {
                SerialM.println(F("on (Line Filter recommended)"));
            } else {
                disableScanlines();
                SerialM.println("off");
            }
            saveUserPrefs();

            uopt->scanlineStrength = slotsObject.slot[currentSlot].scanlinesStrength;
            uopt->wantVdsLineFilter = slotsObject.slot[currentSlot].wantVdsLineFilter;
            uopt->wantStepResponse = slotsObject.slot[currentSlot].wantStepResponse;
            uopt->wantPeaking = slotsObject.slot[currentSlot].wantPeaking;
            result = true;
        }

        fail:
        request->send(200, "application/json", result ? "true" : "false");
    });

    // commands over the already open websocket: "s<cmd>" = serialCommand, "u<cmd>" = userCommand
    // saves a new tcp connection + http request per button press
    webSocket.onEvent([](uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
        if (type == WStype_TEXT && length == 2 && (payload[0] == 's' || payload[0] == 'u')) {
            queueWebCommand((char)payload[0], (char)payload[1]);
        }
    });

    persWM.setConnectNonBlock(true);
    if (WiFi.SSID().length() == 0) {
        // no stored network to connect to > start AP mode right away
        persWM.setupWiFiHandlers();
        persWM.startApMode();
    } else {
        persWM.begin(); // first try connecting to stored network, go AP mode after timeout
    }

    server.begin();    // Webserver for the site
    webSocket.begin(); // Websocket for interaction
    yield();

#ifdef HAVE_PINGER_LIBRARY
    // pinger library
    pinger.OnReceive([](const PingerResponse &response) {
        if (response.ReceivedResponse) {
            Serial.printf(
                "Reply from %s: time=%lums\n",
                response.DestIPAddress.toString().c_str(),
                response.ResponseTime);

            pingLastTime = millis() - 900; // produce a fast stream of pings if connection is good
        } else {
            Serial.printf("Request timed out.\n");
        }

        // Return true to continue the ping sequence.
        // If current event returns false, the ping sequence is interrupted.
        return true;
    });

    pinger.OnEnd([](const PingerResponse &response) {
        // detailed info not necessary
        return true;
    });
#endif
}

void initUpdateOTA()
{
    ArduinoOTA.setHostname("GBS OTA");

    // ArduinoOTA.setPassword("admin");
    // Password can be set with it's md5 value as well
    // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
    //ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

    // update: no password is as (in)secure as this publicly stated hash..
    // rely on the user having to enable the OTA feature on the web ui

    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
            type = "sketch";
        else // U_SPIFFS
            type = "filesystem";

        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        SPIFFS.end();
        SerialM.println("Start updating " + type);
    });
    ArduinoOTA.onEnd([]() {
        SerialM.println("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        SerialM.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        SerialM.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR)
            SerialM.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR)
            SerialM.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR)
            SerialM.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR)
            SerialM.println("Receive Failed");
        else if (error == OTA_END_ERROR)
            SerialM.println("End Failed");
    });
    ArduinoOTA.begin();
    yield();
}

#endif
#endif
