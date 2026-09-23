#include "buzzer.h"
#include "board_config.h"
#include "gpio.h"

static bool is_active_high;
static bool initialized;
static buzzer_pattern_t current_pattern;
static uint32_t pattern_start_ms;

static const gpio_pin_t buzzer_pin_desc = { .port = BUZZER_PORT, .pin = BUZZER_PIN };

static status_t buzzer_drive(bool on)
{
    return gpio_write(buzzer_pin_desc, on == is_active_high);
}

status_t buzzer_init(const buzzer_config_t *config)
{
    if (config == NULL) { return STATUS_ERROR; }
    is_active_high = config->active_high;
    current_pattern = BUZZER_SILENT;
    pattern_start_ms = 0U;
    initialized = true;
    (void)buzzer_drive(false);
    return STATUS_OK;
}

status_t buzzer_set(buzzer_pattern_t pattern)
{
    if ((unsigned)pattern > BUZZER_FAULT) { return STATUS_ERROR; }
    if (!initialized) { return STATUS_NOT_READY; }
    if (current_pattern != pattern) {
        current_pattern = pattern;
        pattern_start_ms = 0U;
    }
    return STATUS_OK;
}

status_t buzzer_update(uint32_t now_ms)
{
    bool on = false;
    uint32_t elapsed;

    if (!initialized) { return STATUS_NOT_READY; }

    if (pattern_start_ms == 0U) {
        pattern_start_ms = now_ms;
    }
    elapsed = now_ms - pattern_start_ms;

    switch (current_pattern) {
    case BUZZER_SILENT:
        on = false;
        break;

    case BUZZER_START:
        /* One-shot chirp: 100 ms ON, then reverts to silent. */
        if (elapsed < 100U) {
            on = true;
        } else {
            current_pattern = BUZZER_SILENT;
            on = false;
        }
        break;

    case BUZZER_OBSTACLE:
        /* Warning beep: 80 ms ON, 120 ms OFF (200 ms period). */
        on = (elapsed % 200U) < 80U;
        break;

    case BUZZER_FAULT:
        /* Alarm beep: 150 ms ON, 150 ms OFF (300 ms period). */
        on = (elapsed % 300U) < 150U;
        break;

    default:
        on = false;
        break;
    }

    return buzzer_drive(on);
}
