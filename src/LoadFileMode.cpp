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
#include "M93xxController.h"

void LoadFileMode(Port& uiPort, LoadDataSource& dataSrc, const char * fileName)
{
    M93xxController m93xxCtr;
    bool startAddrLoaded = false;

    uiPort.Printf("*** LOADING FILE: %s\r\n", fileName);

    while (true) {
        char ch;
        uint16_t data, addr;

        // Update the state of the activity LEDs
        ActivityLED::UpdateState();

        // Update the connection status of the SCL port
        gSCLPort.CheckConnected();

        // Process any timeouts while talking to the M9301/M9312 console;
        // If the console is unresponsive, abort and return to terminal mode.
        if (m93xxCtr.ProcessTimeouts()) {
            uiPort.Write("*** TIMEOUT (no response from console)\r\n");
            break;
        }

        // Check for an interrupt character from the UI port.
        if (uiPort.TryRead(ch) && ch == CTRL_C) {
            uiPort.Write("*** INTERRUPTED\r\n");
            break;
        }

        // Try to read a character from the M9301/M9312 console; if successful...
        if (gSCLPort.TryRead(ch)) {

            // Pass the character to the M93xxController for processing.
            if (!m93xxCtr.ProcessOutput(ch)) {
                uiPort.Write("*** ERROR (unexpected response from console)\r\n");
                break;
            }

            // If the M9301/M9312 is now ready for another command, but all data
            // has been loaded, inform the user that:
            //   a) the start address is being loaded (if the data source has a
            //      start address)
            // or
            //   b) the load command is complete.
            // This is done *before* the character is echoed so that the message
            // appears ahead of the M9301/M9312 prompt.
            if (m93xxCtr.IsReadyForCommand() && dataSrc.AtEnd()) {
                if (dataSrc.GetStartAddress() != NO_ADDR && !startAddrLoaded) {
                    uiPort.Write("*** LOADING START ADDRESS\r\n");
                }
                else {
                    uiPort.Write("*** LOAD COMPLETE\r\n");
                }
            }

            // Echo the character from the M9301/M9312 console so the user sees the
            // commands as they are executed.
            WriteHostAuxPorts(ch);
        }

        // If the M9301/M9312 is still processing the current command, wait until
        // it's done.
        if (!m93xxCtr.IsReadyForCommand()) {
            continue;
        }

        // If the last data word has been deposited...
        if (dataSrc.AtEnd()) {

            // If the data source specifies a start address, issue a final set
            // address command (L) with the start address
            if (dataSrc.GetStartAddress() != NO_ADDR && !startAddrLoaded) {
                m93xxCtr.SetAddress(dataSrc.GetStartAddress());
                startAddrLoaded = true;
                continue;
            }

            // Otherwise the loading process is complete, so return to terminal mode
            break;
        }

        // Get the next word to be loaded from the data source, along with its
        // load address; if the next word is not ready yet, wait until it is.
        if (!dataSrc.GetWord(data, addr)) {
            continue;
        }

        // If the next deposit address is not equal to the load address for
        // the next word, issue a set address (L) command and wait for it
        // to complete.
        if (m93xxCtr.NextDepositAddress() != addr) {
            m93xxCtr.SetAddress(addr);
            continue;
        }

        // Issue a deposit command (D) to load the word into memory at the
        // target address
        m93xxCtr.Deposit(data);

        // Advance the data source to the next word
        dataSrc.Advance();
    }
}
