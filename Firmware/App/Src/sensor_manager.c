#include "sensor_manager.h"
#include "app_config.h"
#include "mpu6050.h"
#include "ultrasonic_hcsr04.h"

/* Nhip thu khoi tao lai IMU khi driver bao chua san sang. Khong phai deadline
 * dieu khien: chi de mot cam bien tuot day hoac len nguon muon van tu noi lai
 * duoc, ma khong bien moi chu ky cua tSensor thanh mot giao dich I2C that bai
 * ton timeout 10 ms. */
#define IMU_REINIT_PERIOD_MS 500U

static uint32_t imu_sampled_ms;
static uint32_t imu_reinit_ms;

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
    const mpu6050_config_t imu_sensor = {
        .address_7bit = mpu6050_default_address(),
        .timeout_ms = I2C_TIMEOUT_MS,
    };
    /* sensor_manager la chu so huu cua cac cam bien trong lop App, nen no khoi
     * tao chung. Tham so sai la loi cau hinh -> bao ERROR de configASSERT trong
     * main() bat duoc ngay, khac han voi NOT_READY cua mot driver chua viet. */
    if (ultrasonic_init(&range_sensor) == STATUS_ERROR) { return STATUS_ERROR; }
    /* IMU chua noi day hoac chua len nguon tra NOT_READY. KHONG duoc coi do la
     * loi chi mang: bat boot fail o day se lam board khong khoi dong duoc chi
     * vi mot soi day long. safety_monitor da chot SAFETY_BIT_SENSOR_FAULT tu
     * luc boot nen xe van khong the chay, va sensor_manager_update() se thu
     * khoi tao lai theo chu ky. */
    (void)mpu6050_init(&imu_sensor);
    imu_sampled_ms = 0U;
    imu_reinit_ms = 0U;
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
/* Nhanh IMU. Deadline rieng o day chu khong trong driver, vi mpu6050 la mot
 * thiet bi doc theo yeu cau — no khong tu biet nen lay mau bao lau mot lan.
 */
static void update_imu(uint32_t now_ms)
{
    const mpu6050_config_t imu_sensor = {
        .address_7bit = mpu6050_default_address(),
        .timeout_ms = I2C_TIMEOUT_MS,
    };
    imu_sample_t imu;
    status_t result;

    if ((uint32_t)(now_ms - imu_sampled_ms) < SENSOR_IMU_PERIOD_MS) { return; }
    imu_sampled_ms = now_ms;

    result = mpu6050_read(&imu);
    if (result == STATUS_OK) {
        /* Publish ca mau hop le lan ma loi: consumer phai thay bang chung cua
         * that bai chu khong phai su im lang. */
        (void)xQueueOverwrite(qImuMailbox, &imu);
        return;
    }
    /* Driver bao chua san sang (chua init duoc, hoac vua tu vo hieu sau mot
     * giao dich hong). Thu lai co gioi han nhip. Mailbox giu nguyen mau cu, no
     * se qua han SENSOR_STALE_MS va safety_monitor chot SENSOR_FAULT — dung
     * huong: khong co du lieu moi thi khong duoc coi la khoe. */
    if ((uint32_t)(now_ms - imu_reinit_ms) >= IMU_REINIT_PERIOD_MS) {
        imu_reinit_ms = now_ms;
        (void)mpu6050_init(&imu_sensor);
    }
}

status_t sensor_manager_update(uint32_t now_ms)
{
    sample_t range;

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

    /* Hai nhanh chay o hai nhip khac nhau (range 60 ms, IMU 10 ms) nhung deu
     * trong mot lan goi: mot producer duy nhat cho ca hai mailbox. */
    update_imu(now_ms);
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
