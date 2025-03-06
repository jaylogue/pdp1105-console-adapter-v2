#ifndef HOST_PORT_H
#define HOST_PORT_H

struct SerialConfig;

class HostPort final
{
public:
    static void Init(void);
    static char Read(void);
    static bool TryRead(char &ch);
    static void Write(char ch);
    static void Write(const char * str);
    static bool SerialConfigChanged(void);
    static void GetSerialConfig(SerialConfig& serialConfig);
};

#endif // HOST_PORT_H