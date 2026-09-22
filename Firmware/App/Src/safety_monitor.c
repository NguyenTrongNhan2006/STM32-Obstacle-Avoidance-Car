#include "safety_monitor.h"
#include "app_config.h"
#include "button.h"
#include "sensor_manager.h"
#include "watchdog.h"

/* Cua so giam sat task-alive. Phai thoa DONG THOI hai rang buoc:
 *   - LON HON chu ky cua task cham nhat duoc giam sat (tDecision, 20 ms), neu
 *     khong se reset oan mot task van khoe.
 *   - NHO HON NHIEU so voi can duoi cua timeout IWDG (333 ms khi LSI chay o
 *     60 kHz — xem watchdog.c), neu khong se reset oan khi moi thu binh thuong.
 * 100 ms cho bien 5 lan o can tren va 3,3 lan o can duoi.
 */
#define ALIVE_WINDOW_MS 100U

EventGroupHandle_t egSafety;
EventGroupHandle_t egAlive;
static StaticEventGroup_t safety_storage;
static StaticEventGroup_t alive_storage;
static uint32_t alive_window_ms;
/* An unknown button state must never look like a release that arms the next
 * press, so the edge detector starts as if the button were already held.
 */
static bool button_was_pressed = true;

static bool sample_fresh(uint32_t timestamp_ms, uint32_t now_ms)
{
    return (uint32_t)(now_ms - timestamp_ms) <= SENSOR_STALE_MS;
}

/* Scale-independent tilt test: cos^2(tilt) = az^2 / (ax^2 + ay^2 + az^2), so the
 * unconfirmed accelerometer full-scale range cancels out and no raw-to-degree
 * conversion is invented here.
 * [DO] assumes accel_raw[2] is the vertical axis and that a level car reads it
 * positive; confirm the mounting orientation before trusting this.
 */
static bool imu_upright(const imu_sample_t *imu)
{
    const int32_t ax = imu->accel_raw[0];
    const int32_t ay = imu->accel_raw[1];
    const int32_t az = imu->accel_raw[2];
    uint64_t vertical;
    uint64_t total;

    if (az <= 0) { return false; }
    vertical = (uint64_t)((int64_t)az * az);
    total = vertical + (uint64_t)((int64_t)ax * ax) + (uint64_t)((int64_t)ay * ay);
    if (total == 0U) { return false; }
    return (vertical * TILT_COS2_DEN) >= (total * TILT_COS2_NUM);
}

status_t safety_init(void)
{
    const button_config_t button = { .debounce_ms = BUTTON_DEBOUNCE_MS };

    if (egSafety == NULL) {
        egSafety = xEventGroupCreateStatic(&safety_storage);
        if (egSafety == NULL) { return STATUS_ERROR; }
        (void)xEventGroupSetBits(egSafety, SAFETY_BIT_STOP | SAFETY_BIT_SENSOR_FAULT);
    }
    if (egAlive == NULL) {
        egAlive = xEventGroupCreateStatic(&alive_storage);
        if (egAlive == NULL) { return STATUS_ERROR; }
    }
    alive_window_ms = 0U;
    /* A button that fails to initialise only removes the rearm path. The boot
     * inhibit stays latched, so the car still cannot run.
     */
    (void)button_init(&button);
    return STATUS_OK;
}

void safety_check_in(EventBits_t task_bit)
{
    if (egAlive == NULL || (task_bit & ~ALIVE_ALL_MASK) != 0U) { return; }
    (void)xEventGroupSetBits(egAlive, task_bit);
}

/* DIEM DUY NHAT trong toan bo firmware duoc phep refresh watchdog, va chi khi
 * DA CHUNG MINH ca hai task duoc giam sat con chay trong cua so vua qua.
 *
 * Thieu bit thi khong lam gi ca — khong log, khong co gang cuu van. Chinh viec
 * KHONG refresh la hanh dong: IWDG het gio va reset MCU, dua STBY ve floating
 * de dien tro keo xuong 10k ngoai giu TB6612 o standby.
 */
static void supervise_tasks(uint32_t now_ms)
{
    EventBits_t checked_in;

    if (egAlive == NULL) { return; }
    if ((uint32_t)(now_ms - alive_window_ms) < ALIVE_WINDOW_MS) { return; }
    alive_window_ms = now_ms;

    /* xEventGroupClearBits tra ve gia tri TRUOC khi xoa, nen doc va dat lai cua
     * so la mot thao tac nguyen tu — khong co khe de mat mot lan check-in. */
    checked_in = xEventGroupClearBits(egAlive, ALIVE_ALL_MASK);
    if ((checked_in & ALIVE_ALL_MASK) == ALIVE_ALL_MASK) {
        (void)watchdog_refresh();
    }
}

/* STATUS_OK means the policy ran, never that the car is safe; ask
 * safety_is_clear_to_run() for that.
 */
status_t safety_update(uint32_t now_ms)
{
    sample_t range;
    imu_sample_t imu;
    bool range_ok = false;
    bool imu_ok = false;
    bool upright = false;
    bool pressed = false;

    /* Giam sat task chay TRUOC va khong phu thuoc phan con lai: neu egSafety
     * chua ton tai thi ham nay thoat som, va khi do watchdog KHONG duoc refresh
     * — dung huong, vi mot he thong chua khoi tao xong khong the tu tuyen bo la
     * khoe. */
    supervise_tasks(now_ms);

    if (egSafety == NULL) { return STATUS_NOT_READY; }

    if (sensor_manager_get_latest(&range, &imu) == STATUS_OK) {
        range_ok = (range.status == SAMPLE_OK) && sample_fresh(range.timestamp_ms, now_ms);
        imu_ok = (imu.status == SAMPLE_OK) && sample_fresh(imu.timestamp_ms, now_ms);
        if (imu_ok) { upright = imu_upright(&imu); }
    }

    /* Faults latch. Absent, NOT_READY or stale data is evidence of a fault and
     * never evidence of health, so nothing below clears a bit on its own.
     */
    if (!range_ok || !imu_ok) {
        (void)xEventGroupSetBits(egSafety, SAFETY_BIT_SENSOR_FAULT);
    }
    if (imu_ok && !upright) {
        (void)xEventGroupSetBits(egSafety, SAFETY_BIT_TILT_FAULT);
    }

    if (button_read(&pressed) == STATUS_OK) {
        if (pressed && !button_was_pressed) {
            if ((xEventGroupGetBits(egSafety) & SAFETY_INHIBIT_MASK) != 0U) {
                /* Deliberate rearm: release only the inhibits currently proven
                 * absent. A press alone never clears an unproven fault.
                 */
                EventBits_t clear = SAFETY_BIT_STOP;
                if (range_ok && imu_ok) { clear |= SAFETY_BIT_SENSOR_FAULT; }
                if (imu_ok && upright) { clear |= SAFETY_BIT_TILT_FAULT; }
                (void)xEventGroupClearBits(egSafety, clear);
            } else {
                (void)xEventGroupSetBits(egSafety, SAFETY_BIT_STOP);
            }
        }
        button_was_pressed = pressed;
    } else {
        button_was_pressed = true;
    }
    return STATUS_OK;
}

bool safety_is_clear_to_run(void)
{
    return egSafety != NULL && (xEventGroupGetBits(egSafety) & SAFETY_INHIBIT_MASK) == 0U;
}
