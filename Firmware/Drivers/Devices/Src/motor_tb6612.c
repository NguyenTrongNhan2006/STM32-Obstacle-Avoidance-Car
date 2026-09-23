#include "motor_tb6612.h"
#include "board_config.h"
#include "gpio.h"
#include "pwm.h"

static bool (*safety_check)(void);
static bool pwm_ready;

/* TB6612FNG truth table (per channel):
 *   STBY=0           -> standby, outputs Hi-Z
 *   IN1=H IN2=L PWM  -> CW  (forward)
 *   IN1=L IN2=H PWM  -> CCW (reverse)
 *   IN1=L IN2=L      -> coast (high impedance)
 *   IN1=H IN2=H      -> short brake
 *
 * Motor A (left):  AIN1=PB0, AIN2=PB1, PWM=TIM3_CH1 (PA6)
 * Motor B (right): BIN1=PB10, BIN2=PB11, PWM=TIM3_CH2 (PA7)
 */

/* Compact pin descriptors for direction lines. */
static const gpio_pin_t ain1 = { .port = MOTOR_AIN1_PORT, .pin = MOTOR_AIN1_PIN };
static const gpio_pin_t ain2 = { .port = MOTOR_AIN2_PORT, .pin = MOTOR_AIN2_PIN };
static const gpio_pin_t bin1 = { .port = MOTOR_BIN1_PORT, .pin = MOTOR_BIN1_PIN };
static const gpio_pin_t bin2 = { .port = MOTOR_BIN2_PORT, .pin = MOTOR_BIN2_PIN };
static const gpio_pin_t stby = { .port = MOTOR_STBY_PORT, .pin = MOTOR_STBY_PIN };

/* Set a single motor's direction pins.  fwd=true -> IN1=H IN2=L (CW). */
static void set_direction(const gpio_pin_t *in1, const gpio_pin_t *in2, bool fwd)
{
    (void)gpio_write(*in1, fwd);
    (void)gpio_write(*in2, !fwd);
}

/* Coast both motors: all direction pins low. */
static void coast_all(void)
{
    (void)gpio_write(ain1, false);
    (void)gpio_write(ain2, false);
    (void)gpio_write(bin1, false);
    (void)gpio_write(bin2, false);
}

status_t motor_init(const motor_config_t *config)
{
    const pwm_config_t pwm_cfg = { .frequency_hz = MOTOR_PWM_HZ };

    gpio_emergency_stop();         /* STBY low first — fault-dominant */
    safety_check = NULL;
    pwm_ready = false;

    if (config == NULL || config->is_safe == NULL) { return STATUS_ERROR; }
    safety_check = config->is_safe;

    coast_all();                   /* direction pins low while STBY is low */

    if (pwm_init(&pwm_cfg) != STATUS_OK) { return STATUS_NOT_READY; }
    pwm_ready = true;

    /* PWM outputs are running at zero duty; STBY stays low.
     * motor_apply will raise STBY only when safety clears and a
     * motion command arrives — never here.
     */
    return STATUS_OK;
}

status_t motor_apply(motor_cmd_t command, uint8_t speed)
{
    uint16_t duty;

    /* ---- Fault-dominant entry: always disable bridge first ---- */
    gpio_emergency_stop();

    if ((unsigned)command > MOTOR_TURN_RIGHT || speed > 100U) { return STATUS_ERROR; }
    if (safety_check == NULL || !safety_check())              { return STATUS_NOT_READY; }
    if (!pwm_ready)                                           { return STATUS_NOT_READY; }

    /* Map 0..100 percent to 0..1000 per-mille for the PWM driver. */
    duty = (uint16_t)speed * 10U;

    switch (command) {
    case MOTOR_STOP:
    case MOTOR_COAST:
        coast_all();
        (void)pwm_write(MOTOR_LEFT_PWM_CHANNEL, 0U);
        (void)pwm_write(MOTOR_RIGHT_PWM_CHANNEL, 0U);
        /* STBY stays low from emergency_stop above — intentional. */
        return STATUS_OK;

    case MOTOR_BRAKE:
        /* Short brake: IN1=H, IN2=H on both channels, STBY high. */
        (void)gpio_write(ain1, true);
        (void)gpio_write(ain2, true);
        (void)gpio_write(bin1, true);
        (void)gpio_write(bin2, true);
        (void)pwm_write(MOTOR_LEFT_PWM_CHANNEL, 1000U);
        (void)pwm_write(MOTOR_RIGHT_PWM_CHANNEL, 1000U);
        (void)gpio_write(stby, true);
        return STATUS_OK;

    case MOTOR_FORWARD:
        set_direction(&ain1, &ain2, true);    /* left  CW  */
        set_direction(&bin1, &bin2, true);    /* right CW  */
        break;

    case MOTOR_BACKWARD:
        set_direction(&ain1, &ain2, false);   /* left  CCW */
        set_direction(&bin1, &bin2, false);   /* right CCW */
        break;

    case MOTOR_TURN_LEFT:
        /* Pivot turn: left reverse, right forward. */
        set_direction(&ain1, &ain2, false);
        set_direction(&bin1, &bin2, true);
        break;

    case MOTOR_TURN_RIGHT:
        /* Pivot turn: left forward, right reverse. */
        set_direction(&ain1, &ain2, true);
        set_direction(&bin1, &bin2, false);
        break;

    default:
        return STATUS_ERROR;
    }

    /* Set duty then enable bridge — direction is already latched. */
    (void)pwm_write(MOTOR_LEFT_PWM_CHANNEL, duty);
    (void)pwm_write(MOTOR_RIGHT_PWM_CHANNEL, duty);
    (void)gpio_write(stby, true);

    return STATUS_OK;
}
