#include "pwm.h"
#include "board_config.h"

/* pwm la chu so huu duy nhat cua TIM3 (bang owners trong board_config.h).
 * Hai kenh deu thuoc cung mot timer nen dung chung tan so va thanh ghi ARR —
 * doi tan so la doi cho ca hai banh, khong the doi rieng mot ben.
 *
 * PSC = 0 de giu nguyen do phan giai toi da: bo dem chay thang 72 MHz, mot chu
 * ky 20 kHz dai 3600 tick. Duty tinh theo phan nghin nen buoc nho nhat la
 * 3.6 tick — mot duty hop le bat ky deu bieu dien duoc chinh xac.
 */
#define PWM_PRESCALER 0U

static TIM_HandleTypeDef motor_timer;
static uint32_t period_ticks;
static bool initialized;

static uint32_t hal_channel(uint8_t channel)
{
    return (channel == MOTOR_LEFT_PWM_CHANNEL) ? TIM_CHANNEL_1 : TIM_CHANNEL_2;
}

status_t pwm_init(const pwm_config_t *config)
{
    GPIO_InitTypeDef pins = {0};
    TIM_OC_InitTypeDef output = {0};
    TIM_ClockConfigTypeDef clock = {0};
    uint32_t timer_hz;

    if (config == NULL || config->frequency_hz == 0U) { return STATUS_ERROR; }
    initialized = false;

    /* TIM3 nam tren APB1, nhan doi clock khi APB1 prescaler khac 1 — giong TIM2.
     * Tinh tu thanh ghi thay vi hard-code 3599 de doi clock tree se lo ra ngay. */
    timer_hz = HAL_RCC_GetPCLK1Freq();
    if ((RCC->CFGR & RCC_CFGR_PPRE1) != RCC_CFGR_PPRE1_DIV1) { timer_hz *= 2U; }
    if ((timer_hz % config->frequency_hz) != 0U) { return STATUS_ERROR; }
    period_ticks = timer_hz / config->frequency_hz;
    if (period_ticks < 2U || period_ticks > 0x10000U) { return STATUS_ERROR; }

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_TIM3_CLK_ENABLE();

    motor_timer.Instance = MOTOR_PWM_TIMER;
    motor_timer.Init.Prescaler = PWM_PRESCALER;
    motor_timer.Init.CounterMode = TIM_COUNTERMODE_UP;
    motor_timer.Init.Period = period_ticks - 1U;
    motor_timer.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    motor_timer.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (HAL_TIM_PWM_Init(&motor_timer) != HAL_OK) { return STATUS_ERROR; }

    clock.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&motor_timer, &clock) != HAL_OK) { return STATUS_ERROR; }

    /* Preload bat: CCR moi chi co hieu luc o dau chu ky sau, nen mot lenh duty
     * den giua chu ky khong tao ra mot xung cut ngan bat thuong. */
    output.OCMode = TIM_OCMODE_PWM1;
    output.Pulse = 0U;                      /* khoi dong o duty 0, KHONG phai duty cu */
    output.OCPolarity = TIM_OCPOLARITY_HIGH;
    output.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&motor_timer, &output, TIM_CHANNEL_1) != HAL_OK ||
        HAL_TIM_PWM_ConfigChannel(&motor_timer, &output, TIM_CHANNEL_2) != HAL_OK) {
        return STATUS_ERROR;
    }
    __HAL_TIM_ENABLE_OCxPRELOAD(&motor_timer, TIM_CHANNEL_1);
    __HAL_TIM_ENABLE_OCxPRELOAD(&motor_timer, TIM_CHANNEL_2);

    /* Cau hinh chan SAU khi CCR da o 0: neu doi chan sang AF truoc, dau ra se
     * theo trang thai chua xac dinh cua thanh ghi trong vai chu ky dau. */
    pins.Pin = MOTOR_LEFT_PWM_PIN | MOTOR_RIGHT_PWM_PIN;
    pins.Mode = GPIO_MODE_AF_PP;
    pins.Pull = GPIO_NOPULL;
    pins.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(MOTOR_PWM_PORT, &pins);

    if (HAL_TIM_PWM_Start(&motor_timer, TIM_CHANNEL_1) != HAL_OK ||
        HAL_TIM_PWM_Start(&motor_timer, TIM_CHANNEL_2) != HAL_OK) {
        return STATUS_ERROR;
    }

    initialized = true;
    return STATUS_OK;
}

status_t pwm_write(uint8_t channel, uint16_t duty_per_mille)
{
    uint32_t compare;

    if ((channel != MOTOR_LEFT_PWM_CHANNEL && channel != MOTOR_RIGHT_PWM_CHANNEL) ||
        duty_per_mille > 1000U) {
        return STATUS_ERROR;
    }
    if (!initialized) { return STATUS_NOT_READY; }

    /* Nhan truoc chia sau, toan bo trong uint32_t: 1000 * 65536 van gon trong
     * 32 bit nen khong mat do chinh xac va khong tran. */
    compare = ((uint32_t)duty_per_mille * period_ticks) / 1000U;
    __HAL_TIM_SET_COMPARE(&motor_timer, hal_channel(channel), compare);
    return STATUS_OK;
}

/* Dua ca hai kenh ve duty 0 — o PWM mode 1 nghia la dau ra giu muc thap suot
 * chu ky. KHONG dung timer: giu no chay thi lenh chay lai co hieu luc ngay o
 * chu ky sau, va quan trong hon la dau ra khong roi vao trang thai khong xac
 * dinh trong luc timer dang tat.
 *
 * Luu y: duty 0 mot minh KHONG lam motor dung han theo nghia dien. Voi TB6612,
 * dung hay khong con phu thuoc IN1/IN2 va STBY — do la viec cua motor_tb6612.
 */
status_t pwm_stop(void)
{
    if (!initialized) { return STATUS_NOT_READY; }
    __HAL_TIM_SET_COMPARE(&motor_timer, TIM_CHANNEL_1, 0U);
    __HAL_TIM_SET_COMPARE(&motor_timer, TIM_CHANNEL_2, 0U);
    return STATUS_OK;
}
