#ifndef AUX_PORT
#define AUX_PORT

struct SerialConfig;

class AuxPort final
{
public:
    static void Init(void);
    static void SetConfig(const SerialConfig& serialConfig);
    static char Read(void);
    static bool TryRead(char &ch);
    static void Write(char ch);
    static void Write(const char * str);
    static void Flush(void);
};

#if defined(AUX_TERM_UART)

inline char AuxPort::Read(void)
{
    return uart_getc(AUX_TERM_UART);
}

inline bool AuxPort::TryRead(char& ch)
{
    return (uart_is_readable(AUX_TERM_UART)) ? (ch = uart_getc(AUX_TERM_UART), true) : false;
}

inline void AuxPort::Write(char ch)
{
    uart_putc(AUX_TERM_UART, ch);
}

inline void AuxPort::Write(const char* str)
{
    uart_puts(AUX_TERM_UART, str);
}

inline void AuxPort::Flush(void)
{
    uart_tx_wait_blocking(AUX_TERM_UART);
}

#else // defined(AUX_TERM_UART)

inline void AuxPort::Init(void)                     { }
inline void AuxPort::SetConfig(const SerialConfig&) { }
inline char AuxPort::Read(void)                     { return 0; }
inline bool AuxPort::TryRead(char&)                 { return false; }
inline void AuxPort::Write(char)                    { }
inline void AuxPort::Write(const char*)             { }
inline void AuxPort::Flush(void)                    { }

#endif // defined(AUX_TERM_UART)

#endif // AUX_PORT
