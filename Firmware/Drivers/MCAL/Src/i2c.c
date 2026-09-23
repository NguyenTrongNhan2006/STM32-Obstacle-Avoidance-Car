#include "i2c.h"
#include "board_config.h"

/* i2c la chu so huu duy nhat cua I2C1 (bang owners trong board_config.h).
 * PB6/PB7 la chan mac dinh cua I2C1 tren F103 nen KHONG can AFIO remap.
 *
 * MO HINH THUC THI — doc ky truoc khi dung:
 * Hai ham i2c_read/i2c_write o day la BOUNDED-BLOCKING, khong phai non-blocking
 * thuc su. Chu ky ham buoc phai nhu vay: chung nhan mot buffer ra va tra ket
 * qua ngay trong mot lan goi, nen khong co cho de tra "dang chay". Doi sang
 * non-blocking that se can tach arm/poll giong timebase_capture_arm().
 *
 * Chi phi thuc te: doc burst 14 byte @400 kHz mat khoang 0,4 ms. Worst case la
 * timeout_ms (10 ms) khi bus hong. Nguoi goi la tSensor (priority 2), nen:
 *   - tSafety (priority 4) van preempt duoc ngay, duong an toan khong bi tre.
 *   - tDecision (priority 1) co the bi tre toi 10 ms tren chu ky 20 ms cua no.
 * Neu do duoc tre that va thay khong chap nhan duoc, chuyen sang
 * HAL_I2C_Mem_Read_IT + semaphore tu i2c_irq_event() — ISR da duoc dinh tuyen
 * san tu buoc 0.
 */

/* Xung clock giai phong bus khong can dung tan so: chi can du cham de slave kip
 * lay mau. Vong lap volatile cho ra vai us o 72 MHz; sai so o day khong quan
 * trong va ham nay chi chay luc init hoac khi bus da hong.
 */
#define RECOVERY_SPIN 200U
#define RECOVERY_CLOCKS 9U

static I2C_HandleTypeDef bus;
static uint32_t bus_hz;
static bool initialized;

static void recovery_half_period(void)
{
    volatile uint32_t spin;
    for (spin = 0U; spin < RECOVERY_SPIN; ++spin) { }
}

static void pins_as_gpio_od(void)
{
    GPIO_InitTypeDef pins = {0};

    HAL_GPIO_WritePin(IMU_I2C_PORT, IMU_SCL_PIN | IMU_SDA_PIN, GPIO_PIN_SET);
    pins.Pin = IMU_SCL_PIN | IMU_SDA_PIN;
    pins.Mode = GPIO_MODE_OUTPUT_OD;
    pins.Pull = GPIO_NOPULL;
    pins.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(IMU_I2C_PORT, &pins);
}

static void pins_as_i2c(void)
{
    GPIO_InitTypeDef pins = {0};

    pins.Pin = IMU_SCL_PIN | IMU_SDA_PIN;
    pins.Mode = GPIO_MODE_AF_OD;
    /* Khong bat pull-up noi: module MPU6050 da co dien tro keo len. Bat them
     * pull-up yeu cua MCU se lam suon tin hieu xau di chu khong tot len. */
    pins.Pull = GPIO_NOPULL;
    pins.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(IMU_I2C_PORT, &pins);
}

/* STUCK-BUS RECOVERY.
 * Neu MCU reset giua mot transaction, slave co the dang giu SDA thap de gui
 * not cac bit con lai va se giu mai — bus chet, khong START nao thanh cong.
 * Cach thoat duy nhat la MCU tu phat xung SCL cho slave xuat het cac bit do,
 * roi phat mot dieu kien STOP.
 */
static void release_stuck_bus(void)
{
    uint32_t clock;

    pins_as_gpio_od();
    recovery_half_period();

    /* Kiem tra neu SDA dang bi slave giu o muc LOW */
    if (HAL_GPIO_ReadPin(IMU_I2C_PORT, IMU_SDA_PIN) == GPIO_PIN_RESET) {
        for (clock = 0U; clock < RECOVERY_CLOCKS; ++clock) {
            HAL_GPIO_WritePin(IMU_I2C_PORT, IMU_SCL_PIN, GPIO_PIN_RESET);
            recovery_half_period();
            HAL_GPIO_WritePin(IMU_I2C_PORT, IMU_SCL_PIN, GPIO_PIN_SET);
            recovery_half_period();
            if (HAL_GPIO_ReadPin(IMU_I2C_PORT, IMU_SDA_PIN) == GPIO_PIN_SET) {
                break;
            }
        }
    }

    /* Phat dieu kien STOP chuan: SCL=L -> SDA=L -> SCL=H -> SDA=H (SDA di len khi SCL o muc cao) */
    HAL_GPIO_WritePin(IMU_I2C_PORT, IMU_SCL_PIN, GPIO_PIN_RESET);
    recovery_half_period();
    HAL_GPIO_WritePin(IMU_I2C_PORT, IMU_SDA_PIN, GPIO_PIN_RESET);
    recovery_half_period();
    HAL_GPIO_WritePin(IMU_I2C_PORT, IMU_SCL_PIN, GPIO_PIN_SET);
    recovery_half_period();
    HAL_GPIO_WritePin(IMU_I2C_PORT, IMU_SDA_PIN, GPIO_PIN_SET);
    recovery_half_period();
}

