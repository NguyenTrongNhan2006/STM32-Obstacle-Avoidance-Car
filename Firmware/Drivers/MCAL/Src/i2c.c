#include "i2c.h"
status_t i2c_init(const i2c_config_t *config)
{
    if (config == NULL || config->bus_hz == 0U) { return STATUS_ERROR; }
    /* IMPLEMENT: I2C1 only; enable vendor I2C sources/config in a later hardware change. */
    return STATUS_NOT_READY;
}
status_t i2c_read(uint8_t address, uint8_t reg, uint8_t *data, size_t length, uint32_t timeout_ms)
{
    (void)reg;
    if (address > 0x7FU || data == NULL || length == 0U || timeout_ms == 0U) { return STATUS_ERROR; }
    /* IMPLEMENT: bounded transaction; output buffer unchanged until implemented. */
    return STATUS_NOT_READY;
}
status_t i2c_write(uint8_t address, uint8_t reg, const uint8_t *data, size_t length, uint32_t timeout_ms)
{
    (void)reg;
    if (address > 0x7FU || data == NULL || length == 0U || timeout_ms == 0U) { return STATUS_ERROR; }
    /* IMPLEMENT: bounded transaction and stuck-bus recovery. */
    return STATUS_NOT_READY;
}
