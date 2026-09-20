#ifndef GPIO_H
#define GPIO_H
#include "project_types.h"
#include "board_config.h"
typedef struct { GPIO_TypeDef *port; uint16_t pin; } gpio_pin_t;
status_t gpio_init(void);
status_t gpio_read(gpio_pin_t pin, bool *high);
status_t gpio_write(gpio_pin_t pin, bool high);
void gpio_emergency_stop(void);
#endif
