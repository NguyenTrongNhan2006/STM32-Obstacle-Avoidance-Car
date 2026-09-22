#include "timebase.h"
#include "board_config.h"
#include "stm32f1xx_hal.h"

/* timebase la chu so huu duy nhat cua TIM2 (xem bang owners trong
 * board_config.h). Khong module nao khac duoc cau hinh lai timer nay.
 *
 * CACH DO: PWM Input Mode.
 * TI1 (chan PA0) duoc noi noi bo toi CA HAI kenh capture:
 *   IC1 — canh len, dong thoi la trigger cua slave mode Reset -> bo dem ve 0
 *   IC2 — canh xuong -> CCR2 chua san do rong xung, tinh bang tick 1 us
 * Phan cung do va chot gia tri, nen do tre cua ISR hay jitter cua scheduler
 * KHONG the lam sai phep do. Day la ly do chon PWM Input Mode thay vi EXTI:
 * EXTI chi bao co canh, thoi diem doc bo dem lai phu thuoc luc ISR chay.
 *
 * CANH BAO PHAN CUNG: PA0 KHONG phai chan 5V-tolerant. Echo cua HC-SR04 xuat
 * 5V nen BAT BUOC ha ap ngoai (chia ap 1k/2k hoac level shifter) truoc khi cam.
 */

_Static_assert(RANGE_CAPTURE_CHANNEL == 1U,
               "PWM Input Mode gan TI1 vao ca IC1 va IC2; chan vat ly phai la CH1");

/* Bo dem 16 bit chay o 1 us/tick -> tran sau 65_536 us. Day la gioi han cung
 * cua mot phep do, doc lap voi deadline mem theo ms ben duoi. */
#define CAPTURE_PERIOD_TICKS 0xFFFFU

/* Trang thai mot phep do.
 * Chi nguoi goi (tSensor) chuyen sang ARMED va tu ARMED ve IDLE.
 * Chi ISR chuyen ARMED -> DONE hoac ARMED -> OVERRUN.
 * Nho phan vai nhu vay, khong can critical section khi doc.
 */
typedef enum {
    CAPTURE_IDLE = 0,   /* chua arm, hoac ket qua da duoc lay */
    CAPTURE_ARMED,      /* dang cho canh xuong */
    CAPTURE_DONE,       /* CCR2 da co do rong xung hop le */
    CAPTURE_OVERRUN     /* bo dem tran truoc khi co canh xuong */
} capture_state_t;

static TIM_HandleTypeDef capture_timer;
static bool initialized;
static uint32_t armed_ms;
static volatile capture_state_t capture_state = CAPTURE_IDLE;
static volatile uint32_t capture_width_us;

