#ifndef AUX_PORT
#define AUX_PORT

#if defined(AUX_TERM_UART)

struct SerialConfig;

class AuxPort final : public Port
{
public:
    AuxPort(void) = default;
    virtual ~AuxPort(void) = default;

    AuxPort(const AuxPort&) = delete;
    AuxPort& operator=(const AuxPort&) = delete;

    void Init(void);
    void SetConfig(const SerialConfig& serialConfig);

    virtual char Read(void);
    virtual bool TryRead(char &ch);
    virtual void Write(char ch);
    virtual void Write(const char * str);
    virtual void Flush(void);
    virtual bool CanWrite(void);
};

extern AuxPort gAuxPort;

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

inline bool AuxPort::CanWrite(void)
{
    return uart_is_writable(SCL_UART);
}

#endif // defined(AUX_TERM_UART)

#endif // AUX_PORT
