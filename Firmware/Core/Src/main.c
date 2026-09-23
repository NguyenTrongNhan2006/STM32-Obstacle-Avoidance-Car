#include "main.h"
#include "board_config.h"
#include "app_config.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "gpio.h"
#include "timebase.h"
#include "uart_debug.h"
#include "robot_car.h"
#include "safety_monitor.h"
#include "sensor_manager.h"

volatile uint32_t g_assert_line;
const char * volatile g_assert_file;
static StaticTask_t task_controls[APP_TASK_COUNT];
static StackType_t task_stacks[APP_TASK_COUNT][APP_TASK_STACK_WORDS];
static StaticTask_t idle_control;
static StackType_t idle_stack[configMINIMAL_STACK_SIZE];
static StaticTask_t timer_control;
static StackType_t timer_stack[configTIMER_TASK_STACK_DEPTH];
_Static_assert(sizeof(StackType_t) == 4U, "Update RAM model for this port");
_Static_assert(HSE_VALUE == BOARD_HSE_HZ, "Clock assumptions differ");
_Static_assert(configPRIO_BITS == __NVIC_PRIO_BITS, "Incorrect NVIC priority width");
_Static_assert(
    sizeof(task_stacks) + sizeof(idle_stack) + sizeof(timer_stack) +
    sizeof(task_controls) + sizeof(idle_control) + sizeof(timer_control) +
    2U * sizeof(StaticQueue_t) + sizeof(sample_t) + sizeof(imu_sample_t) +
    sizeof(StaticEventGroup_t) + APP_KERNEL_DATA_RESERVE_BYTES + APP_MSP_RESERVE_BYTES
    < APP_SRAM_BYTES, "Static RAM planning budget exceeded");

void vApplicationGetIdleTaskMemory(StaticTask_t **control, StackType_t **stack, uint32_t *depth)
{
    *control = &idle_control;
    *stack = idle_stack;
    *depth = configMINIMAL_STACK_SIZE;
}
void vApplicationGetTimerTaskMemory(StaticTask_t **control, StackType_t **stack, uint32_t *depth)
{
    *control = &timer_control;
    *stack = timer_stack;
    *depth = configTIMER_TASK_STACK_DEPTH;
}
void vApplicationAssert(const char *file, uint32_t line)
{
    __disable_irq();
    gpio_emergency_stop();
    g_assert_file = file;
    g_assert_line = line;
    for (;;) { __NOP(); }
}
void vApplicationStackOverflowHook(TaskHandle_t task, char *name)
{
    (void)task;
    (void)name;
    vApplicationAssert(__FILE__, __LINE__);
}
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState = RCC_HSE_ON;
    osc.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLMUL = RCC_PLL_MUL9;
    configASSERT(HAL_RCC_OscConfig(&osc) == HAL_OK);
    clk.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                    RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV2;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    configASSERT(HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2) == HAL_OK);
    configASSERT(SystemCoreClock == BOARD_SYSCLK_HZ);
}
static void task_safety(void *argument)
{
    TickType_t wake = xTaskGetTickCount();
    (void)argument;
    /* safety_update runs the fault-latch / freshness / tilt / rearm policy.
     * Period starts at TASK_SAFETY_PERIOD_MS; target 10 ms after measurement.
     */
    for (;;) {
        (void)safety_update(timebase_now_ms());
        vTaskDelayUntil(&wake, pdMS_TO_TICKS(TASK_SAFETY_PERIOD_MS));
    }
}
static void task_sensor(void *argument)
{
    TickType_t wake = xTaskGetTickCount();
    (void)argument;
    /* task_sensor runs sensor_manager_update periodically.
     * Uses 10 ms period to service range acquisition deadlines and IMU.
     */
    for (;;) {
        (void)sensor_manager_update(timebase_now_ms());
        vTaskDelayUntil(&wake, pdMS_TO_TICKS(10U));
    }
}
static void task_decision(void *argument)
{
    TickType_t wake = xTaskGetTickCount();
    (void)argument;
    /* robot_car_update is the sole caller of motor_apply.
     * Period starts at TASK_DECISION_PERIOD_MS; target 20 ms after measurement.
     */
    for (;;) {
        (void)robot_car_update(timebase_now_ms());
        vTaskDelayUntil(&wake, pdMS_TO_TICKS(TASK_DECISION_PERIOD_MS));
    }
}
static void task_log(void *argument)
{
    (void)argument;
    /* IMPLEMENT (Hung): bounded telemetry; sole runtime UART writer. */
    for (;;) { vTaskDelay(pdMS_TO_TICKS(TASK_LOG_PERIOD_MS)); }
}
static void task_buzzer(void *argument)
{
    (void)argument;
    /* IMPLEMENT (Hung): deadline-based patterns; must block/yield at priority 0. */
    for (;;) { vTaskDelay(pdMS_TO_TICKS(TASK_BUZZER_PERIOD_MS)); }
}
int main(void)
{
    const timebase_config_t time_config = { .capture_tick_hz = 1000000UL };
    const uart_debug_config_t uart_config = { .baud = DEBUG_BAUD, .timeout_ms = UART_TIMEOUT_MS };
    static const TaskFunction_t functions[APP_TASK_COUNT] = {
        task_safety, task_sensor, task_decision, task_log, task_buzzer
    };
    static const char * const names[APP_TASK_COUNT] = {
        TASK_SAFETY_NAME, TASK_SENSOR_NAME, TASK_DECISION_NAME, TASK_LOG_NAME, TASK_BUZZER_NAME
    };
    static const UBaseType_t priorities[APP_TASK_COUNT] = {
        TASK_SAFETY_PRIORITY, TASK_SENSOR_PRIORITY, TASK_DECISION_PRIORITY,
        TASK_LOG_PRIORITY, TASK_BUZZER_PRIORITY
    };
    status_t app_status;
    configASSERT(HAL_Init() == HAL_OK);
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
    configASSERT(gpio_init() == STATUS_OK);
    SystemClock_Config();
    configASSERT(timebase_init(&time_config) == STATUS_OK);
    configASSERT(uart_debug_init(&uart_config) == STATUS_OK);
    /* Motor and PWM are now implemented; sensor drivers still return NOT_READY. */
    app_status = robot_car_init();
    configASSERT(app_status == STATUS_OK || app_status == STATUS_NOT_READY);
    (void)uart_log("Boot: PWM/motor ready, sensors TODO\r\n");
    for (uint32_t index = 0U; index < APP_TASK_COUNT; ++index) {
        configASSERT(xTaskCreateStatic(functions[index], names[index], APP_TASK_STACK_WORDS,
                     NULL, priorities[index], task_stacks[index], &task_controls[index]) != NULL);
    }
    vTaskStartScheduler();
    vApplicationAssert(__FILE__, __LINE__);
    return 0;
}
