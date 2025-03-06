#include "ConsoleAdapter.h"

void TerminalMode(void)
{
    char ch;

    while (true) {

        // Update the connection status of the SCL port
        SCLPort::CheckConnected();

        // Handle requests from the USB host to change the serial configuration.
        if (HostPort::SerialConfigChanged()) {
            SerialConfig serialConfig;
            HostPort::GetSerialConfig(serialConfig);
            SCLPort::SetConfig(serialConfig);
            AuxPort::SetConfig(serialConfig);
        }

        // If the PDP-11 has triggered the READER RUN line, and there is a paper
        // tape file attached, deliver a single byte from the paper tape file to
        // the SCL port.
        if (SCLPort::ReaderRunRequested() && PaperTapeReader::TryRead(ch)) {
            SCLPort::Write(ch);
        }

        // Process characters received from either the USB host or the auxiliary terminal.
        if (SCLPort::CanWrite() && TryReadHostAuxPorts(ch)) {

            // If the menu key was pressed enter menu mode
            if (ch == MENU_KEY) {
                MenuMode();
            }

            // Otherwise forward the character to the SCL port.
            else {
                SCLPort::Write(ch);
            }
        }

        // Forward characters received from the SCL port to both the Host and Aux terminals
        if (SCLPort::TryRead(ch)) {
            WriteHostAuxPorts(ch);
        }

        // Update the state of the activity LEDs
        ActivityLED::UpdateState();
    }
}
