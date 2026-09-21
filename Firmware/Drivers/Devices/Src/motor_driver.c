#include "motor_driver.h"
#include "board_config.h"

static TIM_HandleTypeDef htim2;
static uint8_t motor_initialized = 0;

static uint32_t Motor_Channel(Motor_Id_t motor)
{
    return (motor == MOTOR_LEFT) ? MOTOR_LEFT_PWM_CHANNEL : MOTOR_RIGHT_PWM_CHANNEL;
}

static void Motor_GPIO_Init(void)
{
    GPIO_InitTypeDef gpio_init = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* TIM2 PWM outputs: PA0 (CH1), PA1 (CH2) */
    gpio_init.Pin   = MOTOR_LEFT_PWM_Pin | MOTOR_RIGHT_PWM_Pin;
    gpio_init.Mode  = GPIO_MODE_AF_PP;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio_init);

    /* Direction pins: PB0/PB1 (left IN1/IN2), PB10/PB11 (right IN1/IN2) */
    gpio_init.Pin   = MOTOR_LEFT_IN1_Pin | MOTOR_LEFT_IN2_Pin |
                       MOTOR_RIGHT_IN1_Pin | MOTOR_RIGHT_IN2_Pin;
    gpio_init.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull  = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &gpio_init);

    HAL_GPIO_WritePin(GPIOB,
        MOTOR_LEFT_IN1_Pin | MOTOR_LEFT_IN2_Pin | MOTOR_RIGHT_IN1_Pin | MOTOR_RIGHT_IN2_Pin,
        GPIO_PIN_RESET);
}

static Motor_Status_t Motor_PWM_Init(void)
{
    TIM_ClockConfigTypeDef clock_source = {0};
    TIM_MasterConfigTypeDef master_config = {0};
    TIM_OC_InitTypeDef pwm_config = {0};

    __HAL_RCC_TIM2_CLK_ENABLE();

    htim2.Instance               = MOTOR_PWM_TIM_INSTANCE;
    htim2.Init.Prescaler         = MOTOR_PWM_PRESCALER;
    htim2.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim2.Init.Period            = MOTOR_PWM_PERIOD;
    htim2.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (HAL_TIM_PWM_Init(&htim2) != HAL_OK) {
        return MOTOR_ERR_NOT_INIT;
    }

    clock_source.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim2, &clock_source) != HAL_OK) {
        return MOTOR_ERR_NOT_INIT;
    }

    master_config.MasterOutputTrigger = TIM_TRGO_RESET;
    master_config.MasterSlaveMode     = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &master_config) != HAL_OK) {
        return MOTOR_ERR_NOT_INIT;
    }

    pwm_config.OCMode     = TIM_OCMODE_PWM1;
    pwm_config.Pulse      = 0;
    pwm_config.OCPolarity = TIM_OCPOLARITY_HIGH;
    pwm_config.OCFastMode = TIM_OCFAST_DISABLE;

    if (HAL_TIM_PWM_ConfigChannel(&htim2, &pwm_config, MOTOR_LEFT_PWM_CHANNEL) != HAL_OK) {
        return MOTOR_ERR_NOT_INIT;
    }
    if (HAL_TIM_PWM_ConfigChannel(&htim2, &pwm_config, MOTOR_RIGHT_PWM_CHANNEL) != HAL_OK) {
        return MOTOR_ERR_NOT_INIT;
    }

    HAL_TIM_PWM_Start(&htim2, MOTOR_LEFT_PWM_CHANNEL);
    HAL_TIM_PWM_Start(&htim2, MOTOR_RIGHT_PWM_CHANNEL);

    return MOTOR_OK;
}

Motor_Status_t Motor_Init(void)
{
    Motor_Status_t status;

    Motor_GPIO_Init();
    status = Motor_PWM_Init();
    motor_initialized = (status == MOTOR_OK);
    return status;
}

static void Motor_SetDirection(Motor_Id_t motor, int8_t forward)
{
    GPIO_TypeDef *port = GPIOB;
    uint16_t in1_pin = (motor == MOTOR_LEFT) ? MOTOR_LEFT_IN1_Pin : MOTOR_RIGHT_IN1_Pin;
    uint16_t in2_pin = (motor == MOTOR_LEFT) ? MOTOR_LEFT_IN2_Pin : MOTOR_RIGHT_IN2_Pin;

    if (forward > 0) {
        HAL_GPIO_WritePin(port, in1_pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(port, in2_pin, GPIO_PIN_RESET);
    } else if (forward < 0) {
        HAL_GPIO_WritePin(port, in1_pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(port, in2_pin, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(port, in1_pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(port, in2_pin, GPIO_PIN_RESET);
    }
}

static uint32_t Motor_PercentToTicks(uint8_t speed_percent)
{
    if (speed_percent > 100) {
        speed_percent = 100;
    }
    return ((uint32_t)speed_percent * MOTOR_PWM_PERIOD) / 100U;
}

Motor_Status_t Motor_SetSpeed(Motor_Id_t motor, int16_t speed_percent)
{
    uint8_t magnitude;

    if (!motor_initialized) {
        return MOTOR_ERR_NOT_INIT;
    }
    if (motor >= MOTOR_COUNT || speed_percent < -100 || speed_percent > 100) {
        return MOTOR_ERR_INVALID_ARG;
    }

    magnitude = (uint8_t)((speed_percent < 0) ? -speed_percent : speed_percent);
    Motor_SetDirection(motor, (speed_percent > 0) - (speed_percent < 0));
    __HAL_TIM_SET_COMPARE(&htim2, Motor_Channel(motor), Motor_PercentToTicks(magnitude));

    return MOTOR_OK;
}

Motor_Status_t Motor_Stop(Motor_Id_t motor)
{
    if (!motor_initialized) {
        return MOTOR_ERR_NOT_INIT;
    }
    if (motor >= MOTOR_COUNT) {
        return MOTOR_ERR_INVALID_ARG;
    }

    Motor_SetDirection(motor, 0);
    __HAL_TIM_SET_COMPARE(&htim2, Motor_Channel(motor), 0);
    return MOTOR_OK;
}

Motor_Status_t Motor_Brake(Motor_Id_t motor)
{
    GPIO_TypeDef *port = GPIOB;
    uint16_t in1_pin, in2_pin;

    if (!motor_initialized) {
        return MOTOR_ERR_NOT_INIT;
    }
    if (motor >= MOTOR_COUNT) {
        return MOTOR_ERR_INVALID_ARG;
    }

    in1_pin = (motor == MOTOR_LEFT) ? MOTOR_LEFT_IN1_Pin : MOTOR_RIGHT_IN1_Pin;
    in2_pin = (motor == MOTOR_LEFT) ? MOTOR_LEFT_IN2_Pin : MOTOR_RIGHT_IN2_Pin;
    HAL_GPIO_WritePin(port, in1_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(port, in2_pin, GPIO_PIN_SET);
    __HAL_TIM_SET_COMPARE(&htim2, Motor_Channel(motor), 0);

    return MOTOR_OK;
}

Motor_Status_t Motor_StopAll(void)
{
    Motor_Status_t left_status = Motor_Stop(MOTOR_LEFT);
    Motor_Status_t right_status = Motor_Stop(MOTOR_RIGHT);

    return (left_status == MOTOR_OK && right_status == MOTOR_OK) ? MOTOR_OK : left_status;
}
