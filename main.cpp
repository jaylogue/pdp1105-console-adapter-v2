/*
 * Copyright 2024 Jay Logue
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

/*
 * A Raspberry Pi Pico based USB-to-serial adapter for interfacing with
 * the console of a PDP-11/05. 
 */
 

#include <stdio.h>
#include <stdint.h>

#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/pwm.h"

#include "tusb.h"

// ================================================================================
// CONFIGURATION
// ================================================================================

// UART for speaking to PDP-11/05 SCL/console.  The RX pin for this UART should be
// connected to the PDP-11's "SERIAL OUT (TTL)" signal (D/04 on the SCL connector),
// while the TX pin should be connected to the "SERIAL IN (TTL)" signal (RR/36).
#define SCL_UART uart0
#define SCL_UART_TX_PIN 16
#define SCL_UART_RX_PIN 17

// UART for speaking to an auxiliary terminal (if present).  This allows an serial
// terminal to interact with the PDP-11/05 console in parallel to the host USB
// serial device. Comment out AUX_TERM_UART to disable support for an auxiliary terminal.
#define AUX_TERM_UART uart1
#define AUX_TERM_UART_TX_PIN 4
#define AUX_TERM_UART_RX_PIN 5

// Minimum, maximum and default baud rates
//
// The SCL UART in the PDP-11/05 (e.g. an AY-5-1013) supports up to 40000 baud.
#define MIN_BAUD_RATE 110
#define MAX_BAUD_RATE 38400
#define DEFAULT_BAUD_RATE 9600

// Output pin for the "CLK IN (TTL)" signal to the PDP-11/05 (T/16 on the SCL connector).
// This pin is used to clock the SCL UART inside the PDP-11, allowing the console adapter
// to control the baud rate.  For this to work, the PDP-11's built-in baud rate generator
// must be disabled by grounding the "CLK DISAB (TTL)" line (N/12 on the SCL connector).
#define SCL_CLOCK_PIN 18

// SCL detect pin.  This pin is used to detect whether the console adapter is connected
// to the PDP-11/05 SCL port.  The pin is connected to pin A/01 on the SCL connector,
// which is a ground pin coming from the PDP-11.  When the console adapter is connected,
// the line is pulled low by the PDP-11; when the console adapter is not connected,
// an internal pull-up resistor pulls the line high.
#define SCL_DETECT_PIN 7

// Input pin for "READER RUN +" signal from PDP-11/05 (F/06 on the SCL connector).
#define READER_RUN_PIN 3

// Activity LED pin.
#define ACTIVITY_LED_PIN PICO_DEFAULT_LED_PIN

// Minimum amount of time (in us) that the activity LED should remain either on or off.
#define STATUS_LED_MIN_STATE_TIME_US 30000

// PWM divisor (integral and fractional components).  These values were chosen to minimize
// error in the SCL clock generator frequency over the range of supported baud rates.
#define PWM_DIVISOR_INT 2
#define PWM_DIVISOR_FRACT 3

// Minimum amount of time (in ms) that the status LED should remain either on or off.
#define STATUS_LED_MIN_STATE_TIME_MS 30

// ================================================================================
// GLOBAL STATE
// ================================================================================

static uint sBaudRate = DEFAULT_BAUD_RATE;
static uint sDataBits = 8;
static uint sStopBits = 1;
static uart_parity_t sParity = UART_PARITY_NONE;
static absolute_time_t sLastLEDUpdateTime;
static bool sSerialConfigChanged;
static bool sSCLConnected;

// ================================================================================
// CODE
// ================================================================================

static void InitUARTs(void);
static void ConfigUARTs(void);
static void FlushUARTs(void);
static void InitSCLClock(void);
static void ConfigSCLClock(void);
static void HandleSCLConnectionChange(void);
static void HandleSerialConfigChange(void);
static void UpdateActivityLED(bool activity);

