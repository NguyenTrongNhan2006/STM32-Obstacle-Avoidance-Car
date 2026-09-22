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

/* Dem so lan ISR chay khi TIM2 chua co chu. Bien volatile de doc duoc bang
 * debugger hoac in qua telemetry, khong dung cho logic dieu khien.
 */
volatile uint32_t g_timebase_spurious_irq;

void timebase_irq_capture(void)
{
    /* timebase_init() co y KHONG khoi tao TIM2, nen den day nghia la ai do da
     * bat ngat TIM2 ma khong di qua module chu so huu.
     *
     * KHONG duoc chi `return`: mot ngat chua duoc acknowledge se lap lai vo han
     * va treo he thong im lang. Ghi nhan, tat nguon ngat, roi tat luon o NVIC —
     * hong mot cach quan sat duoc thay vi storm khong dau vet.
     */
    ++g_timebase_spurious_irq;
    TIM2->DIER = 0U;    /* khong con nguon ngat nao tu TIM2 */
    TIM2->SR = 0U;      /* xoa moi co dang cho */
    HAL_NVIC_DisableIRQ(TIM2_IRQn);
}
