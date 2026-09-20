#ifndef BUZZER_H
#define BUZZER_H
#include "project_types.h"
typedef enum { BUZZER_SILENT, BUZZER_START, BUZZER_OBSTACLE, BUZZER_FAULT } buzzer_pattern_t;
typedef struct { bool active_high; } buzzer_config_t;
status_t buzzer_init(const buzzer_config_t *config);
status_t buzzer_set(buzzer_pattern_t pattern);
status_t buzzer_update(uint32_t now_ms);
#endif
