#include "sensor_manager.h"
#include "app_config.h"
#include "ultrasonic_hcsr04.h"
QueueHandle_t qRangeMailbox;
QueueHandle_t qImuMailbox;
static StaticQueue_t range_control;
static StaticQueue_t imu_control;
static uint8_t range_storage[sizeof(sample_t)];
static uint8_t imu_storage[sizeof(imu_sample_t)];
status_t sensor_manager_init(void)
{
    const sample_t range = { .unit = SAMPLE_UNIT_MM, .status = SAMPLE_NOT_READY };
    const imu_sample_t imu = { .status = SAMPLE_NOT_READY };
    const ultrasonic_config_t range_sensor = {
        .echo_timeout_us = ECHO_TIMEOUT_US,
        .sample_period_ms = SENSOR_RANGE_PERIOD_MS,
    };
    /* sensor_manager la chu so huu cua cac cam bien trong lop App, nen no khoi
     * tao chung. Tham so sai la loi cau hinh -> bao ERROR de configASSERT trong
     * main() bat duoc ngay, khac han voi NOT_READY cua mot driver chua viet. */
    if (ultrasonic_init(&range_sensor) == STATUS_ERROR) { return STATUS_ERROR; }
    if (qRangeMailbox == NULL) {
        qRangeMailbox = xQueueCreateStatic(1U, sizeof(sample_t), range_storage, &range_control);
        if (qRangeMailbox == NULL) { return STATUS_ERROR; }
        configASSERT(xQueueOverwrite(qRangeMailbox, &range) == pdPASS);
    }
    if (qImuMailbox == NULL) {
        qImuMailbox = xQueueCreateStatic(1U, sizeof(imu_sample_t), imu_storage, &imu_control);
        if (qImuMailbox == NULL) { return STATUS_ERROR; }
        configASSERT(xQueueOverwrite(qImuMailbox, &imu) == pdPASS);
    }
    return STATUS_OK;
}
status_t sensor_manager_update(uint32_t now_ms)
{
    sample_t range;

    /* now_ms chua duoc dung: deadline cua tung cam bien nam trong chinh driver
     * cua no (ultrasonic giu SENSOR_RANGE_PERIOD_MS), khong phai o day. Giu
     * tham so trong API vi buoc 3 se can no de dieu phoi hai nhip khac nhau. */
    (void)now_ms;
    if (qRangeMailbox == NULL || qImuMailbox == NULL) { return STATUS_NOT_READY; }

    /* Nhanh range. ultrasonic_read() tra STATUS_OK khi DA CO KET LUAN — ke ca
     * khi ket luan do la mat echo. Publish ca hai truong hop: consumer phai
     * nhin thay bang chung cua that bai chu khong phai su im lang, vi mot
     * mailbox khong doi se tro thanh STALE va bi coi la fault. */
    if (ultrasonic_read(&range) == STATUS_OK) {
        /* Copy nguyen struct: gia tri + don vi + timestamp + status di cung
         * nhau, nguoi doc khong bao gio thay mot mau lai nua cu nua moi.
         * timestamp_ms do driver dat luc chup mau, o day khong duoc lam moi. */
        (void)xQueueOverwrite(qRangeMailbox, &range);
    }
    /* Tu choi neu chua den han retrigger hoac con phep do dang chay. */
    (void)ultrasonic_request();

    /* IMPLEMENT (buoc 3): nhanh IMU qua i2c + mpu6050 o nhip SENSOR_IMU_PERIOD_MS,
     * publish vao qImuMailbox. Chung nao chua co, mailbox giu SAMPLE_NOT_READY
     * va safety_monitor van chot SAFETY_BIT_SENSOR_FAULT — dung nhu thiet ke. */
    return STATUS_OK;
}
status_t sensor_manager_get_latest(sample_t *range, imu_sample_t *imu)
{
    if (range == NULL || imu == NULL) { return STATUS_ERROR; }
    *range = (sample_t){ .unit = SAMPLE_UNIT_MM, .status = SAMPLE_NOT_READY };
    *imu = (imu_sample_t){ .status = SAMPLE_NOT_READY };
    if (qRangeMailbox == NULL || qImuMailbox == NULL) { return STATUS_NOT_READY; }
    if (xQueuePeek(qRangeMailbox, range, 0U) != pdPASS ||
        xQueuePeek(qImuMailbox, imu, 0U) != pdPASS) { return STATUS_NOT_READY; }
    /* STATUS_OK means copies available; caller must inspect BOTH sample statuses
     * and ages. Pair is not an atomic simultaneous sensor acquisition.
     */
    return STATUS_OK;
}
