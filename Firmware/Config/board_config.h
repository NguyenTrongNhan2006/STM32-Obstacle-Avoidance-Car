#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include "stm32f1xx_hal.h"

/* ===================== Debug LED ===================== */
#define LED_DEBUG_GPIO_Port            GPIOC
#define LED_DEBUG_Pin                  GPIO_PIN_13

/* ===================== Debug UART (USART1) ===================== */
#define UART_DEBUG_INSTANCE            USART1
#define UART_DEBUG_BAUDRATE            115200U
#define UART_DEBUG_TX_GPIO_Port        GPIOA
#define UART_DEBUG_TX_Pin              GPIO_PIN_9
#define UART_DEBUG_RX_GPIO_Port        GPIOA
#define UART_DEBUG_RX_Pin              GPIO_PIN_10

/* ===================== Motor PWM (TIM2) =====================
 * Left wheel  -> TIM2_CH1 (PA0)
 * Right wheel -> TIM2_CH2 (PA1)
 * Timer clock assumed 72 MHz (APB1 = 36 MHz, TIMx clock x2 when APB1
 * prescaler != 1). Re-check against the actual SystemClock_Config().
 * PSC=71, ARR=999 -> 72e6 / 72 / 1000 = 1 kHz PWM, duty steps 0..999.
 */
#define MOTOR_PWM_TIM_INSTANCE         TIM2
#define MOTOR_PWM_PRESCALER            71U
#define MOTOR_PWM_PERIOD               999U

#define MOTOR_LEFT_PWM_CHANNEL         TIM_CHANNEL_1
#define MOTOR_LEFT_PWM_GPIO_Port       GPIOA
#define MOTOR_LEFT_PWM_Pin             GPIO_PIN_0

#define MOTOR_RIGHT_PWM_CHANNEL        TIM_CHANNEL_2
#define MOTOR_RIGHT_PWM_GPIO_Port      GPIOA
#define MOTOR_RIGHT_PWM_Pin            GPIO_PIN_1

/* Direction pins (H-bridge IN1/IN2 per wheel). Left/right IN1/IN2
 * assignment below is an assumption - confirm against actual wiring
 * before relying on it.
 */
#define MOTOR_LEFT_IN1_GPIO_Port       GPIOB
#define MOTOR_LEFT_IN1_Pin             GPIO_PIN_0
#define MOTOR_LEFT_IN2_GPIO_Port       GPIOB
#define MOTOR_LEFT_IN2_Pin             GPIO_PIN_1

#define MOTOR_RIGHT_IN1_GPIO_Port      GPIOB
#define MOTOR_RIGHT_IN1_Pin            GPIO_PIN_10
#define MOTOR_RIGHT_IN2_GPIO_Port      GPIOB
#define MOTOR_RIGHT_IN2_Pin            GPIO_PIN_11

/* ===================== HC-SR04 ===================== */
#define HCSR04_TRIG_GPIO_Port          GPIOB
#define HCSR04_TRIG_Pin                GPIO_PIN_8

#define HCSR04_ECHO_TIM_INSTANCE       TIM1
#define HCSR04_ECHO_TIM_CHANNEL        TIM_CHANNEL_1
#define HCSR04_ECHO_GPIO_Port          GPIOA
#define HCSR04_ECHO_Pin                GPIO_PIN_8

#endif /* BOARD_CONFIG_H */
