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

#include <ctype.h>

#include "ConsoleAdapter.h"
#include "Settings.h"
#include "PTRProgressBar.h"

bool gTransparentMode;

static void HandleHostSerialConfigChange(void);

void TerminalMode(void)
{
    char ch;
    Port *uiPort, *lastUIPort = &gHostPort;
    bool pdp11Powered = false;
    
    while (true) {

        // Update the status of the SCL port
        gSCLPort.UpdateState();

        // Track the power state of the PDP-11; on a transition from
        // powered to unpowered, clear transparent mode, if enabled.
        if (gSCLPort.IsPowered()) {
            pdp11Powered = true;
        }
        else if (pdp11Powered) {
            pdp11Powered = false;
            gTransparentMode = false;
        }

        // Handle requests from the USB host to change the serial configuration.
        if (gHostPort.ConfigChanged()) {
            HandleHostSerialConfigChange();
        }

        // If the PDP-11 has triggered the READER RUN line and there's data
        // available to be read from a paper tape file, then deliver a single
        // byte from the paper tape to the SCL port.
        if (gSCLPort.ReaderRunRequested() && PaperTapeReader::TryRead(ch)) {
            gSCLPort.ClearReaderRunRequested();
            gSCLPort.Write(ch);

            // Display/update the paper taper reader progress bar
            PTRProgressBar::Update(lastUIPort);
        }

        // Process characters received from either the USB host or the auxiliary terminal.
        if (gSCLPort.CanWrite() && TryReadHostAuxPorts(ch, uiPort)) {

            // If not in transparent mode and the menu key was pressed,
            // enter menu mode
            if (!gTransparentMode && ch == MENU_KEY) {
                PTRProgressBar::Clear();
                MenuMode(*uiPort);
            }

            // Otherwise forward the character to the SCL port...
            else {

                // If in uppercase mode and NOT in transparent mode, convert the
                // character to uppercase before forwarding it.
                if (!gTransparentMode && Settings::UppercaseMode) {
                    ch = toupper(ch);
                }

                gSCLPort.Write(ch);
            }

            lastUIPort = uiPort;
        }

        // Forward characters received from the SCL port to both the Host and Aux ports
        if (gSCLPort.TryRead(ch)) {
            PTRProgressBar::Clear();
            WriteHostAuxPorts(ch);
        }

        // Update the state of the activity LEDs
        ActivityLED::UpdateState();
    }
}

void HandleHostSerialConfigChange(void)
{
    SerialConfig newConfig = gHostPort.GetConfig();

    if (Settings::SCLConfigFollowsUSB) {
        gSCLPort.SetConfig(newConfig);
    }

#if defined(AUX_TERM_UART)
    if (Settings::AuxConfigFollowsUSB) {
        gAuxPort.SetConfig(newConfig);
    }
#endif
}
