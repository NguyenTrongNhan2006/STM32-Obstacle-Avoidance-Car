#ifndef UART_DEBUG_H
#define UART_DEBUG_H
#include "project_types.h"
typedef struct { uint32_t baud; uint32_t timeout_ms; } uart_debug_config_t;
status_t uart_debug_init(const uart_debug_config_t *config);
status_t uart_debug_write(const uint8_t *data, size_t length, uint32_t timeout_ms);
status_t uart_debug_read(uint8_t *data, size_t capacity, uint32_t timeout_ms);
/* Single writer tLog (or pre-scheduler boot); task context only, no shared formatter. */
status_t uart_log(const char *message);
status_t uart_log_u32(const char *label, uint32_t value);
#endif
