#ifndef BUTTON_H
#define BUTTON_H
#include "project_types.h"
typedef struct { uint32_t debounce_ms; } button_config_t;
status_t button_init(const button_config_t *config);
status_t button_read(bool *pressed);
#endif
