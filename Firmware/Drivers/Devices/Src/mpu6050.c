#include "mpu6050.h"
status_t mpu6050_init(const mpu6050_config_t *config)
{
    if (config == NULL || config->address_7bit > 0x7FU || config->timeout_ms == 0U) { return STATUS_ERROR; }
    /* IMPLEMENT: identity check, ranges, sample rate and documented units. */
    return STATUS_NOT_READY;
}
status_t mpu6050_read(imu_sample_t *sample)
{
    if (sample == NULL) { return STATUS_ERROR; }
    *sample = (imu_sample_t){ .status = SAMPLE_NOT_READY };
    /* IMPLEMENT: coherent burst read, signed conversion and acquisition timestamp. */
    return STATUS_NOT_READY;
}
status_t mpu6050_calibrate(void)
{
    /* IMPLEMENT: stationary bias estimation; reject moving calibration. */
    return STATUS_NOT_READY;
}
