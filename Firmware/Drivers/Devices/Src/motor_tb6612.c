#include "motor_tb6612.h"
#include "gpio.h"
static bool (*safety_check)(void);
status_t motor_init(const motor_config_t *config)
{
    gpio_emergency_stop();
    safety_check = NULL;
    if (config == NULL || config->is_safe == NULL) { return STATUS_ERROR; }
    safety_check = config->is_safe;
    /* IMPLEMENT: initialize pwm and direction state while STBY remains low. */
    return STATUS_NOT_READY;
}
status_t motor_apply(motor_cmd_t command, uint8_t speed)
{
    gpio_emergency_stop();
    if ((unsigned)command > MOTOR_TURN_RIGHT || speed > 100U) { return STATUS_ERROR; }
    if (safety_check == NULL || !safety_check()) { return STATUS_NOT_READY; }
    /* IMPLEMENT: fault-dominant command arbitration and safe direction changes.
     * TO DO: resolve brake/coast truth table before enabling STBY.
     */
    return STATUS_NOT_READY;
}
