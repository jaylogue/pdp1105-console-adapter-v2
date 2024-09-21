
#include <stdio.h>
#include <stdint.h>

#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/pwm.h"

#include "tusb.h"

#include "ConsoleAdapter.h"

enum MenuState : uint
{
    MainMenu,
    SelectFile
};

static enum MenuState MenuState;
static char MenuOption;

void MenuMode_Start(void)
{
    if (GlobalState::SystemState != SystemState::MenuMode) {
        stdio_printf("*** MAIN MENU ***\r\n"
                     "  l - Load built-in file via M92x3 monitor\r\n"
                     "  L - Load built-in file via paper tape reader\r\n"
                     "  t - XMODEM download file via M92x3 monitor\r\n"
                     "  T - XMODEM download file via paper tape reader\r\n"
                     "  q - Return to terminal mode\r\n"
                     );

        GlobalState::SystemState = SystemState::MenuMode;
        MenuState = MenuState::MainMenu;
    }
}

void MenuMode_ProcessIO(void)
{
    char ch;

    if (stdio_get_until(&ch, 1, 0) != 1) {
        return;
    }

    GlobalState::Active = true;

    switch (MenuState) {
    case MenuState::MainMenu:
        MenuOption = ch;
        switch (ch) {
        case 'l':
            MenuState = SelectFile:
        }
    }
}
