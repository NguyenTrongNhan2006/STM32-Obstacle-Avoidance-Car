#include "button.h"
#include "board_config.h"
#include "gpio.h"
#include "timebase.h"

/* Polled debounce only. EXTI is not routed yet (every peripheral IRQ still
 * traps), so the event-capture half of this driver arrives with the NVIC
 * bring-up and must not be assumed present.
 */
#define BUTTON_ACTIVE_HIGH ((bool)(BUTTON_ACTIVE_LEVEL == GPIO_PIN_SET))

static uint32_t debounce_ms;
static uint32_t candidate_since_ms;
static bool candidate_pressed;
static bool stable_pressed;
static bool initialized;

static gpio_pin_t button_pin(void)
{
    const gpio_pin_t pin = { .port = BUTTON_PORT, .pin = BUTTON_PIN };
    return pin;
}

status_t button_init(const button_config_t *config)
{
    bool high = false;

    if (config == NULL || config->debounce_ms == 0U) { return STATUS_ERROR; }
    initialized = false;
    if (gpio_read(button_pin(), &high) != STATUS_OK) { return STATUS_NOT_READY; }

    debounce_ms = config->debounce_ms;
    stable_pressed = (high == BUTTON_ACTIVE_HIGH);
    candidate_pressed = stable_pressed;
    candidate_since_ms = timebase_now_ms();
    initialized = true;
    return STATUS_OK;
}

status_t button_read(bool *pressed)
{
    bool high = false;
    bool raw;
    uint32_t now_ms;

    if (pressed == NULL) { return STATUS_ERROR; }
    *pressed = false;
    if (!initialized) { return STATUS_NOT_READY; }
    if (gpio_read(button_pin(), &high) != STATUS_OK) { return STATUS_NOT_READY; }

    raw = (high == BUTTON_ACTIVE_HIGH);
    now_ms = timebase_now_ms();
    if (raw != candidate_pressed) {
        candidate_pressed = raw;
        candidate_since_ms = now_ms;
    } else if ((raw != stable_pressed) &&
               ((uint32_t)(now_ms - candidate_since_ms) >= debounce_ms)) {
        stable_pressed = raw;
    }

    /* Level only. Distinguishing STOP from a deliberate rearm is edge detection
     * and belongs to the safety owner, which knows the current inhibit state.
     */
    *pressed = stable_pressed;
    return STATUS_OK;
}
