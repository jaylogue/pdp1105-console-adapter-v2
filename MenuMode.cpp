#include <inttypes.h>
#include <string.h>
#include <ctype.h>
#include <functional>

#include "ConsoleAdapter.h"
#include "BootstrapLoaderSource.h"

static void MountPaperTape(Port& uiPort);
static void PaperTapeStatus(Port& uiPort);
static char GetMenuSelection(Port& uiPort, std::function<bool(char)> isValidSelection);
static char GetMenuSelection(Port& uiPort, const char validSelections[]);
static size_t SelectFile(Port& uiPort);
static void LoadData(Port& uiPort);

#define CTRL_C '\x03'

void MenuMode(Port& uiPort)
{
    uiPort.Write("MAIN MENU:\r\n"
                 "  m: Mount paper tape         l: Load data using M93xx console\r\n"
                 "  u: Unmount paper tape       x: Upload file via XMODEM\r\n"
                 "  s: Paper tape status        S: Adapter settings\r\n"
                 "  q: Return to terminal mode  Ctrl+^: Send menu character\r\n");

    char sel = GetMenuSelection(uiPort, (const char[]){ 'm', 'u', 's', 'l', 'x', 'c', 'q', MENU_KEY, CTRL_C, 0 });

    switch (sel) {
    case MENU_KEY:
        gSCLPort.Write(MENU_KEY);
        break;
    case 'm':
        MountPaperTape(uiPort);
        break;
    case 'u':
        PaperTapeReader::Unmount();
        break;
    case 's':
        PaperTapeStatus(uiPort);
        break;
    case 'l':
        LoadData(uiPort);
        break;
    default:
        break;
    }
}

void MountPaperTape(Port& uiPort)
{
    auto isValidSel = [](char ch) -> bool {
        return BuiltInFileSet::IsValidFile(ch) || ch == CTRL_C;
    };

    uiPort.Write("SELECT FILE:\r\n");
    BuiltInFileSet::ShowMenu(uiPort);

    char selectedFile = GetMenuSelection(uiPort, isValidSel);

    if (selectedFile != CTRL_C) {
        const char *fileName;
        const uint8_t *fileData;
        size_t fileLen;
        BuiltInFileSet::GetFile(selectedFile, fileName, fileData, fileLen);
        PaperTapeReader::Mount(fileName, fileData, fileLen);
    }
}

void PaperTapeStatus(Port& uiPort)
{
    uiPort.Write("PAPER TAPE STATUS:");
    if (PaperTapeReader::IsMounted()) {
        uiPort.Printf("\r\n  Tape Name: %s\r\n  Position: %" PRIu32 "/%" PRIu32 " (%" PRIu32 "%%)\r\n", 
                      PaperTapeReader::TapeName(),
                      PaperTapeReader::TapePosition(),
                      PaperTapeReader::TapeLength(),
                      PaperTapeReader::TapePosition() * 100 / PaperTapeReader::TapeLength());
    }
    else {
        uiPort.Write(" No tape mounted\r\n");
    }
}

char GetMenuSelection(Port& uiPort, std::function<bool(char)> isValidSelection)
{
    char ch;

    uiPort.Write(": ");

    while (true) {
        // Update the state of the activity LEDs
        ActivityLED::UpdateState();

        // Update the connection status of the SCL port
        gSCLPort.CheckConnected();

        if (!uiPort.TryRead(ch)) {
            continue;
        }
    
        if (isValidSelection(ch)) {
            break;
        }

        uiPort.Write("?\x08");
    }

    if (isprint(ch)) {
        uiPort.Write(ch);
    }
    else if (ch == CTRL_C) {
        uiPort.Write("^C");
    }
    uiPort.Write("\r\n");

    return ch;
}

char GetMenuSelection(Port& uiPort, const char *validSelections)
{
    auto isValidSelection = [validSelections](char ch) -> bool {
        return strchr(validSelections, ch) != NULL;
    };
    return GetMenuSelection(uiPort, isValidSelection);
}

void LoadData(Port& uiPort)
{
    BootstrapLoaderSource bsLoader(017744);

    LoadDataMode(uiPort, bsLoader);
}