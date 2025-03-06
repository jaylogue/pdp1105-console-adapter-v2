#ifndef CONSOLE_ADAPTER_H
#define CONSOLE_ADAPTER_H

#include <stdio.h>
#include <stdint.h>

#include "pico/stdlib.h"
#include "hardware/uart.h"

#include "Config.h"
#include "HostPort.h"
#include "SCLPort.h"
#include "AuxPort.h"
#include "PaperTapeReader.h"
#include "ActivityLED.h"

class LoadDataSource;

// ================================================================================
// MAJOR MODE FUNCTIONS
// ================================================================================

extern void TerminalMode(void);
extern void MenuMode(void);
extern void LoadDataMode(LoadDataSource * dataSrc);
extern void TestLoadDataMode(void);


// ================================================================================
// I/O HELPER FUNCTIONS
// ================================================================================

extern bool TryReadHostAuxPorts(char& ch);
extern void WriteHostAuxPorts(char ch);
extern void WriteHostAuxPorts(const char *str);


// ================================================================================
// UTILITY CLASSES
// ================================================================================

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
    virtual ~LoadDataSource() = default;
    virtual bool GetWord(uint16_t &data, uint16_t &addr) = 0;
    virtual void Advance(void) = 0;
    virtual bool AtEOF(void) = 0;
};

#endif // CONSOLE_ADAPTER_H
