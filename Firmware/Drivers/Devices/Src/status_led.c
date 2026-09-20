#include "status_led.h"
status_t status_led_init(void)
{
    /* IMPLEMENT: use gpio with active-low polarity from board_config. */
    return STATUS_NOT_READY;
}
status_t status_led_set(led_pattern_t pattern)
{
    if ((unsigned)pattern > LED_FAULT) { return STATUS_ERROR; }
    /* IMPLEMENT: deadline-based indication. */
    return STATUS_NOT_READY;
}
