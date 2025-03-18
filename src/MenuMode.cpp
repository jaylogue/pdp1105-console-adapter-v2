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

struct MenuItem
{
    char Selector;
    const char * Text;

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
static void ShowMenu(Port& uiPort, const char * title, const MenuItem * menu, int numCols = 2, int colWidth = -1, int colMargin = 2);
static char GetMenuSelection(Port& uiPort, std::function<bool(char)> isValidSelection);
static char GetMenuSelection(Port& uiPort, const MenuItem * menu);
static bool IsValidMenuSelection(const MenuItem * menu, char sel);
static bool GetSystemMemorySize(Port& uiPort, uint32_t& memSizeKW);
static bool GetInteger(Port& uiPort, uint32_t& val, int base, uint32_t defaultVal);

void MenuMode(Port& uiPort)
{
    static const MenuItem sMainMenu[] = {
        { 'm', "Mount paper tape"               },
        { 'u', "Unmount paper tape"             },
        { 's', "Paper tape status"              },
        { 'l', "Load file using M93xx console"  },
        { 'S', "Adapter settings"               },
        MenuItem::SEPARATOR(),
        { 'q', "Return to terminal mode"        },
        { MENU_KEY, "Send menu character"       },
        MenuItem::HIDDEN(CTRL_C),
        MenuItem::END
    };

    uiPort.Write("\r\n");
    ShowMenu(uiPort, "*** MAIN MENU:", sMainMenu, 2, 26, 2);

    char sel = GetMenuSelection(uiPort, sMainMenu);

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
    ShowMenu(uiPort, "*** SELECT FILE:", fileMenu, 2, -1, 2);
    return GetMenuSelection(uiPort, fileMenu);
}

const MenuItem * BuildFileMenu(bool includeBootstrap)
{
    static MenuItem sFileMenu[MAX_FILES + 7];

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

    *item++ = (MenuItem){ CTRL_C, "Return to terminal mode" };

    *item++ = MenuItem::END;

    return sFileMenu;
}

void ShowMenu(Port& uiPort, const char * title, const MenuItem * menu, int numCols, int colWidth, int colMargin)
{
    // Compute minimal column width if not specified
    if (colWidth < 0) {
        for (auto item = menu; !item->IsEnd(); item++) {
            int itemWidth = (iscntrl(item->Selector) ? 6 : 1) // "Ctrl+x" or "x"
                          + 2                                 // ": "
                          + strlen(item->Text);               // "<item-text>"
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
        size_t numRowsInGroup = (numItemsInGroup + 1) / numCols;

        // Print each row...
        for (size_t row = 0; row < numRowsInGroup; row++) {
            int padding = 0;

            // Print each item in the current row...
            for (size_t i = row; i < numItemsInGroup; i += numRowsInGroup) {
                auto item = menu + i;

                // Pad the previous column out to the column width and add the
                // column margin (whitespace between columns)
                uiPort.Printf("%-*s", padding + colMargin, "");

                // Print the current menu item, translating control character
                // selectors to "Ctrl+x"
                uiPort.Printf("%s%c: %s",
                    (iscntrl(item->Selector)) ? "Ctrl+" : "",
                    (iscntrl(item->Selector)) ? item->Selector + 0x40 : item->Selector,
                    item->Text);

                // Determine the width of the item just printed.
                int itemWidth = (iscntrl(item->Selector) ? 6 : 1) +
                              + 2
                              + strlen(item->Text);

                // Arrange for necessary padding when printing the next column
                padding = colWidth - itemWidth;
            }

            uiPort.Write("\r\n");
        }

        menu += numItemsInGroup;
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

char GetMenuSelection(Port& uiPort, const MenuItem * menu)
{
    auto isValidSel = [menu](char sel) -> bool {
        return IsValidMenuSelection(menu, sel);
    };
    return GetMenuSelection(uiPort, isValidSel);
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