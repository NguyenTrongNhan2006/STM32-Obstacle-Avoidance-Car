#ifndef PROJECT_TYPES_H
#define PROJECT_TYPES_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef enum { STATUS_OK, STATUS_TIMEOUT, STATUS_ERROR, STATUS_NOT_READY } status_t;
typedef enum { SAMPLE_OK, SAMPLE_STALE, SAMPLE_TIMEOUT, SAMPLE_ERROR, SAMPLE_NOT_READY } sample_status_t;
typedef enum { SAMPLE_UNIT_MM } sample_unit_t;
/* timestamp_ms is acquisition time, never the time a consumer reads the mailbox. */
typedef struct {
    uint32_t value;
    sample_unit_t unit;
    uint32_t timestamp_ms;
    sample_status_t status;
} sample_t;
/* Signed raw register values; scale depends on the configured sensor ranges. */
typedef struct {
    int16_t accel_raw[3];
    int16_t gyro_raw[3];
    int16_t temperature_raw;
    uint32_t timestamp_ms;
    sample_status_t status;
} imu_sample_t;
#endif
