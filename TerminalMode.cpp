
#include <stdio.h>
#include <stdint.h>

#include "ConsoleAdapter.h"

void TerminalMode_Start(void)
{
    if (GlobalState::SystemState != SystemState::TerminalMode) {
        WriteHostAux("*** TERMINAL MODE ***\r\n");
        GlobalState::SystemState = SystemState::TerminalMode;
    }
}

void TerminalMode_ProcessIO(void)
{
    char ch;

    // Process characters received from either the USB host or the auxiliary terminal.
    if (CanWriteSCL() && TryReadHostAux(ch)) {

        GlobalState::Active = true;

        // If the menu key was pressed enter menu mode
        if (ch == MENU_KEY) {
            MenuMode_Start();
            return;
        }

        // Otherwise forward the character to the SCL port.
        WriteSCL(ch);
    }

    // Forward characters received from the PDP-11 SCL port to both the Host and Aux terminals
    if (TryReadSCL(ch)) {

        GlobalState::Active = true;

        WriteHostAux(ch);
    }
}
