#include "ConsoleAdapter.h"

#include "hardware/clocks.h"
#include "hardware/pwm.h"
#include "hardware/uart.h"

SCLPort gSCLPort;

bool SCLPort::sReaderRunRequested;

void SCLPort::Init(void)
{
    const SerialConfig kDefaultConfig = { SCL_DEFAULT_BAUD_RATE, 8, 1, SerialConfig::PARITY_NONE };

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
    //
    // NOTE: The SCL detect input is active-low, i.e. a low (GND) value
    // indicates the SCL port is connected.  As a convenience,
    // GPIO_OVERRIDE_INVERT is used to make the connected state appear
    // as 1 when read by gpio_get().
    gpio_init(SCL_DETECT_PIN);
    gpio_set_dir(SCL_DETECT_PIN, GPIO_IN);
    gpio_set_inover(SCL_DETECT_PIN, GPIO_OVERRIDE_INVERT);
    gpio_set_irqover(SCL_DETECT_PIN, GPIO_OVERRIDE_INVERT);
    gpio_pull_up(SCL_DETECT_PIN);

    // Initialize the READER_RUN input pin and arrange to receive an 
    // interrupt when it transitions to active.
    //
    // NOTE: As seen by the Pico, the READER_RUN input is active-low, i.e.
    // a low (GND) value on the READER_RUN pin indicates the PDP-11 has
    // activated its READER RUN + output.  As a convenience, 
    // GPIO_OVERRIDE_INVERT is used to make the active state appear as 1
    // when read by gpio_get().
    gpio_init(READER_RUN_PIN);
    gpio_set_dir(READER_RUN_PIN, GPIO_IN);
    gpio_set_inover(READER_RUN_PIN, GPIO_OVERRIDE_INVERT);
    gpio_set_irqover(READER_RUN_PIN, GPIO_OVERRIDE_INVERT);
    gpio_pull_up(READER_RUN_PIN);
    gpio_add_raw_irq_handler(READER_RUN_PIN, HandleReaderRunIRQ);
    gpio_set_irq_enabled(READER_RUN_PIN, GPIO_IRQ_EDGE_RISE, true);
    irq_set_enabled(IO_IRQ_BANK0, true);
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

void SCLPort::ConfigSCLClock(uint32_t bitRate)
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

void SCLPort::HandleReaderRunIRQ(void)
{
    if ((gpio_get_irq_event_mask(READER_RUN_PIN) & GPIO_IRQ_EDGE_RISE) != 0) {
        gpio_acknowledge_irq(READER_RUN_PIN, GPIO_IRQ_EDGE_RISE);
        sReaderRunRequested = true;
    }
}