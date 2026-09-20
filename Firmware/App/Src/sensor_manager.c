#include "sensor_manager.h"
QueueHandle_t qRangeMailbox;
QueueHandle_t qImuMailbox;
static StaticQueue_t range_control;
static StaticQueue_t imu_control;
static uint8_t range_storage[sizeof(sample_t)];
static uint8_t imu_storage[sizeof(imu_sample_t)];
status_t sensor_manager_init(void)
{
    const sample_t range = { .unit = SAMPLE_UNIT_MM, .status = SAMPLE_NOT_READY };
    const imu_sample_t imu = { .status = SAMPLE_NOT_READY };
    if (qRangeMailbox == NULL) {
        qRangeMailbox = xQueueCreateStatic(1U, sizeof(sample_t), range_storage, &range_control);
        if (qRangeMailbox == NULL) { return STATUS_ERROR; }
        configASSERT(xQueueOverwrite(qRangeMailbox, &range) == pdPASS);
    }
    if (qImuMailbox == NULL) {
        qImuMailbox = xQueueCreateStatic(1U, sizeof(imu_sample_t), imu_storage, &imu_control);
        if (qImuMailbox == NULL) { return STATUS_ERROR; }
        configASSERT(xQueueOverwrite(qImuMailbox, &imu) == pdPASS);
    }
    return STATUS_OK;
}
status_t sensor_manager_update(uint32_t now_ms)
{
    (void)now_ms;
    /* IMPLEMENT: independent range/IMU deadlines and xQueueOverwrite copies.
     * Reading existing samples must not refresh their timestamps.
     */
    return STATUS_NOT_READY;
}
status_t sensor_manager_get_latest(sample_t *range, imu_sample_t *imu)
{
    if (range == NULL || imu == NULL) { return STATUS_ERROR; }
    *range = (sample_t){ .unit = SAMPLE_UNIT_MM, .status = SAMPLE_NOT_READY };
    *imu = (imu_sample_t){ .status = SAMPLE_NOT_READY };
    if (qRangeMailbox == NULL || qImuMailbox == NULL) { return STATUS_NOT_READY; }
    if (xQueuePeek(qRangeMailbox, range, 0U) != pdPASS ||
        xQueuePeek(qImuMailbox, imu, 0U) != pdPASS) { return STATUS_NOT_READY; }
    /* STATUS_OK means copies available; caller must inspect BOTH sample statuses
     * and ages. Pair is not an atomic simultaneous sensor acquisition.
     */
    return STATUS_OK;
}
