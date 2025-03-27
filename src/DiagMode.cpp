/*
 * Copyright 2024-2025 Jay Logue
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ConsoleAdapter.h"
#include "Settings.h"
#include "Menu.h"

void DiagMode_BasicIOTest(Port& uiPort)
{
    char ch, lastSent = 0, lastRcvd = 0;
    uint32_t mismatchCount = 0;
    uint64_t nextSendTime = 0;
    bool paused = false;
    constexpr uint64_t kMaxSendIntervalUS = 500000;

    uiPort.Write(
        "*** BASIC I/O TEST\r\n"
        "\r\n"
        "Repeatedly send characters and verify that each character is echoed.\r\n"
        "\r\n"
        "Test with PDP-11/05 console echo program:\r\n"
        "\r\n"
        "  001000: 012700  start:   mov #CRSR,r0    ; get Console RSR address in r0\r\n"
        "  001002: 177560\r\n"
        "  001004: 105710  waitrx:  tstb (r0)       ; if no input character available...\r\n"
        "  001006: 002376           bge waitrx      ;    continue waiting\r\n"
        "  001010: 116001           movb 2(r0), r1  ; read input character\r\n"
        "  001012: 000002\r\n"
        "  001014: 110160           movb r1, 6(r0)  ; echo character\r\n"
        "  001016: 000006\r\n"
        "  001020: 000771           br waitrx       ; loop\r\n"
        "\r\n"
        "SPACE to pause/resume test\r\n"
        "Ctrl+C to exit test\r\n"
        "\r\n"
    );

    while (true) {
        bool updateStatus = false;

        // Update the state of the activity LEDs
        ActivityLED::UpdateState();

        // Listen for control keys
        if (TryReadHostAuxPorts(ch)) {
            if (ch == CTRL_C) {
                uiPort.Write("\r\n*** EXIT\r\n");
                break;
            }
            if (ch == ' ') {
                paused = !paused;
                updateStatus = true;
            }
        }

        // Send character to PDP-11
        if (time_us_64() >= nextSendTime && !paused) {
            if (nextSendTime != 0 && lastRcvd != lastSent) {
                mismatchCount++;
            }
            lastSent++;
            gSCLPort.Write(lastSent);
            nextSendTime = time_us_64() + kMaxSendIntervalUS;
            updateStatus = true;
        }

        // Receive character from PDP-11 and check against last sent character
        if (gSCLPort.TryRead(lastRcvd)) {
            if (lastRcvd == lastSent) {
                nextSendTime = 0; // send next character immediately
            }
            else {
                mismatchCount++;
            }
            updateStatus = true;
        }

        if (updateStatus) {
            uiPort.Printf("\rLast Sent: %3d, Last Rcvd: %3d, Mismaches: %d", 
                lastSent, lastRcvd, mismatchCount);
        }
    }
}

void DiagMode_ReaderRunTest(Port& uiPort)
{
    char ch;
    int lastRRState = -1;
    uint8_t localRRCount = 0;
    uint8_t pdp11RRCount = 0;
    bool paused = false;

    uiPort.Write(
        "*** READER RUN INTERFACE TEST\r\n"
        "\r\n"
        "Send characters in response to RR signals, count RR signals seen and compare\r\n"
        "to counts received from the PDP-11.\r\n"
        "\r\n"
        "Test with PDP-11/05 READER RUN TEST program:\r\n"
        "\r\n"
        "  001000: 012700  start:   mov #CRSR,r0    ; get Console RSR address in r0\r\n"
        "  001002: 177560\r\n"
        "  001004: 005001           clr r1          ; clear counter register\r\n"
        "  001006: 105210  loop:    incb (r0)       ; enable READER RUN\r\n"
        "  001010: 105710  waitrx:  tstb (r0)       ; if no input character available...\r\n"
        "  001012: 002376           bge waitrx      ;    continue waiting\r\n"
        "  001014: 116002           movb 2(r0), r2  ; read input character\r\n"
        "  001016: 000002\r\n"
        "  001020: 105201           incb r1         ; increment counter\r\n"
        "  001022: 110160           movb r1, 6(r0)  ; output counter value\r\n"
        "  001024: 000006\r\n"
        "  001026: 000767           br loop         ; continue receiving\r\n"
        "\r\n"
        "SPACE to pause/resume test\r\n"
        "Ctrl+C to exit test\r\n"
        "\r\n"
    );

    while (true) {
        bool updateStatus = false;

        // Update the state of the activity LEDs
        ActivityLED::UpdateState();

        if (TryReadHostAuxPorts(ch)) {
            if (ch == CTRL_C) {
                uiPort.Write("\r\n*** EXIT\r\n");
                break;
            }
            if (ch == ' ') {
                paused = !paused;
                updateStatus = true;
            }
        }

        // Get RR count from PDP-11
        if (gSCLPort.TryRead(ch)) {
            pdp11RRCount = (uint8_t)ch;
            updateStatus = true;
        }

        int rrState = (int)gpio_get(READER_RUN_PIN);
        if (rrState != lastRRState) {
            lastRRState = rrState;
            updateStatus = true;
        }

        if (gSCLPort.ReaderRunRequested() && !paused) {
            gSCLPort.ClearReaderRunRequested();
            localRRCount++;
            gSCLPort.Write('.');
            updateStatus = true;
        }

        if (updateStatus) {
            bool mismatch = (((int)localRRCount - (int)pdp11RRCount) > 1);

            uiPort.Printf("\rREADER RUN: %s, local RR count: %3u, PDP-11 RR count: %3u %s", 
                rrState ? "HIGH" : " LOW",
                localRRCount,
                pdp11RRCount,
                (mismatch) ? "[MISMATCH]" : "          ");
        }
    }
}

extern "C" uint8_t __SettingsStorageStart;
extern "C" uint8_t __SettingsStorageEnd;

void DiagMode_SettingsTest(Port& uiPort)
{
    static const MenuItem sMenuItems[] = {
        { 's', "Save settings"                  },
        { 't', "Save settings x10"      },
        MenuItem::SEPARATOR(),
        { '\e', "Return to terminal mode"       },
        MenuItem::HIDDEN(CTRL_C),
        MenuItem::END()
    };

    static const Menu sMenu = {
        .Title = "SETTINGS TEST:",
        .Items = sMenuItems,
        .NumCols = 2,
        .ColWidth = -1,
        .ColMargin = 2
    };

    while (true) {
        Settings::PrintStats(uiPort);

        sMenu.Show(uiPort);

        switch (sMenu.GetSelection(uiPort)) {
        case 's':
            Settings::Save();
            break;
        case 't':
            for (int i = 0; i < 10; i++) {
                Settings::Save();
            }
            break;
        default:
            return;
        }
    }
}