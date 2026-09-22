#ifndef I2C_H
#define I2C_H
#include "project_types.h"
typedef struct { uint32_t bus_hz; } i2c_config_t;
status_t i2c_init(const i2c_config_t *config);
/* Address is 7-bit, timeout is nonzero milliseconds, buffers live through return. */
status_t i2c_read(uint8_t address, uint8_t reg, uint8_t *data, size_t length, uint32_t timeout_ms);
status_t i2c_write(uint8_t address, uint8_t reg, const uint8_t *data, size_t length, uint32_t timeout_ms);
/* Diem vao ISR cua I2C1 — i2c la chu so huu duy nhat cua bus nay.
 * stm32f1xx_it.c chi dinh tuyen, khong tu doc/ghi thanh ghi I2C1.
 */
void i2c_irq_event(void);
void i2c_irq_error(void);
#endif
