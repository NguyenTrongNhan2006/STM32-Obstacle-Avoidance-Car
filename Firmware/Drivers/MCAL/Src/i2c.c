#include "i2c.h"
#include "board_config.h"
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

/* Dem so lan ISR chay khi I2C1 chua co chu. Chi de chan doan. */
volatile uint32_t g_i2c_spurious_irq;

/* i2c_init() con tra NOT_READY nen I2C1 chua duoc cau hinh. Neu mot trong hai
 * ISR duoi day chay, nghia la ngat da duoc bat ma khong qua module chu so huu.
 * Giong TIM2: phai tat nguon ngat va tat NVIC, vi chi `return` se sinh
 * interrupt storm — co EV/ERR cua I2C khong tu xoa.
 */
static void i2c_shutdown_irq(void)
{
    ++g_i2c_spurious_irq;
    IMU_I2C->CR2 &= (uint16_t)~(I2C_CR2_ITEVTEN | I2C_CR2_ITBUFEN | I2C_CR2_ITERREN);
    IMU_I2C->SR1 = 0U;  /* xoa co loi; co trang thai tu xoa khi doc SR1/SR2 */
    HAL_NVIC_DisableIRQ(I2C1_EV_IRQn);
    HAL_NVIC_DisableIRQ(I2C1_ER_IRQn);
}

void i2c_irq_event(void)
{
    i2c_shutdown_irq();
}

void i2c_irq_error(void)
{
    i2c_shutdown_irq();
}
