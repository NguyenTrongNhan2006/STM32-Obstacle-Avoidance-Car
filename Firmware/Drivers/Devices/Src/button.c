#include "button.h"
status_t button_init(const button_config_t *config)
{
    if (config == NULL || config->debounce_ms == 0U) { return STATUS_ERROR; }
    /* IMPLEMENT: EXTI event capture plus task-side debounce. */
    return STATUS_NOT_READY;
}
status_t button_read(bool *pressed)
{
    if (pressed == NULL) { return STATUS_ERROR; }
    *pressed = false;
    /* IMPLEMENT: separate STOP event from deliberate START/rearm. */
    return STATUS_NOT_READY;
}
