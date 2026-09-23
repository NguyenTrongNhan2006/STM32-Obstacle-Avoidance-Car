#ifndef ROBOT_CAR_H
#define ROBOT_CAR_H
#include "project_types.h"
#include "obstacle_avoidance.h"

/* Sole App caller of motor_apply. Single owner tDecision.
 * Init is pre-scheduler; STATUS_NOT_READY is expected until devices are implemented.
 */
status_t robot_car_init(void);
status_t robot_car_update(uint32_t now_ms);
car_state_t robot_car_get_state(void);
#endif
