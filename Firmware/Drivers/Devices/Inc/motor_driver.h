#ifndef MOTOR_DRIVER_H
#define MOTOR_DRIVER_H

#include <stdint.h>

/* Task: Nhan.
 * Drives two DC motors through a generic H-bridge (IN1/IN2 per wheel)
 * with PWM speed control on TIM2_CH1 (left) / TIM2_CH2 (right).
 * See Config/board_config.h for pin/timer assignment.
 */

typedef enum {
    MOTOR_LEFT = 0,
    MOTOR_RIGHT,
    MOTOR_COUNT
} Motor_Id_t;

typedef enum {
    MOTOR_OK = 0,
    MOTOR_ERR_INVALID_ARG,
    MOTOR_ERR_NOT_INIT
} Motor_Status_t;

/* Configures TIM2 PWM channels and direction GPIOs. Call once before any other Motor_* call. */
Motor_Status_t Motor_Init(void);

/* speed_percent in [-100, 100]: sign selects direction, magnitude selects duty cycle.
 * 0 coasts the wheel. */
Motor_Status_t Motor_SetSpeed(Motor_Id_t motor, int16_t speed_percent);

/* Coasts the wheel (duty = 0, both IN pins low). */
Motor_Status_t Motor_Stop(Motor_Id_t motor);

/* Actively brakes the wheel (both IN pins high, duty = 0). */
Motor_Status_t Motor_Brake(Motor_Id_t motor);

/* Stops both wheels. */
Motor_Status_t Motor_StopAll(void);

#endif /* MOTOR_DRIVER_H */
