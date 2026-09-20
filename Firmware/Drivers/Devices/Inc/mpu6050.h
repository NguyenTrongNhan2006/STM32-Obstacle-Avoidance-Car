#ifndef MPU6050_H
#define MPU6050_H
#include "project_types.h"
typedef struct { uint8_t address_7bit; uint32_t timeout_ms; } mpu6050_config_t;
status_t mpu6050_init(const mpu6050_config_t *config);
status_t mpu6050_read(imu_sample_t *sample);
status_t mpu6050_calibrate(void);
#endif
