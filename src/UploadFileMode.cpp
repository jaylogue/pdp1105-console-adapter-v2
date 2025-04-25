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

#include <stdio.h>
#include <stdint.h>

#include "xmodem_server.h"

#include "ConsoleAdapter.h"
#include "UploadFileMode.h"
#include "PaperTapeReader.h"

uint8_t gUploadedFile[MAX_UPLOAD_FILE_SIZE];
size_t gUploadedFileLen;

bool UploadFileMode(Port& uiPort)
{
    struct xmodem_server xdm;
    uint32_t blockNum;
    int rxLen;
    char ch;
    bool fileTooBig = false;

    const auto txChar = [](struct xmodem_server * /* xdm */, uint8_t byte, void *cbData) {
        Port& xmodemPort = *(Port *)cbData;
        xmodemPort.Write((char)byte);
    };

    gUploadedFileLen = 0;

    if (PaperTapeReader::TapeDataBuf() == gUploadedFile) {
        PaperTapeReader::Unmount();
    }

    xmodem_server_init(&xdm, txChar, &uiPort);

    uiPort.Write(TITLE_PREFIX "AWAITING FILE UPLOAD\r\n");

    while (!xmodem_server_is_done(&xdm)) {

        // Update the state of the activity LEDs
        ActivityLED::UpdateState();

        // If a character is availabled from the sender...
        if (uiPort.TryRead(ch)) {

            // If the file transfer is just starting, allow the sender to
            // abort by sending a Ctrl+C.
            xmodem_server_state state = xmodem_server_get_state(&xdm);
            if (state == XMODEM_STATE_START) {
                if (ch == CTRL_C) {
                    uiPort.Write(" " TITLE_PREFIX "FILE UPLOAD ABORTED\r\n");
                    return false;
                }
            }

            // Process the character received from the sender.
            xmodem_server_rx_byte(&xdm, (uint8_t)ch);

            // If a full packet has been received, check that it doesn't exceed the
            // maximum file size.  If it does, force the transfer to abort.
            if (xmodem_server_get_state(&xdm) == XMODEM_STATE_PROCESS_PACKET &&
                xdm.packet_size + gUploadedFileLen > sizeof(gUploadedFile)) {
                fileTooBig = true;
                uiPort.Write(CAN);
                uiPort.Write(CAN);
                xdm.error_count = UINT32_MAX;
            }
        }

        // Process received data and append it to the file buffer.  Check for any
        // transmission timeouts.
        rxLen = xmodem_server_process(&xdm, &gUploadedFile[gUploadedFileLen], &blockNum,
            (int64_t)(time_us_64() / 1000));
        if (rxLen > 0) {
            gUploadedFileLen += (size_t)rxLen;
        }
    }

    // Give the sender's transfer program some time to finish
    // During this time, discard any extraneous characters from the sender
    {
        uint64_t waitEndTime = time_us_64() + 250000;
        while (time_us_64() < waitEndTime) {
            ActivityLED::UpdateState();
            uiPort.TryRead(ch);
        }
    }

    if (xmodem_server_get_state(&xdm) != XMODEM_STATE_SUCCESSFUL) {
        uiPort.Printf(TITLE_PREFIX "FILE UPLOAD FAILED: %s\r\n",
                      (fileTooBig) ? "File too big" : "Communication error");
        gUploadedFileLen = 0;
        return false;
    }

    uiPort.Write(TITLE_PREFIX "FILE UPLOAD COMPLETE\r\n");
    return true;
}

void TrimXMODEMPadding(void)
{
    static constexpr char XMODEM_PAD = '\x1A';

    while (gUploadedFileLen > 0 && gUploadedFile[gUploadedFileLen-1] == XMODEM_PAD) {
        gUploadedFileLen--;
    }
}
