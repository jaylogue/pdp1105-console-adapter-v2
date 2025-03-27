/*
 * Copyright 2024-2025 Jay Logue
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
    uart_set_format(AUX_TERM_UART, serialConfig.DataBits, serialConfig.StopBits, (uart_parity_t)serialConfig.Parity);

    sConfig = serialConfig;
}

#endif // defined(AUX_TERM_UART)
