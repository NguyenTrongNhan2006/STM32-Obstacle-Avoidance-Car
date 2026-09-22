#include "uart_debug.h"
#include "board_config.h"
static UART_HandleTypeDef uart;
static uint32_t log_timeout_ms;
static bool initialized;
status_t uart_debug_init(const uart_debug_config_t *config)
{
    GPIO_InitTypeDef pins = {0};
    if (config == NULL || config->baud == 0U || config->timeout_ms == 0U) { return STATUS_ERROR; }
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();
    pins.Pin = DEBUG_TX_PIN;
    pins.Mode = GPIO_MODE_AF_PP;
    pins.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DEBUG_UART_PORT, &pins);
    pins.Pin = DEBUG_RX_PIN;
    pins.Mode = GPIO_MODE_INPUT;
    pins.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(DEBUG_UART_PORT, &pins);
    uart.Instance = DEBUG_UART;
    uart.Init.BaudRate = config->baud;
    uart.Init.WordLength = UART_WORDLENGTH_8B;
    uart.Init.StopBits = UART_STOPBITS_1;
    uart.Init.Parity = UART_PARITY_NONE;
    uart.Init.Mode = UART_MODE_TX_RX;
    uart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    uart.Init.OverSampling = UART_OVERSAMPLING_16;
    initialized = HAL_UART_Init(&uart) == HAL_OK;
    log_timeout_ms = config->timeout_ms;
    return initialized ? STATUS_OK : STATUS_ERROR;
}
status_t uart_debug_write(const uint8_t *data, size_t length, uint32_t timeout_ms)
{
    HAL_StatusTypeDef result;
    if (data == NULL || length == 0U || length > UINT16_MAX || timeout_ms == 0U) { return STATUS_ERROR; }
    if (!initialized) { return STATUS_NOT_READY; }
    result = HAL_UART_Transmit(&uart, data, (uint16_t)length, timeout_ms);
    return result == HAL_OK ? STATUS_OK : (result == HAL_TIMEOUT ? STATUS_TIMEOUT : STATUS_ERROR);
}
/* Nhan lenh qua UART.
 *
 * Khong block khi duong truyen im: neu chua co byte nao trong thanh ghi nhan
 * thi tra STATUS_NOT_READY ngay, khong cho het timeout_ms. Chi khi da bat dau
 * co du lieu ham moi cho cho du `capacity` byte, va cho co bien theo timeout_ms.
 * Nho vay nguoi goi co the poll moi chu ky ma khong dot thoi gian CPU.
 *
 * GIOI HAN: van la polling, khong co ring buffer. Byte den trong luc khong ai
 * goi ham nay se bi mat, va HAL bao ORE. Mot giao thuc lenh that su phai dung
 * ngat hoac DMA kem ring buffer — khi do doi sang HAL_UART_Receive_IT va giai
 * phong USART1_IRQHandler khoi bay UNIMPLEMENTED_IRQ.
 */
status_t uart_debug_read(uint8_t *data, size_t capacity, uint32_t timeout_ms)
{
    HAL_StatusTypeDef result;

    if (data == NULL || capacity == 0U || capacity > UINT16_MAX || timeout_ms == 0U) {
        return STATUS_ERROR;
    }
    if (!initialized) { return STATUS_NOT_READY; }

    /* Overrun lam co ORE dinh lai va moi lan doc sau do deu bao loi. Xoa truoc
     * khi thu doc, neu khong mot lan tran duy nhat se lam duong nhan chet han. */
    if (__HAL_UART_GET_FLAG(&uart, UART_FLAG_ORE) != RESET) {
        __HAL_UART_CLEAR_OREFLAG(&uart);
    }
    if (__HAL_UART_GET_FLAG(&uart, UART_FLAG_RXNE) == RESET) {
        return STATUS_NOT_READY;   /* duong truyen im — thoat ngay, khong cho */
    }

    result = HAL_UART_Receive(&uart, data, (uint16_t)capacity, timeout_ms);
    if (result == HAL_OK) { return STATUS_OK; }
    /* Nhan thieu byte tinh la TIMEOUT: nguoi goi khong biet duoc bao nhieu byte
     * da vao buffer, nen khong duoc phep dung du lieu do. */
    return (result == HAL_TIMEOUT) ? STATUS_TIMEOUT : STATUS_ERROR;
}
status_t uart_log(const char *message)
{
    size_t length = 0U;
    if (message == NULL) { return STATUS_ERROR; }
    while (length < 128U && message[length] != '\0') { ++length; }
    if (length == 128U) { return STATUS_ERROR; }
    if (length == 0U) { return initialized ? STATUS_OK : STATUS_NOT_READY; }
    return uart_debug_write((const uint8_t *)message, length, log_timeout_ms);
}
status_t uart_log_u32(const char *label, uint32_t value)
{
    char digits[10];
    char output[12];
    size_t count = 0U;
    size_t index;
    status_t result = uart_log(label);
    if (result != STATUS_OK) { return result; }
    do { digits[count++] = (char)('0' + value % 10U); value /= 10U; } while (value != 0U);
    for (index = 0U; index < count; ++index) { output[index] = digits[count - 1U - index]; }
    output[count++] = '\r';
    output[count++] = '\n';
    return uart_debug_write((const uint8_t *)output, count, log_timeout_ms);
}
