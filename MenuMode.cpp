#include "ConsoleAdapter.h"
#include "BootstrapLoaderSource.h"

static void TestLoadDataMode(void);

void MenuMode(void)
{
    char ch;

    while (true) {
        WriteHostAuxPorts("*** MENU:\r\n"
                          "m      - Mount paper tape\r\n"
                          "u      - Unmount paper tape\r\n"
                          "l      - Load data using M93xx console\r\n"
                          "x      - Upload file via XMODEM\r\n"
                          "Ctrl+^ - Send Ctrl+^ character\r\n"
                          "q      - Return to terminal mode\r\n"
                          ": ");

        while (!TryReadHostAuxPorts(ch)) {
        }

        if (ch != MENU_KEY) {
            WriteHostAuxPorts(ch);
            WriteHostAuxPorts("\r\n");
        }

        switch (ch) {
        case MENU_KEY:
            SCLPort::Write(MENU_KEY);
            return;
        case 'l':
            TestLoadDataMode();
            return;
        case 'q':
            return;
        default:
            break;
        }
    }
}

void TestLoadDataMode(void)
{
    BootstrapLoaderSource bsLoader(017744);

    LoadDataMode(&bsLoader);
}