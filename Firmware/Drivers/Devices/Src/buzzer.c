#include "buzzer.h"
#include "app_config.h"
#include "board_config.h"
#include "gpio.h"

/* ============================================================================
 * BUZZER ACTIVE — bat/tat bang GPIO, KHONG phat duoc cao do
 * ----------------------------------------------------------------------------
 * board_config.h ghi "[DO] active buzzer + suitable transistor stage". Buzzer
 * active tu sinh dao dong ben trong, firmware chi dong/ngat nguon cho no. Nghia
 * la CHON CAO DO LA VIEC KHONG LAM DUOC o day — mau chi khac nhau o nhip, do
 * dai tieng va khoang lang.
 *
 * Muon phat tone that thi phai doi sang buzzer passive va cap mot timer PWM.
 * TIM2 dang do echo, TIM3 dang chay motor, nen ung vien duy nhat la TIM4 (con
 * trong). Do la mot quyet dinh ve quyen so huu timer, phai chot trong
 * board_config.h truoc khi viet code.
 *
 * DO PHAN GIAI: buzzer_update() duoc goi tu tBuzzer o nhip TASK_BUZZER_PERIOD_MS
 * = 100 ms. Moi buoc trong mau BAT BUOC la boi cua 100 ms — dat 60 ms thi no
 * van keo dai 100 ms va ca mau bi lech. Co _Static_assert ben duoi giu luat nay.
 * ============================================================================ */

typedef struct {
    bool on;
    uint16_t duration_ms;
} buzzer_step_t;

/* BUZZER_START duoc dung lam tin hieu "xe dang song va dang chay": mot tieng
 * ngan moi 2 giay. Day la canh bao cho nguoi xung quanh, khong phai hieu ung. */
static const buzzer_step_t pattern_start[] = {
    { true, 100U }, { false, 1900U }
};
/* Gap vat can: hai tieng ngan roi nghi. */
static const buzzer_step_t pattern_obstacle[] = {
    { true, 100U }, { false, 100U }, { true, 100U }, { false, 700U }
};
/* Fault: nhip deu, nhanh, khong dut — de phan biet ro voi hai mau tren. */
static const buzzer_step_t pattern_fault[] = {
    { true, 200U }, { false, 200U }
};

_Static_assert(TASK_BUZZER_PERIOD_MS == 100U,
               "Moi buoc trong cac mau coi la boi cua 100 ms; doi chu ky tBuzzer thi phai tinh lai");

static bool drive_active_high;
static bool initialized;
/* Mau duoc yeu cau. tDecision ghi qua buzzer_set(), tBuzzer doc trong
 * buzzer_update(). Mot writer, mot reader, kieu enum vua mot word nen ghi/doc
 * nguyen tu tren Cortex-M3 — khong can mailbox hay khoa cho mot bien quan sat. */
static volatile buzzer_pattern_t requested = BUZZER_SILENT;
/* CAR_STOP co the chi keo dai 20 ms, ngan hon chu ky tBuzzer 100 ms.
 * Giu mot su kien cho den khi task chu so huu GPIO bat dau mau canh bao. */
static volatile bool obstacle_pending;
static buzzer_pattern_t active = BUZZER_SILENT;
static uint32_t step_index;
static uint32_t step_started_ms;

static gpio_pin_t buzzer_pin(void)
{
    const gpio_pin_t pin = { .port = BUZZER_PORT, .pin = BUZZER_PIN };
    return pin;
}

static void drive(bool on)
{
    (void)gpio_write(buzzer_pin(), on == drive_active_high);
}

static const buzzer_step_t *steps_of(buzzer_pattern_t pattern, uint32_t *count)
{
    switch (pattern) {
    case BUZZER_START:    *count = 2U; return pattern_start;
    case BUZZER_OBSTACLE: *count = 4U; return pattern_obstacle;
    case BUZZER_FAULT:    *count = 2U; return pattern_fault;
    case BUZZER_SILENT:
    default:              *count = 0U; return NULL;
    }
}

status_t buzzer_init(const buzzer_config_t *config)
{
    if (config == NULL) { return STATUS_ERROR; }
    initialized = false;
    drive_active_high = config->active_high;
    requested = BUZZER_SILENT;
    obstacle_pending = false;
    active = BUZZER_SILENT;
    step_index = 0U;
    step_started_ms = 0U;
    /* Chan PB12 da duoc gpio_init() dat output push-pull va keo xuong thap. */
    if (gpio_write(buzzer_pin(), !drive_active_high) != STATUS_OK) { return STATUS_NOT_READY; }
    initialized = true;
    return STATUS_OK;
}

status_t buzzer_set(buzzer_pattern_t pattern)
{
    if ((unsigned)pattern > BUZZER_FAULT) { return STATUS_ERROR; }
    if (!initialized) { return STATUS_NOT_READY; }
    /* Chi ghi y dinh. Viec doi mau thuc su xay ra trong buzzer_update(), tren
     * task so huu chan — nguoi goi khong bao gio cham vao GPIO tu day. */
    if (pattern == BUZZER_OBSTACLE && requested != BUZZER_OBSTACLE) {
        obstacle_pending = true;
    }
    if (pattern == BUZZER_FAULT || pattern == BUZZER_SILENT) {
        obstacle_pending = false;
    }
    requested = pattern;
    return STATUS_OK;
}

/* Non-blocking tuyet doi: mot lan goi chi day mau tien mot buoc neu da den han
 * roi tra ve. KHONG bao gio cho het thoi luong mot tieng bip — tBuzzer chay o
 * priority 0 nen mot vong cho o day se doi CPU voi Idle task.
 */
status_t buzzer_update(uint32_t now_ms)
{
    const buzzer_step_t *steps;
    uint32_t count = 0U;

    if (!initialized) { return STATUS_NOT_READY; }

    /* FAULT va SILENT luon uu tien. Mau OBSTACLE da bat dau phai phat het mot
     * lan, ke ca FSM chuyen sang TURN truoc khi tBuzzer duoc lap lich. */
    if (((requested == BUZZER_FAULT || requested == BUZZER_SILENT) &&
         requested != active) ||
        (active != BUZZER_OBSTACLE &&
         (obstacle_pending || requested != active))) {
        active = obstacle_pending ? BUZZER_OBSTACLE : requested;
        obstacle_pending = false;
        step_index = 0U;
        step_started_ms = now_ms;
        steps = steps_of(active, &count);
        drive((count > 0U) && steps[0].on);
        return STATUS_OK;
    }

    steps = steps_of(active, &count);
    if (count == 0U) {
        drive(false);
        return STATUS_OK;
    }
    if ((uint32_t)(now_ms - step_started_ms) < steps[step_index].duration_ms) {
        return STATUS_OK;
    }

    if (active == BUZZER_OBSTACLE && step_index + 1U == count) {
        /* Canh bao da phat het. Ve mau hien tai cua FSM o nhip ke tiep. */
        active = obstacle_pending ? BUZZER_OBSTACLE : requested;
        obstacle_pending = false;
        step_index = 0U;
        step_started_ms = now_ms;
        steps = steps_of(active, &count);
        drive((count > 0U) && steps[0].on);
        return STATUS_OK;
    }

    /* Cong them thoi luong da dinh thay vi gan bang now_ms: sai so cua tung
     * buoc khong cong don qua ca mau. */
    step_started_ms += steps[step_index].duration_ms;
    step_index = (step_index + 1U) % count;
    drive(steps[step_index].on);
    return STATUS_OK;
}
