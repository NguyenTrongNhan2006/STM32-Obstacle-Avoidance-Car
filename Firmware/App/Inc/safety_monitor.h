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
status_t safety_init(void);
status_t safety_update(uint32_t now_ms);
bool safety_is_clear_to_run(void);
#endif
