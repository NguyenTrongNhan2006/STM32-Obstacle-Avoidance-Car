#include "ultrasonic_hcsr04.h"
#include "board_config.h"
#include "stm32f1xx_hal.h"
#include "timebase.h"

/* State machine for asynchronous, non-blocking ultrasonic measurement. */
typedef enum {
    US_IDLE = 0,
    US_WAIT_RISING,
    US_WAIT_FALLING,
    US_READY
} us_state_t;

/* TIM2 Input Capture handle. Timer clock = 72 MHz.
 * Prescaler = 71 -> 1 MHz counter clock (1 tick = 1 microsecond).
 * ARR = 0xFFFF (65535 microseconds max un-wrapped range).
 * PA0 = TIM2_CH1 (direct input, rising edge)
 * PA0 = TIM2_CH2 (indirect input, falling edge)
 */
static TIM_HandleTypeDef htim2;

/* Variables shared between ISR and task context. */
static volatile us_state_t capture_state;
static volatile uint32_t t_rise;
static volatile uint32_t t_fall;
static volatile uint32_t pulse_width_us;
static volatile uint32_t capture_timestamp_ms;

/* Configuration and anti-overlap tracking. */
static uint32_t last_trigger_ms;
static uint32_t timeout_us_cfg;
static uint32_t sample_period_cfg_ms;
static bool initialized;

/* 3-tap median filter to reject acoustic reflection glitches. */
#define FILTER_TAP_SIZE 3U
static uint32_t filter_buf[FILTER_TAP_SIZE];
static uint8_t filter_count;

static uint32_t median_filter_apply(uint32_t raw_val)
{
    filter_buf[filter_count % FILTER_TAP_SIZE] = raw_val;
    if (filter_count < FILTER_TAP_SIZE) {
        filter_count++;
        return raw_val;
    }
    uint32_t a = filter_buf[0];
    uint32_t b = filter_buf[1];
    uint32_t c = filter_buf[2];

    if ((a <= b && b <= c) || (c <= b && b <= a)) { return b; }
    if ((b <= a && a <= c) || (c <= a && a <= b)) { return a; }
    return c;
}

status_t ultrasonic_init(const ultrasonic_config_t *config)
{
    TIM_IC_InitTypeDef ic1 = {0};
    TIM_IC_InitTypeDef ic2 = {0};
    GPIO_InitTypeDef gpio = {0};

    if (config == NULL || config->echo_timeout_us == 0U || config->sample_period_ms == 0U) {
        return STATUS_ERROR;
    }

    initialized = false;
    timeout_us_cfg = config->echo_timeout_us;
    sample_period_cfg_ms = config->sample_period_ms;
    filter_count = 0U;
    capture_state = US_IDLE;

    /* Ensure Trigger pin (PA1) is LOW. */
    HAL_GPIO_WritePin(RANGE_TRIG_PORT, RANGE_TRIG_PIN, GPIO_PIN_RESET);

    /* Configure Echo pin (PA0) as Input Floating. */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    gpio.Pin = RANGE_ECHO_PIN;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(RANGE_ECHO_PORT, &gpio);

    /* Configure TIM2: 1 MHz counter (1 tick = 1 us). */
    __HAL_RCC_TIM2_CLK_ENABLE();
    htim2.Instance = RANGE_CAPTURE_TIMER;
    htim2.Init.Prescaler = 71U; /* 72 MHz / (71 + 1) = 1 MHz */
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 0xFFFFU;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_IC_Init(&htim2) != HAL_OK) {
        return STATUS_ERROR;
    }

    /* Channel 1: Direct TI on PA0, captures RISING edge. */
    ic1.ICPolarity = TIM_ICPOLARITY_RISING;
    ic1.ICSelection = TIM_ICSELECTION_DIRECTTI;
    ic1.ICPrescaler = TIM_ICPSC_DIV1;
    ic1.ICFilter = 0x04U; /* Digital filter: 4 samples to suppress noise spikes */
    if (HAL_TIM_IC_ConfigChannel(&htim2, &ic1, TIM_CHANNEL_1) != HAL_OK) {
        return STATUS_ERROR;
    }

    /* Channel 2: Indirect TI on PA0, captures FALLING edge. */
    ic2.ICPolarity = TIM_ICPOLARITY_FALLING;
    ic2.ICSelection = TIM_ICSELECTION_INDIRECTTI;
    ic2.ICPrescaler = TIM_ICPSC_DIV1;
    ic2.ICFilter = 0x04U;
    if (HAL_TIM_IC_ConfigChannel(&htim2, &ic2, TIM_CHANNEL_2) != HAL_OK) {
        return STATUS_ERROR;
    }

    /* Start both capture channels in hardware. */
    if (HAL_TIM_IC_Start(&htim2, TIM_CHANNEL_1) != HAL_OK) { return STATUS_ERROR; }
    if (HAL_TIM_IC_Start(&htim2, TIM_CHANNEL_2) != HAL_OK) { return STATUS_ERROR; }

    /* Start base timer continuous counter. */
    if (HAL_TIM_Base_Start(&htim2) != HAL_OK) { return STATUS_ERROR; }

    /* Set interrupt priority (CMSIS 5..15) and enable NVIC IRQ. */
    HAL_NVIC_SetPriority(TIM2_IRQn, IRQ_PRIO_ECHO, 0U);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);

    initialized = true;
    return STATUS_OK;
}

