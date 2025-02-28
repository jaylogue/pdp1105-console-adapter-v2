


#ifndef GLOBALS_H
#define GLOBALS_H

#include "Config.h"

// ================================================================================
// GLOBAL STATE
// ================================================================================

enum SystemState : uint
{
    TerminalMode,
    MenuMode,
    LoadFileMode_Monitor,
    LoadFileMode_PaperTapeReader,
    UploadFileMode_Monitor,
    UploadFileMode_PaperTapeReader
};

class GlobalState final
{
public:
    static ::SystemState SystemState;
    static uint BaudRate;
    static uint DataBits;
    static uint StopBits;
    static uart_parity_t Parity;
    static bool SCLConnected;
    static bool Active;
};


// ================================================================================
// MODE FUNCTIONS
// ================================================================================

extern void TerminalMode_Start(void);
extern void TerminalMode_ProcessIO(void);
extern void TerminalMode_HandleSerialConfigChange(void);
extern void MenuMode_Start(void);
extern void MenuMode_ProcessIO(void);
extern void LoadFileMode_ProcessIO(void);
extern void UploadFileMode_ProcessIO(void);

#endif // GLOBALS_H