/*
 * Copyright 2024-2025 Jay Logue
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
#include "Menu.h"

static void MountPaperTape(Port& uiPort);
static void AdapterStatus(Port& uiPort);
static void AdapterVersion(Port& uiPort);
static void LoadFile(Port& uiPort);
static void LoadBootstrapLoader(Port& uiPort);
static void LoadAbsoluteLoader(Port& uiPort);
static void LoadLDAFile(Port& uiPort, const char * fileName, const uint8_t * fileData, size_t fileLen);
static void LoadSimpleFile(Port& uiPort, const char * fileName, const uint8_t * fileData, size_t fileLen);
static void UploadAndLoadFile(Port& uiPort);
static void LoadPreviouslyUploadedFile(Port& uiPort);
static char SelectFile(Port& uiPort, const char * title, bool includeBootstrap);
static const Menu * BuildFileMenu(const char * title, bool includeBootstrap);
static void SettingsMenu(Port& uiPort);
static void DiagMenu(Port& uiPort);
static bool GetSystemMemorySize(Port& uiPort, uint32_t& memSizeKW);
static bool GetInteger(Port& uiPort, uint32_t& val, unsigned base, uint32_t defaultVal = UINT32_MAX);
static bool GetSerialConfig(Port& uiPort, const char * title, SerialConfig& serialConfig);
static bool GetShowPTRProgress(Port& uiPort, Settings::ShowPTRProgress_t& showProgressBar);
static const char * ToString(const SerialConfig& serialConfig, char * buf, size_t bufSize);
static const char * ToString(bool val, char * buf, size_t bufSize);
static const char * ToString(Settings::ShowPTRProgress_t val, char * buf, size_t bufSize);

void MenuMode(Port& uiPort)
{
    static const MenuItem sMenuItems[] = {
        { 'm', "Mount paper tape"               },
        { 'u', "Unmount paper tape"             },
        { 's', "Adapter status"                 },
        { 'l', "Load file using M93xx console"  },
        { 'S', "Adapter settings"               },
        { 'v', "Adapter version"                },
        MenuItem::SEPARATOR(),
        { '\e', "Return to terminal mode"        },
        { MENU_KEY, "Send menu character"       },
        MenuItem::HIDDEN(CTRL_C),
        MenuItem::HIDDEN(CTRL_D),
        MenuItem::END()
    };
    static const Menu sMenu = {
        .Title = "MAIN MENU:",
        .Items = sMenuItems,
        .NumCols = 2,
        .ColWidth = 28,
        .ColMargin = 2
    };

    sMenu.Show(uiPort);

    switch (sMenu.GetSelection(uiPort)) {
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
        AdapterStatus(uiPort);
        break;
    case 'l':
        LoadFile(uiPort);
        break;
    case 'S':
        SettingsMenu(uiPort);
        break;
    case 'v':
        AdapterVersion(uiPort);
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
    return (char)(index < 10 ? '0' + index : 'a' + (index - 10));
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
    
    char sel = SelectFile(uiPort, "MOUNT PAPER TAPE:", false);
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
        FileLib::GetFile(SelectorToFileIndex(sel), fileName, fileData, fileLen);
        break;
    }

    PaperTapeReader::Mount(fileName, fileData, fileLen);

    uiPort.Printf(TITLE_PREFIX "MOUNTED PAPER TAPE: %s (%u bytes)\r\n", 
        PaperTapeReader::TapeName(), PaperTapeReader::TapeLength());
}

void AdapterStatus(Port& uiPort)
{
    char buf[30];

    uiPort.Write("\r\n" TITLE_PREFIX "ADAPTER STATUS:\r\n");

    uiPort.Printf("  SCL Port: %s (%s), %s\r\n",
        ToString(gSCLPort.GetConfig(), buf, sizeof(buf)),
        (Settings::SCLConfigFollowsUSB && gSCLPort.GetConfig() != Settings::SCLConfig) 
            ? "set via USB"
            : "default",
        gSCLPort.CheckConnected() ? "connected" : "disconnected"
    );

    uiPort.Printf("  AUX Port: %s (%s)\r\n",
        ToString(gAuxPort.GetConfig(), buf, sizeof(buf)),
        (Settings::AuxConfigFollowsUSB && gAuxPort.GetConfig() != Settings::AuxConfig) 
            ? "set via USB"
            : "default"
    );

    if (PaperTapeReader::IsMounted()) {
        uiPort.Printf("  Paper Tape Reader: %s\r\n    Position: %" PRIu32 "/%" PRIu32 " (%" PRIu32 "%%)\r\n", 
            PaperTapeReader::TapeName(),
            PaperTapeReader::TapePosition(),
            PaperTapeReader::TapeLength(),
            (PaperTapeReader::TapePosition() * 100) / PaperTapeReader::TapeLength());
    }
    else {
        uiPort.Write("  Paper Tape Reader: No tape mounted\r\n");
    }
}

void AdapterVersion(Port& uiPort)
{
#if !defined(GIT_COMMIT_ID)
#define GIT_COMMIT_ID "unknown"
#endif

#if !defined(GIT_BRANCH)
#define GIT_BRANCH "unknown"
#endif

    static const uint32_t kCompatibleHWRevs[] = COMPATIBLE_HW_REVS;
    static constexpr size_t kNumCompatibleHWRevs = sizeof(kCompatibleHWRevs) / sizeof(kCompatibleHWRevs[0]);

    uiPort.Printf(
        "\r\n"
        TITLE_PREFIX "PDP-11/05 Console Adapter V2\r\n"
        "  Target Hardware Revision: %.2g",
        ((double)HW_REV) / 10);

    if (kNumCompatibleHWRevs != 0) {
        uiPort.Write(" (compatible with");
        for (size_t i = 0; i < kNumCompatibleHWRevs; i++) {
            uiPort.Printf(" %.2g", ((double)kCompatibleHWRevs[i]) / 10);
        }
        uiPort.Write(")");
    }
    uiPort.Write("\r\n");

    uiPort.Write(
        "  Build Time: " __DATE__ " " __TIME__ "\r\n"
        "  Commit Id: " GIT_COMMIT_ID " (branch " GIT_BRANCH ")\r\n\r\n");
}

void LoadFile(Port& uiPort)
{
    const char *fileName;
    const uint8_t *fileData;
    size_t fileLen;

    char sel = SelectFile(uiPort, "LOAD FILE:", true);
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
    case '\e':
        break;
    default:
        FileLib::GetFile(SelectorToFileIndex(sel), fileName, fileData, fileLen);
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
        uiPort.Write(INPUT_PROMPT "INPUT LOAD ADDRESS (in octal): " );
        if (!GetInteger(uiPort, loadAddr, 8, sDefaultLoadAddr)) {
            return;
        }
    } while (loadAddr > UINT16_MAX);

    sDefaultLoadAddr = loadAddr;

    SimpleDataSource dataSource(fileData, fileLen, loadAddr);

    const char nameFormat[] = "%s (binary format, %zu bytes, load address %06" PRIo32 ")";
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

char SelectFile(Port& uiPort, const char * title, bool includeBootstrap)
{
    const Menu * fileMenu = BuildFileMenu(title, includeBootstrap);
    fileMenu->Show(uiPort);
    return fileMenu->GetSelection(uiPort);
}

const Menu * BuildFileMenu(const char * title, bool includeBootstrap)
{
    static MenuItem sMenuItems[MAX_FILES + 10];
    static Menu sMenu = {
        .Items = sMenuItems,
        .NumCols = 2,
        .ColWidth = -1,
        .ColMargin = 2
    };

    sMenu.Title = title;

    MenuItem * item = sMenuItems;

    size_t numFiles = FileLib::NumFiles();
    if (numFiles > 0) {
        for (size_t i = 0; i < numFiles; i++) {
            *item++ = (MenuItem) { FileIndexToSelector(i), FileLib::GetFileName(i) };
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

    *item++ = MenuItem::END();

    return &sMenu;
}

void SettingsMenu(Port& uiPort)
{
    static char sSCLConfigValue[30];
    static char sAuxConfigValue[30];
    static char sSCLConfigFollowsUSBValue[30];
    static char sAuxConfigFollowsHostValue[30];
    static char sShowPTRProgressValue[30];
    static char sUppercaseModeValue[30];

    static const MenuItem sMenuItems[] = {
        { 's', "Default SCL config", sSCLConfigValue            },
        { 'S', "SCL follows USB",    sSCLConfigFollowsUSBValue  },
        { 'p', "Show PTR progress",  sShowPTRProgressValue      },
        { 'a', "Default AUX config", sAuxConfigValue            },
        { 'A', "AUX follows USB",    sAuxConfigFollowsHostValue },
        { 'u', "Uppercase mode",     sUppercaseModeValue        },
        MenuItem::SEPARATOR(),
        { '\e', "Return to terminal mode"                       },
        MenuItem::HIDDEN(CTRL_C),
        MenuItem::END()
    };
    static const Menu sMenu = {
        .Title = "SETTINGS MENU:",
        .Items = sMenuItems,
        .NumCols = 2,
        .ColWidth = 35,
        .ColMargin = 2
    };

    while (true) {
        bool reconfigPorts = false;
    
        ToString(Settings::SCLConfig, sSCLConfigValue, sizeof(sSCLConfigValue));
        ToString(Settings::AuxConfig, sAuxConfigValue, sizeof(sAuxConfigValue));
        ToString(Settings::SCLConfigFollowsUSB, sSCLConfigFollowsUSBValue, sizeof(sSCLConfigFollowsUSBValue));
        ToString(Settings::AuxConfigFollowsUSB, sAuxConfigFollowsHostValue, sizeof(sAuxConfigFollowsHostValue));
        ToString(Settings::ShowPTRProgress, sShowPTRProgressValue, sizeof(sShowPTRProgressValue));
        ToString(Settings::UppercaseMode, sUppercaseModeValue, sizeof(sUppercaseModeValue));

        sMenu.Show(uiPort);

        switch (sMenu.GetSelection(uiPort)) {
        case 's':
            if (!GetSerialConfig(uiPort, "CHANGE SCL PORT CONFIG:", Settings::SCLConfig)) {
                continue;
            }
            reconfigPorts = true;
            break;
        case 'S':
            Settings::SCLConfigFollowsUSB = !Settings::SCLConfigFollowsUSB;
            reconfigPorts = true;
            break;
        case 'a':
            if (!GetSerialConfig(uiPort, "CHANGE AUX PORT CONFIG:", Settings::AuxConfig)) {
                continue;
            }
            reconfigPorts = true;
            break;
        case 'A':
            Settings::AuxConfigFollowsUSB = !Settings::AuxConfigFollowsUSB;
            reconfigPorts = true;
            break;
        case 'p':
            if (!GetShowPTRProgress(uiPort, Settings::ShowPTRProgress)) {
                continue;
            }
            break;
        case 'u':
            Settings::UppercaseMode = !Settings::UppercaseMode;
            break;
        default:
            return;
        }

        Settings::Save();
        uiPort.Write("Settings saved\r\n");

        if (reconfigPorts) {
            gSCLPort.SetConfig(Settings::SCLConfigFollowsUSB ? gHostPort.GetConfig() : Settings::SCLConfig);
            gAuxPort.SetConfig(Settings::AuxConfigFollowsUSB ? gHostPort.GetConfig() : Settings::AuxConfig);
        }
    }
}

void DiagMenu(Port& uiPort)
{
    static const MenuItem sMenuItems[] = {
        { 'b', "BASIC I/O TEST"                 },
        { 'r', "READER RUN INTERFACE TEST"      },
        { 's', "SETTINGS TEST"                  },
        MenuItem::SEPARATOR(),
        { '\e', "Return to terminal mode"       },
        MenuItem::HIDDEN(CTRL_C),
        MenuItem::END()
    };

    static const Menu sMenu = {
        .Title = "DIAG MENU:",
        .Items = sMenuItems,
        .NumCols = 2,
        .ColWidth = -1,
        .ColMargin = 2
    };

    sMenu.Show(uiPort);

    switch (sMenu.GetSelection(uiPort)) {
    case 'b':
        DiagMode_BasicIOTest(uiPort);
        break;
    case 'r':
        DiagMode_ReaderRunTest(uiPort);
        break;
    case 's':
        DiagMode_SettingsTest(uiPort);
        break;
    default:
        break;
    }
}

bool GetSystemMemorySize(Port& uiPort, uint32_t& memSizeKW)
{
    static uint32_t sDefaultMemSizeKW = 28;

    do {
        uiPort.Write(INPUT_PROMPT "INPUT SYSTEM MEMORY SIZE (in KW): ");
        if (!GetInteger(uiPort, memSizeKW, 10, sDefaultMemSizeKW)) {
            return false;
        }
    } while (memSizeKW < 4 || memSizeKW > 28);

    sDefaultMemSizeKW = memSizeKW;

    return true;
}

bool GetInteger(Port& uiPort, uint32_t& val, unsigned base, uint32_t defaultVal)
{
    int valLen = 0;
    int defaultLen = 0;
    char ch;

    val = 0;

    // If a default value was supplied, display it and reposition the cursor
    // over the first digit
    if (defaultVal != UINT32_MAX) {
        defaultLen = uiPort.Printf((base == 10) ? "%" PRIu32 : "%" PRIo32, defaultVal);
        for (auto i = 0; i < defaultLen; i++) { uiPort.Write(BS); }
    }

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
                if (valLen == 0) {
                    val = defaultVal;
                }
                uiPort.Write("\r\n");
                return true;
            }

            if ((base == 10 && isdigit(ch)) ||
                (base == 8 && ch >= '0' && ch <= '7')) {
                uiPort.Write(ch);
                val = (val * base) + (ch - '0');
                valLen++;

                // If this is the first digit to be entered, erase the remaining characters
                // of the default value, if any, and reposition the cursor back to where
                // the next digit will be entered.
                if (valLen == 1) {
                    for (auto i = 1; i < defaultLen; i++) { uiPort.Write(' '); }
                    for (auto i = 1; i < defaultLen; i++) { uiPort.Write(BS); }
                }
            }

            else if ((ch == BS || ch == DEL) && valLen > 0) {
                uiPort.Write(RUBOUT);
                val = val / base;
                valLen--;

                // If all the input digits have been erased, redisplay the default value
                // if one was supplied.
                if (valLen == 0 && defaultVal != UINT32_MAX) {
                    defaultLen = uiPort.Printf((base == 10) ? "%" PRIu32 : "%" PRIo32, defaultVal);
                    for (auto i = 0; i < defaultLen; i++) { uiPort.Write(BS); }
                }
            }
        }
    }
}

bool GetSerialConfig(Port& uiPort, const char * title, SerialConfig& serialConfig)
{
    static const MenuItem sMenuItems[] = {
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
        MenuItem::END()
    };

    SerialConfig newSerialConfig = serialConfig;
    char promptBuf[100];
    char serialConfigBuf[12];
    Menu menu = {
        .Title = title,
        .Items = sMenuItems,
        .NumCols = 3,
        .ColWidth = -1,
        .ColMargin = 2
    };

    constexpr auto PARITY_NONE = SerialConfig::PARITY_NONE;
    constexpr auto PARITY_ODD  = SerialConfig::PARITY_ODD;
    constexpr auto PARITY_EVEN = SerialConfig::PARITY_EVEN;

    newSerialConfig.StopBits = 1; // only one choice

    menu.Show(uiPort);

    while (true) {
        snprintf(promptBuf, sizeof(promptBuf), "\r" INPUT_PROMPT "%s  \r" INPUT_PROMPT,
            ToString(newSerialConfig, serialConfigBuf, sizeof(serialConfigBuf)));

        switch (menu.GetSelection(uiPort, promptBuf, false, false)) {
        case '0': newSerialConfig.BitRate = 110;   break;
        case '1': newSerialConfig.BitRate = 300;   break;
        case '2': newSerialConfig.BitRate = 600;   break;
        case '3': newSerialConfig.BitRate = 1200;  break;
        case '4': newSerialConfig.BitRate = 2400;  break;
        case '5': newSerialConfig.BitRate = 4800;  break;
        case '6': newSerialConfig.BitRate = 9600;  break;
        case '7': newSerialConfig.BitRate = 19200; break;
        case '8': newSerialConfig.BitRate = 38400; break;
        case 'a': newSerialConfig.DataBits = 8; newSerialConfig.Parity = PARITY_NONE; break;
        case 'b': newSerialConfig.DataBits = 7; newSerialConfig.Parity = PARITY_EVEN; break;
        case 'c': newSerialConfig.DataBits = 7; newSerialConfig.Parity = PARITY_ODD;  break;
        case '\r':
            serialConfig = newSerialConfig;
            uiPort.Write("\r\n");
            return true;
        default:
            uiPort.Write("\r\n");
            return false;
        }
    }
}

bool GetShowPTRProgress(Port& uiPort, Settings::ShowPTRProgress_t& showProgressBar)
{
    static const MenuItem sMenuItems[] = {
        { '0', "On"                 },
        { '1', "USB only" },
        { '2', "Off"                },
        MenuItem::HIDDEN(CTRL_C),
        MenuItem::HIDDEN('\e'),
        MenuItem::END()
    };
    static const Menu sMenu = {
        .Title = "SHOW PAPER TAPE READER PROGRESS BAR:",
        .Items = sMenuItems,
        .NumCols = 1,
        .ColWidth = -1,
        .ColMargin = 2
    };

    sMenu.Show(uiPort);

    switch (sMenu.GetSelection(uiPort)) {
    case '0':
        showProgressBar = Settings::ShowPTRProgress_Enabled;
        return true;
    case '1':
        showProgressBar = Settings::ShowPTRProgress_USBOnly;
        return true;
    case '2':
        showProgressBar = Settings::ShowPTRProgress_Disabled;
        return true;
    default:
        return false;
    }
}

const char * ToString(const SerialConfig& serialConfig, char * buf, size_t bufSize)
{
    snprintf(buf, bufSize, "%" PRIu32 "-%" PRIu8 "-%c-%" PRIu8,
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

    switch (val) {
    case Settings::ShowPTRProgress_Enabled:  valStr = "on";       break;
    case Settings::ShowPTRProgress_USBOnly:  valStr = "USB only"; break;
    default:
    case Settings::ShowPTRProgress_Disabled: valStr = "off";      break;
    }
    strncpy(buf, valStr, bufSize);
    return buf;
}