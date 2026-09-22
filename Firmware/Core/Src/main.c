#include "main.h"
#include "board_config.h"
#include "app_config.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "gpio.h"
#include "i2c.h"
#include "timebase.h"
#include "uart_debug.h"
#include "watchdog.h"
#include "buzzer.h"
#include "robot_car.h"
#include "safety_monitor.h"
#include "sensor_manager.h"

/* Nhip in trang thai dinh ky cua tLog. Khong phai deadline dieu khien. */
#define LOG_HEARTBEAT_MS 1000U

/* Ket qua lan goi motor_apply() gan nhat, do tDecision ghi va tLog doc.
 * Chi de quan sat: no cho thay duong an toan co dang cat lenh motor hay khong
 * (STATUS_NOT_READY = bi cat, STATUS_OK = da dat ra phan cung).
 * uint32_t ghi/doc nguyen tu tren Cortex-M3 va khong co quyet dinh nao dua tren
 * bien nay, nen khong can mailbox hay khoa. */
static volatile uint32_t last_motor_status = (uint32_t)STATUS_NOT_READY;

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
/* Ca nam task dung xTaskDelayUntil chu khong phai vTaskDelay: chu ky duoc tinh
 * tu lan danh thuc truoc, nen khong troi theo thoi gian xu ly cua than vong lap.
 *
 * Chu ky lay tu nhom *_TARGET_/SENSOR_* trong app_config.h, khong phai nhom
 * TASK_*_PERIOD_MS (nhom do la sleep tam cua skeleton). Cac hang so nay van la
 * [DO] - phai do lai response time truoc khi coi la da chot.
 *
 * Moi ham duoc goi o day deu con tra STATUS_NOT_READY o mot phan duong di.
 * Do la trang thai dung: lop App da co logic, con driver thi chua.
 */
