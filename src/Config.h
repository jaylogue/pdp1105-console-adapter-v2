#ifndef CONFIG_H
#define CONFIG_H

// ================================================================================
// CONFIGURATION
// ================================================================================

// UART for speaking to PDP-11/05 SCL/console.  The RX pin for this UART should be
// connected to the PDP-11's "SERIAL OUT (TTL)" signal (D/04 on the SCL connector),
// while the TX pin should be connected to the "SERIAL IN (TTL)" signal (RR/36).
#define SCL_UART uart0
#define SCL_UART_TX_PIN 12
#define SCL_UART_RX_PIN 13

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
#define DEFAULT_BAUD_RATE 1200

// Output pin for the "CLK IN (TTL)" signal to the PDP-11/05 (T/16 on the SCL connector).
// This pin is used to clock the SCL UART inside the PDP-11, allowing the console adapter
// to control the baud rate.  For this to work, the PDP-11's built-in baud rate generator
// must be disabled by grounding the "CLK DISAB (TTL)" line (N/12 on the SCL connector).
#define SCL_CLOCK_PIN 6

// SCL detect pin.  This pin is used to detect whether the console adapter is connected
// to the PDP-11/05 SCL port.  The pin is connected to pin A/01 on the SCL connector,
// which is a ground pin coming from the PDP-11.  When the console adapter is connected,
// the line is pulled low by the PDP-11; when the console adapter is not connected,
// an internal pull-up resistor pulls the line high.
#define SCL_DETECT_PIN 2

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

// Maximum file name length
#define MAX_FILE_NAME_LEN 32

// Input character to invoke the adapter menu while in terminal mode
#define MENU_KEY '\036' // Ctrl+^

// Maximum size of an uploaded file
#define MAX_UPLOAD_FILE_SIZE (64*1024)

#endif // CONFIG_H