#include "pwm.h"
status_t pwm_init(const pwm_config_t *config)
{
    if (config == NULL || config->frequency_hz == 0U) { return STATUS_ERROR; }
    /* IMPLEMENT: own TIM3, initialize both channels with zero duty. */
    return STATUS_NOT_READY;
}
status_t pwm_write(uint8_t channel, uint16_t duty_per_mille)
{
    if (channel < 1U || channel > 2U || duty_per_mille > 1000U) { return STATUS_ERROR; }
    /* IMPLEMENT: update compare register; never change another timer. */
    return STATUS_NOT_READY;
}
status_t pwm_stop(void)
{
    /* IMPLEMENT: disable both PWM outputs. */
    return STATUS_NOT_READY;
}
