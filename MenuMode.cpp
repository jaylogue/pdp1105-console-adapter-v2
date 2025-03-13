#include <inttypes.h>
#include <string.h>
#include <ctype.h>
#include <functional>

#include "ConsoleAdapter.h"
#include "BootstrapDataSource.h"
#include "LDADataSource.h"
#include "SimpleDataSource.h"

static void MountPaperTape(Port& uiPort);
static void PaperTapeStatus(Port& uiPort);
static char GetMenuSelection(Port& uiPort, std::function<bool(char)> isValidSelection);
static char GetMenuSelection(Port& uiPort, const char validSelections[]);
static size_t SelectFile(Port& uiPort);
static void LoadData(Port& uiPort);
static void LoadBootstrapLoader(Port& uiPort);
static void LoadLDAFile(Port& uiPort, const char * fileName, const uint8_t * fileData, size_t fileLen);
static void LoadSimpleFile(Port& uiPort, const char * fileName, const uint8_t * fileData, size_t fileLen);
static bool GetInteger(Port& uiPort, uint32_t& val, int base, uint32_t defaultVal);

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
        LoadBootstrapLoader(uiPort);
        return;
    }

    const char *fileName;
    const uint8_t *fileData;
    size_t fileLen;
    FileSet::GetFile(selectedFile, fileName, fileData, fileLen);

    if (LDAReader::IsValidLDAFile(fileData, fileLen)) {
        LoadLDAFile(uiPort, fileName, fileData, fileLen);
        return;
    }

    LoadSimpleFile(uiPort, fileName, fileData, fileLen);
}

void LoadBootstrapLoader(Port& uiPort)
{
    uint32_t memSizeKW;
    static uint32_t sDefaultMemSizeKW = 28;

    do {
        uiPort.Printf("*** SYSTEM MEMORY SIZE (KW) [%" PRId32 "]: ", sDefaultMemSizeKW);
        if (!GetInteger(uiPort, memSizeKW, 10, sDefaultMemSizeKW)) {
            return;
        }
    } while (memSizeKW < 4 || memSizeKW > 28);

    sDefaultMemSizeKW = memSizeKW;

    uint16_t loadAddr = BootstrapDataSource::MemSizeToLoadAddr(memSizeKW);

    BootstrapDataSource bsLoader(loadAddr);

    const char nameFormat[] = "Bootstrap Loader (start address %06o)";
    char nameBuf[sizeof(nameFormat) + 6];
    snprintf(nameBuf, sizeof(nameBuf), nameFormat, loadAddr);

    LoadDataMode(uiPort, bsLoader, nameBuf);
}

void LoadLDAFile(Port& uiPort, const char * fileName, const uint8_t * fileData, size_t fileLen)
{
    LDADataSource dataSource(fileData, fileLen);

    const char nameFormat[] = "%s (LDA file)";
    char nameBuf[sizeof(nameFormat) + MAX_FILE_NAME_LEN];
    snprintf(nameBuf, sizeof(nameBuf), nameFormat, fileName);

    LoadDataMode(uiPort, dataSource, nameBuf);
}

void LoadSimpleFile(Port& uiPort, const char * fileName, const uint8_t * fileData, size_t fileLen)
{
    static uint32_t loadAddr;
    static uint32_t sDefaultLoadAddr = 0;

    do {
        uiPort.Printf("*** LOAD ADDRESS [%" PRId32 "]: ", sDefaultLoadAddr);
        if (!GetInteger(uiPort, loadAddr, 8, sDefaultLoadAddr)) {
            return;
        }
    } while (loadAddr > UINT16_MAX);

    sDefaultLoadAddr = loadAddr;

    SimpleDataSource dataSource(fileData, fileLen, loadAddr);

    const char nameFormat[] = "%s (simple data file, load address %06o)";
    char nameBuf[sizeof(nameFormat) + MAX_FILE_NAME_LEN + 6];
    snprintf(nameBuf, sizeof(nameBuf), nameFormat, fileName, loadAddr);

    LoadDataMode(uiPort, dataSource, nameBuf);
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

bool GetInteger(Port& uiPort, uint32_t& val, int base, uint32_t defaultVal)
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

            if ((base == 10 && isdigit(ch)) ||
                (base == 8 && ch >= '0' && ch <= '7')) {
                uiPort.Write(ch);
                val = (val * base) + (ch - '0');
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