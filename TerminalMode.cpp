
#include <stdio.h>
#include <stdint.h>

#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/pwm.h"

#include "tusb.h"

#include "ConsoleAdapter.h"
#include "KeySeqMatcher.h"


static const char * const sMenuModeKeySequences[] = {
    "\0010\015",    // TVI-912 FUNC+0
    "\001I\015",    // TVI-920 F10
    "\033Ox",       // vt100 F10
    "\033[21~",     // ANSI, xterm, vt220, et al. F10
    NULL
};

static KeySeqMatcher sMenuModeKSM(sMenuModeKeySequences, 10); // TODO: set proper timeout


static void ProcessInputChar(char ch);
static void ProcessOutputChar(char ch);

void TerminalMode_Start(void)
{
    if (GlobalState::SystemState != SystemState::TerminalMode) {
        stdio_printf("*** TERMINAL MODE ***\r\n");
        GlobalState::SystemState = SystemState::TerminalMode;
    }

    sMenuModeKSM.Reset();
    // TODO: update timeout for sMenuModeKSM
}

void TerminalMode_ProcessIO(void)
{
    char ch;
    const char* queuedChars;
    size_t numQueuedChars;

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

    // Check for timeout in key sequence detection; if so, forward 
    // any queue characters to the SCL port.
    if (sMenuModeKSM.CheckTimeout(queuedChars, numQueuedChars)) {
        for (size_t i = 0; i < numQueuedChars; i++) {
            uart_putc(SCL_UART, queuedChars[i]);
        }
    }
}

void TerminalMode_HandleSerialConfigChange(void)
{
    // TODO: update timeout for sMenuModeKSM
}

void ProcessInputChar(char ch)
{
    const char* queuedChars;
    size_t numQueuedChars;

    // Watch for menu mode key sequences...
    int result = sMenuModeKSM.Match(ch, queuedChars, numQueuedChars);

    // Forward any characters that were previously queued to the SCL port.
    for (size_t i = 0; i < numQueuedChars; i++) {
        uart_putc(SCL_UART, queuedChars[i]);
    }

    // If a menu mode key sequence was matched, activate menu mode.
    if (result >= 0) {
        MenuMode_Start();
    }

    // Otherwise, if the input character is not part of a menu mode key sequence,
    // forward it to the SCL port.
    else if (result == KeySeqMatcher::NO_MATCH) {
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