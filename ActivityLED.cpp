#include "ConsoleAdapter.h"

static absolute_time_t sLastLEDUpdateTime;
static bool sActive;

void ActivityLED::Init(void)
{
    // Setup activity LED pin
    gpio_init(ACTIVITY_LED_PIN);
    gpio_set_dir(ACTIVITY_LED_PIN, GPIO_OUT);
    gpio_put(ACTIVITY_LED_PIN, true);
    sLastLEDUpdateTime = get_absolute_time();
}

void ActivityLED::TxActive(void)
{
    sActive = true;
}

void ActivityLED::RxActive(void)
{
    sActive = true;
}

void ActivityLED::SysActive(void)
{
    sActive = true;
}

void ActivityLED::UpdateState(void)
{
    absolute_time_t now = get_absolute_time();
    bool ledState = gpio_get(ACTIVITY_LED_PIN);
    int64_t ledStateDur = absolute_time_diff_us(sLastLEDUpdateTime, now);

    // If there is activity, and activity LED is currently ON and has been for
    // the minimum time, turn it OFF to signal activity to the user.
    if (sActive) {
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

    sActive = false;
}