status_t timebase_init(const timebase_config_t *config)
{
    GPIO_InitTypeDef pins = {0};
    TIM_IC_InitTypeDef channel = {0};
    TIM_SlaveConfigTypeDef slave = {0};
    uint32_t timer_hz;
    uint32_t prescaler;

    if (config == NULL || config->capture_tick_hz == 0U) { return STATUS_ERROR; }

    /* TIM2 nam tren APB1. RM0008: khi APB1 prescaler khac 1, clock cap cho
     * timer duoc NHAN DOI so voi PCLK1. Voi SYSCLK 72 MHz -> PCLK1 36 MHz ->
     * clock timer 72 MHz. Tinh tu thanh ghi thay vi hard-code de doi clock
     * tree se lo ra ngay o day chu khong lam sai am tham phep do. */
    timer_hz = HAL_RCC_GetPCLK1Freq();
    if ((RCC->CFGR & RCC_CFGR_PPRE1) != RCC_CFGR_PPRE1_DIV1) { timer_hz *= 2U; }
    if ((timer_hz % config->capture_tick_hz) != 0U) { return STATUS_ERROR; }
    prescaler = timer_hz / config->capture_tick_hz;
    if (prescaler == 0U || prescaler > 0x10000U) { return STATUS_ERROR; }

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_TIM2_CLK_ENABLE();

    /* Trig (PA1) da do gpio_init() cau hinh. Echo (PA0) thuoc TI1 cua TIM2 nen
     * timebase cau hinh o day — mot chan, mot chu so huu. */
    pins.Pin = RANGE_ECHO_PIN;
    pins.Mode = GPIO_MODE_INPUT;
    pins.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(RANGE_ECHO_PORT, &pins);

    capture_timer.Instance = RANGE_CAPTURE_TIMER;
    capture_timer.Init.Prescaler = prescaler - 1U;
    capture_timer.Init.CounterMode = TIM_COUNTERMODE_UP;
    capture_timer.Init.Period = CAPTURE_PERIOD_TICKS;
    capture_timer.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    capture_timer.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_IC_Init(&capture_timer) != HAL_OK) { return STATUS_ERROR; }

    channel.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
    channel.ICSelection = TIM_ICSELECTION_DIRECTTI;     /* IC1 <- TI1 */
    channel.ICPrescaler = TIM_ICPSC_DIV1;
    channel.ICFilter = 0U;
    if (HAL_TIM_IC_ConfigChannel(&capture_timer, &channel, TIM_CHANNEL_1) != HAL_OK) {
        return STATUS_ERROR;
    }

    channel.ICPolarity = TIM_INPUTCHANNELPOLARITY_FALLING;
    channel.ICSelection = TIM_ICSELECTION_INDIRECTTI;   /* IC2 <- cung TI1 */
    if (HAL_TIM_IC_ConfigChannel(&capture_timer, &channel, TIM_CHANNEL_2) != HAL_OK) {
        return STATUS_ERROR;
    }

    /* Canh len cua Echo reset bo dem ve 0, nen CCR2 doc duoc chinh la do rong
     * xung chu khong phai mot moc thoi gian tuyet doi can tru di. */
    slave.SlaveMode = TIM_SLAVEMODE_RESET;
    slave.InputTrigger = TIM_TS_TI1FP1;
    slave.TriggerPolarity = TIM_TRIGGERPOLARITY_RISING;
    slave.TriggerPrescaler = TIM_TRIGGERPRESCALER_DIV1;
    slave.TriggerFilter = 0U;
    if (HAL_TIM_SlaveConfigSynchro(&capture_timer, &slave) != HAL_OK) { return STATUS_ERROR; }

    /* URS = 1: CHI tran bo dem moi sinh ngat update.
     * BAT BUOC phai co. Slave mode Reset cung sinh update event moi lan no reset
     * bo dem — tuc moi canh len cua Echo. Khong dat URS thi ISR se hieu nham
     * moi canh len thanh mot lan tran bo dem va bao OVERRUN cho phep do dang
     * chay. Loi nay khong lam treo may, no chi lam moi phep do deu that bai. */
    RANGE_CAPTURE_TIMER->CR1 |= TIM_CR1_URS;

    if (HAL_TIM_IC_Start(&capture_timer, TIM_CHANNEL_1) != HAL_OK) { return STATUS_ERROR; }
    if (HAL_TIM_IC_Start_IT(&capture_timer, TIM_CHANNEL_2) != HAL_OK) { return STATUS_ERROR; }
    __HAL_TIM_ENABLE_IT(&capture_timer, TIM_IT_UPDATE);

    /* IRQ_PRIO_ECHO = 5 >= configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY.
     * ISR duoi day khong goi API FreeRTOS nao, nhung van giu trong dai hop le
     * de sau nay them notify FromISR khong phai chinh lai bang priority. */
    HAL_NVIC_SetPriority(TIM2_IRQn, IRQ_PRIO_ECHO, 0U);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);

    capture_state = CAPTURE_IDLE;
    initialized = true;
    return STATUS_OK;
}

uint32_t timebase_now_ms(void) { return HAL_GetTick(); }

