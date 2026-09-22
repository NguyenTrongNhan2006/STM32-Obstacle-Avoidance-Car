#ifndef SAFETY_MONITOR_H
#define SAFETY_MONITOR_H
#include "project_types.h"
#include "FreeRTOS.h"
#include "event_groups.h"
#define SAFETY_BIT_STOP ((EventBits_t)1U << 0)
#define SAFETY_BIT_SENSOR_FAULT ((EventBits_t)1U << 1)
#define SAFETY_BIT_TILT_FAULT ((EventBits_t)1U << 2)
#define SAFETY_INHIBIT_MASK (SAFETY_BIT_STOP | SAFETY_BIT_SENSOR_FAULT | SAFETY_BIT_TILT_FAULT)
/* Read-only outside safety_monitor. Only safety owner may change/rearm these bits. */
extern EventGroupHandle_t egSafety;

/* Task-alive supervision. Chi giam sat hai task nam tren duong an toan:
 *   tSensor   — khong co mau moi thi moi quyet dinh sau do deu dua tren du lieu chet
 *   tDecision — khong chay thi khong ai goi motor_apply(), lenh cu o nguyen phan cung
 * tLog va tBuzzer KHONG duoc giam sat: chung treo thi xe van an toan, va dua
 * chung vao chi lam tang rui ro reset oan.
 * tSafety khong tu check-in — chinh no chay moi refresh duoc watchdog, nen no
 * treo la watchdog tu het gio.
 */
#define ALIVE_BIT_SENSOR ((EventBits_t)1U << 0)
#define ALIVE_BIT_DECISION ((EventBits_t)1U << 1)
#define ALIVE_ALL_MASK (ALIVE_BIT_SENSOR | ALIVE_BIT_DECISION)
extern EventGroupHandle_t egAlive;

status_t safety_init(void);
status_t safety_update(uint32_t now_ms);
bool safety_is_clear_to_run(void);
/* Moi task duoc giam sat goi mot lan moi vong lap voi bit cua rieng no. */
void safety_check_in(EventBits_t task_bit);
#endif
