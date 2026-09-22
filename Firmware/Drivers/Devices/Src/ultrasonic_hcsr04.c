#include "ultrasonic_hcsr04.h"
#include "board_config.h"
#include "gpio.h"
#include "timebase.h"

/* [CO DINH] Theo datasheet HC-SR04:
 *   - Trig can mot xung muc cao toi thieu 10 us.
 *   - Toc do am thanh 343 m/s o 20 C  =>  0,343 mm/us.
 *     Song di va ve nen quang duong bang mot nua:
 *         mm = us * 343 / 2000 = us * 1715 / 10000
 *     Dung so nguyen, khong dung so thuc: MCU khong co FPU.
 *   - Pham vi tin cay khoang 20..4000 mm. Ngoai khoang do module tra ket qua
 *     khong on dinh.
 *
 * [DO] Nhan cam bien chua duoc xac nhan (board_config.h ghi "tam goi HC-SR04,
 * can kiem tra nhan HR-04"). Neu la module khac, doi lai ba hang so duoi day
 * theo datasheet that truoc khi tin vao so do.
 */
#define ULTRASONIC_TRIG_PULSE_US 10U
#define ULTRASONIC_MM_NUM        1715U
#define ULTRASONIC_MM_DEN        10000U
#define ULTRASONIC_MIN_VALID_MM  20U
#define ULTRASONIC_MAX_VALID_MM  4000U

static uint32_t echo_timeout_us;
static uint32_t sample_period_ms;
static uint32_t triggered_ms;
static bool measuring;
static bool initialized;

static gpio_pin_t trig_pin(void)
{
    const gpio_pin_t pin = { .port = RANGE_TRIG_PORT, .pin = RANGE_TRIG_PIN };
    return pin;
}

/* Xung 10 us la YEU CAU CUA DATASHEET, khong phai mot lan cho su kien. Do bang
 * chinh bo dem 1 us/tick cua TIM2 thay vi vong lap dem chu ky, nen thoi luong
 * khong doi theo muc toi uu cua trinh bien dich.
 *
 * Day van la busy-wait, nhung co ba dieu lam no khac mot ham delay cua HAL
 * (ten ham do bi cam trong toan bo source, ke ca trong comment — xem
 * tools/check_constraints.sh):
 *   - Do dai co dinh 10 us (720 chu ky @ 72 MHz), khong phu thuoc du lieu.
 *   - Goi moi 60 ms => 0,017% thoi gian CPU.
 *   - Chay trong tSensor (priority 2), nen tSafety (priority 4) van gianh
 *     duoc CPU ngay; tre toi da ma no gay ra cho tSafety la 10 us tren chu ky
 *     10 ms cua task do.
 * Neu sau nay can bo han busy-wait: dung TIM4 (dang con trong) o One-Pulse
 * Mode de phan cung tu phat xung. Do la mot quyet dinh ve quyen so huu timer,
 * phai chot trong board_config.h truoc.
 */
static void trig_pulse(void)
{
    uint16_t start;

    (void)gpio_write(trig_pin(), true);
    start = (uint16_t)RANGE_CAPTURE_TIMER->CNT;
    /* Phep tru 16 bit khong dau nen dung ca khi bo dem tran giua chung. Neu mot
     * canh len lac cua Echo reset bo dem giua luc nay, hieu se lon va vong lap
     * thoat som: xung ngan hon 10 us, cam bien khong kich, va phep do ket thuc
     * bang SAMPLE_TIMEOUT o chu ky sau. Hong ve phia an toan, khong treo. */
    while ((uint16_t)((uint16_t)RANGE_CAPTURE_TIMER->CNT - start) < ULTRASONIC_TRIG_PULSE_US) {
    }
    (void)gpio_write(trig_pin(), false);
}

