#include "exti.h"
status_t exti_init(const exti_config_t *config)
{
    if (config == NULL || config->line != 8U || config->priority < 5U ||
        config->priority > 15U) { return STATUS_ERROR; }
    /* IMPLEMENT: EXTI8 falling edge, clear pending before enabling NVIC. */
    return STATUS_NOT_READY;
}
status_t exti_read_pending(bool *pending)
{
    if (pending == NULL) { return STATUS_ERROR; }
    *pending = false;
    return STATUS_NOT_READY;
}
void exti_irq_capture(void)
{
    /* IMPLEMENT: acknowledge EXTI8, capture event and notify task using FromISR. */
}
