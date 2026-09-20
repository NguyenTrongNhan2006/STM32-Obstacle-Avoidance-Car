#include "safety_monitor.h"
EventGroupHandle_t egSafety;
static StaticEventGroup_t safety_storage;
status_t safety_init(void)
{
    if (egSafety == NULL) {
        egSafety = xEventGroupCreateStatic(&safety_storage);
        if (egSafety == NULL) { return STATUS_ERROR; }
        (void)xEventGroupSetBits(egSafety, SAFETY_BIT_STOP | SAFETY_BIT_SENSOR_FAULT);
    }
    return STATUS_OK;
}
status_t safety_update(uint32_t now_ms)
{
    (void)now_ms;
    /* IMPLEMENT: STOP latch, freshness, IMU tilt and explicit safe rearm.
     * Do not clear a fault because a sample is absent or NOT_READY.
     */
    return STATUS_NOT_READY;
}
bool safety_is_clear_to_run(void)
{
    return egSafety != NULL && (xEventGroupGetBits(egSafety) & SAFETY_INHIBIT_MASK) == 0U;
}