status_t timebase_capture_arm(void)
{
    if (!initialized) { return STATUS_NOT_READY; }

    /* Tat rieng hai nguon ngat cua TIM2 thay vi __disable_irq(): chi chan dung
     * ISR co the ghi vao capture_state, khong lam tre bat ky ngat nao khac. */
    __HAL_TIM_DISABLE_IT(&capture_timer, TIM_IT_CC2 | TIM_IT_UPDATE);

    capture_state = CAPTURE_ARMED;
    capture_width_us = 0U;
    armed_ms = HAL_GetTick();

    __HAL_TIM_CLEAR_FLAG(&capture_timer,
                         TIM_FLAG_CC1 | TIM_FLAG_CC2 | TIM_FLAG_CC2OF | TIM_FLAG_UPDATE);
    __HAL_TIM_SET_COUNTER(&capture_timer, 0U);   /* ghi CNT khong sinh update event */

    __HAL_TIM_ENABLE_IT(&capture_timer, TIM_IT_CC2 | TIM_IT_UPDATE);
    return STATUS_OK;
}

/* Khong block. Moi lan goi chi bao cao trang thai hien tai:
 *   STATUS_OK        — *pulse_us chua do rong xung do duoc
 *   STATUS_TIMEOUT   — het han hoac bo dem tran ma khong co canh xuong
 *   STATUS_NOT_READY — dang do, chua co ket qua (hoac chua arm)
 * Nguoi goi phai truyen cung mot timeout_us qua cac lan goi cua mot phep do.
 */
status_t timebase_capture_us(uint32_t *pulse_us, uint32_t timeout_us)
{
    uint32_t timeout_ms;

    if (pulse_us == NULL || timeout_us == 0U) { return STATUS_ERROR; }
    *pulse_us = 0U;
    if (!initialized) { return STATUS_NOT_READY; }

    switch (capture_state) {
    case CAPTURE_DONE:
        *pulse_us = capture_width_us;
        capture_state = CAPTURE_IDLE;
        return STATUS_OK;

    case CAPTURE_OVERRUN:
        capture_state = CAPTURE_IDLE;
        return STATUS_TIMEOUT;

    case CAPTURE_ARMED:
        /* Deadline mem, do phan giai 1 ms cua HAL tick — lam tron len de khong
         * bao timeout som hon yeu cau. Deadline cung la lan tran bo dem o
         * 65_536 us, do ISR phat hien. Hai lop nay doc lap nhau: mat canh
         * xuong thi lop cung bat, treo ca ISR thi lop mem van bat. */
        timeout_ms = (timeout_us + 999U) / 1000U;
        if ((uint32_t)(HAL_GetTick() - armed_ms) >= timeout_ms) {
            capture_state = CAPTURE_IDLE;
            return STATUS_TIMEOUT;
        }
        return STATUS_NOT_READY;

    case CAPTURE_IDLE:
    default:
        return STATUS_NOT_READY;
    }
}

void timebase_irq_capture(void)
{
    TIM_TypeDef *const timer = RANGE_CAPTURE_TIMER;
    const uint32_t flags = timer->SR;

    /* Doc thanh ghi truc tiep thay vi goi HAL_TIM_IRQHandler: ISR nay chi chot
     * mot gia tri va dat mot co. Khong thuat toan, khong cho bus, khong log,
     * khong goi API RTOS. Bit trong TIM_SR la rc_w0 nen ghi ~FLAG chi xoa dung
     * co do va giu nguyen cac co khac.
     */
    if ((flags & TIM_SR_UIF) != 0U) {
        timer->SR = ~TIM_SR_UIF;
        /* Nho URS = 1, UIF chi den tu tran bo dem that: xung Echo dai hon
         * 65_536 us, tuc khong co canh xuong hop le trong tam do. */
        if (capture_state == CAPTURE_ARMED) { capture_state = CAPTURE_OVERRUN; }
    }

    if ((flags & TIM_SR_CC2IF) != 0U) {
        const uint32_t width_us = timer->CCR2;   /* doc CCR2 tu xoa CC2IF */
        /* Neu ca UIF lan CC2IF cung den trong mot lan doc (canh xuong roi dung
         * luc tran), nhanh tren da dat OVERRUN va gia tri nay bi bo qua. Ngu y
         * nghieng ve phia bao timeout — an toan hon la bao mot khoang cach sai. */
        if (capture_state == CAPTURE_ARMED) {
            capture_width_us = width_us;
            capture_state = CAPTURE_DONE;
        }
    }

    if ((flags & TIM_SR_CC2OF) != 0U) {
        timer->SR = ~TIM_SR_CC2OF;
    }
}
