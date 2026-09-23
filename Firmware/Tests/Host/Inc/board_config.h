#ifndef TEST_BOARD_CONFIG_H
#define TEST_BOARD_CONFIG_H
#include <stdint.h>
typedef struct { uint32_t unused; } GPIO_TypeDef;
typedef struct {
    uint32_t Pin, Mode, Speed, Pull;
} GPIO_InitTypeDef;
typedef struct {
    uint32_t BaudRate, WordLength, StopBits, Parity, Mode, HwFlowCtl,
             OverSampling;
} UART_InitTypeDef;
typedef struct {
    void *Instance;
    UART_InitTypeDef Init;
} UART_HandleTypeDef;
typedef enum { HAL_OK, HAL_ERROR, HAL_TIMEOUT } HAL_StatusTypeDef;
#define RESET 0U
#define DEBUG_UART ((void *)1)
#define DEBUG_UART_PORT ((GPIO_TypeDef *)2)
#define DEBUG_TX_PIN 0x200U
#define DEBUG_RX_PIN 0x400U
#define GPIO_MODE_AF_PP 1U
#define GPIO_MODE_INPUT 2U
#define GPIO_SPEED_FREQ_HIGH 3U
#define GPIO_NOPULL 0U
#define UART_WORDLENGTH_8B 0U
#define UART_STOPBITS_1 0U
#define UART_PARITY_NONE 0U
#define UART_MODE_TX_RX 0U
#define UART_HWCONTROL_NONE 0U
#define UART_OVERSAMPLING_16 0U
#define UART_FLAG_ORE 1U
#define UART_FLAG_RXNE 2U
#define __HAL_RCC_GPIOA_CLK_ENABLE() ((void)0)
#define __HAL_RCC_USART1_CLK_ENABLE() ((void)0)
#define __HAL_UART_GET_FLAG(handle, flag) (0U)
#define __HAL_UART_CLEAR_OREFLAG(handle) ((void)0)
void HAL_GPIO_Init(GPIO_TypeDef *port, const GPIO_InitTypeDef *pins);
HAL_StatusTypeDef HAL_UART_Init(UART_HandleTypeDef *handle);
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *handle,
                                    const uint8_t *data, uint16_t length,
                                    uint32_t timeout_ms);
HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef *handle, uint8_t *data,
                                   uint16_t length, uint32_t timeout_ms);
#define IMU_ADDRESS_7BIT 0x68U
#define BUZZER_PORT ((GPIO_TypeDef *)1)
#define BUZZER_PIN 0x1000U
#endif
