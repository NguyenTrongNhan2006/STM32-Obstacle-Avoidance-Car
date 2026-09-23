#include "motor_tb6612.h"
#include "app_config.h"
#include "board_config.h"
#include "gpio.h"
#include "pwm.h"
#include "timebase.h"

/* ============================================================================
 * BANG CHAN LY TB6612FNG — mot kenh (A hoac B)
 * ----------------------------------------------------------------------------
 *  STBY  IN1  IN2  PWM     Che do
 *  ----  ---  ---  ---     ------------------------------------------------
 *   L     x    x    x      Standby: ca hai dau ra HIGH-Z, motor tu quay theo
 *                          quan tinh. Day la trang thai sau reset va la duong
 *                          cat khan cap.
 *   H     H    H    x      Short brake: noi tat hai dau motor -> ham dien tu.
 *   H     H    L    H      Quay chieu thuan, toc do theo duty
 *   H     H    L    L      Short brake (PWM thap tu ham trong chu ky)
 *   H     L    H    H      Quay chieu nguoc, toc do theo duty
 *   H     L    H    L      Short brake
 *   H     L    L    x      Stop: HIGH-Z, motor tu quay theo quan tinh
 *
 * DIEU CAN NHO: o che do quay, nua chu ky PWM thap la SHORT BRAKE chu khong
 * phai coast. Do la ly do 20 kHz — duoi nguong nghe va du nhanh de dong co
 * khong bi giat theo tung chu ky ham.
 *
 * Chieu "thuan" phu thuoc cach noi day motor. Neu mot banh quay nguoc so voi
 * mong doi, DAO HAI DAY cua banh do; khong sua thuat toan ne vat can va cung
 * khong them co dao chieu trong file nay.
 * ============================================================================ */

typedef enum {
    WHEEL_COAST = 0,   /* IN1=L IN2=L, high-Z */
    WHEEL_BRAKE,       /* IN1=H IN2=H, ham dien tu */
    WHEEL_FORWARD,     /* IN1=H IN2=L */
    WHEEL_REVERSE      /* IN1=L IN2=H */
} wheel_dir_t;

static bool (*safety_check)(void);
static bool initialized;
static wheel_dir_t applied_left = WHEEL_COAST;
static wheel_dir_t applied_right = WHEEL_COAST;
static bool brake_window_active;
static uint32_t brake_started_ms;
static uint16_t current_duty_per_mille = 0U;
static uint32_t last_ramp_ms = 0U;

static gpio_pin_t pin_of(GPIO_TypeDef *port, uint16_t pin)
{
    const gpio_pin_t handle = { .port = port, .pin = pin };
    return handle;
}

/* Bo loc doc PWM (Slew-rate Limiter): gioi han toc do bien thien d(duty)/dt
 * de khoi dong mem, triet tieu xung dong gay sut ap pin (Brown-out) va chong truot banh.
 */
static uint16_t apply_slew_rate(uint16_t target_duty, uint32_t now_ms)
{
    uint32_t dt_ms;
    uint32_t max_delta;

    if (last_ramp_ms == 0U) {
        last_ramp_ms = now_ms;
    }
    dt_ms = now_ms - last_ramp_ms;
    last_ramp_ms = now_ms;
    if (dt_ms > 100U) { dt_ms = 100U; }

    /* MOTOR_RAMP_RATE_PER_MS (3 phan nghin / ms) */
    max_delta = dt_ms * MOTOR_RAMP_RATE_PER_MS;
    if (max_delta < 10U) { max_delta = 10U; }

    if (target_duty > current_duty_per_mille) {
        if ((uint32_t)(target_duty - current_duty_per_mille) > max_delta) {
            current_duty_per_mille += (uint16_t)max_delta;
        } else {
            current_duty_per_mille = target_duty;
        }
    } else {
        /* Giam toc nhanh hon de dam bao cu ly phanh an toan */
        uint32_t max_decel = max_delta * 2U;
        if ((uint32_t)(current_duty_per_mille - target_duty) > max_decel) {
            current_duty_per_mille -= (uint16_t)max_decel;
        } else {
            current_duty_per_mille = target_duty;
        }
    }
    return current_duty_per_mille;
}

