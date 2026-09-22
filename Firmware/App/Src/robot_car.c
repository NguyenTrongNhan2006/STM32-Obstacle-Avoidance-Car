#include "robot_car.h"
#include "app_config.h"
#include "buzzer.h"
#include "motor_tb6612.h"
#include "obstacle_avoidance.h"
#include "safety_monitor.h"
#include "sensor_manager.h"
#include "status_led.h"

static avoidance_context_t avoidance;
static bool safety_was_clear;

static uint8_t speed_for(motor_cmd_t command)
{
    switch (command) {
    case MOTOR_FORWARD:
    case MOTOR_BACKWARD:
        return DRIVE_SPEED_PERCENT;
    case MOTOR_TURN_LEFT:
    case MOTOR_TURN_RIGHT:
        return TURN_SPEED_PERCENT;
    default:
        return 0U;
    }
}

static led_pattern_t pattern_for(car_state_t state)
{
    if (state == CAR_FAULT) { return LED_FAULT; }
    return (state == CAR_IDLE) ? LED_IDLE : LED_RUNNING;
}

/* Coi bao theo trang thai, giong den nhung nghe duoc tu xa va khi khong nhin
 * thay xe. CAR_STOP tach rieng khoi CAR_FAULT: gap vat can la hanh vi binh
 * thuong, con fault la xe da bo cuoc va dang cho nguoi xac nhan.
 */
static buzzer_pattern_t sound_for(car_state_t state)
{
    switch (state) {
    case CAR_FAULT: return BUZZER_FAULT;
    case CAR_STOP:  return BUZZER_OBSTACLE;
    case CAR_IDLE:  return BUZZER_SILENT;
    default:        return BUZZER_START;   /* dang di chuyen: bao cho nguoi xung quanh */
    }
}

status_t robot_car_init(void)
{
    const motor_config_t config = { .is_safe = safety_is_clear_to_run };
    const buzzer_config_t sound = { .active_high = true };
    if (safety_init() != STATUS_OK || sensor_manager_init() != STATUS_OK ||
        obstacle_avoidance_init(&avoidance) != STATUS_OK) { return STATUS_ERROR; }
    safety_was_clear = false;
    (void)status_led_init();
    /* [DO] active_high phu thuoc tang transistor: NPN low-side -> true,
     * PNP high-side -> false. Do bang dong ho truoc khi tin vao mau coi. */
    (void)buzzer_init(&sound);
    return motor_init(&config);
}

status_t robot_car_update(uint32_t now_ms)
{
    sample_t range;
    imu_sample_t imu;
    motor_cmd_t request = MOTOR_STOP;
    const bool safe_now = safety_is_clear_to_run();

    /* The only path out of a latched FSM fault: the safety owner cleared the
     * inhibit after a deliberate rearm. Edge-triggered, so a persistently clear
     * safety state cannot keep resetting the FSM.
     */
    if (safe_now && !safety_was_clear && avoidance.state == CAR_FAULT) {
        (void)obstacle_avoidance_init(&avoidance);
    }
    safety_was_clear = safe_now;

    /* A failed snapshot leaves the request at MOTOR_STOP rather than reusing a
     * previous command.
     */
    if (sensor_manager_get_latest(&range, &imu) == STATUS_OK) {
        (void)obstacle_avoidance_update(&avoidance, &range, &imu, now_ms, &request);
    }
    (void)status_led_set(pattern_for(avoidance.state));
    /* Chi ghi y dinh; tBuzzer moi la task cham vao chan coi. */
    (void)buzzer_set(sound_for(avoidance.state));

    /* Sole motor command boundary in the App: exactly one call per update, on
     * every path. motor_apply still re-checks the safety predicate itself.
     */
    return motor_apply(request, speed_for(request));
}
