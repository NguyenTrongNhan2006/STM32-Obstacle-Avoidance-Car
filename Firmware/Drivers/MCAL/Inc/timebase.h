#ifndef TIMEBASE_H
#define TIMEBASE_H
#include "project_types.h"
typedef struct { uint32_t capture_tick_hz; } timebase_config_t;
status_t timebase_init(const timebase_config_t *config);
uint32_t timebase_now_ms(void);
status_t timebase_capture_us(uint32_t *pulse_us, uint32_t timeout_us);
/* Diem vao ISR cua TIM2 — timebase la chu so huu duy nhat cua timer nay.
 * stm32f1xx_it.c chi dinh tuyen, khong tu doc/ghi thanh ghi TIM2.
 */
void timebase_irq_capture(void);
#endif
