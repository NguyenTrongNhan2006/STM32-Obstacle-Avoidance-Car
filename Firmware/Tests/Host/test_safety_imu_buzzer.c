#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "safety_monitor.h"
#include "sensor_manager.h"
#include "button.h"
#include "watchdog.h"
#include "gpio.h"
#include "mpu6050.h"
#include "i2c.h"
#include "timebase.h"
#include "buzzer.h"
#include "uart_debug.h"

static uint32_t now_ms;
static uint32_t watchdog_refreshes;
static uint32_t emergency_stops;
static bool stby_high;
static bool button_pressed;
static bool buzzer_high;
static bool imu_data_ready;
static bool imu_bus_fault;
static int16_t imu_raw_z;
static uint32_t burst_reads;
static char uart_output[256];
static size_t uart_output_length;

void HAL_GPIO_Init(GPIO_TypeDef *port, const GPIO_InitTypeDef *pins)
{ (void)port; (void)pins; }
HAL_StatusTypeDef HAL_UART_Init(UART_HandleTypeDef *handle)
{ (void)handle; return HAL_OK; }
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *handle,
                                    const uint8_t *data, uint16_t length,
                                    uint32_t timeout_ms)
{
    (void)handle; (void)timeout_ms;
    assert(uart_output_length + length < sizeof(uart_output));
    memcpy(uart_output + uart_output_length, data, length);
    uart_output_length += length;
    uart_output[uart_output_length] = '\0';
    return HAL_OK;
}
HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef *handle, uint8_t *data,
                                   uint16_t length, uint32_t timeout_ms)
{ (void)handle; (void)data; (void)length; (void)timeout_ms; return HAL_TIMEOUT; }

EventGroupHandle_t xEventGroupCreateStatic(StaticEventGroup_t *storage)
{
    storage->bits = 0U;
    return storage;
}
EventBits_t xEventGroupSetBits(EventGroupHandle_t group, EventBits_t bits)
{ return group->bits |= bits; }
EventBits_t xEventGroupClearBits(EventGroupHandle_t group, EventBits_t bits)
{
    const EventBits_t previous = group->bits;
    group->bits &= ~bits;
    return previous;
}
EventBits_t xEventGroupGetBits(EventGroupHandle_t group) { return group->bits; }
status_t button_init(const button_config_t *config)
{ (void)config; return STATUS_OK; }
status_t button_read(bool *pressed)
{ *pressed = button_pressed; return STATUS_OK; }
status_t watchdog_refresh(void)
{ ++watchdog_refreshes; return STATUS_OK; }
void gpio_emergency_stop(void)
{ ++emergency_stops; stby_high = false; }
status_t gpio_write(gpio_pin_t pin, bool high)
{
    (void)pin;
    buzzer_high = high;
    return STATUS_OK;
}
status_t sensor_manager_get_latest(sample_t *range, imu_sample_t *imu)
{
    *range = (sample_t){ .value = 1000U, .status = SAMPLE_OK,
                         .timestamp_ms = now_ms };
    *imu = (imu_sample_t){ .accel_raw = {0, 0, 16384},
                           .status = SAMPLE_OK, .timestamp_ms = now_ms };
    return STATUS_OK;
}
uint32_t timebase_now_ms(void) { return now_ms; }
status_t i2c_write(uint8_t address, uint8_t reg, const uint8_t *data,
                   size_t length, uint32_t timeout_ms)
{
    (void)address; (void)reg; (void)data; (void)length; (void)timeout_ms;
    return imu_bus_fault ? STATUS_TIMEOUT : STATUS_OK;
}
status_t i2c_read(uint8_t address, uint8_t reg, uint8_t *data,
                  size_t length, uint32_t timeout_ms)
{
    (void)address; (void)timeout_ms;
    if (imu_bus_fault) { return STATUS_TIMEOUT; }
    memset(data, 0, length);
    if (reg == 0x75U && length == 1U) { data[0] = 0x68U; }
    if (reg == 0x3AU && length == 1U) {
        data[0] = imu_data_ready ? 1U : 0U;
        imu_data_ready = false; /* the sensor exposes each fresh sample once */
    }
    if (reg == 0x3BU && length == 14U) {
        ++burst_reads;
        data[4] = 0x40U;
        data[12] = (uint8_t)((uint16_t)imu_raw_z >> 8U);
        data[13] = (uint8_t)imu_raw_z;
    }
    return STATUS_OK;
}

