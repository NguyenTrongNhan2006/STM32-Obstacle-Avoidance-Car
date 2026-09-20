#include "obstacle_avoidance.h"
status_t obstacle_avoidance_init(avoidance_context_t *context)
{
    if (context == NULL) { return STATUS_ERROR; }
    *context = (avoidance_context_t){ .state = CAR_IDLE };
    return STATUS_OK;
}
status_t obstacle_avoidance_update(avoidance_context_t *context, const sample_t *range,
                                  const imu_sample_t *imu, uint32_t now_ms, motor_cmd_t *request)
{
    (void)now_ms;
    if (request == NULL) { return STATUS_ERROR; }
    *request = MOTOR_STOP;
    if (context == NULL || range == NULL || imu == NULL) { return STATUS_ERROR; }
    /* IMPLEMENT: nonblocking FSM using unsigned elapsed-time comparisons. */
    return STATUS_NOT_READY;
}
