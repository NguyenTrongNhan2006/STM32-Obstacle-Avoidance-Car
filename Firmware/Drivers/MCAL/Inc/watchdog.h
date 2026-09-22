#ifndef WATCHDOG_H
#define WATCHDOG_H
#include "project_types.h"
typedef struct { uint32_t timeout_ms; } watchdog_config_t;
/* IWDG doc lap voi PLL: no chay bang LSI nen van dem ca khi clock tree hong.
 *
 * MOT CHIEU: da khoi tao thi KHONG THE tat bang phan mem. Chi co reset moi dung
 * duoc no. Vi vay watchdog_init() phai la buoc khoi tao CUOI CUNG truoc khi tao
 * task — moi thu chay sau no deu phai tu lo refresh dung han.
 */
status_t watchdog_init(const watchdog_config_t *config);
/* Chi chu so huu duong an toan duoc goi. Xem safety_monitor.c. */
status_t watchdog_refresh(void);
/* Lan reset vua roi co phai do IWDG khong. Doc mot lan luc khoi tao roi xoa co,
 * nen goi sau watchdog_init() moi co gia tri dung.
 */
bool watchdog_caused_last_reset(void);
#endif
