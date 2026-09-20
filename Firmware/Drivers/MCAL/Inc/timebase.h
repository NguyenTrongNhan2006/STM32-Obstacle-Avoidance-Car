#ifndef TIMEBASE_H
#define TIMEBASE_H
#include "project_types.h"
typedef struct { uint32_t capture_tick_hz; } timebase_config_t;
status_t timebase_init(const timebase_config_t *config);
uint32_t timebase_now_ms(void);
status_t timebase_capture_us(uint32_t *pulse_us, uint32_t timeout_us);
#endif