static void test_safety(void)
{
    uint32_t before;
    assert(safety_init() == STATUS_OK);
    now_ms = 100U;
    safety_check_in(ALIVE_BIT_SENSOR | ALIVE_BIT_DECISION);
    assert(safety_update(now_ms) == STATUS_OK);
    assert(watchdog_refreshes == 1U);
    button_pressed = true;
    now_ms = 110U;
    assert(safety_update(now_ms) == STATUS_OK);
    assert(safety_is_clear_to_run());
    stby_high = true; /* a Decision command has enabled the bridge */

    before = emergency_stops;
    safety_check_in(ALIVE_BIT_SENSOR);
    now_ms = 200U;
    assert(safety_update(now_ms) == STATUS_OK);
    assert(emergency_stops > before && !stby_high);
    assert((xEventGroupGetBits(egSafety) & SAFETY_BIT_TASK_FAULT) != 0U);
    assert(!safety_is_clear_to_run() && watchdog_refreshes == 1U);

    button_pressed = false;
    now_ms = 210U;
    (void)safety_update(now_ms);
    button_pressed = true;
    now_ms = 220U;
    (void)safety_update(now_ms);
    assert(!safety_is_clear_to_run()); /* rearm while a task is missing fails */

    safety_check_in(ALIVE_BIT_SENSOR | ALIVE_BIT_DECISION);
    now_ms = 300U;
    (void)safety_update(now_ms);
    assert(watchdog_refreshes == 2U && !safety_is_clear_to_run());
    button_pressed = false;
    now_ms = 310U;
    (void)safety_update(now_ms);
    button_pressed = true;
    now_ms = 320U;
    (void)safety_update(now_ms);
    assert(safety_is_clear_to_run()); /* deliberate rearm after recovery */
}

static void test_imu(void)
{
    const mpu6050_config_t config = { .address_7bit = 0x68U, .timeout_ms = 10U };
    imu_sample_t sample;
    uint32_t index;

    imu_raw_z = 200;
    assert(mpu6050_init(&config) == STATUS_OK);
    assert(!mpu6050_is_calibrated());
    assert(mpu6050_read(&sample) == STATUS_NOT_READY);
    for (index = 0U; index < 10U; ++index) {
        assert(mpu6050_calibrate_gyro() == STATUS_NOT_READY);
    }
    assert(burst_reads == 0U); /* no data-ready, no duplicate samples */
    for (index = 0U; index < 64U; ++index) {
        imu_data_ready = true;
        now_ms += 10U;
        assert(mpu6050_calibrate_gyro() ==
               ((index == 63U) ? STATUS_OK : STATUS_NOT_READY));
    }
    assert(burst_reads == 64U && mpu6050_is_calibrated());
    assert(mpu6050_read(&sample) == STATUS_OK && sample.gyro_raw[2] == 0);

    assert(mpu6050_init(&config) == STATUS_OK);
    assert(!mpu6050_is_calibrated());
    assert(mpu6050_read(&sample) == STATUS_NOT_READY);
    imu_raw_z = 500;
    imu_data_ready = true;
    assert(mpu6050_calibrate_gyro() == STATUS_ERROR);
    assert(!mpu6050_is_calibrated());
    imu_raw_z = 200;
    imu_data_ready = true;
    assert(mpu6050_calibrate_gyro() == STATUS_NOT_READY);
    imu_bus_fault = true;
    assert(mpu6050_calibrate_gyro() == STATUS_NOT_READY);
    assert(!mpu6050_is_initialized() && !mpu6050_is_calibrated());
    imu_bus_fault = false;
    assert(mpu6050_init(&config) == STATUS_OK);
    assert(!mpu6050_is_calibrated());
}

static void test_buzzer(void)
{
    const buzzer_config_t config = { .active_high = true };
    assert(buzzer_init(&config) == STATUS_OK);
    assert(buzzer_set(BUZZER_START) == STATUS_OK);
    assert(buzzer_update(0U) == STATUS_OK && buzzer_high);
    assert(buzzer_set(BUZZER_OBSTACLE) == STATUS_OK);
    assert(buzzer_set(BUZZER_START) == STATUS_OK); /* 20 ms STOP was missed by task */
    assert(buzzer_update(100U) == STATUS_OK && buzzer_high);
    assert(buzzer_update(200U) == STATUS_OK && !buzzer_high);
    assert(buzzer_update(300U) == STATUS_OK && buzzer_high);
    assert(buzzer_update(400U) == STATUS_OK && !buzzer_high);
    assert(buzzer_update(1100U) == STATUS_OK && buzzer_high);
    assert(buzzer_set(BUZZER_OBSTACLE) == STATUS_OK);
    assert(buzzer_update(1200U) == STATUS_OK && buzzer_high);
    assert(buzzer_set(BUZZER_FAULT) == STATUS_OK);
    assert(buzzer_update(1300U) == STATUS_OK && buzzer_high);
    assert(buzzer_update(1500U) == STATUS_OK && !buzzer_high);
    assert(buzzer_set(BUZZER_SILENT) == STATUS_OK);
    assert(buzzer_update(1600U) == STATUS_OK && !buzzer_high);
}

static void test_uart(void)
{
    const uart_debug_config_t config = { .baud = 115200U, .timeout_ms = 20U };
    assert(uart_debug_init(&config) == STATUS_OK);
    assert(uart_log_i32("yaw=", -10) == STATUS_OK);
    assert(uart_log_i32("min=", INT32_MIN) == STATUS_OK);
    assert(uart_log_i32("max=", INT32_MAX) == STATUS_OK);
    assert(uart_log_i32("zero=", 0) == STATUS_OK);
    assert(strcmp(uart_output,
                  "yaw=-10\r\nmin=-2147483648\r\nmax=2147483647\r\nzero=0\r\n") == 0);
}

int main(void)
{
    test_safety();
    test_imu();
    test_buzzer();
    test_uart();
    puts("Host tests passed: safety, IMU calibration, buzzer, signed UART.");
    return 0;
}
