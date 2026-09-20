#ifndef EXTI_H
#define EXTI_H
#include "project_types.h"
typedef struct { uint8_t line; uint8_t priority; } exti_config_t;
status_t exti_init(const exti_config_t *config);
status_t exti_read_pending(bool *pending);
void exti_irq_capture(void);
#endif
