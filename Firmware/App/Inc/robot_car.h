#ifndef ROBOT_CAR_H
#define ROBOT_CAR_H
#include "project_types.h"
/* Sole App caller of motor_apply. Single owner tDecision.
 * Init is pre-scheduler. Motor driver is active; keep hardware off until the
 * bridge and pinout have been verified on the bench.
 */
status_t robot_car_init(void);
status_t robot_car_update(uint32_t now_ms);
#endif
