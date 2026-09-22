#ifndef OBSTACLE_AVOIDANCE_H
#define OBSTACLE_AVOIDANCE_H
#include "project_types.h"
#include "motor_tb6612.h"
typedef enum { CAR_IDLE, CAR_FORWARD, CAR_STOP, CAR_TURN_LEFT,
               CAR_TURN_RIGHT, CAR_CHECK, CAR_FAULT } car_state_t;
typedef struct { car_state_t state; uint32_t entered_ms; uint8_t attempts; } avoidance_context_t;
/* Transitions implemented in obstacle_avoidance.c; guard-by-guard table in
 * docs/diagrams/fsm_control_flow.md.
 * IDLE -> FORWARD: deliberate START + safety clear + valid range.
 * FORWARD -> STOP: range <= stop threshold.
 * STOP -> TURN_LEFT/RIGHT: stopped, retry budget available.
 * TURN_* -> CHECK: deadline or relative yaw target reached.
 * CHECK -> FORWARD: fresh clear range; otherwise STOP/retry.
 * Any -> FAULT: stale/invalid sensor, tilt, timeout or exhausted attempts.
 * Any -> IDLE: STOP; FAULT needs explicit rearm, never automatic restart.
 */
status_t obstacle_avoidance_init(avoidance_context_t *context);
status_t obstacle_avoidance_update(avoidance_context_t *context, const sample_t *range,
                                  const imu_sample_t *imu, uint32_t now_ms, motor_cmd_t *request);
#endif