int main()
{
    // Initialize stdio
    // Disable automatic translation of CR/LF
    stdio_usb_init();
    stdio_set_translate_crlf(&stdio_usb, false);

    // Setup activity LED pin
    gpio_init(ACTIVITY_LED_PIN);
    gpio_set_dir(ACTIVITY_LED_PIN, GPIO_OUT);
    gpio_put(ACTIVITY_LED_PIN, true);
    sLastLEDUpdateTime = get_absolute_time();

    // Initialize the SCL detect pin
    gpio_init(SCL_DETECT_PIN);
    gpio_set_dir(SCL_DETECT_PIN, GPIO_IN);
    gpio_set_inover(SCL_DETECT_PIN, GPIO_OVERRIDE_INVERT);
    gpio_pull_up(SCL_DETECT_PIN);

    // Initialize SCL and auxilary UARTs
    InitUARTs();

    // Setup SCL clock generator
    InitSCLClock();

    // Main loop...
    while (true) {
        bool active = false;
        char ch = 0;

        // Detect when the console adapter is connected/disconnected from the 
        // PDP-11 SCL port.
        if (gpio_get(SCL_DETECT_PIN) != sSCLConnected) {
            HandleSCLConnectionChange();
            active = true;
        }

        // Handle requests from the USB host to change the serial configuration.
        if (sSerialConfigChanged) {
            HandleSerialConfigChange();
            active = true;
        }

        // Copy characters received from the USB host to the PDP-11's SCL port.
        if (uart_is_writable(SCL_UART) && stdio_get_until(&ch, 1, 0) == 1) {
            uart_putc(SCL_UART, ch);
            active = true;
        }

        // Copy characters received from the auxiliary terminal port to the
        // SCL port.
#if defined(AUX_TERM_UART)
        if (uart_is_writable(SCL_UART) && uart_is_readable(AUX_TERM_UART)) {
            ch = uart_getc(AUX_TERM_UART);
            uart_putc(SCL_UART, ch);
            active = true;
        }
#endif

        // Copy characters received from the PDP-11 SCL port to the USB host
        // and the auxiliary terminal port.
        if (uart_is_readable(SCL_UART)) {
            ch = uart_getc(SCL_UART);
            stdio_putchar(ch);
#if defined(AUX_TERM_UART)
            uart_putc(AUX_TERM_UART, ch);
#endif
            active = true;
        }

        // Update the state of the activity LED
        UpdateActivityLED(active);
    }
}

void InitUARTs(void)
{
    // Setup the SCL UART
    uart_init(SCL_UART, sBaudRate);
    uart_set_fifo_enabled(SCL_UART, true);
    gpio_set_function(SCL_UART_RX_PIN, UART_FUNCSEL_NUM(SCL_UART, SCL_UART_RX_PIN));
    gpio_set_function(SCL_UART_TX_PIN, UART_FUNCSEL_NUM(SCL_UART, SCL_UART_TX_PIN));
    
    // Setup the auxilairy terminal UART
#if defined(AUX_TERM_UART)
    uart_init(AUX_TERM_UART, sBaudRate);
    uart_set_fifo_enabled(AUX_TERM_UART, true);
    gpio_set_function(AUX_TERM_UART_RX_PIN, UART_FUNCSEL_NUM(AUX_TERM_UART, AUX_TERM_UART_RX_PIN));
    gpio_set_function(AUX_TERM_UART_TX_PIN, UART_FUNCSEL_NUM(AUX_TERM_UART, AUX_TERM_UART_TX_PIN));
#endif

    // Configure the speed and format of the UARTs.
    ConfigUARTs();
}

void ConfigUARTs(void)
{
    // Set the baud rate and format for the SCL UART
    uart_set_baudrate(SCL_UART, sBaudRate);
    uart_set_format(SCL_UART, sDataBits, sStopBits, sParity);

    // Set the baud rate and format for the auxiliary terminal UART
#if defined(AUX_TERM_UART)
    uart_set_baudrate(AUX_TERM_UART, sBaudRate);
    uart_set_format(AUX_TERM_UART, sDataBits, sStopBits, sParity);
#endif
}

void FlushUARTs(void)
{
    uart_tx_wait_blocking(SCL_UART);

#if defined(AUX_TERM_UART)
    uart_tx_wait_blocking(AUX_TERM_UART);
#endif
}

void InitSCLClock(void)
{
    uint slice = pwm_gpio_to_slice_num(SCL_CLOCK_PIN);
    gpio_set_function(SCL_CLOCK_PIN, GPIO_FUNC_PWM);
    pwm_set_clkdiv_int_frac(slice, PWM_DIVISOR_INT, PWM_DIVISOR_FRACT);
    ConfigSCLClock();
    pwm_set_enabled(pwm_gpio_to_slice_num(SCL_CLOCK_PIN), true);
}

void ConfigSCLClock(void)
{
    // Compute the SCL clock rate from the configured baud rate.
    uint32_t sclClockRate = sBaudRate * 16;

    // Compute the divisor needed to produce the SCL clock from the
    // system's peripheral clock.
    float sclClockDiv = ((float)clock_get_hz(clk_peri)) / sclClockRate;

    // Compute the PWM counter wrap value needed to generate the
    // target SCL clock rate given the configured PWM divisor.
    constexpr float pwmDiv = PWM_DIVISOR_INT + (PWM_DIVISOR_FRACT / 16.0F);
    uint16_t pwmWrapValue = (uint16_t)((sclClockDiv / pwmDiv) + 0.5) - 1;

    // Set the PWM wrap register to the computed value.
    pwm_set_wrap(pwm_gpio_to_slice_num(SCL_CLOCK_PIN), pwmWrapValue);

    // Set the PWM comparator value (a.k.a. "level") to half the wrap
    // value (i.e. 50% duty cycle).
    pwm_set_gpio_level(SCL_CLOCK_PIN, (pwmWrapValue + 1) / 2);
}

