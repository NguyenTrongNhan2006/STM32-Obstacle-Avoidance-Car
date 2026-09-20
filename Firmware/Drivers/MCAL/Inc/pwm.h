#ifndef PWM_H
#define PWM_H
#include "project_types.h"
typedef struct { uint32_t frequency_hz; } pwm_config_t;
status_t pwm_init(const pwm_config_t *config);
status_t pwm_write(uint8_t channel, uint16_t duty_per_mille);
status_t pwm_stop(void);
#endif
