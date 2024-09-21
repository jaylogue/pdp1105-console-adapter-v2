
#include <stdio.h>
#include <stdint.h>

#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/pwm.h"

#include "tusb.h"

#include "ConsoleAdapter.h"


static void ProcessInputChar(char ch);
static void ProcessOutputChar(char ch);

void TerminalMode_Start(void)
{
    if (GlobalState::SystemState != SystemState::TerminalMode) {
        stdio_printf("*** TERMINAL MODE ***\r\n");
        GlobalState::SystemState = SystemState::TerminalMode;
    }
}

void TerminalMode_ProcessIO(void)
{
    char ch;

    // Process characters received from the USB host.
    if (uart_is_writable(SCL_UART) && stdio_get_until(&ch, 1, 0) == 1) {
        ProcessInputChar(ch);
        GlobalState::Active = true;
    }

    // Process characters received from the auxiliary terminal port.
#if defined(AUX_TERM_UART)
    if (uart_is_writable(SCL_UART) && uart_is_readable(AUX_TERM_UART)) {
        ch = uart_getc(AUX_TERM_UART);
        ProcessInputChar(ch);
        GlobalState::Active = true;
    }
#endif

    // Process characters received from the PDP-11 SCL port.
    if (uart_is_readable(SCL_UART)) {
        ch = uart_getc(SCL_UART);
        ProcessOutputChar(ch);
        GlobalState::Active = true;
    }
}

void ProcessInputChar(char ch)
{
    if (ch == MENU_KEY) {
        MenuMode_Start();
    }

    else {
        uart_putc(SCL_UART, ch);
    }
}

void ProcessOutputChar(char ch)
{
    stdio_putchar(ch);

#if defined(AUX_TERM_UART)
    uart_putc(AUX_TERM_UART, ch);
#endif
}