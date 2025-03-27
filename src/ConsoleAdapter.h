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
    uint8_t Parity;
    static constexpr uint8_t PARITY_NONE = UART_PARITY_NONE;
    static constexpr uint8_t PARITY_ODD  = UART_PARITY_ODD;
    static constexpr uint8_t PARITY_EVEN = UART_PARITY_EVEN;

    bool operator==(const SerialConfig& other) const {
        return BitRate == other.BitRate &&
               DataBits == other.DataBits &&
               StopBits == other.StopBits &&
               Parity == other.Parity;
    }
    bool operator!=(const SerialConfig& other) const { return !(*this == other); }
};

class LoadDataSource
{
public:
    LoadDataSource(void) = default;
    virtual ~LoadDataSource(void) = default;
    virtual bool GetWord(uint16_t &data, uint16_t &addr) = 0;
    virtual void Advance(void) = 0;
    virtual bool AtEnd(void) = 0;
    virtual uint16_t GetStartAddress(void) = 0;
};

// ================================================================================
// MAJOR MODE FUNCTIONS
// ================================================================================

extern void TerminalMode(void);
extern void MenuMode(Port& uiPort);
extern void LoadFileMode(Port& uiPort, LoadDataSource& dataSrc, const char * fileName);
extern void DiagMode_BasicIOTest(Port& uiPort);
extern void DiagMode_ReaderRunTest(Port& uiPort);
extern void DiagMode_SettingsTest(Port& uiPort);

// ================================================================================
// UTILITY FUNCTIONS
// ================================================================================

extern bool TryReadHostAuxPorts(char& ch);
extern bool TryReadHostAuxPorts(char& ch, Port *& port);
extern void WriteHostAuxPorts(char ch);
extern void WriteHostAuxPorts(const char* str);

// ================================================================================
// GENERAL CONSTANTS
// ================================================================================

constexpr static uint16_t NO_ADDR = UINT16_MAX;
constexpr static char CTRL_C = '\x03';
constexpr static char CTRL_D = '\x04';
constexpr static char BS = '\x08';
constexpr static char DEL = '\x7F';
constexpr static const char * RUBOUT = "\x08 \x08";
constexpr static char CAN = '\x18';

// ================================================================================
// COMMON HEADERS
// ================================================================================

#include "ActivityLED.h"
#include "HostPort.h"
#include "SCLPort.h"
#include "AuxPort.h"
#include "PaperTapeReader.h"
#include "FileSet.h"

#endif // CONSOLE_ADAPTER_H
