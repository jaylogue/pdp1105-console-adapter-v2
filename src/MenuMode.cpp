#include <inttypes.h>
#include <string.h>
#include <ctype.h>
#include <functional>

#include "ConsoleAdapter.h"
#include "BootstrapLoader.h"
#include "AbsoluteLoader.h"
#include "LDADataSource.h"
#include "SimpleDataSource.h"
#include "UploadFileMode.h"

static void MountPaperTape(Port& uiPort);
static void PaperTapeStatus(Port& uiPort);
static void LoadFile(Port& uiPort);
static void LoadBootstrapLoader(Port& uiPort);
static void LoadAbsoluteLoader(Port& uiPort);
static void LoadLDAFile(Port& uiPort, const char * fileName, const uint8_t * fileData, size_t fileLen);
static void LoadSimpleFile(Port& uiPort, const char * fileName, const uint8_t * fileData, size_t fileLen);
static void UploadAndLoadFile(Port& uiPort);
static void LoadPreviouslyUploadedFile(Port& uiPort);
static char GetMenuSelection(Port& uiPort, std::function<bool(char)> isValidSelection);
static char GetMenuSelection(Port& uiPort, const char validSelections[]);
static bool GetSystemMemorySize(Port& uiPort, uint32_t& memSizeKW);
static bool GetInteger(Port& uiPort, uint32_t& val, int base, uint32_t defaultVal);

void MenuMode(Port& uiPort)
{
    uiPort.Write("\r\n*** MAIN MENU:\r\n"
                 "  m: Mount paper tape         l: Load file using M93xx console\r\n"
                 "  u: Unmount paper tape       S: Adapter settings\r\n"
                 "  s: Paper tape status\r\n"
                 "  -----\r\n"
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
        LoadFile(uiPort);
        break;
    default:
        break;
    }
}

