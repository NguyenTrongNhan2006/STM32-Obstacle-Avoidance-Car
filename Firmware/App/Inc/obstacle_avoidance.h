#ifndef OBSTACLE_AVOIDANCE_H
#define OBSTACLE_AVOIDANCE_H

#include <stdint.h>

/* Task: Hung.
 * Decides motor commands from sensor input. States follow the FSM
 * proposed in About/PROJECT_PLAN.md ("Đề xuất hành vi").
 * Interface only - implementation not written yet.
 */

typedef enum {
    OA_STATE_IDLE = 0,
    OA_STATE_FORWARD,
    OA_STATE_STOP,
    OA_STATE_TURN_LEFT,
    OA_STATE_TURN_RIGHT,
    OA_STATE_CHECK,
    OA_STATE_FAULT
} ObstacleAvoidance_State_t;

typedef struct {
    uint16_t front_distance_mm;
    uint8_t  front_distance_valid;   /* 0 = stale/error: do not treat as clear path */
} ObstacleAvoidance_Input_t;

void ObstacleAvoidance_Init(void);
ObstacleAvoidance_State_t ObstacleAvoidance_GetState(void);

/* Call periodically from the main loop with the latest sensor snapshot. */
void ObstacleAvoidance_Update(const ObstacleAvoidance_Input_t *sensor_input, uint32_t now_ms);

/* STOP request; highest priority, valid from any state. */
void ObstacleAvoidance_RequestStop(void);

/* Required to leave FAULT once the fault condition has cleared. */
void ObstacleAvoidance_AcknowledgeFault(void);

#endif /* OBSTACLE_AVOIDANCE_H */
