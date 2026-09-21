#include "obstacle_avoidance.h"
#include "app_config.h"
#include "safety_monitor.h"

static void enter_state(avoidance_context_t *context, car_state_t state, uint32_t now_ms)
{
    context->state = state;
    context->entered_ms = now_ms;
}

/* There are no side sensors, so a heading is a trial rather than a measured
 * clear direction. Alternate so one blocked side is not retried forever.
 */
static car_state_t next_turn(uint8_t attempts)
{
    return ((attempts & 1U) != 0U) ? CAR_TURN_LEFT : CAR_TURN_RIGHT;
}

status_t obstacle_avoidance_init(avoidance_context_t *context)
{
    if (context == NULL) { return STATUS_ERROR; }
    *context = (avoidance_context_t){ .state = CAR_IDLE };
    return STATUS_OK;
}

status_t obstacle_avoidance_update(avoidance_context_t *context, const sample_t *range,
                                  const imu_sample_t *imu, uint32_t now_ms, motor_cmd_t *request)
{
    uint32_t in_state_ms;
    bool range_valid;
    bool blocked;
    bool clear_ahead;

    if (request == NULL) { return STATUS_ERROR; }
    *request = MOTOR_STOP;
    if (context == NULL || range == NULL || imu == NULL) { return STATUS_ERROR; }
    /* Tilt policy belongs to safety_monitor, which owns the inhibit bits. */
    (void)imu;

    /* FAULT is terminal here. Only a deliberate rearm, applied by robot_car
     * re-initialising this context after the safety owner clears the inhibit,
     * leaves it; nothing restarts motion automatically.
     */
    if (context->state == CAR_FAULT) { return STATUS_OK; }

    in_state_ms = now_ms - context->entered_ms;
    range_valid = (range->status == SAMPLE_OK) &&
                  ((uint32_t)(now_ms - range->timestamp_ms) <= SENSOR_STALE_MS);

    if (!range_valid) {
        /* Losing the only forward sensor while moving is a fault. While idle it
         * just means there is no evidence to start on.
         */
        if (context->state != CAR_IDLE) { enter_state(context, CAR_FAULT, now_ms); }
        return STATUS_OK;
    }
    if (!safety_is_clear_to_run()) {
        if (context->state != CAR_IDLE) { enter_state(context, CAR_IDLE, now_ms); }
        return STATUS_OK;
    }

    /* Different entry and exit thresholds; a single threshold would chatter. */
    blocked = range->value <= D_STOP_MM;
    clear_ahead = range->value >= D_CLEAR_MM;

    switch (context->state) {
    case CAR_IDLE:
        if (clear_ahead) {
            context->attempts = 0U;
            enter_state(context, CAR_FORWARD, now_ms);
            *request = MOTOR_FORWARD;
        }
        break;

    case CAR_FORWARD:
        if (blocked) { enter_state(context, CAR_STOP, now_ms); }
        else { *request = MOTOR_FORWARD; }
        break;

    case CAR_STOP:
        if ((uint32_t)context->attempts >= MAX_AVOIDANCE_ATTEMPTS) {
            enter_state(context, CAR_FAULT, now_ms);
        } else {
            context->attempts++;
            enter_state(context, next_turn(context->attempts), now_ms);
        }
        break;

    case CAR_TURN_LEFT:
    case CAR_TURN_RIGHT:
        if (in_state_ms >= T_TURN_MS) {
            enter_state(context, CAR_CHECK, now_ms);
        } else {
            *request = (context->state == CAR_TURN_LEFT) ? MOTOR_TURN_LEFT : MOTOR_TURN_RIGHT;
        }
        break;

    case CAR_CHECK: {
        /* Only accept a sample acquired after entering CHECK. A reading taken
         * before the turn says nothing about the new heading. Both sides are
         * elapsed times, so this survives tick wrap.
         */
        const uint32_t sample_age_ms = now_ms - range->timestamp_ms;
        if (sample_age_ms <= in_state_ms) {
            if (clear_ahead) {
                context->attempts = 0U;
                enter_state(context, CAR_FORWARD, now_ms);
                *request = MOTOR_FORWARD;
            } else if ((uint32_t)context->attempts >= MAX_AVOIDANCE_ATTEMPTS) {
                enter_state(context, CAR_FAULT, now_ms);
            } else {
                context->attempts++;
                enter_state(context, next_turn(context->attempts), now_ms);
            }
        }
        break;
    }

    case CAR_FAULT:
    default:
        break;
    }
    return STATUS_OK;
}
