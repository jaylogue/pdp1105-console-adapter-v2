#include "ConsoleAdapter.h"

#include "hardware/clocks.h"
#include "hardware/pwm.h"
#include "hardware/uart.h"

static bool sLastReaderRun;

static void ConfigSCLClock(uint32_t bitRate);

void SCLPort::Init(void)
{
    const SerialConfig kDefaultConfig = { 1200, 8, 1, SerialConfig::PARITY_NONE };

    // Setup the SCL UART
    uart_init(SCL_UART, kDefaultConfig.BitRate);
    uart_set_fifo_enabled(SCL_UART, true);
    gpio_set_function(SCL_UART_RX_PIN, UART_FUNCSEL_NUM(SCL_UART, SCL_UART_RX_PIN));
    gpio_set_function(SCL_UART_TX_PIN, UART_FUNCSEL_NUM(SCL_UART, SCL_UART_TX_PIN));

    // Setup the SCL clock generator
    uint slice = pwm_gpio_to_slice_num(SCL_CLOCK_PIN);
    gpio_set_function(SCL_CLOCK_PIN, GPIO_FUNC_PWM);
    pwm_set_clkdiv_int_frac(slice, PWM_DIVISOR_INT, PWM_DIVISOR_FRACT);
    ConfigSCLClock(kDefaultConfig.BitRate);
    pwm_set_enabled(pwm_gpio_to_slice_num(SCL_CLOCK_PIN), true);

    // Configure the SCL UART
    SetConfig(kDefaultConfig);

    // Initialize the SCL detect pin
    gpio_init(SCL_DETECT_PIN);
    gpio_set_dir(SCL_DETECT_PIN, GPIO_IN);
    gpio_set_inover(SCL_DETECT_PIN, GPIO_OVERRIDE_INVERT);
    gpio_pull_up(SCL_DETECT_PIN);

    // Initialize the READER_RUN input pin
    gpio_init(READER_RUN_PIN);
    gpio_set_dir(READER_RUN_PIN, GPIO_IN);
    gpio_set_inover(READER_RUN_PIN, GPIO_OVERRIDE_INVERT);
    gpio_pull_up(READER_RUN_PIN);
    sLastReaderRun = gpio_get(READER_RUN_PIN);
}

void SCLPort::SetConfig(const SerialConfig& serialConfig)
{
    // Wait for the UART transmission queue to empty.
    Flush();

    // Set the baud rate and format for the SCL UART
    uart_set_baudrate(SCL_UART, serialConfig.BitRate);
    uart_set_format(SCL_UART, serialConfig.DataBits, serialConfig.StopBits, serialConfig.Parity);

    // The SCL clock based on the new serial configuration.
    ConfigSCLClock(serialConfig.BitRate);
}

char SCLPort::Read(void)
{
    char ch = uart_getc(SCL_UART);
    ActivityLED::TxActive();
    return ch;
}

bool SCLPort::TryRead(char& ch)
{
    if (uart_is_readable(SCL_UART)) {
        ch = uart_getc(SCL_UART);
        ActivityLED::TxActive();
        return true;
    }
    return false;
}

void SCLPort::Write(char ch)
{
    uart_putc(SCL_UART, ch);
    ActivityLED::RxActive();
}

void SCLPort::Write(const char* str)
{
    uart_puts(SCL_UART, str);
    ActivityLED::RxActive();
}

void SCLPort::Flush(void)
{
    uart_tx_wait_blocking(SCL_UART);
}

bool SCLPort::CanWrite(void)
{
    return uart_is_writable(SCL_UART);
}

bool SCLPort::CheckConnected(void)
{
    bool connected = gpio_get(SCL_DETECT_PIN);

    // While the console adapter is connected to the PDP-11's SCL port,
    // configure the SCL UART TX pin to output serial data in inverted format 
    // When the console adapter is not connected to the SCL port, use normal
    // TX signaling.
    //
    // The PDP-11/05 SCL port expects the SERIAL IN (TTL) signal (pin RR/36)
    // to use inverted signaling (i.e. 0V = MARK, +V = SPACE).  However,
    // when the console adapter is not connected to the SCL port, it is
    // convenient to use normal signaling so that a loopback test can be
    // performed by jumpering pins RR/36 and D/04 on SCL connector.
    gpio_set_outover(SCL_UART_TX_PIN, connected ? GPIO_OVERRIDE_INVERT : GPIO_OVERRIDE_NORMAL);

    return connected;
}

void SCLPort::ClearReaderRun(void)
{
    sLastReaderRun = gpio_get(READER_RUN_PIN);
}

bool SCLPort::ReaderRunRequested(void)
{
    bool readerRun = gpio_get(READER_RUN_PIN);
    bool readerRunReq = (readerRun && !sLastReaderRun);
    sLastReaderRun = readerRun;
    return readerRunReq;
}

static void ConfigSCLClock(uint32_t bitRate)
{
    // Compute the SCL clock rate from the configured baud rate.
    uint32_t sclClockRate = bitRate * 16;

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
