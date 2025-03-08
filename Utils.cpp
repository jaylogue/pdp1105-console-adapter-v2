#include <stdarg.h>
#include <pico/printf.h>

#include "ConsoleAdapter.h"

int Port::Printf(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    
    // Create a local lambda to write each character to this port
    auto write_char = [](char ch, void* arg) -> void {
        Port* port = static_cast<Port*>(arg);
        port->Write(ch);
    };
    
    // Use vfctprintf from Pico SDK to format and output the string
    int result = vfctprintf(write_char, this, format, args);
    
    va_end(args);
    return result;
}

bool TryReadHostAuxPorts(char& ch) 
{
    return gHostPort.TryRead(ch)
#if defined(AUX_TERM_UART)
        || gAuxPort.TryRead(ch)
#endif
    ;
}

bool TryReadHostAuxPorts(char& ch, Port *& port)
{
    if (gHostPort.TryRead(ch)) {
        port = &gHostPort;
        return true;
    }

#if defined(AUX_TERM_UART)
    if (gAuxPort.TryRead(ch)) {
        port = &gAuxPort;
        return true;
    }
#endif

    port = NULL;
    return false;
}

void WriteHostAuxPorts(char ch)
{
    gHostPort.Write(ch);
#if defined(AUX_TERM_UART)
    gAuxPort.Write(ch);
#endif
}

void WriteHostAuxPorts(const char* str)
{
    gHostPort.Write(str);
#if defined(AUX_TERM_UART)
    gAuxPort.Write(str);
#endif
}
