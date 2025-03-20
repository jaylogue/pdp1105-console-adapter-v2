#ifndef HOST_PORT_H
#define HOST_PORT_H

struct SerialConfig;

class HostPort final : public Port
{
public:
    HostPort(void) = default;
    virtual ~HostPort(void) = default;

    HostPort(const HostPort&) = delete;
    HostPort& operator=(const HostPort&) = delete;

    void Init(void);
    bool SerialConfigChanged(void);
    void GetSerialConfig(SerialConfig& serialConfig);

    virtual char Read(void);
    virtual bool TryRead(char &ch);
    virtual void Write(char ch);
    virtual void Write(const char * str);
    virtual void Flush(void);
    virtual bool CanWrite(void);
};

extern HostPort gHostPort;

inline char HostPort::Read(void)
{
    char ch = stdio_getchar();
    ActivityLED::SysActive();
    return ch;
}

inline bool HostPort::TryRead(char& ch)
{
    if (stdio_get_until(&ch, 1, 0) == 1) {
        ActivityLED::SysActive();
        return true;
    }
    return false;
}

inline void HostPort::Write(char ch)
{
    stdio_putchar_raw(ch);
    ActivityLED::SysActive();
}

inline void HostPort::Write(const char * str)
{
    stdio_put_string(str, strlen(str), false, false);
    ActivityLED::SysActive();
}

inline void HostPort::Flush(void)
{
    stdio_flush();
}

inline bool HostPort::CanWrite(void)
{
    return true;
}

#endif // HOST_PORT_H