#ifndef TIMEBASE_H
#define TIMEBASE_H
#include "project_types.h"
typedef struct { uint32_t capture_tick_hz; } timebase_config_t;
status_t timebase_init(const timebase_config_t *config);
uint32_t timebase_now_ms(void);
/* Bat dau mot phep do moi: xoa ket qua cu, dat lai bo dem, mo ngat capture.
 * Phai goi NGAY TRUOC khi kich xung Trig. Khong co ham nay thi
 * timebase_capture_us() khong the phan biet ket qua moi voi ket qua con sot
 * lai cua lan do truoc.
 */
status_t timebase_capture_arm(void);
/* KHONG block. Tra STATUS_NOT_READY khi phep do dang chay, STATUS_OK kem do
 * rong xung khi xong, STATUS_TIMEOUT khi het han hoac bo dem tran.
 */
status_t timebase_capture_us(uint32_t *pulse_us, uint32_t timeout_us);
/* Diem vao ISR cua TIM2 — timebase la chu so huu duy nhat cua timer nay.
 * stm32f1xx_it.c chi dinh tuyen, khong tu doc/ghi thanh ghi TIM2.
 */
void timebase_irq_capture(void);
#endif
