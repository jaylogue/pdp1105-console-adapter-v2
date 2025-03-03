
#include <stdio.h>
#include <stdint.h>

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
        WriteHostAux("*** MENU ***\r\n"
                     "  l - Load built-in file using M92x3 monitor\r\n"
                     "  L - Load built-in file using paper tape reader\r\n"
                     "  u - Upload file using M92x3 monitor\r\n"
                     "  U - Upload file using paper tape reader\r\n"
                     "  q - Return to terminal mode\r\n"
                     );

        GlobalState::SystemState = SystemState::MenuMode;
        MenuState = MenuState::MainMenu;
    }
}

void MenuMode_ProcessIO(void)
{
    char ch;

    if (!TryReadHostAux(ch)) {
        return;
    }

    GlobalState::Active = true;

    switch (MenuState) {
    case MenuState::MainMenu:
        MenuOption = ch;
        switch (ch) {
        case 'q':
            TerminalMode_Start();
            break;
        case 'l':
            LoadFileMode_Start();
            break;
        default:
            break;
        }
    }
}
