#include "ConsoleAdapter.h"

bool TryReadHostAuxPorts(char& ch) 
{
    return HostPort::TryRead(ch) || AuxPort::TryRead(ch);
}

void WriteHostAuxPorts(char ch)
{
    HostPort::Write(ch);
    AuxPort::Write(ch);
}

void WriteHostAuxPorts(const char* str)
{
    HostPort::Write(str);
    AuxPort::Write(str);
}
