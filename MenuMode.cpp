#include <inttypes.h>
#include <string.h>
#include <ctype.h>
#include <functional>

#include "ConsoleAdapter.h"
#include "BootstrapDataSource.h"
#include "LDADataSource.h"

static void MountPaperTape(Port& uiPort);
static void PaperTapeStatus(Port& uiPort);
static char GetMenuSelection(Port& uiPort, std::function<bool(char)> isValidSelection);
static char GetMenuSelection(Port& uiPort, const char validSelections[]);
static size_t SelectFile(Port& uiPort);
static void LoadData(Port& uiPort);
static bool GetInteger(Port& uiPort, uint32_t& val, uint32_t defaultVal);

static uint32_t sDefaultMemSizeKW = 28;

#define CTRL_C '\x03'
#define BS '\x08'
#define DEL '\x7F'
#define RUBOUT "\x08 \x08"

void MenuMode(Port& uiPort)
{
    uiPort.Write("\r\n*** MAIN MENU:\r\n"
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
        return FileSet::IsValidFileKey(ch) || ch == CTRL_C;
    };

    uiPort.Write("*** SELECT FILE:\r\n");
    FileSet::ShowMenu(uiPort);

    char selectedFile = GetMenuSelection(uiPort, isValidSel);

    if (selectedFile != CTRL_C) {
        const char *fileName;
        const uint8_t *fileData;
        size_t fileLen;
        FileSet::GetFile(selectedFile, fileName, fileData, fileLen);
        PaperTapeReader::Mount(fileName, fileData, fileLen);
    }
}

void PaperTapeStatus(Port& uiPort)
{
    uiPort.Write("*** PAPER TAPE STATUS:");
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

void LoadData(Port& uiPort)
{
    auto isValidSel = [](char ch) -> bool {
        return FileSet::IsValidFileKey(ch) || ch == 'B' || ch == CTRL_C;
    };

    uiPort.Write("*** SELECT FILE:\r\n");
    uiPort.Write("  B: Bootstrap Loader\r\n\r\n");
    FileSet::ShowMenu(uiPort);

    char selectedFile = GetMenuSelection(uiPort, isValidSel);

    if (selectedFile == CTRL_C) {
        return;
    }

    if (selectedFile == 'B') {

        uint32_t memSize;

        do {
            uiPort.Printf("*** SYSTEM MEMORY SIZE (KW) [%" PRId32 "]: ", sDefaultMemSizeKW);
            if (!GetInteger(uiPort, memSize, sDefaultMemSizeKW)) {
                return;
            }
        } while (memSize < 4 || memSize > 28);

        sDefaultMemSizeKW = memSize;

        uint16_t loadAddr = BootstrapDataSource::MemSizeToLoadAddr(memSize);

        BootstrapDataSource bsLoader(loadAddr);

        LoadDataMode(uiPort, bsLoader);
    }

    else {
        const char *fileName;
        const uint8_t *fileData;
        size_t fileLen;
        FileSet::GetFile(selectedFile, fileName, fileData, fileLen);

        if (LDAReader::IsValidLDAFile(fileData, fileLen)) {
            LDADataSource ldaSource(fileName, fileData, fileLen);
            LoadDataMode(uiPort, ldaSource);
        }
        else {
            stdio_printf("Not LDA file\r\n");
        }
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

bool GetInteger(Port& uiPort, uint32_t& val, uint32_t defaultVal)
{
    int charCount = 0;
    char ch;

    val = 0;

    while (true) {
        // Update the state of the activity LEDs
        ActivityLED::UpdateState();

        // Update the connection status of the SCL port
        gSCLPort.CheckConnected();

        if (uiPort.TryRead(ch)) {

            if (ch == CTRL_C) {
                uiPort.Write("^C\r\n");
                return false;
            }

            if (ch == '\r') {
                if (charCount == 0) {
                    val = defaultVal;
                }
                uiPort.Write("\r\n");
                return true;
            }

            if (isdigit(ch)) {
                uiPort.Write(ch);
                val = (val * 10) + ch - '0';
                charCount++;
            }

            else if ((ch == BS || ch == DEL) && charCount > 0) {
                uiPort.Write(RUBOUT);
                val = val / 10;
                charCount--;
            }
        }
    }
}