static status_t configure_peripheral(void)
{
    /* Reset ngoai vi truoc khi cau hinh: I2C1 cua F1 co the ket o trang thai
     * BUSY sau mot lan loi bus va chi thoat duoc bang SWRST. */
    __HAL_RCC_I2C1_FORCE_RESET();
    __HAL_RCC_I2C1_RELEASE_RESET();

    bus.Instance = IMU_I2C;
    bus.Init.ClockSpeed = bus_hz;
    bus.Init.DutyCycle = I2C_DUTYCYCLE_2;
    bus.Init.OwnAddress1 = 0U;
    bus.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    bus.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    bus.Init.OwnAddress2 = 0U;
    bus.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    bus.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    return (HAL_I2C_Init(&bus) == HAL_OK) ? STATUS_OK : STATUS_ERROR;
}

status_t i2c_init(const i2c_config_t *config)
{
    if (config == NULL || config->bus_hz == 0U || config->bus_hz > 400000U) {
        return STATUS_ERROR;
    }
    initialized = false;
    bus_hz = config->bus_hz;

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_I2C1_CLK_ENABLE();

    /* Giai phong bus TRUOC khi giao chan cho ngoai vi: sau khi da o che do AF,
     * MCU khong con tu lai duoc SCL nua. */
    release_stuck_bus();
    pins_as_i2c();

    if (configure_peripheral() != STATUS_OK) { return STATUS_ERROR; }
    initialized = true;
    return STATUS_OK;
}

/* Sau moi giao dich that bai.
 * NACK (AF) la ket qua BINH THUONG khi thiet bi vang mat hoac chua san sang:
 * chi can xoa co, khong dung cham gi den bus. Chi khi bus ket o BUSY moi phai
 * phat xung giai phong va dung lai ngoai vi — viec do ton vai chuc us nen
 * khong duoc lam o moi lan loi.
 */
static void recover_after_fault(void)
{
    __HAL_I2C_CLEAR_FLAG(&bus, I2C_FLAG_AF);

    if (__HAL_I2C_GET_FLAG(&bus, I2C_FLAG_BUSY) != RESET) {
        (void)HAL_I2C_DeInit(&bus);
        release_stuck_bus();
        pins_as_i2c();
        initialized = (configure_peripheral() == STATUS_OK);
    }
}

static status_t map_hal_status(HAL_StatusTypeDef result)
{
    if (result == HAL_OK) { return STATUS_OK; }
    recover_after_fault();
    return (result == HAL_TIMEOUT) ? STATUS_TIMEOUT : STATUS_ERROR;
}

/* Neu mot lan recover_after_fault() truoc do khong dung lai duoc ngoai vi,
 * `initialized` con la false. Khong co cho nay thi bus chet vinh vien: moi lan
 * goi sau se tra NOT_READY ma khong ai thu cau hinh lai. Thu dung mot lan moi
 * lan goi — co gioi han, khong lap vo han.
 */
static bool ensure_ready(void)
{
    if (!initialized) {
        initialized = (configure_peripheral() == STATUS_OK);
    }
    return initialized;
}

status_t i2c_read(uint8_t address, uint8_t reg, uint8_t *data, size_t length, uint32_t timeout_ms)
{
    if (address > 0x7FU || data == NULL || length == 0U || length > UINT16_MAX ||
        timeout_ms == 0U) {
        return STATUS_ERROR;
    }
    if (bus_hz == 0U || !ensure_ready()) { return STATUS_NOT_READY; }

    /* HAL nhan dia chi da dich trai 1 bit; bit R/W do chinh HAL dat. */
    return map_hal_status(HAL_I2C_Mem_Read(&bus, (uint16_t)((uint16_t)address << 1U),
                                           reg, I2C_MEMADD_SIZE_8BIT,
                                           data, (uint16_t)length, timeout_ms));
}

status_t i2c_write(uint8_t address, uint8_t reg, const uint8_t *data, size_t length,
                   uint32_t timeout_ms)
{
    if (address > 0x7FU || data == NULL || length == 0U || length > UINT16_MAX ||
        timeout_ms == 0U) {
        return STATUS_ERROR;
    }
    if (bus_hz == 0U || !ensure_ready()) { return STATUS_NOT_READY; }

    /* HAL_I2C_Mem_Write khong sua buffer nhung nhan con tro khong const. */
    return map_hal_status(HAL_I2C_Mem_Write(&bus, (uint16_t)((uint16_t)address << 1U),
                                            reg, I2C_MEMADD_SIZE_8BIT,
                                            (uint8_t *)(uintptr_t)data,
                                            (uint16_t)length, timeout_ms));
}

/* Dem so lan ISR chay khi khong ai bat ngat I2C1. Chi de chan doan. */
volatile uint32_t g_i2c_spurious_irq;

/* O che do bounded-blocking hien tai, ngat cua I2C1 KHONG duoc bat trong NVIC,
 * nen hai ham nay khong chay. Chung ton tai lam luoi an toan: neu ai do bat
 * ngat ma chua chuyen driver sang che do IT, co EV/ERR khong tu xoa se sinh
 * interrupt storm. Ghi nhan, tat nguon ngat, tat NVIC — hong mot cach quan sat
 * duoc thay vi treo im lang.
 */
static void shutdown_spurious_irq(void)
{
    ++g_i2c_spurious_irq;
    IMU_I2C->CR2 &= ~(uint32_t)(I2C_CR2_ITEVTEN | I2C_CR2_ITBUFEN | I2C_CR2_ITERREN);
    IMU_I2C->SR1 = 0U;
    HAL_NVIC_DisableIRQ(I2C1_EV_IRQn);
    HAL_NVIC_DisableIRQ(I2C1_ER_IRQn);
}

void i2c_irq_event(void)
{
    shutdown_spurious_irq();
}

void i2c_irq_error(void)
{
    shutdown_spurious_irq();
}
