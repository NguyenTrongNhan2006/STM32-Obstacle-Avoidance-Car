#include "safety_monitor.h"
#include "app_config.h"
#include "button.h"
#include "sensor_manager.h"

EventGroupHandle_t egSafety;
static StaticEventGroup_t safety_storage;
/* An unknown button state must never look like a release that arms the next
 * press, so the edge detector starts as if the button were already held.
 */
static bool button_was_pressed = true;

static bool sample_fresh(uint32_t timestamp_ms, uint32_t now_ms)
{
    return (uint32_t)(now_ms - timestamp_ms) <= SENSOR_STALE_MS;
}

/* Scale-independent tilt test: cos^2(tilt) = az^2 / (ax^2 + ay^2 + az^2), so the
 * unconfirmed accelerometer full-scale range cancels out and no raw-to-degree
 * conversion is invented here.
 * [DO] assumes accel_raw[2] is the vertical axis and that a level car reads it
 * positive; confirm the mounting orientation before trusting this.
 */
static bool imu_upright(const imu_sample_t *imu)
{
    const int32_t ax = imu->accel_raw[0];
    const int32_t ay = imu->accel_raw[1];
    const int32_t az = imu->accel_raw[2];
    uint64_t vertical;
    uint64_t total;

    if (az <= 0) { return false; }
    vertical = (uint64_t)((int64_t)az * az);
    total = vertical + (uint64_t)((int64_t)ax * ax) + (uint64_t)((int64_t)ay * ay);
    if (total == 0U) { return false; }
    return (vertical * TILT_COS2_DEN) >= (total * TILT_COS2_NUM);
}

status_t safety_init(void)
{
    const button_config_t button = { .debounce_ms = BUTTON_DEBOUNCE_MS };

    if (egSafety == NULL) {
        egSafety = xEventGroupCreateStatic(&safety_storage);
        if (egSafety == NULL) { return STATUS_ERROR; }
        (void)xEventGroupSetBits(egSafety, SAFETY_BIT_STOP | SAFETY_BIT_SENSOR_FAULT);
    }
    /* A button that fails to initialise only removes the rearm path. The boot
     * inhibit stays latched, so the car still cannot run.
     */
    (void)button_init(&button);
    return STATUS_OK;
}

/* STATUS_OK means the policy ran, never that the car is safe; ask
 * safety_is_clear_to_run() for that.
 */
status_t safety_update(uint32_t now_ms)
{
    sample_t range;
    imu_sample_t imu;
    bool range_ok = false;
    bool imu_ok = false;
    bool upright = false;
    bool pressed = false;

    if (egSafety == NULL) { return STATUS_NOT_READY; }

    if (sensor_manager_get_latest(&range, &imu) == STATUS_OK) {
        range_ok = (range.status == SAMPLE_OK) && sample_fresh(range.timestamp_ms, now_ms);
        imu_ok = (imu.status == SAMPLE_OK) && sample_fresh(imu.timestamp_ms, now_ms);
        if (imu_ok) { upright = imu_upright(&imu); }
    }

    /* Faults latch. Absent, NOT_READY or stale data is evidence of a fault and
     * never evidence of health, so nothing below clears a bit on its own.
     */
    if (!range_ok || !imu_ok) {
        (void)xEventGroupSetBits(egSafety, SAFETY_BIT_SENSOR_FAULT);
    }
    if (imu_ok && !upright) {
        (void)xEventGroupSetBits(egSafety, SAFETY_BIT_TILT_FAULT);
    }

    if (button_read(&pressed) == STATUS_OK) {
        if (pressed && !button_was_pressed) {
            if ((xEventGroupGetBits(egSafety) & SAFETY_INHIBIT_MASK) != 0U) {
                /* Deliberate rearm: release only the inhibits currently proven
                 * absent. A press alone never clears an unproven fault.
                 */
                EventBits_t clear = SAFETY_BIT_STOP;
                if (range_ok && imu_ok) { clear |= SAFETY_BIT_SENSOR_FAULT; }
                if (imu_ok && upright) { clear |= SAFETY_BIT_TILT_FAULT; }
                (void)xEventGroupClearBits(egSafety, clear);
            } else {
                (void)xEventGroupSetBits(egSafety, SAFETY_BIT_STOP);
            }
        }
        button_was_pressed = pressed;
    } else {
        button_was_pressed = true;
    }
    return STATUS_OK;
}

bool safety_is_clear_to_run(void)
{
    return egSafety != NULL && (xEventGroupGetBits(egSafety) & SAFETY_INHIBIT_MASK) == 0U;
}
