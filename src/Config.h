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

#ifndef CONFIG_H
#define CONFIG_H

// ================================================================================
// CONFIGURATION
// ================================================================================

// Target Hardware Revision. This should correspond to the revision of the Console
// Adapter schematic multiplied by 10.
#ifndef HW_REV
#define HW_REV 40
#endif
#if HW_REV != 30 && HW_REV != 40 && HW_REV != 41
#error Unsupported Hardware Revision
#endif

// UART for speaking to PDP-11/05 SCL/console.  The RX pin for this UART should be
// connected to the PDP-11's "SERIAL OUT (TTL)" signal (D/04 on the SCL connector),
// while the TX pin should be connected to the "SERIAL IN (TTL)" signal (RR/36).
#define SCL_UART uart0
#if HW_REV >= 40
#define SCL_UART_TX_PIN 16
#define SCL_UART_RX_PIN 1
#else
#define SCL_UART_TX_PIN 12
#define SCL_UART_RX_PIN 13
#endif

// UART for speaking to an auxiliary terminal (if present).  This allows an serial
// terminal to interact with the PDP-11/05 console in parallel to the host USB
// serial device. Comment out AUX_TERM_UART to disable support for an auxiliary terminal.
#define AUX_TERM_UART uart1
#define AUX_TERM_UART_TX_PIN 8
#define AUX_TERM_UART_RX_PIN 9

// Minimum, maximum and default baud rates
//
// The SCL UART in the PDP-11/05 (e.g. an AY-5-1013) supports up to 40000 baud.
#define MIN_BAUD_RATE 110
#define MAX_BAUD_RATE 38400
#define SCL_DEFAULT_BAUD_RATE 9600
#define AUX_DEFAULT_BAUD_RATE 9600

// Output pin for the "CLK IN (TTL)" signal to the PDP-11/05 (T/16 on the SCL connector).
// This pin is used to clock the SCL UART inside the PDP-11, allowing the console adapter
// to control the baud rate.  For this to work, the PDP-11's built-in baud rate generator
// must be disabled by grounding the "CLK DISAB (TTL)" line (N/12 on the SCL connector).
#if HW_REV >= 40
#define SCL_CLOCK_PIN 22
#else
#define SCL_CLOCK_PIN 6
#endif

// SCL detect pin.  This pin is used to detect whether the console adapter is connected
// to the PDP-11/05 SCL port.  The pin is connected to one of the ground pins (e.g. A/01)
// on the SCL connector.  When the console adapter is connected to the PDP-11, the line
// is pulled low; when the console adapter is not connected, an internal pull-up resistor
// pulls the line high.
#if HW_REV >= 40
#define SCL_DETECT_PIN 18
#else
#define SCL_DETECT_PIN 2
#endif

// Input pin for "READER RUN +" signal from PDP-11/05 (F/06 on the SCL connector).
#define READER_RUN_PIN 3

// System Activity LED pin.
#if HW_REV == 40
#define SYS_ACTIVITY_LED_PIN 14
#elif HW_REV >= 41
#define SYS_ACTIVITY_LED_PIN 15
#else
#define SYS_ACTIVITY_LED_PIN PICO_DEFAULT_LED_PIN
#endif

// Tx Activity LED pin.
#if HW_REV == 40
#define TX_ACTIVITY_LED_PIN 15
#elif HW_REV >= 41
#define TX_ACTIVITY_LED_PIN 14
#endif

// Rx Activity LED pin.
#if HW_REV >= 40
#define RX_ACTIVITY_LED_PIN 17
#endif

// Minimum amount of time (in us) that an activity LED should remain either on or off.
#define LED_MIN_STATE_TIME_US 40000

// PWM divisor (integral and fractional components).  These values were chosen to minimize
// error in the SCL clock generator frequency over the range of supported baud rates.
#define PWM_DIVISOR_INT 2
#define PWM_DIVISOR_FRACT 3

// Maximum number of files
// NOTE: must be <= available selection keys ('0'-'9' and 'a'-'z')
#define MAX_FILES 36

// Maximum file name length
#define MAX_FILE_NAME_LEN 32

// Input character to invoke the adapter menu while in terminal mode
#define MENU_KEY '\036' // Ctrl+^

// Maximum size of an uploaded file
#define MAX_UPLOAD_FILE_SIZE (64*1024)

// Width of the paper tape reader progress bar
#define PROGRESS_BAR_WIDTH 20

// Prefix identifying a request for user input
#define INPUT_PROMPT ">>> "

// Prefix identifying menu titles or other notable UI output
#define TITLE_PREFIX "*** "

#endif // CONFIG_H
