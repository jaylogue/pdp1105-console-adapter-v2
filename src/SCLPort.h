#ifndef SCL_PORT_H
#define SCL_PORT_H

struct SerialConfig;

class SCLPort final : public Port
{
public:
    SCLPort(void) = default;
    virtual ~SCLPort(void) = default;

    SCLPort(const SCLPort&) = delete;
    SCLPort& operator=(const SCLPort&) = delete;

    void Init(void);
    void SetConfig(const SerialConfig& serialConfig);
    bool CheckConnected(void);
    bool ReaderRunRequested(void);
    void ClearReaderRunRequested(void);

    virtual char Read(void);
    virtual bool TryRead(char &ch);
    virtual void Write(char ch);
    virtual void Write(const char * str);
    virtual void Flush(void);
    virtual bool CanWrite(void);

private:
    static bool sReaderRunRequested;

    static void ConfigSCLClock(uint32_t bitRate);
    static void HandleReaderRunIRQ(void);
};

extern SCLPort gSCLPort;

inline bool SCLPort::ReaderRunRequested(void)
{
    return sReaderRunRequested;
}

inline void SCLPort::ClearReaderRunRequested(void)
{
    sReaderRunRequested = false;
}

inline char SCLPort::Read(void)
{
    char ch = uart_getc(SCL_UART);
    ActivityLED::TxActive();
    return ch;
}

inline bool SCLPort::TryRead(char& ch)
{
    if (uart_is_readable(SCL_UART)) {
        ch = uart_getc(SCL_UART);
        ActivityLED::TxActive();
        return true;
    }
    return false;
}

inline void SCLPort::Write(char ch)
{
    uart_putc(SCL_UART, ch);
    ActivityLED::RxActive();
}

inline void SCLPort::Write(const char* str)
{
    uart_puts(SCL_UART, str);
    ActivityLED::RxActive();
}

inline void SCLPort::Flush(void)
{
    uart_tx_wait_blocking(SCL_UART);
}

inline bool SCLPort::CanWrite(void)
{
    return uart_is_writable(SCL_UART);
}

#endif // SCL_PORT_H
