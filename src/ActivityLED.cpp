#include "ConsoleAdapter.h"

ActivityLED::LEDState ActivityLED::sTxLED;
ActivityLED::LEDState ActivityLED::sRxLED;
ActivityLED::LEDState ActivityLED::sSysLED;

void ActivityLED::Init(void)
{
    // Setup Tx LED pin
    gpio_init(TX_ACTIVITY_LED_PIN);
    gpio_set_dir(TX_ACTIVITY_LED_PIN, GPIO_OUT);
    sTxLED.GPIO = TX_ACTIVITY_LED_PIN;

    // Setup Rx LED pin
    gpio_init(RX_ACTIVITY_LED_PIN);
    gpio_set_dir(RX_ACTIVITY_LED_PIN, GPIO_OUT);
    sRxLED.GPIO = RX_ACTIVITY_LED_PIN;

    // Setup system activity LED pin
    //   NOTE: System activity LED is inverted (normally ON, active OFF)
    //   so that it doubles as a power indicator.
    gpio_init(SYS_ACTIVITY_LED_PIN);
    gpio_set_dir(SYS_ACTIVITY_LED_PIN, GPIO_OUT);
    gpio_set_outover(SYS_ACTIVITY_LED_PIN, GPIO_OVERRIDE_INVERT);
    gpio_set_inover(SYS_ACTIVITY_LED_PIN, GPIO_OVERRIDE_INVERT);
    sSysLED.GPIO = SYS_ACTIVITY_LED_PIN;

    // Setup Pico onboard LED pin to track system activity LED
#ifdef PICO_DEFAULT_LED_PIN
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
#if !PICO_DEFAULT_LED_PIN_INVERTED
    gpio_set_outover(PICO_DEFAULT_LED_PIN, GPIO_OVERRIDE_INVERT);
    gpio_set_inover(PICO_DEFAULT_LED_PIN, GPIO_OVERRIDE_INVERT);
#endif
#endif // PICO_DEFAULT_LED_PIN

    UpdateState();
}

void ActivityLED::UpdateState(void)
{
    sTxLED.UpdateState();
    sRxLED.UpdateState();
    sSysLED.UpdateState();
}

void ActivityLED::LEDState::UpdateState(void)
{
    uint64_t now = time_us_64();
    uint64_t ledStateDur = now - LastUpdateTime;

    // If the LED has been in the current state for the minimum amount of time...
    if (ledStateDur >= LED_MIN_STATE_TIME_US) {

        // If the LED is not in the desired state, update its state and note the
        // time.
        if (gpio_get(GPIO) != IsActive) {
            gpio_put(GPIO, IsActive);
            LastUpdateTime = now;

            // Update Pico onboard LED to track system activity LED
#ifdef PICO_DEFAULT_LED_PIN
            if (GPIO == SYS_ACTIVITY_LED_PIN) {
                gpio_put(PICO_DEFAULT_LED_PIN, IsActive);
            }
#endif
        }
    }

    // Reset the active flag
    IsActive = false;
}
