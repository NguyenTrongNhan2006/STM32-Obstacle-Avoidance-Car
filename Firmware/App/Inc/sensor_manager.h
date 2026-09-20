#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H
#include "project_types.h"
#include "FreeRTOS.h"
#include "queue.h"
/* Single producer tSensor. Consumers peek copies; never receive/drain a mailbox. */
extern QueueHandle_t qRangeMailbox;
extern QueueHandle_t qImuMailbox;
status_t sensor_manager_init(void);
status_t sensor_manager_update(uint32_t now_ms);
status_t sensor_manager_get_latest(sample_t *range, imu_sample_t *imu);
#endif
