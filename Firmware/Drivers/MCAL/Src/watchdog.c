#include "watchdog.h"
#include "stm32f1xx_hal.h"

/* IWDG dem xuong tu Reload o tan so LSI / prescaler:
 *     timeout = Reload * prescaler / LSI
 *
 * ============================================================================
 * LSI KHONG CHINH XAC — day la dieu quan trong nhat ve khoi nay
 * ============================================================================
 * LSI_VALUE = 40 kHz chi la gia tri DIEN HINH. Datasheet STM32F103 cho dai
 * 30..60 kHz theo linh kien va nhiet do. Nen mot timeout danh nghia 500 ms
 * thuc te nam trong khoang:
 *     LSI 60 kHz -> 333 ms   (som nhat)
 *     LSI 30 kHz -> 667 ms   (muon nhat)
 *
 * HE QUA: nhip refresh phai nam duoi can DUOI (333 ms) voi bien du rong, khong
 * phai duoi gia tri danh nghia. safety_monitor refresh moi 100 ms — bien 3,3 lan.
 * Dung suy luan tu con so 500 ms khi chon nhip refresh.
 *
 * Bo prescaler cua IWDG la 4, 8, 16, ..., 256 va Reload toi da 4095.
 */
#define IWDG_RELOAD_MAX 4095U
#define IWDG_PRESCALER_MIN_DIV 4U
#define IWDG_PRESCALER_MAX_DIV 256U

static IWDG_HandleTypeDef watchdog;
static bool last_reset_by_watchdog;
static bool initialized;

status_t watchdog_init(const watchdog_config_t *config)
{
    uint32_t divider;
    uint32_t prescaler_code;
    uint32_t reload = 0U;
    bool found = false;

    if (config == NULL || config->timeout_ms == 0U) { return STATUS_ERROR; }

    /* Doc nguyen nhan reset TRUOC khi xoa co. Co nay giu qua reset va chi mat
     * khi bi xoa hoac khi mat nguon, nen day la bang chung duy nhat cho biet
     * lan khoi dong nay den tu watchdog hay tu nut reset. */
    last_reset_by_watchdog = (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST) != RESET);
    __HAL_RCC_CLEAR_RESET_FLAGS();

    /* Chon prescaler NHO NHAT ma Reload van vua 12 bit: prescaler cang nho thi
     * buoc dem cang min, timeout dat duoc cang sat gia tri yeu cau. */
    prescaler_code = 0U;
    for (divider = IWDG_PRESCALER_MIN_DIV; divider <= IWDG_PRESCALER_MAX_DIV; divider *= 2U) {
        const uint32_t ticks = ((uint32_t)LSI_VALUE / 1000U) * config->timeout_ms / divider;
        if (ticks >= 1U && ticks <= IWDG_RELOAD_MAX) {
            reload = ticks;
            found = true;
            break;
        }
        ++prescaler_code;
    }
    if (!found) { return STATUS_ERROR; }

    watchdog.Instance = IWDG;
    watchdog.Init.Prescaler = prescaler_code;
    watchdog.Init.Reload = reload;
    /* HAL_IWDG_Init() bat LSI, ghi cau hinh va KHOI DONG watchdog ngay. Tu day
     * tro di khong con duong lui. */
    if (HAL_IWDG_Init(&watchdog) != HAL_OK) { return STATUS_ERROR; }

    initialized = true;
    return STATUS_OK;
}

status_t watchdog_refresh(void)
{
    if (!initialized) { return STATUS_NOT_READY; }
    return (HAL_IWDG_Refresh(&watchdog) == HAL_OK) ? STATUS_OK : STATUS_ERROR;
}

bool watchdog_caused_last_reset(void)
{
    return last_reset_by_watchdog;
}