static void set_wheel(wheel_dir_t direction, gpio_pin_t in1, gpio_pin_t in2,
                      uint8_t channel, uint16_t duty_per_mille)
{
    bool level_in1;
    bool level_in2;
    uint16_t duty = duty_per_mille;

    switch (direction) {
    case WHEEL_FORWARD: level_in1 = true;  level_in2 = false; break;
    case WHEEL_REVERSE: level_in1 = false; level_in2 = true;  break;
    case WHEEL_BRAKE:   level_in1 = true;  level_in2 = true;  duty = 0U; break;
    case WHEEL_COAST:
    default:            level_in1 = false; level_in2 = false; duty = 0U; break;
    }
    (void)pwm_write(channel, duty);
    (void)gpio_write(in1, level_in1);
    (void)gpio_write(in2, level_in2);
}

/* Trang thai an toan. Thu tu quan trong:
 *   1. Ha STBY TRUOC — day la duong cat nhanh nhat va khong phu thuoc gi khac.
 *   2. Roi moi don dep duty va cac chan huong.
 * Lam nguoc lai se co mot khoang ngan bridge van bat trong luc cac chan huong
 * dang doi trang thai.
 */
static void enter_safe_state(void)
{
    gpio_emergency_stop();
    (void)pwm_stop();
    (void)gpio_write(pin_of(MOTOR_AIN1_PORT, MOTOR_AIN1_PIN), false);
    (void)gpio_write(pin_of(MOTOR_AIN2_PORT, MOTOR_AIN2_PIN), false);
    (void)gpio_write(pin_of(MOTOR_BIN1_PORT, MOTOR_BIN1_PIN), false);
    (void)gpio_write(pin_of(MOTOR_BIN2_PORT, MOTOR_BIN2_PIN), false);
    applied_left = WHEEL_COAST;
    applied_right = WHEEL_COAST;
    brake_window_active = false;
    current_duty_per_mille = 0U;
    last_ramp_ms = 0U;
}

/* Dat chieu va duty cho ca hai banh roi MOI bat bridge. Nguoc lai se co mot
 * khoang ngan motor chay theo chieu cu.
 */
static void drive(wheel_dir_t left, wheel_dir_t right, uint16_t duty_per_mille)
{
    set_wheel(left, pin_of(MOTOR_AIN1_PORT, MOTOR_AIN1_PIN),
              pin_of(MOTOR_AIN2_PORT, MOTOR_AIN2_PIN),
              MOTOR_LEFT_PWM_CHANNEL, duty_per_mille);
    set_wheel(right, pin_of(MOTOR_BIN1_PORT, MOTOR_BIN1_PIN),
              pin_of(MOTOR_BIN2_PORT, MOTOR_BIN2_PIN),
              MOTOR_RIGHT_PWM_CHANNEL, duty_per_mille);
    (void)gpio_write(pin_of(MOTOR_STBY_PORT, MOTOR_STBY_PIN), true);
}

static void decode(motor_cmd_t command, wheel_dir_t *left, wheel_dir_t *right)
{
    switch (command) {
    case MOTOR_FORWARD:    *left = WHEEL_FORWARD; *right = WHEEL_FORWARD; break;
    case MOTOR_BACKWARD:   *left = WHEEL_REVERSE; *right = WHEEL_REVERSE; break;
    case MOTOR_TURN_LEFT:  *left = WHEEL_REVERSE; *right = WHEEL_FORWARD; break;
    case MOTOR_TURN_RIGHT: *left = WHEEL_FORWARD; *right = WHEEL_REVERSE; break;
    case MOTOR_COAST:      *left = WHEEL_COAST;   *right = WHEEL_COAST;   break;
    case MOTOR_STOP:
    case MOTOR_BRAKE:
    default:
        /* STOP va BRAKE cho ra cung mot to hop chan. Giu ca hai ten vi chung
         * khac nhau ve Y DINH: STOP la "khong di nua", BRAKE la "ham lai".
         * Chon short brake chu khong phai coast cho STOP de xe khong troi tiep
         * sau khi FSM da quyet dinh dung. */
        *left = WHEEL_BRAKE;
        *right = WHEEL_BRAKE;
        break;
    }
}

