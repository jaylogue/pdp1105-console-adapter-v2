#include "ConsoleAdapter.h"

#if defined(AUX_TERM_UART)

#include "hardware/clocks.h"
#include "hardware/pwm.h"
#include "hardware/uart.h"

AuxPort gAuxPort;

SerialConfig AuxPort::sConfig = { AUX_DEFAULT_BAUD_RATE, 8, 1, SerialConfig::PARITY_NONE };

void AuxPort::Init(void)
{
    // Setup the auxilairy terminal UART
    uart_init(AUX_TERM_UART, sConfig.BitRate);
    uart_set_fifo_enabled(AUX_TERM_UART, true);
    gpio_set_function(AUX_TERM_UART_RX_PIN, UART_FUNCSEL_NUM(AUX_TERM_UART, AUX_TERM_UART_RX_PIN));
    gpio_set_function(AUX_TERM_UART_TX_PIN, UART_FUNCSEL_NUM(AUX_TERM_UART, AUX_TERM_UART_TX_PIN));
    SetConfig(sConfig);
}

void AuxPort::SetConfig(const SerialConfig& serialConfig)
{
    // Wait for the UART transmission queue to empty.
    Flush();

    // Set the baud rate and format for the auxilairy terminal UART
    uart_set_baudrate(AUX_TERM_UART, serialConfig.BitRate);
    uart_set_format(AUX_TERM_UART, serialConfig.DataBits, serialConfig.StopBits, serialConfig.Parity);

    sConfig = serialConfig;
}

#endif // defined(AUX_TERM_UART)
