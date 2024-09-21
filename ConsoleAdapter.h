


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
    LoadFile_Monitor,
    LoadFile_PaperTapeReader,
    TransferFile_Monitor,
    TransferFile_PaperTapeReader
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
// INPUT/OUTPUT FUNCTIONS
// ================================================================================

extern void TerminalMode_Start(void);
extern bool TerminalMode_ProcessIO(void);
extern void MenuMode_Start(void);
extern void MenuMode_ProcessIO(void);
extern void LoadFile_ProcessIO(void);
extern void TransferFile_ProcessIO(void);

#endif // GLOBALS_H