static bool reverses(wheel_dir_t from, wheel_dir_t to)
{
    return (from == WHEEL_FORWARD && to == WHEEL_REVERSE) ||
           (from == WHEEL_REVERSE && to == WHEEL_FORWARD);
}

status_t motor_init(const motor_config_t *config)
{
    const pwm_config_t pwm_settings = { .frequency_hz = MOTOR_PWM_HZ };

    gpio_emergency_stop();
    safety_check = NULL;
    initialized = false;
    if (config == NULL || config->is_safe == NULL) { return STATUS_ERROR; }

    /* pwm_init() dat ca hai CCR ve 0 truoc khi giao chan cho timer, nen bridge
     * khong bao gio thay mot duty rac trong luc khoi tao. STBY van dang thap. */
    if (pwm_init(&pwm_settings) != STATUS_OK) { return STATUS_NOT_READY; }

    safety_check = config->is_safe;
    initialized = true;
    enter_safe_state();
    return STATUS_OK;
}

/* Ranh gioi lenh motor duy nhat. robot_car la nguoi goi binh thuong duy nhat.
 *
 * MOI duong thoat som deu di qua enter_safe_state(): khong co nhanh nao roi ra
 * khoi ham nay ma de bridge bat trong khi dieu kien chua du. Predicate an toan
 * duoc kiem tra LAI o day chu khong tin vao viec App da kiem tra truoc do.
 */
status_t motor_apply(motor_cmd_t command, uint8_t speed)
{
    wheel_dir_t left;
    wheel_dir_t right;
    uint32_t now_ms;

    if ((unsigned)command > MOTOR_TURN_RIGHT || speed > 100U) {
        enter_safe_state();
        return STATUS_ERROR;
    }
    if (!initialized) {
        enter_safe_state();
        return STATUS_NOT_READY;
    }
    /* Fault-dominant: bat ky ly do nao khien safety khong clear deu ep ve trang
     * thai an toan, bat ke FSM dang yeu cau gi. */
    if (safety_check == NULL || !safety_check()) {
        enter_safe_state();
        return STATUS_NOT_READY;
    }

    decode(command, &left, &right);
    now_ms = timebase_now_ms();

    /* Cua so short brake chen giua hai chieu nguoc nhau. Khong dung vTaskDelay
     * o day: ham nay phai tra ve ngay de tSafety va chinh tDecision con chay
     * duoc. Dung so sanh deadline, giong cach FSM do thoi luong mot state. */
    if (brake_window_active) {
        if ((uint32_t)(now_ms - brake_started_ms) < MOTOR_DIRECTION_BRAKE_MS) {
            drive(WHEEL_BRAKE, WHEEL_BRAKE, 0U);
            return STATUS_OK;
        }
        brake_window_active = false;
    }

    if (reverses(applied_left, left) || reverses(applied_right, right)) {
        /* Dao chieu truc tiep bi cam: mo tay doi dien trong khi tay hien tai
         * chua tat het dong se gay xung dong lon qua cau H, va momen nguoc dap
         * vao banh dang quay la mot cu soc co khi. Chen brake roi thu lai o chu
         * ky sau — lenh KHONG bi mat, no chi den muon MOTOR_DIRECTION_BRAKE_MS. */
        brake_window_active = true;
        brake_started_ms = now_ms;
        drive(WHEEL_BRAKE, WHEEL_BRAKE, 0U);
        applied_left = WHEEL_BRAKE;
        applied_right = WHEEL_BRAKE;
        return STATUS_OK;
    }

    /* speed la phan tram 0..100, pwm_write nhan phan nghin 0..1000.
     * Qua bo loc doc (PWM slew-rate limiter) de khoi dong mem, chong sut ap pin va truot banh. */
    {
        uint16_t target_duty = (uint16_t)((uint32_t)speed * 10U);
        uint16_t ramped_duty = apply_slew_rate(target_duty, now_ms);
        drive(left, right, ramped_duty);
    }
    applied_left = left;
    applied_right = right;
    return STATUS_OK;
}
