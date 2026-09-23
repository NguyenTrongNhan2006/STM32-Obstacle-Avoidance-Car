#include "sensor_manager.h"
#include "app_config.h"
#include "ultrasonic_hcsr04.h"

QueueHandle_t qRangeMailbox;
QueueHandle_t qImuMailbox;
static StaticQueue_t range_control;
static StaticQueue_t imu_control;
static uint8_t range_storage[sizeof(sample_t)];
static uint8_t imu_storage[sizeof(imu_sample_t)];
static uint32_t last_range_req_ms;

status_t sensor_manager_init(void)
{
    const sample_t range = { .unit = SAMPLE_UNIT_MM, .status = SAMPLE_NOT_READY };
    const imu_sample_t imu = { .status = SAMPLE_NOT_READY };
    const ultrasonic_config_t us_cfg = {
        .echo_timeout_us = ECHO_TIMEOUT_US,
        .sample_period_ms = SENSOR_RANGE_PERIOD_MS
    };

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

    last_range_req_ms = 0U;
    return ultrasonic_init(&us_cfg);
}

status_t sensor_manager_update(uint32_t now_ms)
{
    sample_t range_sample;
    status_t status;

    /* 1. Periodic range trigger deadline (60 ms retrigger period) */
    if ((uint32_t)(now_ms - last_range_req_ms) >= SENSOR_RANGE_PERIOD_MS) {
        if (ultrasonic_request() == STATUS_OK) {
            last_range_req_ms = now_ms;
        }
    }

    /* 2. Read latest measurement (non-blocking) */
    status = ultrasonic_read(&range_sample);
    if (status == STATUS_OK || status == STATUS_TIMEOUT) {
        if (qRangeMailbox != NULL) {
            (void)xQueueOverwrite(qRangeMailbox, &range_sample);
        }
    }

    return STATUS_OK;
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
