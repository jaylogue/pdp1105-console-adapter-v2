#ifndef SCL_PORT_H
#define SCL_PORT_H

struct SerialConfig;

class SCLPort final
{
public:
    static void Init(void);
    static void SetConfig(const SerialConfig& serialConfig);
    static char Read(void);
    static bool TryRead(char &ch);
    static void Write(char ch);
    static void Write(const char * str);
    static void Flush(void);
    static bool CanWrite(void);
    static bool CheckConnected(void);
    static void ClearReaderRun(void);
    static bool ReaderRunRequested(void);
};

#endif // SCL_PORT_H
