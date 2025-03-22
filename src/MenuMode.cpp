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
#include "Settings.h"

#define INPUT_PROMPT    ">>> "
#define MENU_PREFIX     "*** "

struct MenuItem
{
    char Selector;
    const char * Text;
    const char * Value;

    static const MenuItem END;
    static const MenuItem SEPARATOR(const char * text = "-----")  { return (MenuItem) { '-', text }; }
    static const MenuItem HIDDEN(char selector) { return (MenuItem) { selector, "" }; }

    bool IsEnd(void) const { return Text == NULL; }
    bool IsSeparator(void) const { return !IsEnd() && Selector == '-'; }
    bool IsHidden(void) const { return !IsEnd() && Text[0] == 0; }
    bool IsSelectable(void) const { return !IsEnd() && !IsSeparator(); }
};

const MenuItem MenuItem::END = { 0, NULL };

static void MountPaperTape(Port& uiPort);
static void PaperTapeStatus(Port& uiPort);
static void LoadFile(Port& uiPort);
static void LoadBootstrapLoader(Port& uiPort);
static void LoadAbsoluteLoader(Port& uiPort);
static void LoadLDAFile(Port& uiPort, const char * fileName, const uint8_t * fileData, size_t fileLen);
static void LoadSimpleFile(Port& uiPort, const char * fileName, const uint8_t * fileData, size_t fileLen);
static void UploadAndLoadFile(Port& uiPort);
static void LoadPreviouslyUploadedFile(Port& uiPort);
static char SelectFile(Port& uiPort, bool includeBootstrap);
static const MenuItem * BuildFileMenu(bool includeBootstrap);
static void SettingsMenu(Port& uiPort);
static void DiagMenu(Port& uiPort);
static void ShowMenu(Port& uiPort, const char * title, const MenuItem * menu, int numCols = 2, int colWidth = -1, int colMargin = 2);
static char GetMenuSelection(Port& uiPort, std::function<bool(char)> isValidSelection, const char * prompt = INPUT_PROMPT, bool echoSel = true);
static char GetMenuSelection(Port& uiPort, const MenuItem * menu, const char * prompt = INPUT_PROMPT, bool echoSel = true);
static bool IsValidMenuSelection(const MenuItem * menu, char sel);
static bool GetSystemMemorySize(Port& uiPort, uint32_t& memSizeKW);
static bool GetInteger(Port& uiPort, uint32_t& val, int base, uint32_t defaultVal);
static bool GetSerialConfig(Port& uiPort, const char * title, SerialConfig& serialConfig);
static bool GetShowPTRProgress(Port& uiPort, Settings::ShowPTRProgress_t& showProgressBar);
static const char * ToString(const SerialConfig& serialConfig, char * buf, size_t bufSize);
static const char * ToString(bool val, char * buf, size_t bufSize);
static const char * ToString(Settings::ShowPTRProgress_t val, char * buf, size_t bufSize);

void MenuMode(Port& uiPort)
{
    static const MenuItem sMainMenu[] = {
        { 'm', "Mount paper tape"               },
        { 'u', "Unmount paper tape"             },
        { 's', "Paper tape status"              },
        { 'l', "Load file using M93xx console"  },
        { 'S', "Adapter settings"               },
        MenuItem::SEPARATOR(),
        { '\e', "Return to terminal mode"        },
        { MENU_KEY, "Send menu character"       },
        MenuItem::HIDDEN(CTRL_C),
        MenuItem::HIDDEN(CTRL_D),
        MenuItem::END
    };

    ShowMenu(uiPort, "\r\n" MENU_PREFIX "MAIN MENU:", sMainMenu, 2, 26, 2);

    char sel = GetMenuSelection(uiPort, sMainMenu);
    uiPort.Write("\r\n");

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
    case 'S':
        SettingsMenu(uiPort);
        break;
    case CTRL_D:
        DiagMenu(uiPort);
        break;
    default:
        break;
    }
}

