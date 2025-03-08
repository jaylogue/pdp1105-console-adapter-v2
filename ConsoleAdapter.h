#ifndef CONSOLE_ADAPTER_H
#define CONSOLE_ADAPTER_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/uart.h"

#include "Config.h"

// ================================================================================
// UTILITY CLASSES
// ================================================================================

class Port
{
public:
    Port(void) = default;
    virtual ~Port(void) = default;
    virtual char Read(void) = 0;
    virtual bool TryRead(char &ch) = 0;
    virtual void Write(char ch) = 0;
    virtual void Write(const char * str) = 0;
    virtual bool CanWrite(void) = 0;
    virtual void Flush(void) = 0;
    int Printf(const char* format, ...);
};

struct SerialConfig final
{
    uint32_t BitRate;
    uint8_t DataBits;
    uint8_t StopBits;
    uart_parity_t Parity;
    static constexpr uart_parity_t PARITY_NONE = UART_PARITY_NONE;
    static constexpr uart_parity_t PARITY_ODD  = UART_PARITY_ODD;
    static constexpr uart_parity_t PARITY_EVEN = UART_PARITY_EVEN;
};

class LoadDataSource
{
public:
    LoadDataSource(void) = default;
    virtual ~LoadDataSource(void) = default;
    virtual bool GetWord(uint16_t &data, uint16_t &addr) = 0;
    virtual void Advance(void) = 0;
    virtual bool AtEOF(void) = 0;
    virtual uint16_t GetStartAddress(void) = 0;

    constexpr static uint16_t NO_ADDR = UINT16_MAX;
};

// ================================================================================
// MAJOR MODE FUNCTIONS
// ================================================================================

extern void TerminalMode(void);
extern void MenuMode(Port& uiPort);
extern void LoadDataMode(Port& uiPort, LoadDataSource& dataSrc);

// ================================================================================
// UTILITY FUNCTIONS
// ================================================================================

extern bool TryReadHostAuxPorts(char& ch);
extern bool TryReadHostAuxPorts(char& ch, Port *& port);
extern void WriteHostAuxPorts(char ch);
extern void WriteHostAuxPorts(const char* str);

// ================================================================================
// COMMON HEADERS
// ================================================================================

#include "ActivityLED.h"
#include "HostPort.h"
#include "SCLPort.h"
#include "AuxPort.h"
#include "PaperTapeReader.h"
#include "BuiltInFileSet.h"

#endif // CONSOLE_ADAPTER_H