status_t ultrasonic_request(void)
{
    uint32_t now;

    if (!initialized) { return STATUS_NOT_READY; }

    now = timebase_now_ms();

    /* Anti-overlap guard: if a measurement is actively in flight, check timeout first. */
    if (capture_state == US_WAIT_RISING || capture_state == US_WAIT_FALLING) {
        uint32_t max_wait_ms = (timeout_us_cfg / 1000U) + 5U;
        if ((uint32_t)(now - last_trigger_ms) < max_wait_ms) {
            /* Active pulse still travelling; do not emit overlapping sound wave. */
            return STATUS_OK;
        }
        /* Previous pulse timed out; disable interrupts before restarting. */
        __HAL_TIM_DISABLE_IT(&htim2, TIM_IT_CC1 | TIM_IT_CC2);
    }

    /* Arm input capture state machine: listen for Rising edge first. */
    __HAL_TIM_DISABLE_IT(&htim2, TIM_IT_CC1 | TIM_IT_CC2);
    __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_CC1 | TIM_FLAG_CC2);
    capture_state = US_WAIT_RISING;
    __HAL_TIM_ENABLE_IT(&htim2, TIM_IT_CC1);

    /* Generate 12 us trigger pulse on PA1 using hardware timer counter (deterministic). */
    HAL_GPIO_WritePin(RANGE_TRIG_PORT, RANGE_TRIG_PIN, GPIO_PIN_SET);
    {
        uint16_t start_tick = (uint16_t)__HAL_TIM_GET_COUNTER(&htim2);
        while ((uint16_t)((uint16_t)__HAL_TIM_GET_COUNTER(&htim2) - start_tick) < 12U) {
            __NOP();
        }
    }
    HAL_GPIO_WritePin(RANGE_TRIG_PORT, RANGE_TRIG_PIN, GPIO_PIN_RESET);

    last_trigger_ms = now;
    return STATUS_OK;
}

void ultrasonic_capture_isr(void)
{
    /* Event 1: Rising edge detected on PA0 (Echo pulse start). */
    if (__HAL_TIM_GET_FLAG(&htim2, TIM_FLAG_CC1) && __HAL_TIM_GET_IT_SOURCE(&htim2, TIM_IT_CC1)) {
        __HAL_TIM_CLEAR_IT(&htim2, TIM_IT_CC1);
        t_rise = HAL_TIM_ReadCapturedValue(&htim2, TIM_CHANNEL_1);
        capture_state = US_WAIT_FALLING;
        /* Enable Falling edge interrupt on Channel 2. */
        __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_CC2);
        __HAL_TIM_ENABLE_IT(&htim2, TIM_IT_CC2);
    }

    /* Event 2: Falling edge detected on PA0 (Echo pulse end). */
    if (__HAL_TIM_GET_FLAG(&htim2, TIM_FLAG_CC2) && __HAL_TIM_GET_IT_SOURCE(&htim2, TIM_IT_CC2)) {
        __HAL_TIM_CLEAR_IT(&htim2, TIM_IT_CC2);
        t_fall = HAL_TIM_ReadCapturedValue(&htim2, TIM_CHANNEL_2);
        if (t_fall >= t_rise) {
            pulse_width_us = t_fall - t_rise;
        } else {
            /* 16-bit counter rollover (ARR = 0xFFFF). */
            pulse_width_us = (0xFFFFU - t_rise) + t_fall + 1U;
        }
        capture_timestamp_ms = timebase_now_ms();
        capture_state = US_READY;
        /* Disable interrupts until next request. */
        __HAL_TIM_DISABLE_IT(&htim2, TIM_IT_CC1 | TIM_IT_CC2);
    }
}

status_t ultrasonic_read(sample_t *sample)
{
    uint32_t now;

    if (sample == NULL) { return STATUS_ERROR; }
    *sample = (sample_t){ .unit = SAMPLE_UNIT_MM, .status = SAMPLE_NOT_READY };

    if (!initialized) { return STATUS_NOT_READY; }

    now = timebase_now_ms();

    /* Path 1: Complete measurement captured by ISR. */
    if (capture_state == US_READY) {
        capture_state = US_IDLE;

        if (pulse_width_us > timeout_us_cfg || pulse_width_us == 0U) {
            sample->value = 0U;
            sample->unit = SAMPLE_UNIT_MM;
            sample->timestamp_ms = (uint32_t)capture_timestamp_ms;
            sample->status = SAMPLE_TIMEOUT;
            return STATUS_TIMEOUT;
        }

        /* Distance conversion: d = (pulse_us * 343 mm/ms) / 2000 mm. */
        uint32_t raw_mm = ((uint32_t)pulse_width_us * 343U + 1000U) / 2000U;
        sample->value = median_filter_apply(raw_mm);
        sample->unit = SAMPLE_UNIT_MM;
        sample->timestamp_ms = (uint32_t)capture_timestamp_ms;
        sample->status = SAMPLE_OK;
        return STATUS_OK;
    }

    /* Path 2: Measurement in flight; check if timeout deadline exceeded. */
    if (capture_state == US_WAIT_RISING || capture_state == US_WAIT_FALLING) {
        uint32_t max_wait_ms = (timeout_us_cfg / 1000U) + 5U;
        if ((uint32_t)(now - last_trigger_ms) > max_wait_ms) {
            __HAL_TIM_DISABLE_IT(&htim2, TIM_IT_CC1 | TIM_IT_CC2);
            capture_state = US_IDLE;
            sample->value = 0U;
            sample->unit = SAMPLE_UNIT_MM;
            sample->timestamp_ms = now;
            sample->status = SAMPLE_TIMEOUT;
            return STATUS_TIMEOUT;
        }
        /* Still waiting for echo return wave. */
        return STATUS_NOT_READY;
    }

    return STATUS_NOT_READY;
}
