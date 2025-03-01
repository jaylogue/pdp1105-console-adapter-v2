


#ifndef CONSOLE_ADAPTER_H
#define CONSOLE_ADAPTER_H

#include "Config.h"

#include "pico/stdlib.h"


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
extern void MenuMode_Start(void);
extern void MenuMode_ProcessIO(void);
extern void LoadFileMode_ProcessIO(void);
extern void UploadFileMode_ProcessIO(void);


// ================================================================================
// I/O HELPER FUNCTIONS
// ================================================================================

inline bool TryReadHost(char &ch)           { return stdio_get_until(&ch, 1, 0) == 1; }
inline char ReadHost(void)                  { return stdio_getchar(); }
inline void WriteHost(char ch)              { stdio_putchar_raw(ch); }
inline void WriteHost(const char * str)     { stdio_puts_raw(str); }

#ifdef AUX_TERM_UART
inline bool TryReadAux(char &ch)            { return (uart_is_readable(AUX_TERM_UART)) ? (ch = uart_getc(AUX_TERM_UART), true) : false; }
inline char ReadAux(void)                   { return uart_getc(AUX_TERM_UART); }
inline void WriteAux(char ch)               { uart_putc(AUX_TERM_UART, ch); }
inline void WriteAux(const char * str)      { uart_puts(AUX_TERM_UART, str); }
inline bool CanReadAux(void)                { return uart_is_readable(AUX_TERM_UART); }
inline bool CanWriteAux(void)               { return uart_is_writable(AUX_TERM_UART); }
#else
inline bool TryReadAux(char &ch)            { return false; }
inline char ReadAux(void)                   { return 0; }
inline void WriteAux(char ch)               {  }
inline void WriteAux(const char * str)      {  }
inline bool CanReadAux(void)                { return false; }
inline bool CanWriteAux(void)               { return false; }
#endif

inline bool TryReadSCL(char &ch)            { return (uart_is_readable(SCL_UART)) ? (ch = uart_getc(SCL_UART), true) : false; }
inline char ReadSCL(void)                   { return uart_getc(SCL_UART); }
inline void WriteSCL(char ch)               { uart_putc(SCL_UART, ch); }
inline void WriteSCL(const char * str)      { uart_puts(SCL_UART, str); }
inline bool CanReadSCL(void)                { return uart_is_readable(SCL_UART); }
inline bool CanWriteSCL(void)               { return uart_is_writable(SCL_UART); }

inline bool TryReadHostAux(char &ch)        { return TryReadHost(ch) || TryReadAux(ch); }
inline void WriteHostAux(char ch)           { WriteHost(ch); WriteAux(ch); }
static void WriteHostAux(const char * str)  { for (; *str != 0; str++) WriteHostAux(*str); }

#endif // CONSOLE_ADAPTER_H