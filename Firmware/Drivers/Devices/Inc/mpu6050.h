#ifndef MPU6050_H
#define MPU6050_H
#include "project_types.h"
typedef struct { uint8_t address_7bit; uint32_t timeout_ms; } mpu6050_config_t;
/* Dia chi 7 bit theo chan AD0 cua board. De o day de lop App khoi tao duoc cam
 * bien ma khong phai include board_config.h — board_config keo theo
 * stm32f1xx_hal.h, va App khong duoc nhin thay kieu du lieu cua HAL.
 */
uint8_t mpu6050_default_address(void);
status_t mpu6050_init(const mpu6050_config_t *config);
status_t mpu6050_read(imu_sample_t *sample);
status_t mpu6050_calibrate(void);
status_t mpu6050_calibrate_gyro(void);
bool mpu6050_is_initialized(void);
bool mpu6050_is_calibrated(void);
int16_t mpu6050_yaw_rate_dps(const imu_sample_t *sample);
bool mpu6050_tilt_exceeded(const imu_sample_t *sample);
#endif