void MountPaperTape(Port& uiPort)
{
    const char * fileName;
    const uint8_t * fileData;
    size_t fileLen;

    auto isValidSel = [](char ch) -> bool {
        return (FileSet::IsValidFileKey(ch)
                || ch == 'A' || ch == 'X' 
                || (gUploadedFileLen != 0 && ch == 'P')
                || ch == CTRL_C);
    };

    uiPort.Write("*** SELECT FILE:\r\n");
    uiPort.Write("  A: Absolute Loader\r\n");
    uiPort.Write("  X: Upload file via XMODEM\r\n");
    if (gUploadedFileLen != 0) {
        uiPort.Write("  P: Previously uploaded file\r\n");
    }
    uiPort.Write("  -----\r\n");
    FileSet::ShowMenu(uiPort);

    char selection = GetMenuSelection(uiPort, isValidSel);
    switch (selection) {
    case 'A':
        fileName = "Absolute Loader";
        fileData = gAbsoluteLoaderPaperTapeFile;
        fileLen = gAbsoluteLoaderPaperTapeFileLen;
        break;
    case 'X':
        UploadFileMode(uiPort);
        /* fall thru */
    case 'P':
        fileName = "UPLOADED FILE";
        fileData = gUploadedFile;
        fileLen = gUploadedFileLen;
        break;
    case CTRL_C:
        return;
    default:
        FileSet::GetFile(selection, fileName, fileData, fileLen);
        break;
    }

    PaperTapeReader::Mount(fileName, fileData, fileLen);
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

void LoadFile(Port& uiPort)
{
    const char *fileName;
    const uint8_t *fileData;
    size_t fileLen;

    auto isValidSel = [](char ch) -> bool {
        return (FileSet::IsValidFileKey(ch)
                || ch == 'A' || ch == 'B' || ch == 'X'
                || (gUploadedFileLen != 0 && ch == 'P')
                || ch == CTRL_C);
    };

    uiPort.Write("*** SELECT FILE:\r\n");
    uiPort.Write("  A: Absolute Loader\r\n");
    uiPort.Write("  B: Bootstrap Loader\r\n");
    uiPort.Write("  X: Upload file via XMODEM\r\n");
    if (gUploadedFileLen != 0) {
        uiPort.Write("  P: Previously uploaded file\r\n");
    }
    uiPort.Write("  -----\r\n");
    FileSet::ShowMenu(uiPort);

    char selection = GetMenuSelection(uiPort, isValidSel);
    switch (selection) {
    case 'A':
        LoadAbsoluteLoader(uiPort);
        break;
    case 'B':
        LoadBootstrapLoader(uiPort);
        break;
    case 'X':
        UploadAndLoadFile(uiPort);
        break;
    case 'P':
        LoadPreviouslyUploadedFile(uiPort);
        break;
    case CTRL_C:
        break;
    default:
        FileSet::GetFile(selection, fileName, fileData, fileLen);
        if (LDAReader::IsValidLDAFile(fileData, fileLen)) {
            LoadLDAFile(uiPort, fileName, fileData, fileLen);
        }
        else {
            LoadSimpleFile(uiPort, fileName, fileData, fileLen);
        }
    }
}

void LoadBootstrapLoader(Port& uiPort)
{
    uint32_t memSizeKW;

    if (!GetSystemMemorySize(uiPort, memSizeKW)) {
        return;
    }

    uint16_t loadAddr = BootstrapLoaderDataSource::MemSizeToLoadAddr(memSizeKW);

    BootstrapLoaderDataSource dataSource(loadAddr);

    const char nameFormat[] = "Bootstrap Loader (load address %06o)";
    char nameBuf[sizeof(nameFormat) + 6];
    snprintf(nameBuf, sizeof(nameBuf), nameFormat, loadAddr);

    LoadFileMode(uiPort, dataSource, nameBuf);
}

void LoadAbsoluteLoader(Port& uiPort)
{
    uint32_t memSizeKW;

    if (!GetSystemMemorySize(uiPort, memSizeKW)) {
        return;
    }

    uint16_t loadAddr = AbsoluteLoaderDataSource::MemSizeToLoadAddr(memSizeKW);

    AbsoluteLoaderDataSource dataSource(loadAddr);

    const char nameFormat[] = "Absolute Loader (load address %06o)";
    char nameBuf[sizeof(nameFormat) + 6];
    snprintf(nameBuf, sizeof(nameBuf), nameFormat, loadAddr);

    LoadFileMode(uiPort, dataSource, nameBuf);
}

void LoadLDAFile(Port& uiPort, const char * fileName, const uint8_t * fileData, size_t fileLen)
{
    LDADataSource dataSource(fileData, fileLen);

    const char nameFormat[] = "%s (LDA format, %u bytes)";
    char nameBuf[sizeof(nameFormat) + MAX_FILE_NAME_LEN + 10];
    snprintf(nameBuf, sizeof(nameBuf), nameFormat, fileName, (unsigned)fileLen);

    LoadFileMode(uiPort, dataSource, nameBuf);
}

void LoadSimpleFile(Port& uiPort, const char * fileName, const uint8_t * fileData, size_t fileLen)
{
    static uint32_t loadAddr;
    static uint32_t sDefaultLoadAddr = 0;

    do {
        uiPort.Printf("*** ENTER LOAD ADDRESS [%" PRId32 "]: ", sDefaultLoadAddr);
        if (!GetInteger(uiPort, loadAddr, 8, sDefaultLoadAddr)) {
            return;
        }
    } while (loadAddr > UINT16_MAX);

    sDefaultLoadAddr = loadAddr;

    SimpleDataSource dataSource(fileData, fileLen, loadAddr);

    const char nameFormat[] = "%s (binary format, %u bytes, load address %06o)";
    char nameBuf[sizeof(nameFormat) + MAX_FILE_NAME_LEN + 10 + 6];
    snprintf(nameBuf, sizeof(nameBuf), nameFormat, fileName, fileLen, loadAddr);

    LoadFileMode(uiPort, dataSource, nameBuf);
}

void UploadAndLoadFile(Port& uiPort)
{
    if (UploadFileMode(uiPort)) {
        LoadPreviouslyUploadedFile(uiPort);
    }
}

void LoadPreviouslyUploadedFile(Port& uiPort)
{
    if (LDAReader::IsValidLDAFile(gUploadedFile, gUploadedFileLen)) {
        LoadLDAFile(uiPort, "UPLOADED FILE", gUploadedFile, gUploadedFileLen);
    }
    else {
        TrimXMODEMPadding();
        LoadSimpleFile(uiPort, "UPLOADED FILE", gUploadedFile, gUploadedFileLen);
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

bool GetSystemMemorySize(Port& uiPort, uint32_t& memSizeKW)
{
    static uint32_t sDefaultMemSizeKW = 28;

    do {
        uiPort.Printf("*** ENTER SYSTEM MEMORY SIZE (KW) [%" PRId32 "]: ", sDefaultMemSizeKW);
        if (!GetInteger(uiPort, memSizeKW, 10, sDefaultMemSizeKW)) {
            return false;
        }
    } while (memSizeKW < 4 || memSizeKW > 28);

    sDefaultMemSizeKW = memSizeKW;

    return true;
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