#include "ConsoleAdapter.h"

void TerminalMode(void)
{
    char ch;
    Port * uiPort;

    while (true) {

        // Update the connection status of the SCL port
        gSCLPort.CheckConnected();

        // Handle requests from the USB host to change the serial configuration.
        if (gHostPort.SerialConfigChanged()) {
            SerialConfig serialConfig;
            gHostPort.GetSerialConfig(serialConfig);
            gSCLPort.SetConfig(serialConfig);
#if defined(AUX_TERM_UART)
            gAuxPort.SetConfig(serialConfig);
#endif
        }

        // If the PDP-11 has triggered the READER RUN line, and there is a paper
        // tape file attached, deliver a single byte from the paper tape file to
        // the SCL port.
        if (gSCLPort.ReaderRunRequested() && PaperTapeReader::TryRead(ch)) {
            gSCLPort.Write(ch);
        }

        // Process characters received from either the USB host or the auxiliary terminal.
        if (gSCLPort.CanWrite() && TryReadHostAuxPorts(ch, uiPort)) {

            // If the menu key was pressed enter menu mode
            if (ch == MENU_KEY) {
                MenuMode(*uiPort);
            }

            // Otherwise forward the character to the SCL port.
            else {
                gSCLPort.Write(ch);
            }
        }

        // Forward characters received from the SCL port to both the Host and Aux terminals
        if (gSCLPort.TryRead(ch)) {
            WriteHostAuxPorts(ch);
        }

        // Update the state of the activity LEDs
        ActivityLED::UpdateState();
    }
}
