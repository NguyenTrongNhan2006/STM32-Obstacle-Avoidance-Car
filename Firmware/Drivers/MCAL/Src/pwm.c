#include "pwm.h"
#include "board_config.h"

/* TIM3 clock path: SYSCLK 72 MHz -> AHB /1 -> APB1 /2 = 36 MHz.
 * APB1 prescaler != 1, so timer clock = 2 * APB1 = 72 MHz.
 * For 20 kHz: ARR+1 = 72 MHz / 20 kHz = 3600 => ARR = 3599, PSC = 0.
 * duty_per_mille 0..1000 maps to CCR = duty * 3600 / 1000.
 */
#define TIM_CLK_HZ   72000000UL

static TIM_HandleTypeDef htim;
static uint32_t period;          /* ARR + 1, cached for duty conversion */
static bool initialized;

status_t pwm_init(const pwm_config_t *config)
{
    TIM_OC_InitTypeDef oc = {0};

    if (config == NULL || config->frequency_hz == 0U) { return STATUS_ERROR; }

    initialized = false;
    period = TIM_CLK_HZ / config->frequency_hz;
    if (period == 0U || period > 0xFFFFUL) { return STATUS_ERROR; }

    __HAL_RCC_TIM3_CLK_ENABLE();

    htim.Instance               = MOTOR_PWM_TIMER;
    htim.Init.Prescaler         = 0U;                   /* no prescaler */
    htim.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim.Init.Period            = period - 1U;           /* ARR = 3599 for 20 kHz */
    htim.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (HAL_TIM_PWM_Init(&htim) != HAL_OK) { return STATUS_ERROR; }

    /* Configure both channels with zero duty so motors stay off. */
    oc.OCMode     = TIM_OCMODE_PWM1;
    oc.Pulse      = 0U;
    oc.OCPolarity = TIM_OCPOLARITY_HIGH;
    oc.OCFastMode = TIM_OCFAST_DISABLE;

    if (HAL_TIM_PWM_ConfigChannel(&htim, &oc, TIM_CHANNEL_1) != HAL_OK) { return STATUS_ERROR; }
    if (HAL_TIM_PWM_ConfigChannel(&htim, &oc, TIM_CHANNEL_2) != HAL_OK) { return STATUS_ERROR; }

    /* PA6 = TIM3_CH1, PA7 = TIM3_CH2: configure as AF push-pull. */
    {
        GPIO_InitTypeDef gpio = {0};
        __HAL_RCC_GPIOA_CLK_ENABLE();
        gpio.Pin   = MOTOR_LEFT_PWM_PIN | MOTOR_RIGHT_PWM_PIN;
        gpio.Mode  = GPIO_MODE_AF_PP;
        gpio.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(MOTOR_PWM_PORT, &gpio);
    }

    /* Start both channels — output stays low because Pulse = 0. */
    if (HAL_TIM_PWM_Start(&htim, TIM_CHANNEL_1) != HAL_OK) { return STATUS_ERROR; }
    if (HAL_TIM_PWM_Start(&htim, TIM_CHANNEL_2) != HAL_OK) { return STATUS_ERROR; }

    initialized = true;
    return STATUS_OK;
}

status_t pwm_write(uint8_t channel, uint16_t duty_per_mille)
{
    uint32_t tim_channel;
    uint32_t ccr;

    if (channel < 1U || channel > 2U || duty_per_mille > 1000U) { return STATUS_ERROR; }
    if (!initialized) { return STATUS_NOT_READY; }

    tim_channel = (channel == 1U) ? TIM_CHANNEL_1 : TIM_CHANNEL_2;
    ccr = (uint32_t)duty_per_mille * period / 1000U;
    __HAL_TIM_SET_COMPARE(&htim, tim_channel, ccr);
    return STATUS_OK;
}

status_t pwm_stop(void)
{
    if (!initialized) { return STATUS_NOT_READY; }
    __HAL_TIM_SET_COMPARE(&htim, TIM_CHANNEL_1, 0U);
    __HAL_TIM_SET_COMPARE(&htim, TIM_CHANNEL_2, 0U);
    return STATUS_OK;
}
