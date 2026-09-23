#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H
#include "stm32f1xx_hal.h"
/* [CO DINH] STM32F103C8T6: 64 KiB Flash, 20 KiB SRAM, maximum clock 72 MHz.
 * [DO] Proposed Blue Pill pin map: confirm schematic, crystal and module variants.
 * HSE assumed 8 MHz. SWD PA13/PA14 and oscillator pins stay reserved.
 * No side IR sensors. HC-SR04 Echo MUST be reduced to 3.3 V externally;
 * PA0 is not assumed 5 V tolerant. Do not move I2C just to avoid level shifting.
 * Add external 10k pull-down on TB6612 STBY; firmware alone cannot protect reset.
 */
#define BOARD_HSE_HZ 8000000UL
#define BOARD_SYSCLK_HZ 72000000UL
#define MOTOR_PWM_PORT GPIOA
#define MOTOR_LEFT_PWM_PIN GPIO_PIN_6
#define MOTOR_RIGHT_PWM_PIN GPIO_PIN_7
#define MOTOR_PWM_TIMER TIM3
#define MOTOR_LEFT_PWM_CHANNEL 1U
#define MOTOR_RIGHT_PWM_CHANNEL 2U
#define MOTOR_PWM_HZ 20000UL /* [DO] validate driver, noise and motor current */
#define MOTOR_AIN1_PORT GPIOB
#define MOTOR_AIN1_PIN GPIO_PIN_0
#define MOTOR_AIN2_PORT GPIOB
#define MOTOR_AIN2_PIN GPIO_PIN_1
#define MOTOR_BIN1_PORT GPIOB
#define MOTOR_BIN1_PIN GPIO_PIN_10
#define MOTOR_BIN2_PORT GPIOB
#define MOTOR_BIN2_PIN GPIO_PIN_11
#define MOTOR_STBY_PORT GPIOB
#define MOTOR_STBY_PIN GPIO_PIN_5
#define RANGE_TRIG_PORT GPIOA
#define RANGE_TRIG_PIN GPIO_PIN_1
#define RANGE_ECHO_PORT GPIOA
#define RANGE_ECHO_PIN GPIO_PIN_0
#define RANGE_CAPTURE_TIMER TIM2
#define RANGE_CAPTURE_CHANNEL 1U
#define IMU_I2C I2C1
#define IMU_I2C_PORT GPIOB
#define IMU_SCL_PIN GPIO_PIN_6
#define IMU_SDA_PIN GPIO_PIN_7
#define IMU_ADDRESS_7BIT 0x68U /* [DO] verify AD0 and board pull-ups at 3.3 V */
/* [DO] Fast mode. Reachable speed depends on the actual pull-ups and wire
 * length, not on this constant. Drop to 100000 first if the bus is unreliable;
 * a bus that only works at 100 kHz is a wiring finding, not a firmware setting.
 */
#define IMU_I2C_SPEED_HZ 100000UL
#define BUTTON_PORT GPIOA
#define BUTTON_PIN GPIO_PIN_8
#define BUTTON_ACTIVE_LEVEL GPIO_PIN_RESET
#define STATUS_LED_PORT GPIOC
#define STATUS_LED_PIN GPIO_PIN_13
#define STATUS_LED_ACTIVE_LEVEL GPIO_PIN_RESET
#define BUZZER_PORT GPIOB
#define BUZZER_PIN GPIO_PIN_12 /* [DO] active buzzer + suitable transistor stage */
#define DEBUG_UART USART1
#define DEBUG_UART_PORT GPIOA
#define DEBUG_TX_PIN GPIO_PIN_9
#define DEBUG_RX_PIN GPIO_PIN_10
#define DEBUG_BAUD 115200UL
/* Owners: pwm -> TIM3; timebase -> TIM2 capture;
 * i2c -> I2C1; uart_debug -> USART1; exti -> EXTI8; TIM4 spare.
 * SysTick -> kernel + HAL tick wrapper. No timer/bus may have two owners.
 * [CO DINH] CMSIS priorities: smaller number is more urgent.
 * ISR using RTOS FromISR APIs must use a number >= 5 (and <= 15).
 * TIM2 va EXTI8 da duoc bat NVIC trong module so huu; I2C van polling.
 */
#define IRQ_PRIO_ECHO 5U
#define IRQ_PRIO_BUTTON 5U
#define IRQ_PRIO_I2C 6U
#define IRQ_PRIO_UART 7U
#define IRQ_PRIO_SYSTICK 15U
#endif
