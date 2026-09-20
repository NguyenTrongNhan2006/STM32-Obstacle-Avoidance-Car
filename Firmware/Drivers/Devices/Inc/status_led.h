#ifndef STATUS_LED_H
#define STATUS_LED_H
#include "project_types.h"
typedef enum { LED_IDLE, LED_RUNNING, LED_FAULT } led_pattern_t;
status_t status_led_init(void);
status_t status_led_set(led_pattern_t pattern);
#endif