status_t ultrasonic_init(const ultrasonic_config_t *config)
{
    if (config == NULL || config->echo_timeout_us == 0U || config->sample_period_ms == 0U) {
        return STATUS_ERROR;
    }
    /* Trig (PA1) da duoc gpio_init() dat output push-pull va keo xuong thap.
     * Echo (PA0) do timebase cau hinh vi TIM2 thuoc so huu cua no. */
    initialized = false;
    if (gpio_write(trig_pin(), false) != STATUS_OK) { return STATUS_NOT_READY; }

    echo_timeout_us = config->echo_timeout_us;
    sample_period_ms = config->sample_period_ms;
    triggered_ms = timebase_now_ms();
    measuring = false;
    initialized = true;
    return STATUS_OK;
}

/* Xin mot phep do moi. Tu choi bang STATUS_NOT_READY — khong phai loi — khi
 * con phep do dang chay hoac chua den han retrigger. Nho vay nguoi goi cu goi
 * moi chu ky ma khong pha vo rang buoc thoi gian cua datasheet.
 */
status_t ultrasonic_request(void)
{
    uint32_t now_ms;

    if (!initialized) { return STATUS_NOT_READY; }
    if (measuring) { return STATUS_NOT_READY; }

    now_ms = timebase_now_ms();
    /* Nghi giua hai lan trigger de tieng vang cua lan truoc tat han; kich som
     * hon se do trung vao echo cu va cho ra khoang cach sai. */
    if ((uint32_t)(now_ms - triggered_ms) < sample_period_ms) { return STATUS_NOT_READY; }

    /* Arm TRUOC khi kich: echo co the ve sau ~450 us, nhung arm truoc thi khong
     * co cua so nao de lot mat canh len. */
    if (timebase_capture_arm() != STATUS_OK) { return STATUS_NOT_READY; }

    triggered_ms = now_ms;
    measuring = true;
    trig_pulse();
    return STATUS_OK;
}

/* STATUS_OK nghia la DA CO KET LUAN cho phep do, khong phai la do duoc:
 * sample->status moi noi ket qua hop le hay that bai. STATUS_NOT_READY nghia
 * la phep do con dang chay hoac chua co phep do nao.
 */
status_t ultrasonic_read(sample_t *sample)
{
    uint32_t pulse_us = 0U;
    status_t capture;

    if (sample == NULL) { return STATUS_ERROR; }
    *sample = (sample_t){ .unit = SAMPLE_UNIT_MM, .status = SAMPLE_NOT_READY };
    if (!initialized || !measuring) { return STATUS_NOT_READY; }

    capture = timebase_capture_us(&pulse_us, echo_timeout_us);
    if (capture == STATUS_NOT_READY) { return STATUS_NOT_READY; }

    measuring = false;
    /* Thoi diem chup mau = luc phat Trig, khong phai luc doc mailbox. Moc nay
     * som hon thoi diem echo ve, nen mau luon duoc coi la GIA hon thuc te mot
     * chut — lech ve phia an toan cho moi phep kiem tra freshness. */
    sample->timestamp_ms = triggered_ms;
    sample->value = 0U;

    if (capture == STATUS_TIMEOUT) {
        /* Mat echo. KHONG duoc bien thanh 0 mm hay mot khoang cach rat xa roi
         * dung nhu phep do hop le: thieu cam bien khong dong nghia duong trong. */
        sample->status = SAMPLE_TIMEOUT;
        return STATUS_OK;
    }
    if (capture != STATUS_OK) {
        sample->status = SAMPLE_ERROR;
        return STATUS_OK;
    }

    /* pulse_us toi da la 65_535 (bo dem 16 bit), nen 65_535 * 1715 = 112_392_525
     * van nam gon trong uint32_t. */
    sample->value = (pulse_us * ULTRASONIC_MM_NUM) / ULTRASONIC_MM_DEN;
    if (sample->value < ULTRASONIC_MIN_VALID_MM || sample->value > ULTRASONIC_MAX_VALID_MM) {
        /* Ngoai pham vi tin cay -> danh dau loi. Duoi 20 mm module tra so nhay
         * lung tung; bao SAMPLE_OK o day se cho FSM mot con so no khong duoc
         * phep tin. safety_monitor coi moi status khac SAMPLE_OK la fault, tuc
         * xe dung — do la huong hong dung. */
        sample->status = SAMPLE_ERROR;
    } else {
        sample->status = SAMPLE_OK;
    }
    return STATUS_OK;
}
