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

#include "tusb.h"
#include "HostPort.h"

HostPort gHostPort;

SerialConfig HostPort::sSerialConfig = { SCL_DEFAULT_BAUD_RATE, 8, 1, SerialConfig::PARITY_NONE };
static bool sSerialConfigChanged;

void HostPort::Init(void)
{
    // Initialize stdio
    // Disable automatic translation of CR/LF
    stdio_usb_init();
    stdio_set_translate_crlf(&stdio_usb, false);
}

bool HostPort::ConfigChanged(void)
{
    return sSerialConfigChanged;
}

const SerialConfig& HostPort::GetConfig(void)
{
    if (sSerialConfigChanged) {

        sSerialConfigChanged = false;

        // Get the serial configuration sent from the host (known as the USB CDC line
        // coding configuration).
        cdc_line_coding_t lineConfig;
        tud_cdc_get_line_coding(&lineConfig);

        // Accept the proposed baud rate if it is within the supported range.
        if (lineConfig.bit_rate >= MIN_BAUD_RATE && lineConfig.bit_rate <= MAX_BAUD_RATE) {
            sSerialConfig.BitRate = lineConfig.bit_rate;
        }

        // Accept the serial format if supported.
        //
        // Supported formats are 8-N-1, 8-N-2, 7-E-1, 7-E-2, 7-O-1 and 7-O-2.
        //
        // The SCL port on the PDP11/05 is hard-wired to use 8-N-2 format.
        // However with the appropriate software on the PDP side, it can be
        // made to look like 7-E-2 or 7-O-2 to the connected terminal (and
        // indeed, Mini-UNIX does exactly that).
        //
        // 2 stop bits is unusual in modern systems, and most termimal programs
        // default to 1 stop bit. So as a convenience, the code accepts 1 stop
        // bit, but forces the use of 2 bits over the SCL port.
        //
        // Both 1 and 2 stop bits are supported for the Aux port.
        // 
        if (lineConfig.stop_bits == CDC_LINE_CODING_STOP_BITS_1 ||
            lineConfig.stop_bits == CDC_LINE_CODING_STOP_BITS_2) {
        
            if (lineConfig.data_bits == 8 && lineConfig.parity == CDC_LINE_CODING_PARITY_NONE) {
                sSerialConfig.DataBits = 8;
                sSerialConfig.Parity = SerialConfig::PARITY_NONE;
                sSerialConfig.StopBits = (lineConfig.stop_bits == CDC_LINE_CODING_STOP_BITS_1) ? 1 : 2;
            }
            else if (lineConfig.data_bits == 7 && lineConfig.parity == CDC_LINE_CODING_PARITY_EVEN) {
                sSerialConfig.DataBits = 7;
                sSerialConfig.Parity = SerialConfig::PARITY_EVEN;
                sSerialConfig.StopBits = (lineConfig.stop_bits == CDC_LINE_CODING_STOP_BITS_1) ? 1 : 2;
            }
            else if (lineConfig.data_bits == 7 && lineConfig.parity == CDC_LINE_CODING_PARITY_ODD) {
                sSerialConfig.DataBits = 7;
                sSerialConfig.Parity = SerialConfig::PARITY_ODD;
                sSerialConfig.StopBits = (lineConfig.stop_bits == CDC_LINE_CODING_STOP_BITS_1) ? 1 : 2;
            }
        }
    }

    return sSerialConfig;
}

// Called by the USB stack to signal that the USB host has changed the serial
// configuraiton.
extern "C"
void tud_cdc_line_coding_cb(uint8_t /* itf */, cdc_line_coding_t const* /* p_line_coding */)
{
    sSerialConfigChanged = true;
}
