#ifndef ULTRASONIC_HCSR04_H
#define ULTRASONIC_HCSR04_H

#include <stdint.h>
#include "stm32f1xx_hal.h"

/* Task: Hung.
 * Trig pulse on GPIO (HCSR04_TRIG_* in board_config.h), echo pulse width
 * measured via TIM1_CH1 input capture (HCSR04_ECHO_* in board_config.h).
 * Interface only - implementation not written yet.
 */

typedef enum {
    HCSR04_OK = 0,      /* distance_mm holds a fresh, valid sample */
    HCSR04_PENDING,     /* trigger sent, waiting for echo */
    HCSR04_TIMEOUT,     /* no echo received within timeout */
    HCSR04_NOT_INIT
} HCSR04_Status_t;

typedef struct {
    uint16_t        distance_mm;
    uint32_t        timestamp_ms;
    HCSR04_Status_t status;
} HCSR04_Sample_t;

HCSR04_Status_t HCSR04_Init(void);

/* Starts one trigger pulse + measurement cycle. Non-blocking. */
HCSR04_Status_t HCSR04_TriggerMeasurement(void);

/* Reads the most recently completed sample; does not block or trigger. */
HCSR04_Status_t HCSR04_GetLastSample(HCSR04_Sample_t *out_sample);

/* Call from HAL_TIM_IC_CaptureCallback() when htim->Instance == TIM1. */
void HCSR04_IC_CaptureCallback(TIM_HandleTypeDef *htim);

#endif /* ULTRASONIC_HCSR04_H */
