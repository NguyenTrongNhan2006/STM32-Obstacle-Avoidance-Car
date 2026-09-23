#include "mpu6050.h"
#include "board_config.h"
#include "app_config.h"
#include "i2c.h"
#include "timebase.h"

/* [CO DINH] Register map MPU-6000/6050, TDK InvenSense.
 * Doi chieu voi datasheet truoc khi doi bat ky gia tri nao o day; khong sao
 * chep gia tri thanh ghi tu mot module khac khi chua hieu y nghia.
 */
#define MPU_REG_SMPLRT_DIV   0x19U
#define MPU_REG_CONFIG       0x1AU
#define MPU_REG_GYRO_CONFIG  0x1BU
#define MPU_REG_ACCEL_CONFIG 0x1CU
#define MPU_REG_ACCEL_XOUT_H 0x3BU
#define MPU_REG_PWR_MGMT_1   0x6BU
#define MPU_REG_WHO_AM_I     0x75U

#define MPU_WHO_AM_I_VALUE   0x68U

/* PWR_MGMT_1 = 0x01: bo SLEEP va chon nguon clock la PLL tham chieu gyro X.
 * On dinh hon RC noi bo (gia tri 0x00) — datasheet khuyen dung. */
#define MPU_PWR_WAKE_PLL_X   0x01U
/* CONFIG = 0x03: DLPF 44 Hz cho accel, 42 Hz cho gyro. Cat nhieu rung dong co
 * ma van du nhanh cho kiem tra nghieng. */
#define MPU_CONFIG_DLPF_44HZ 0x03U
/* Voi DLPF bat, gyro output rate la 1 kHz. SMPLRT_DIV = 7 -> 125 Hz, thoai mai
 * so voi nhip doc 10 ms (100 Hz) cua tSensor. */
#define MPU_SMPLRT_DIV_125HZ 0x07U
/* Ca hai thang do dat o muc nhay nhat: gyro +-250 deg/s, accel +-2 g.
 * LUU Y: safety_monitor kiem tra nghieng bang TY SO binh phuong cac truc nen
 * thang do tu triet tieu — doi hai gia tri nay khong lam sai phep kiem tra do. */
#define MPU_GYRO_FS_250DPS   0x00U
#define MPU_ACCEL_FS_2G      0x00U

/* accel 6 byte + temperature 2 byte + gyro 6 byte, lien tiep tu ACCEL_XOUT_H */
#define MPU_BURST_LENGTH     14U

/* So mau cho mot lan hieu chuan va nguong coi la dung yen. 400 LSB @ +-250
 * deg/s tuong duong khoang 3 deg/s — du chat de loai truong hop xe dang bi
 * cam tren tay. [DO] chua kiem tren cam bien that. */
#define MPU_CALIB_SAMPLES    64U
#define MPU_CALIB_MAX_RATE   400

static uint8_t address;
static uint32_t timeout_ms;
static bool initialized;
static int16_t gyro_bias[3];

static int16_t be16(const uint8_t *raw)
{
    return (int16_t)(((uint16_t)raw[0] << 8) | (uint16_t)raw[1]);
}

static status_t write_register(uint8_t reg, uint8_t value)
{
    return i2c_write(address, reg, &value, 1U, timeout_ms);
}

uint8_t mpu6050_default_address(void)
{
    return (uint8_t)IMU_ADDRESS_7BIT;
}

/* STATUS_NOT_READY  — khong noi chuyen duoc voi thiet bi (chua noi day, bus
 *                     hong, nguon chua len). Nguoi goi nen thu lai sau.
 * STATUS_ERROR      — noi chuyen duoc nhung WHO_AM_I sai: dia chi tro toi mot
 *                     con chip khac. Thu lai bao nhieu lan cung vo ich.
 */
status_t mpu6050_init(const mpu6050_config_t *config)
{
    uint8_t who = 0U;

    if (config == NULL || config->address_7bit > 0x7FU || config->timeout_ms == 0U) {
        return STATUS_ERROR;
    }
    initialized = false;
    address = config->address_7bit;
    timeout_ms = config->timeout_ms;

    if (i2c_read(address, MPU_REG_WHO_AM_I, &who, 1U, timeout_ms) != STATUS_OK) {
        return STATUS_NOT_READY;
    }
    if (who != MPU_WHO_AM_I_VALUE) { return STATUS_ERROR; }

    /* Danh thuc truoc, cau hinh sau: o che do SLEEP thiet bi van nhan ghi
     * nhung khong lay mau, de nham tuong da cau hinh xong. */
    if (write_register(MPU_REG_PWR_MGMT_1, MPU_PWR_WAKE_PLL_X) != STATUS_OK ||
        write_register(MPU_REG_CONFIG, MPU_CONFIG_DLPF_44HZ) != STATUS_OK ||
        write_register(MPU_REG_SMPLRT_DIV, MPU_SMPLRT_DIV_125HZ) != STATUS_OK ||
        write_register(MPU_REG_GYRO_CONFIG, MPU_GYRO_FS_250DPS) != STATUS_OK ||
        write_register(MPU_REG_ACCEL_CONFIG, MPU_ACCEL_FS_2G) != STATUS_OK) {
        return STATUS_NOT_READY;
    }

    gyro_bias[0] = 0;
    gyro_bias[1] = 0;
    gyro_bias[2] = 0;
    initialized = true;
    return STATUS_OK;
}

/* STATUS_OK nghia la DA CO KET LUAN cho lan doc, khong phai la doc duoc:
 * sample->status moi noi du lieu co dung duoc hay khong.
 *
 * Chinh sach tilt KHONG nam o day. Driver chi tra raw signed kem timestamp va
 * status; viec "nghieng bao nhieu thi dung" thuoc safety_monitor, noi so huu
 * cac bit inhibit va biet trang thai hien tai cua he thong. Xem imu_upright()
 * trong App/Src/safety_monitor.c — no so sanh ty so binh phuong cac truc nen
 * khong can biet thang do da cau hinh.
 */
