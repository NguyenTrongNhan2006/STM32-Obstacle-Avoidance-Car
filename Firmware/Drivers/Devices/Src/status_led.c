#include "status_led.h"
#include "board_config.h"
#include "gpio.h"
#include "timebase.h"

/* Indication timing only; these are not control deadlines. */
#define LED_IDLE_PERIOD_MS 1000U
#define LED_IDLE_ON_MS 60U
#define LED_FAULT_PERIOD_MS 200U
#define LED_FAULT_ON_MS 100U

#define LED_ACTIVE_HIGH ((bool)(STATUS_LED_ACTIVE_LEVEL == GPIO_PIN_SET))

static bool initialized;

static gpio_pin_t led_pin(void)
{
    const gpio_pin_t pin = { .port = STATUS_LED_PORT, .pin = STATUS_LED_PIN };
    return pin;
}

static status_t led_drive(bool on)
{
    return gpio_write(led_pin(), on == LED_ACTIVE_HIGH);
}

status_t status_led_init(void)
{
    initialized = false;
    if (led_drive(false) != STATUS_OK) { return STATUS_NOT_READY; }
    initialized = true;
    return STATUS_OK;
}

/* Recomputes the output from the current time instead of owning a timer or
 * sleeping, so calling this periodically animates the pattern and calling it
 * once still leaves a defined level.
 */
status_t status_led_set(led_pattern_t pattern)
{
    uint32_t phase_ms;
    bool on;

    if ((unsigned)pattern > LED_FAULT) { return STATUS_ERROR; }
    if (!initialized) { return STATUS_NOT_READY; }

    switch (pattern) {
    case LED_RUNNING:
        on = true;
        break;
    case LED_FAULT:
        phase_ms = timebase_now_ms() % LED_FAULT_PERIOD_MS;
        on = phase_ms < LED_FAULT_ON_MS;
        break;
    case LED_IDLE:
    default:
        phase_ms = timebase_now_ms() % LED_IDLE_PERIOD_MS;
        on = phase_ms < LED_IDLE_ON_MS;
        break;
    }
    return led_drive(on);
}
