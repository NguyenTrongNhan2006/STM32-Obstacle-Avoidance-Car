#ifndef ULTRASONIC_HCSR04_H
#define ULTRASONIC_HCSR04_H
#include "project_types.h"
typedef struct { uint32_t echo_timeout_us; uint32_t sample_period_ms; } ultrasonic_config_t;
status_t ultrasonic_init(const ultrasonic_config_t *config);
status_t ultrasonic_request(void);
status_t ultrasonic_read(sample_t *sample);
#endif
