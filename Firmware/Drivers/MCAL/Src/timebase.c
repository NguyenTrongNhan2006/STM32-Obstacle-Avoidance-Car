#include "timebase.h"
#include "stm32f1xx_hal.h"
status_t timebase_init(const timebase_config_t *config)
{
    if (config == NULL || config->capture_tick_hz == 0U) { return STATUS_ERROR; }
    /* HAL tick is ready. TIM2 microsecond capture is deliberately not initialized. */
    return STATUS_OK;
}
uint32_t timebase_now_ms(void) { return HAL_GetTick(); }
status_t timebase_capture_us(uint32_t *pulse_us, uint32_t timeout_us)
{
    if (pulse_us == NULL || timeout_us == 0U) { return STATUS_ERROR; }
    *pulse_us = 0U;
    /* IMPLEMENT: TIM2 edge capture, overflow handling and bounded deadline. */
    return STATUS_NOT_READY;
}
