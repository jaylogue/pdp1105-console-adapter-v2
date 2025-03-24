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

        // Accept the serial format if the proposed format is supported.
        //
        // The serial port on the PDP11/05 is hard-wired to use 8-N-1 format.
        // However with the appropriate software on the PDP side, it can be
        // made to look like 7-E-1 or 7-O-1 to the connected terminal (and
        // indeed, Mini-UNIX does exactly that).  Therefore, we limit the
        // serial format to one of those three choices. All other combinations
        // are silently ignored.
        if (lineConfig.data_bits == 8 &&
            lineConfig.parity == CDC_LINE_CODING_PARITY_NONE &&
            lineConfig.stop_bits == CDC_LINE_CODING_STOP_BITS_1) {
            sSerialConfig.DataBits = 8;
            sSerialConfig.Parity = SerialConfig::PARITY_NONE;
            sSerialConfig.StopBits = 1;
        }
        else if (lineConfig.data_bits == 7 && 
                 lineConfig.parity == CDC_LINE_CODING_PARITY_EVEN &&
                 lineConfig.stop_bits == CDC_LINE_CODING_STOP_BITS_1) {
            sSerialConfig.DataBits = 7;
            sSerialConfig.Parity = SerialConfig::PARITY_EVEN;
            sSerialConfig.StopBits = 1;
        }
        else if (lineConfig.data_bits == 7 && 
                 lineConfig.parity == CDC_LINE_CODING_PARITY_ODD &&
                 lineConfig.stop_bits == CDC_LINE_CODING_STOP_BITS_1) {
            sSerialConfig.DataBits = 7;
            sSerialConfig.Parity = SerialConfig::PARITY_ODD;
            sSerialConfig.StopBits = 1;
        }
    }

    return sSerialConfig;
}

// Called by the USB stack to signal that the USB host has changed the serial
// configuraiton.
extern "C"
void tud_cdc_line_coding_cb(uint8_t itf, cdc_line_coding_t const* p_line_coding)
{
    sSerialConfigChanged = true;
}
