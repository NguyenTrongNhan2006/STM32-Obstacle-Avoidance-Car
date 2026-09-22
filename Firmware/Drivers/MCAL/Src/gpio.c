#include "gpio.h"
void gpio_emergency_stop(void)
{
    __HAL_RCC_GPIOB_CLK_ENABLE();
    HAL_GPIO_WritePin(MOTOR_STBY_PORT, MOTOR_STBY_PIN, GPIO_PIN_RESET);
}
status_t gpio_init(void)
{
    GPIO_InitTypeDef cfg = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();
    gpio_emergency_stop();
    HAL_GPIO_WritePin(GPIOB, MOTOR_AIN1_PIN | MOTOR_AIN2_PIN |
                     MOTOR_BIN1_PIN | MOTOR_BIN2_PIN | BUZZER_PIN, GPIO_PIN_RESET);
    cfg.Pin = MOTOR_STBY_PIN | MOTOR_AIN1_PIN | MOTOR_AIN2_PIN |
              MOTOR_BIN1_PIN | MOTOR_BIN2_PIN | BUZZER_PIN;
    cfg.Mode = GPIO_MODE_OUTPUT_PP;
    cfg.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &cfg);
    HAL_GPIO_WritePin(RANGE_TRIG_PORT, RANGE_TRIG_PIN, GPIO_PIN_RESET);
    cfg.Pin = RANGE_TRIG_PIN;
    HAL_GPIO_Init(RANGE_TRIG_PORT, &cfg);
    HAL_GPIO_WritePin(STATUS_LED_PORT, STATUS_LED_PIN, GPIO_PIN_SET);
    cfg.Pin = STATUS_LED_PIN;
    HAL_GPIO_Init(STATUS_LED_PORT, &cfg);
    cfg.Pin = BUTTON_PIN;
    cfg.Mode = GPIO_MODE_INPUT;
    cfg.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(BUTTON_PORT, &cfg);
    /* Cac chan con lai do chinh module so huu chung cau hinh, khong phai o day:
     *   PA0  Echo      -> timebase (TIM2 input capture)
     *   PA6  PA7 PWM   -> pwm      (TIM3)
     *   PB6  PB7 I2C   -> i2c      (I2C1)
     *   PA9  PA10 UART -> uart_debug (USART1)
     * Mot chan, mot chu so huu — gpio_init() chi lo cac chan GPIO thuan tuy. */
    return STATUS_OK;
}
status_t gpio_read(gpio_pin_t pin, bool *high)
{
    if (pin.port == NULL || pin.pin == 0U || high == NULL) { return STATUS_ERROR; }
    *high = HAL_GPIO_ReadPin(pin.port, pin.pin) == GPIO_PIN_SET;
    return STATUS_OK;
}
status_t gpio_write(gpio_pin_t pin, bool high)
{
    if (pin.port == NULL || pin.pin == 0U) { return STATUS_ERROR; }
    HAL_GPIO_WritePin(pin.port, pin.pin, high ? GPIO_PIN_SET : GPIO_PIN_RESET);
    return STATUS_OK;
}
