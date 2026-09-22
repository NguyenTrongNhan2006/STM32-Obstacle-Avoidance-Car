#ifndef APP_CONFIG_H
#define APP_CONFIG_H
/* [DO] Initial task layout, stacks and periods need measured high-water marks. */
#define APP_TASK_COUNT 5U
#define APP_TASK_STACK_WORDS 256U
#define TASK_SAFETY_NAME "tSafety"
#define TASK_SENSOR_NAME "tSensor"
#define TASK_DECISION_NAME "tDecision"
#define TASK_LOG_NAME "tLog"
#define TASK_BUZZER_NAME "tBuzzer"
#define TASK_SAFETY_PRIORITY 4U
#define TASK_SENSOR_PRIORITY 2U
#define TASK_DECISION_PRIORITY 1U
#define TASK_LOG_PRIORITY 0U
#define TASK_BUZZER_PRIORITY 0U
/* Skeleton sleep only; these are NOT the eventual safety/control deadlines. */
#define TASK_SAFETY_PERIOD_MS 100U
#define TASK_SENSOR_PERIOD_MS 100U
#define TASK_DECISION_PERIOD_MS 100U
#define TASK_LOG_PERIOD_MS 100U
#define TASK_BUZZER_PERIOD_MS 100U
/* [DO] Targets after implementation: validate against response-time measurements. */
#define SENSOR_RANGE_PERIOD_MS 60U
#define SENSOR_IMU_PERIOD_MS 10U
#define SAFETY_TARGET_PERIOD_MS 10U
#define DECISION_TARGET_PERIOD_MS 20U
#define SENSOR_STALE_MS 200U
#define I2C_TIMEOUT_MS 10U
#define ECHO_TIMEOUT_US 30000UL
#define UART_TIMEOUT_MS 20U
#define D_STOP_MM 250U
#define D_CLEAR_MM 350U
#define T_TURN_MS 400U
#define MAX_AVOIDANCE_ATTEMPTS 3U
#define TILT_LIMIT_DEG 30U
#define BUTTON_DEBOUNCE_MS 25U
#define DRIVE_SPEED_PERCENT 35U
#define TURN_SPEED_PERCENT 30U
/* [DO] Short-brake window inserted by motor_tb6612 before any forward<->reverse
 * change. Two separate reasons, both unmeasured:
 *   electrical  - let the H-bridge current decay before the opposite arm turns on
 *   mechanical  - the wheel is still spinning; reversing torque into it is a shock
 * Measure the real decay/settle time before trusting this number. Too small is
 * a stress on the driver and gearbox, too large is a slow avoidance manoeuvre.
 */
#define MOTOR_DIRECTION_BRAKE_MS 60U
/* [CO DINH] Nominal IWDG timeout. The LSI that clocks it is only specified as
 * 30..60 kHz, so the real window is 333..667 ms. Any refresh cadence must be
 * derived from the 333 ms lower bound, never from this nominal value.
 * safety_monitor refreshes every ALIVE_WINDOW_MS = 100 ms.
 */
#define WATCHDOG_TIMEOUT_MS 500U
/* [DO] cos^2(TILT_LIMIT_DEG) as an exact integer ratio: cos^2(30 deg) = 3/4.
 * The ratio is NOT derived from TILT_LIMIT_DEG at compile time; change both
 * together. It lets the tilt test compare squared accelerometer components
 * without knowing the configured full-scale range.
 */
#define TILT_COS2_NUM 3U
#define TILT_COS2_DEN 4U
/* [DO] d_stop >= v_max*(t_sample+t_scheduling+t_actuation)
 *                 + measured braking/coasting distance + margin.
 * Use consistent units; all constants above are unmeasured proposals.
 *
 * Static RAM budget (bytes; actual sizeof checks in main.c):
 * application stacks: 5*256*sizeof(StackType_t) = 5120
 * idle + timer stacks: 2*128*sizeof(StackType_t) = 1024
 * task control blocks: 7*sizeof(StaticTask_t)
 * sensor mailboxes: 2*sizeof(StaticQueue_t)+sizeof(sample_t)+sizeof(imu_sample_t)
 * safety event group: sizeof(StaticEventGroup_t)
 * reserve 4096 for kernel globals/timer queue/HAL/data/alignment;
 * reserve 1024 for linker MSP/interrupt stack; final map must fit 20480.
 * Reserve is a planning allowance, not a substitute for map/stack measurement.
 */
#define APP_SRAM_BYTES 20480U
#define APP_KERNEL_DATA_RESERVE_BYTES 4096U
#define APP_MSP_RESERVE_BYTES 1024U
#endif
