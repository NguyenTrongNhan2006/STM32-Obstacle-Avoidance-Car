#include "buzzer.h"
status_t buzzer_init(const buzzer_config_t *config)
{
    if (config == NULL) { return STATUS_ERROR; }
    /* IMPLEMENT: confirm active/passive type and transistor circuit first. */
    return STATUS_NOT_READY;
}
status_t buzzer_set(buzzer_pattern_t pattern)
{
    if ((unsigned)pattern > BUZZER_FAULT) { return STATUS_ERROR; }
    /* IMPLEMENT: select pattern, never block on its duration. */
    return STATUS_NOT_READY;
}
status_t buzzer_update(uint32_t now_ms)
{
    (void)now_ms;
    /* IMPLEMENT: advance pattern deadline in tBuzzer. */
    return STATUS_NOT_READY;
}