status_t mpu6050_read(imu_sample_t *sample)
{
    uint8_t raw[MPU_BURST_LENGTH];
    uint32_t started_ms;
    status_t result;

    if (sample == NULL) { return STATUS_ERROR; }
    *sample = (imu_sample_t){ .status = SAMPLE_NOT_READY };
    if (!initialized) { return STATUS_NOT_READY; }

    /* Moc thoi gian lay TRUOC giao dich: som hon thoi diem du lieu thuc su ve,
     * nen mau luon duoc coi la gia hon thuc te mot chut — lech ve phia an toan
     * cho kiem tra freshness, giong cach ultrasonic dat timestamp. */
    started_ms = timebase_now_ms();

    /* Doc burst mot lan: accel, temperature va gyro thuoc cung mot lan chup cua
     * cam bien. Doc tung thanh ghi rieng se ghep du lieu cua hai lan chup khac
     * nhau va cho ra mot trang thai khong bao gio ton tai that. */
    result = i2c_read(address, MPU_REG_ACCEL_XOUT_H, raw, MPU_BURST_LENGTH, timeout_ms);
    sample->timestamp_ms = started_ms;

    if (result != STATUS_OK) {
        /* Buoc thiet bi ve trang thai chua khoi tao. Mot thiet bi vua sut nguon
         * se quay lai voi cau hinh mac dinh (dang SLEEP, thang do khac); doc
         * tiep ma khong cau hinh lai se cho ra so lieu vo nghia nhung trong
         * nhu that. Nguoi goi phai chay lai mpu6050_init(). */
        initialized = false;
        sample->status = (result == STATUS_TIMEOUT) ? SAMPLE_TIMEOUT : SAMPLE_ERROR;
        return STATUS_OK;
    }

    sample->accel_raw[0] = be16(&raw[0]);
    sample->accel_raw[1] = be16(&raw[2]);
    sample->accel_raw[2] = be16(&raw[4]);
    sample->temperature_raw = be16(&raw[6]);
    sample->gyro_raw[0] = (int16_t)(be16(&raw[8]) - gyro_bias[0]);
    sample->gyro_raw[1] = (int16_t)(be16(&raw[10]) - gyro_bias[1]);
    sample->gyro_raw[2] = (int16_t)(be16(&raw[12]) - gyro_bias[2]);
    sample->status = SAMPLE_OK;
    return STATUS_OK;
}

/* Uoc luong bias cua gyro khi xe DUNG YEN va tru vao cac lan doc sau.
 * Tu choi neu phat hien chuyen dong: hieu chuan trong luc xe dang di se hoc
 * nham toc do goc that thanh bias va lam moi phep do sau do sai theo.
 *
 * KHONG cham vao accel: safety_monitor kiem tra nghieng bang ty so binh phuong
 * cac truc, tru bias accel se pha chinh phep do trong luc dung de so.
 *
 * Ham nay CHUA duoc goi o dau. No phai duoc goi co chu dich khi da biet chac
 * xe dung yen, khong phai tu dong luc khoi dong.
 */
status_t mpu6050_calibrate(void)
{
    int32_t total[3] = {0, 0, 0};
    uint32_t count;

    if (!initialized) { return STATUS_NOT_READY; }

    gyro_bias[0] = 0;
    gyro_bias[1] = 0;
    gyro_bias[2] = 0;

    for (count = 0U; count < MPU_CALIB_SAMPLES; ++count) {
        imu_sample_t sample;
        uint32_t axis;

        if (mpu6050_read(&sample) != STATUS_OK || sample.status != SAMPLE_OK) {
            return STATUS_NOT_READY;
        }
        for (axis = 0U; axis < 3U; ++axis) {
            const int32_t rate = sample.gyro_raw[axis];
            if (rate > MPU_CALIB_MAX_RATE || rate < -MPU_CALIB_MAX_RATE) {
                return STATUS_ERROR;   /* dang chuyen dong -> tu choi hieu chuan */
            }
            total[axis] += rate;
        }
    }

    gyro_bias[0] = (int16_t)(total[0] / (int32_t)MPU_CALIB_SAMPLES);
    gyro_bias[1] = (int16_t)(total[1] / (int32_t)MPU_CALIB_SAMPLES);
    gyro_bias[2] = (int16_t)(total[2] / (int32_t)MPU_CALIB_SAMPLES);
    return STATUS_OK;
}

status_t mpu6050_calibrate_gyro(void)
{
    return mpu6050_calibrate();
}

int16_t mpu6050_yaw_rate_dps(const imu_sample_t *sample)
{
    if (sample == NULL || sample->status != SAMPLE_OK) { return 0; }
    /* FS_SEL=0 (+-250 dps) -> 131 LSB/(deg/s) */
    return (int16_t)(sample->gyro_raw[2] / 131);
}

bool mpu6050_tilt_exceeded(const imu_sample_t *sample)
{
    if (sample == NULL || sample->status != SAMPLE_OK) { return false; }
    const int32_t ax = sample->accel_raw[0];
    const int32_t ay = sample->accel_raw[1];
    const int32_t az = sample->accel_raw[2];
    uint64_t vertical;
    uint64_t total;

    if (az <= 0) { return true; }
    vertical = (uint64_t)((int64_t)az * az);
    total = vertical + (uint64_t)((int64_t)ax * ax) + (uint64_t)((int64_t)ay * ay);
    if (total == 0U) { return true; }
    return (vertical * TILT_COS2_DEN) < (total * TILT_COS2_NUM);
}
