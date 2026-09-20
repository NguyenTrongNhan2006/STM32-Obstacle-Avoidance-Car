#include "robot_car.h"
#include "motor_tb6612.h"
#include "obstacle_avoidance.h"
#include "safety_monitor.h"
#include "sensor_manager.h"
static avoidance_context_t avoidance;
status_t robot_car_init(void)
{
    const motor_config_t config = { .is_safe = safety_is_clear_to_run };
    if (safety_init() != STATUS_OK || sensor_manager_init() != STATUS_OK ||
        obstacle_avoidance_init(&avoidance) != STATUS_OK) { return STATUS_ERROR; }
    return motor_init(&config);
}
status_t robot_car_update(uint32_t now_ms)
{
    (void)now_ms;
    /* IMPLEMENT: peek snapshots, request FSM command, then call motor_apply once.
     * Before real motion: prove STOP/fault latency including blocked/stalled decision.
     */
    return motor_apply(MOTOR_STOP, 0U);
}