static void task_safety(void *argument)
{
    TickType_t last_wake = xTaskGetTickCount();

    (void)argument;
    for (;;) {
        (void)safety_update(timebase_now_ms());
        (void)xTaskDelayUntil(&last_wake, pdMS_TO_TICKS(SAFETY_TARGET_PERIOD_MS));
    }
}
static void task_sensor(void *argument)
{
    TickType_t last_wake = xTaskGetTickCount();

    (void)argument;
    /* Chay o nhip nhanh nhat trong hai nguon (IMU 10 ms). Deadline rieng cua
     * range (60 ms) do chinh sensor_manager_update() giu, khong phai task.
     */
    for (;;) {
        (void)sensor_manager_update(timebase_now_ms());
        /* Check-in SAU khi da lam viec: bao "da chay xong mot vong", khong phai
         * "da vao ham". Dat truoc se bao khoe ngay ca khi than vong lap treo. */
        safety_check_in(ALIVE_BIT_SENSOR);
        (void)xTaskDelayUntil(&last_wake, pdMS_TO_TICKS(SENSOR_IMU_PERIOD_MS));
    }
}
static void task_decision(void *argument)
{
    TickType_t last_wake = xTaskGetTickCount();

    (void)argument;
    /* robot_car_update() khong block: no chup mailbox, chay FSM roi goi
     * motor_apply() dung mot lan. Diem block duy nhat la dong xTaskDelayUntil.
     */
    for (;;) {
        last_motor_status = (uint32_t)robot_car_update(timebase_now_ms());
        safety_check_in(ALIVE_BIT_DECISION);
        (void)xTaskDelayUntil(&last_wake, pdMS_TO_TICKS(DECISION_TARGET_PERIOD_MS));
    }
}
static void task_log(void *argument)
{
    TickType_t last_wake = xTaskGetTickCount();
    uint32_t previous_bits = UINT32_MAX;    /* gia tri khong the xay ra -> in ngay lan dau */
    uint32_t since_heartbeat_ms = 0U;

    (void)argument;
    /* Writer UART duy nhat luc runtime. In khi egSafety doi, cong them nhip
     * dinh ky de biet he thong con song. HAL_UART_Transmit la busy-wait chu
     * khong phai block cua RTOS, nen giu luong log thap va task nay o priority 0.
     */
    for (;;) {
        const uint32_t bits =
            (egSafety != NULL) ? (uint32_t)xEventGroupGetBits(egSafety) : 0U;

        since_heartbeat_ms += TASK_LOG_PERIOD_MS;
        if ((bits != previous_bits) || (since_heartbeat_ms >= LOG_HEARTBEAT_MS)) {
            sample_t range;
            imu_sample_t imu;

            (void)uart_log_u32("safety=", bits);
            if (sensor_manager_get_latest(&range, &imu) == STATUS_OK) {
                (void)uart_log_u32("range_st=", (uint32_t)range.status);
                /* Chi in gia tri khi mau hop le. In ca khi TIMEOUT/ERROR se tao
                 * ra mot con so trong duong nhu do duoc — dung thu can tranh. */
                if (range.status == SAMPLE_OK) {
                    (void)uart_log_u32("range_mm=", range.value);
                    (void)uart_log_u32("range_age_ms=",
                                       (uint32_t)(timebase_now_ms() - range.timestamp_ms));
                }
                (void)uart_log_u32("imu_st=", (uint32_t)imu.status);
            }
            /* 0 = OK (lenh da ra phan cung), 3 = NOT_READY (duong an toan da cat) */
            (void)uart_log_u32("motor_st=", last_motor_status);
            previous_bits = bits;
            since_heartbeat_ms = 0U;
        }
        (void)xTaskDelayUntil(&last_wake, pdMS_TO_TICKS(TASK_LOG_PERIOD_MS));
    }
}
static void task_buzzer(void *argument)
{
    TickType_t last_wake = xTaskGetTickCount();

    (void)argument;
    /* buzzer_update() chi day deadline cua mau coi roi tra ve ngay - khong bao
     * gio duoc cho het thoi luong mot tieng bip trong than vong lap nay.
     */
    for (;;) {
        (void)buzzer_update(timebase_now_ms());
        (void)xTaskDelayUntil(&last_wake, pdMS_TO_TICKS(TASK_BUZZER_PERIOD_MS));
    }
}
int main(void)
{
    const timebase_config_t time_config = { .capture_tick_hz = 1000000UL };
    const uart_debug_config_t uart_config = { .baud = DEBUG_BAUD, .timeout_ms = UART_TIMEOUT_MS };
    const i2c_config_t bus_config = { .bus_hz = IMU_I2C_SPEED_HZ };
    const watchdog_config_t dog_config = { .timeout_ms = WATCHDOG_TIMEOUT_MS };
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
    /* Core khoi tao MCAL; App khoi tao Devices. i2c_init() chi cau hinh ngoai
     * vi va giai phong bus neu no dang ket — khong co giao dich nao, nen mot
     * MPU6050 chua noi day KHONG lam assert nay that bai. */
    configASSERT(i2c_init(&bus_config) == STATUS_OK);
    /* Current device stub intentionally reports NOT_READY, but static App objects exist. */
    app_status = robot_car_init();
    configASSERT(app_status == STATUS_OK || app_status == STATUS_NOT_READY);

    /* Watchdog khoi tao SAU CUNG trong day init va NGAY TRUOC khi tao task.
     * Ly do: mot khi chay thi khong tat duoc nua, nen cang it code chay truoc no
     * cang it nguy co boot loop. Bo qua giai doan init KHONG lam mat an toan —
     * gpio_init() da ha STBY tu dong dau tien, nen treo trong init van de motor
     * o standby. */
    configASSERT(watchdog_init(&dog_config) == STATUS_OK);

    (void)uart_log("boot: sensing+actuation live, watchdog armed\r\n");
    /* 1 = lan reset vua roi la do watchdog, tuc mot task giam sat da treo. */
    (void)uart_log_u32("reset_by_wdg=", watchdog_caused_last_reset() ? 1U : 0U);
    for (uint32_t index = 0U; index < APP_TASK_COUNT; ++index) {
        configASSERT(xTaskCreateStatic(functions[index], names[index], APP_TASK_STACK_WORDS,
                     NULL, priorities[index], task_stacks[index], &task_controls[index]) != NULL);
    }
    vTaskStartScheduler();
    vApplicationAssert(__FILE__, __LINE__);
    return 0;
}
