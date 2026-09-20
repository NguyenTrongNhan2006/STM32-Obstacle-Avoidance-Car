#ifndef MOTOR_TB6612_H
#define MOTOR_TB6612_H
#include "project_types.h"
typedef enum {
    MOTOR_STOP, MOTOR_COAST, MOTOR_BRAKE, MOTOR_FORWARD,
    MOTOR_BACKWARD, MOTOR_TURN_LEFT, MOTOR_TURN_RIGHT
} motor_cmd_t;
typedef struct { bool (*is_safe)(void); } motor_config_t;
status_t motor_init(const motor_config_t *config);
/* Sole normal motor-command boundary, called only by robot_car.
 * Injected safety predicate checks the App event group without upward includes.
 * Speed is 0..100 percent. No callback/false/fault must force STBY low.
 * Skeleton always disables the bridge, including when the predicate is true.
 */
status_t motor_apply(motor_cmd_t command, uint8_t speed);
#endif
