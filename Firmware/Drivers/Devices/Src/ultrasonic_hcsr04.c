#include "ultrasonic_hcsr04.h"
status_t ultrasonic_init(const ultrasonic_config_t *config)
{
    if (config == NULL || config->echo_timeout_us == 0U || config->sample_period_ms == 0U) { return STATUS_ERROR; }
    /* IMPLEMENT: timebase capture + trigger state machine, no busy wait. */
    return STATUS_NOT_READY;
}
status_t ultrasonic_request(void)
{
    /* IMPLEMENT: one bounded measurement, reject overlap and respect retrigger interval. */
    return STATUS_NOT_READY;
}
status_t ultrasonic_read(sample_t *sample)
{
    if (sample == NULL) { return STATUS_ERROR; }
    *sample = (sample_t){ .unit = SAMPLE_UNIT_MM, .status = SAMPLE_NOT_READY };
    /* IMPLEMENT: publish capture timestamp; timeout is never valid open space. */
    return STATUS_NOT_READY;
}