static inline
char FileIndexToSelector(size_t index)
{
    return (char)(index < 10 ? '0' + index : 'a' + (index = 10));
}

static inline
size_t SelectorToFileIndex(char sel)
{
    return (isdigit(sel) ? sel - '0' : (sel - 'a') + 10);
}

void MountPaperTape(Port& uiPort)
{
    const char * fileName;
    const uint8_t * fileData;
    size_t fileLen;
    
    char sel = SelectFile(uiPort, false);
    uiPort.Write("\r\n");

    switch (sel) {
    case 'A':
        fileName = "Absolute Loader";
        fileData = gAbsoluteLoaderPaperTapeFile;
        fileLen = gAbsoluteLoaderPaperTapeFileLen;
        break;
    case 'X':
        if (!UploadFileMode(uiPort)) {
            return;
        }
        /* fall thru */
    case 'P':
        fileName = "UPLOADED FILE";
        fileData = gUploadedFile;
        fileLen = gUploadedFileLen;
        break;
    case CTRL_C:
    case '\e':
        return;
    default:
        FileSet::GetFile(SelectorToFileIndex(sel), fileName, fileData, fileLen);
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

    char sel = SelectFile(uiPort, true);
    uiPort.Write("\r\n");

    switch (sel) {
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
        FileSet::GetFile(SelectorToFileIndex(sel), fileName, fileData, fileLen);
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

char SelectFile(Port& uiPort, bool includeBootstrap)
{
    const MenuItem * fileMenu = BuildFileMenu(includeBootstrap);
    ShowMenu(uiPort, "\r\n" MENU_PREFIX "SELECT FILE:", fileMenu, 2, -1, 2);
    return GetMenuSelection(uiPort, fileMenu);
}

const MenuItem * BuildFileMenu(bool includeBootstrap)
{
    static MenuItem sFileMenu[MAX_FILES + 10];

    MenuItem * item = sFileMenu;

    size_t numFiles = FileSet::NumFiles();
    if (numFiles > 0) {
        for (size_t i = 0; i < numFiles; i++) {
            *item++ = (MenuItem) { FileIndexToSelector(i), FileSet::GetFileName(i) };
        }
    }
    else {
        *item++ = MenuItem::SEPARATOR("(no files)");
    }

    *item++ = MenuItem::SEPARATOR();

    *item++ = (MenuItem){ 'A', "Absolute Loader" };

    if (includeBootstrap) {
        *item++ = (MenuItem){ 'B', "Bootstrap Loader" };
    }

    *item++ = (MenuItem){ 'X', "Upload file via XMODEM" };
    if (gUploadedFileLen != 0) {
        *item++ = (MenuItem){ 'P', "Previously uploaded file" };
    }

    *item++ = (MenuItem){ '\e', "Return to terminal mode" };

    *item++ = MenuItem::HIDDEN(CTRL_C);

    *item++ = MenuItem::END;

    return sFileMenu;
}

void SettingsMenu(Port& uiPort)
{
    SerialConfig serialConfig;

    static char sSCLConfigValue[30];
    static char sAuxConfigValue[30];
    static char sSCLFollowsHostValue[30];
    static char sAuxFollowsHostValue[30];
    static char sShowPTRProgressValue[30];

    static const MenuItem sSettingsMenu[] = {
        { 's', "SCL serial config", sSCLConfigValue         },
        { 'S', "SCL follows host",  sSCLFollowsHostValue    },
        { 'p', "Show PTR progress", sShowPTRProgressValue   },
        { 'a', "Aux serial config", sAuxConfigValue         },
        { 'A', "Aux follows host",  sAuxFollowsHostValue    },
        MenuItem::SEPARATOR(),
        { '\e', "Return to terminal mode"                    },
        MenuItem::HIDDEN(CTRL_C),
        MenuItem::END
    };

    while (true) {

        ToString(Settings::SCLConfig, sSCLConfigValue, sizeof(sSCLConfigValue));
        ToString(Settings::AuxConfig, sAuxConfigValue, sizeof(sAuxConfigValue));
        ToString(Settings::SCLConfigFollowsHost, sSCLFollowsHostValue, sizeof(sSCLFollowsHostValue));
        ToString(Settings::AuxConfigFollowsHost, sAuxFollowsHostValue, sizeof(sAuxFollowsHostValue));
        ToString(Settings::ShowPTRProgress, sShowPTRProgressValue, sizeof(sShowPTRProgressValue));

        ShowMenu(uiPort, "\r\n" MENU_PREFIX "SETTINGS MENU:", sSettingsMenu, 2, 33, 2);

        char sel = GetMenuSelection(uiPort, sSettingsMenu);
        uiPort.Write("\r\n");

        switch (sel) {
        case 's':
            if (!GetSerialConfig(uiPort, "\r\n" MENU_PREFIX "CHANGE SCL SERIAL CONFIG:", Settings::SCLConfig)) {
                continue;
            }
            break;
        case 'S':
            Settings::SCLConfigFollowsHost = !Settings::SCLConfigFollowsHost;
            break;
        case 'a':
            if (!GetSerialConfig(uiPort, "\r\n" MENU_PREFIX "CHANGE AUX SERIAL CONFIG:", Settings::AuxConfig)) {
                continue;
            }
            break;
        case 'A':
            Settings::AuxConfigFollowsHost = !Settings::AuxConfigFollowsHost;
            break;
        case 'p':
            if (!GetShowPTRProgress(uiPort, Settings::ShowPTRProgress)) {
                continue;
            }
            break;
        default:
            return;
        }

        Settings::Save();
        uiPort.Write("\r\nSettings saved\r\n");
    }
}

void DiagMenu(Port& uiPort)
{
    static const MenuItem sDiagMenu[] = {
        { 'b', "BASIC I/O TEST"                 },
        { 'r', "READER RUN INTERFACE TEST"      },
        MenuItem::SEPARATOR(),
        { '\e', "Return to terminal mode"        },
        MenuItem::HIDDEN(CTRL_C),
        MenuItem::END
    };

    ShowMenu(uiPort, "\r\n" MENU_PREFIX "DIAG MENU:", sDiagMenu, 2, 26, 2);

    char sel = GetMenuSelection(uiPort, sDiagMenu);
    uiPort.Write("\r\n");

    switch (sel) {
    case 'b':
        DiagMode_BasicIOTest(uiPort);
        break;
    case 'r':
        DiagMode_ReaderRunTest(uiPort);
        break;
    default:
        break;
    }
}

void ShowMenu(Port& uiPort, const char * title, const MenuItem * menu, int numCols, int colWidth, int colMargin)
{
    // Compute minimal column width if not specified
    if (colWidth < 0) {
        for (auto item = menu; !item->IsEnd(); item++) {
            int itemWidth = (iscntrl(item->Selector) ? 6 : 1) // "CTRL+x" or "x"
                          + 2                                 // ": "
                          + strlen(item->Text);               // "<item-text>"
            if (item->Value != NULL) {
                itemWidth += strlen(item->Value);             // "<item-value>"
            }
            if (colWidth < itemWidth) {
                colWidth = itemWidth;
            }
        }
    }

    uiPort.Printf("%s\r\n", title);

    // Render all the visible items in the menu
    while (!menu->IsEnd()) {

        // If the current item is hidden, skip it.
        if (menu->IsHidden()) {
            menu++;
            continue;
        }

        // If the current item is a separator, print it and skip to the next item.
        if (menu->IsSeparator()) {
            uiPort.Printf("%-*s%s\r\n", colMargin, "", menu->Text);
            menu++;
            continue;
        }

        // Determine the number of menu items in the current group
        size_t numItemsInGroup = 0;
        for (auto item = menu; item->IsSelectable() && !item->IsHidden(); item++) {
            numItemsInGroup++;
        }

        // Determine the number of rows needed to display group
        size_t numRowsInGroup = (numItemsInGroup + (numCols - 1)) / numCols;

        // Print each row...
        for (size_t row = 0; row < numRowsInGroup; row++) {
            int padding = 0;

            // Print each item in the current row...
            for (size_t i = row; i < numItemsInGroup; i += numRowsInGroup) {
                auto item = menu + i;
                int itemWidth = 0;

                // Pad the previous column out to the column width and add the
                // column margin (whitespace before column)
                uiPort.Printf("%-*s", padding + colMargin, "");

                // Print the selector for the current menu item, translating control
                // characters to printable text (e.g. "CTRL+x")
                if (item->Selector == '\r') {
                    uiPort.Write("ENTER");
                    itemWidth += 5;
                }
                else if (item->Selector == '\e') {
                    uiPort.Write("ESC");
                    itemWidth += 3;
                }
                else if (iscntrl(item->Selector)) {
                    uiPort.Write("CTRL+");
                    uiPort.Write(item->Selector + 0x40);
                    itemWidth += 6;
                }
                else {
                    uiPort.Write(item->Selector);
                    itemWidth += 1;
                }

                // Print the menu item text
                uiPort.Write(") ");
                uiPort.Write(item->Text);
                itemWidth += (2 + strlen(item->Text));

                // If the menu item has a value, print it right-justified in the 
                // column.
                if (item->Value != NULL) {
                    int valueWidth = strlen(item->Value);
                    int valuePadding = colWidth - (itemWidth + valueWidth);
                    while (valuePadding-- > 0) { uiPort.Write('.'); }
                    uiPort.Write(item->Value);
                    itemWidth = colWidth;
                }

                // Arrange for necessary padding when printing the next column
                padding = MAX(colWidth - itemWidth, 0);
            }

            uiPort.Write("\r\n");
        }

        menu += numItemsInGroup;
    }
}

char GetMenuSelection(Port& uiPort, std::function<bool(char)> isValidSelection, const char * prompt, bool echoSel)
{
    char ch;

    uiPort.Write(prompt);

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

        if (echoSel) {
            uiPort.Write("?\x08");
        }
    }

    if (echoSel) {
        if (isprint(ch)) {
            uiPort.Write(ch);
        }
        else if (ch == CTRL_C) {
            uiPort.Write("^C");
        }
        else if (ch == '\e') {
            uiPort.Write("ESC");
        }
    }

    return ch;
}

char GetMenuSelection(Port& uiPort, const MenuItem * menu, const char * prompt, bool echoSel)
{
    auto isValidSel = [menu](char sel) -> bool {
        return IsValidMenuSelection(menu, sel);
    };
    return GetMenuSelection(uiPort, isValidSel, prompt, echoSel);
}

bool IsValidMenuSelection(const MenuItem * menu, char sel)
{
    for (auto item = menu; !item->IsEnd(); item++) {
        if (item->IsSelectable() && sel == item->Selector) {
            return true;
        }
    }
    return false;
}

bool GetSystemMemorySize(Port& uiPort, uint32_t& memSizeKW)
{
    static uint32_t sDefaultMemSizeKW = 28;

    do {
        uiPort.Printf(INPUT_PROMPT "ENTER SYSTEM MEMORY SIZE (KW) [%" PRId32 "]: ", sDefaultMemSizeKW);
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

        // Read and process a character if available...
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

bool GetSerialConfig(Port& uiPort, const char * title, SerialConfig& serialConfig)
{
    SerialConfig newSerialConfig = serialConfig;
    char promptBuf[100];
    char serialConfigBuf[12];

    static const MenuItem sSerialConfigMenu[] = {
        { '0', "110"   },
        { '1', "300"   },
        { '2', "600"   },
        { '3', "1200"  },
        { '4', "2400"  },
        { '5', "4800"  },
        { '6', "9600"  },
        { '7', "19200" },
        { '8', "38400" },
        MenuItem::SEPARATOR(),
        { 'a', "8-N-1" },
        { 'b', "7-E-1" },
        { 'c', "7-O-1" },
        MenuItem::SEPARATOR(),
        { '\r', "Accept"        },
        { '\e', "Abort"        },
        MenuItem::HIDDEN(CTRL_C),
        MenuItem::END
    };

    newSerialConfig.StopBits = 1; // only one choice

    ShowMenu(uiPort, title, sSerialConfigMenu, 3, -1, 2);

    while (true) {
        snprintf(promptBuf, sizeof(promptBuf), "\r" INPUT_PROMPT "%s  \r" INPUT_PROMPT,
            ToString(newSerialConfig, serialConfigBuf, sizeof(serialConfigBuf)));

        char sel = GetMenuSelection(uiPort, sSerialConfigMenu, promptBuf, false);

        switch (sel) {
        case '0': newSerialConfig.BitRate = 110;   break;
        case '1': newSerialConfig.BitRate = 300;   break;
        case '2': newSerialConfig.BitRate = 600;   break;
        case '3': newSerialConfig.BitRate = 1200;  break;
        case '4': newSerialConfig.BitRate = 2400;  break;
        case '5': newSerialConfig.BitRate = 4800;  break;
        case '6': newSerialConfig.BitRate = 9600;  break;
        case '7': newSerialConfig.BitRate = 19200; break;
        case '8': newSerialConfig.BitRate = 38400; break;
        case 'a': newSerialConfig.DataBits = 8; newSerialConfig.Parity = SerialConfig::PARITY_NONE; break;
        case 'b': newSerialConfig.DataBits = 7; newSerialConfig.Parity = SerialConfig::PARITY_EVEN; break;
        case 'c': newSerialConfig.DataBits = 7; newSerialConfig.Parity = SerialConfig::PARITY_ODD;  break;
        case '\r':
            serialConfig = newSerialConfig;
            return true;
        case '\e':
        case CTRL_C:
            return false;
        }
    }
}

bool GetShowPTRProgress(Port& uiPort, Settings::ShowPTRProgress_t& showProgressBar)
{
    static const MenuItem sShowPTRProgressMenu[] = {
        { 'o', "On"          },
        { 'h', "Host-only"   },
        { 'f', "Off"         },
        MenuItem::HIDDEN(CTRL_C),
        MenuItem::HIDDEN('\e'),
        MenuItem::END
    };

    ShowMenu(uiPort, "\r\n" MENU_PREFIX "SHOW PAPER TAPE PROGRESS BAR:", sShowPTRProgressMenu, 3, -1, 2);

    char sel = GetMenuSelection(uiPort, sShowPTRProgressMenu);

    switch (sel) {
    case 'o':
        showProgressBar = Settings::ShowPTRProgress_Enabled;
        return true;
    case 'h':
        showProgressBar = Settings::ShowPTRProgress_HostOnly;
        return true;
    case 'f':
        showProgressBar = Settings::ShowPTRProgress_Disabled;
        return true;
    default:
        return false;
    }
}

const char * ToString(const SerialConfig& serialConfig, char * buf, size_t bufSize)
{
    snprintf(buf, bufSize, "%u-%u-%c-%u",
        serialConfig.BitRate,
        serialConfig.DataBits,
        serialConfig.Parity == SerialConfig::PARITY_NONE ? 'N' : (serialConfig.Parity == SerialConfig::PARITY_EVEN ? 'E' : 'O'),
        serialConfig.StopBits);
    return buf;
}

const char * ToString(bool val, char * buf, size_t bufSize)
{
    if (bufSize > 0) {
        strncpy(buf, (val) ? "on" : "off", bufSize);
        buf[bufSize-1] = 0;
    }
    return buf;
}

const char * ToString(Settings::ShowPTRProgress_t val, char * buf, size_t bufSize)
{
    const char * valStr;

    switch (Settings::ShowPTRProgress) {
    case Settings::ShowPTRProgress_Enabled:  valStr = "on";        break;
    case Settings::ShowPTRProgress_HostOnly: valStr = "host-only"; break;
    default:
    case Settings::ShowPTRProgress_Disabled: valStr = "off";       break;
    }
    strncpy(buf, valStr, bufSize);
    return buf;
}