void HandleSCLConnectionChange(void)
{
    sSCLConnected = gpio_get(SCL_DETECT_PIN);

    // While the console adapter is connected to the PDP-11's SCL port,
    // configure the SCL UART TX pin to output serial data in inverted format 
    // When the console adapter is not connected to the SCL port, use normal
    // TX signaling.
    //
    // The PDP-11/05 SCL port expects the SERIAL IN (TTL) signal (pin RR/36)
    // to use inverted signaling (i.e. 0V = MARK, +V = SPACE).  However,
    // when the console adapter is not connected to the SCL port, it is
    // convenient to use normal signaling to that a loopback test can be
    // performed by jumpering pins RR/36 and D/04 on SCL connector.
    gpio_set_outover(SCL_UART_TX_PIN, 
                     sSCLConnected ? GPIO_OVERRIDE_INVERT : GPIO_OVERRIDE_NORMAL);
}

void HandleSerialConfigChange(void)
{
    cdc_line_coding_t usbSerialConfig;

    sSerialConfigChanged = false;

    // Get the serial configuration sent from the host (known as the USB CDC line
    // coding configuration).
    tud_cdc_get_line_coding(&usbSerialConfig);

    // Adjust the baud rate if the proposed value is within an acceptable range.
    if (usbSerialConfig.bit_rate >= MIN_BAUD_RATE &&
        usbSerialConfig.bit_rate <= MAX_BAUD_RATE) {
        sBaudRate = usbSerialConfig.bit_rate;
    }

    // Adjust the serial format if the proposed format is acceptable.
    // The serial port on the PDP11/05 is hard-wired to use 8-N-1 format.
    // However with the appropriate software on the PDP side, it can be
    // made to look like 7-E-1 or 7-O-1 to the connected terminal (and
    // indeed, Mini-UNIX does exactly that).  Therefore, we limit the
    // serial format to one of those three choices. All other combinations
    // are silently ignored.
    if (usbSerialConfig.data_bits == 8 &&
        usbSerialConfig.parity == CDC_LINE_CODING_PARITY_NONE &&
        usbSerialConfig.stop_bits == CDC_LINE_CODING_STOP_BITS_1) {
        sDataBits = 8;
        sParity = UART_PARITY_NONE;
        sStopBits = 1;
    }
    else if (usbSerialConfig.data_bits == 7 && 
             usbSerialConfig.parity == CDC_LINE_CODING_PARITY_EVEN &&
             usbSerialConfig.stop_bits == CDC_LINE_CODING_STOP_BITS_1) {
        sDataBits = 7;
        sParity = UART_PARITY_EVEN;
        sStopBits = 1;
    }
    else if (usbSerialConfig.data_bits == 7 && 
             usbSerialConfig.parity == CDC_LINE_CODING_PARITY_ODD &&
             usbSerialConfig.stop_bits == CDC_LINE_CODING_STOP_BITS_1) {
        sDataBits = 7;
        sParity = UART_PARITY_ODD;
        sStopBits = 1;
    }

    // Wait for the UART transmission queues to empty.
    FlushUARTs();

    // Reconfigure the UARTs and the SCL clock based on the new serial
    // configuration.
    ConfigUARTs();
    ConfigSCLClock();

#if 0
    printf("Serial config changed: %u %u-%c-%u\r\n",
           sBaudRate, sDataBits, 
           (sParity == UART_PARITY_NONE)
                ? 'N'
                : (sParity == UART_PARITY_EVEN)
                    ? 'E'
                    : 'O',
           sStopBits);
#endif
}

void UpdateActivityLED(bool activity)
{
    absolute_time_t now = get_absolute_time();
    bool ledState = gpio_get(ACTIVITY_LED_PIN);
    int64_t ledStateDur = absolute_time_diff_us(sLastLEDUpdateTime, now);

    // If there is activity, and activity LED is currently ON and has been for
    // the minimum time, turn it OFF to signal activity to the user.
    if (activity) {
        if (ledState && ledStateDur >= STATUS_LED_MIN_STATE_TIME_US) {
            gpio_put(ACTIVITY_LED_PIN, false);
            sLastLEDUpdateTime = now;
        }
    }

    // Otherwise, if there is no activity, and the activity LED is currently OFF
    // and has been for the minimum time, turn it back ON again.
    else {
        if (!ledState && ledStateDur >= STATUS_LED_MIN_STATE_TIME_US)
        {
            gpio_put(ACTIVITY_LED_PIN, true);
            sLastLEDUpdateTime = now;
        }
    }
}

// Called by the USB stack to signal that the USB host has changed the serial
// configuraiton.
extern "C"
void tud_cdc_line_coding_cb(uint8_t itf, cdc_line_coding_t const* p_line_coding)
{
    sSerialConfigChanged = true;
}
