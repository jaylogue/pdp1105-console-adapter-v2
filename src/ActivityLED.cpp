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

#if defined(TX_ACTIVITY_LED_PIN)
ActivityLED::LEDState ActivityLED::sTxLED;
#endif
#if defined(RX_ACTIVITY_LED_PIN)
ActivityLED::LEDState ActivityLED::sRxLED;
#endif
ActivityLED::LEDState ActivityLED::sSysLED;

void ActivityLED::Init(void)
{
    // Setup Tx LED pin, if supported
#if defined(TX_ACTIVITY_LED_PIN)
    gpio_init(TX_ACTIVITY_LED_PIN);
    gpio_set_dir(TX_ACTIVITY_LED_PIN, GPIO_OUT);
    sTxLED.GPIO = TX_ACTIVITY_LED_PIN;
#endif

    // Setup Rx LED pin, if supported
#if defined(RX_ACTIVITY_LED_PIN)
    gpio_init(RX_ACTIVITY_LED_PIN);
    gpio_set_dir(RX_ACTIVITY_LED_PIN, GPIO_OUT);
    sRxLED.GPIO = RX_ACTIVITY_LED_PIN;
#endif

    // Setup system activity LED pin
    //   NOTE: System activity LED is inverted (normally ON, active OFF)
    //   so that it doubles as a power indicator.
    gpio_init(SYS_ACTIVITY_LED_PIN);
    gpio_set_dir(SYS_ACTIVITY_LED_PIN, GPIO_OUT);
    gpio_set_outover(SYS_ACTIVITY_LED_PIN, GPIO_OVERRIDE_INVERT);
    gpio_set_inover(SYS_ACTIVITY_LED_PIN, GPIO_OVERRIDE_INVERT);
    sSysLED.GPIO = SYS_ACTIVITY_LED_PIN;

    // Setup Pico onboard LED pin to track system activity LED
    // (unless the activity LED is the onboard LED).
#if SYS_ACTIVITY_LED_PIN != PICO_DEFAULT_LED_PIN
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
#if !PICO_DEFAULT_LED_PIN_INVERTED
    gpio_set_outover(PICO_DEFAULT_LED_PIN, GPIO_OVERRIDE_INVERT);
    gpio_set_inover(PICO_DEFAULT_LED_PIN, GPIO_OVERRIDE_INVERT);
#endif
#endif // SYS_ACTIVITY_LED_PIN != PICO_DEFAULT_LED_PIN

    UpdateState();
}

void ActivityLED::UpdateState(void)
{
#if defined(TX_ACTIVITY_LED_PIN)
    sTxLED.UpdateState();
#endif
#if defined(RX_ACTIVITY_LED_PIN)
    sRxLED.UpdateState();
#endif
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
#if SYS_ACTIVITY_LED_PIN != PICO_DEFAULT_LED_PIN
            if (GPIO == SYS_ACTIVITY_LED_PIN) {
                gpio_put(PICO_DEFAULT_LED_PIN, IsActive);
            }
#endif
        }
    }

    // Reset the active flag
    IsActive = false;
}
