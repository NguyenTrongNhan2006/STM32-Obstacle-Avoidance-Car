#include "stm32f1xx_it.h"
#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "exti.h"
#include "i2c.h"
#include "timebase.h"
void xPortSysTickHandler(void);
void SysTick_Handler(void)
{
    /* One HAL millisecond before and after scheduler start; tick hook stays disabled. */
    HAL_IncTick();
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
        xPortSysTickHandler();
    }
}
/* SVC/PendSV are defined directly by port.c using FreeRTOSConfig aliases.
 * TO DO: route each remaining peripheral IRQ to its module owner before enabling it.
 * Traps are bring-up diagnostics, not the final fault-handling implementation.
 */
#define UNIMPLEMENTED_IRQ(name) void name(void) { configASSERT(0); }

/* Cac IRQ da co chu so huu. File nay CHI dinh tuyen — no khong doc/ghi thanh
 * ghi cua ngoai vi, vi lam the se tao chu so huu thu hai cho timer/bus do.
 *
 * Dinh tuyen phai co TRUOC khi bat ngat trong NVIC. Neu con la
 * UNIMPLEMENTED_IRQ, ngat dau tien se roi vao configASSERT(0) va treo may.
 * Hien tai chua ngat ngoai vi nao duoc bat, nen ba handler duoi day chua chay.
 */
void TIM2_IRQHandler(void)     { timebase_irq_capture(); }   /* Echo capture — buoc 2 */
void I2C1_EV_IRQHandler(void)  { i2c_irq_event(); }          /* MPU6050      — buoc 3 */
void I2C1_ER_IRQHandler(void)  { i2c_irq_error(); }          /* MPU6050      — buoc 3 */
void EXTI9_5_IRQHandler(void)  { exti_irq_capture(); }       /* Nut PA8      — buoc 5 */
UNIMPLEMENTED_IRQ(NMI_Handler)
UNIMPLEMENTED_IRQ(HardFault_Handler)
UNIMPLEMENTED_IRQ(MemManage_Handler)
UNIMPLEMENTED_IRQ(BusFault_Handler)
UNIMPLEMENTED_IRQ(UsageFault_Handler)
UNIMPLEMENTED_IRQ(DebugMon_Handler)
UNIMPLEMENTED_IRQ(WWDG_IRQHandler)
UNIMPLEMENTED_IRQ(PVD_IRQHandler)
UNIMPLEMENTED_IRQ(TAMPER_IRQHandler)
UNIMPLEMENTED_IRQ(RTC_IRQHandler)
UNIMPLEMENTED_IRQ(FLASH_IRQHandler)
UNIMPLEMENTED_IRQ(RCC_IRQHandler)
UNIMPLEMENTED_IRQ(EXTI0_IRQHandler)
UNIMPLEMENTED_IRQ(EXTI1_IRQHandler)
UNIMPLEMENTED_IRQ(EXTI2_IRQHandler)
UNIMPLEMENTED_IRQ(EXTI3_IRQHandler)
UNIMPLEMENTED_IRQ(EXTI4_IRQHandler)
UNIMPLEMENTED_IRQ(DMA1_Channel1_IRQHandler)
UNIMPLEMENTED_IRQ(DMA1_Channel2_IRQHandler)
UNIMPLEMENTED_IRQ(DMA1_Channel3_IRQHandler)
UNIMPLEMENTED_IRQ(DMA1_Channel4_IRQHandler)
UNIMPLEMENTED_IRQ(DMA1_Channel5_IRQHandler)
UNIMPLEMENTED_IRQ(DMA1_Channel6_IRQHandler)
UNIMPLEMENTED_IRQ(DMA1_Channel7_IRQHandler)
UNIMPLEMENTED_IRQ(ADC1_2_IRQHandler)
UNIMPLEMENTED_IRQ(USB_HP_CAN1_TX_IRQHandler)
UNIMPLEMENTED_IRQ(USB_LP_CAN1_RX0_IRQHandler)
UNIMPLEMENTED_IRQ(CAN1_RX1_IRQHandler)
UNIMPLEMENTED_IRQ(CAN1_SCE_IRQHandler)
UNIMPLEMENTED_IRQ(TIM1_BRK_IRQHandler)
UNIMPLEMENTED_IRQ(TIM1_UP_IRQHandler)
UNIMPLEMENTED_IRQ(TIM1_TRG_COM_IRQHandler)
UNIMPLEMENTED_IRQ(TIM1_CC_IRQHandler)
UNIMPLEMENTED_IRQ(TIM3_IRQHandler)
UNIMPLEMENTED_IRQ(TIM4_IRQHandler)
UNIMPLEMENTED_IRQ(I2C2_EV_IRQHandler)
UNIMPLEMENTED_IRQ(I2C2_ER_IRQHandler)
UNIMPLEMENTED_IRQ(SPI1_IRQHandler)
UNIMPLEMENTED_IRQ(SPI2_IRQHandler)
UNIMPLEMENTED_IRQ(USART1_IRQHandler)
UNIMPLEMENTED_IRQ(USART2_IRQHandler)
UNIMPLEMENTED_IRQ(USART3_IRQHandler)
UNIMPLEMENTED_IRQ(EXTI15_10_IRQHandler)
UNIMPLEMENTED_IRQ(RTC_Alarm_IRQHandler)
UNIMPLEMENTED_IRQ(USBWakeUp_IRQHandler)
