/*
 * main.c
 *
 * MOC A2 — noi dung o day chi de chung minh duong build thong suot:
 * startup -> SystemInit -> main, link dung linker script, ra .elf/.hex/.bin.
 *
 * NOI DUNG THAT THUOC MOC A3 va dang bi chan: clock tree, HAL_Init,
 * override HAL_InitTick (kernel so huu SysTick), NVIC priority grouping 4,
 * vApplicationGetIdleTaskMemory/vApplicationGetTimerTaskMemory cho static
 * allocation, tao Task va vTaskStartScheduler.
 *
 * Tat ca deu can so lieu trong board_config.h / app_config.h / FreeRTOSConfig.h.
 * Khong doan bua o day.
 */

#include "stm32f1xx.h"

int main(void)
{
    for (;;)
    {
        __NOP();
    }
}
