#ifndef MAIN_H
#define MAIN_H
#include <stdint.h>
extern volatile uint32_t g_assert_line;
extern const char * volatile g_assert_file;
void SystemClock_Config(void);
#endif
