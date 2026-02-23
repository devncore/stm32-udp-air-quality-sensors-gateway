/**
 * @file hal_interface.c
 * @brief HAL Interface implementation (opaque pointer pattern)
 *
 * Bridges abstract driver interfaces to STM32 HAL.
 *
 * UART RX strategy:
 *   - Blocking HAL_UART_Receive is used for all reception.
 *   - No interrupts, no ring buffer. The caller specifies the number
 *     of bytes to read and a timeout; the HAL polls the RXNE flag
 *     internally and returns when all bytes arrive or the timeout
 *     expires. Partial reads (timeout with some bytes received)
 *     return the actual byte count.
 */

#include "app/hal_interface.h"

#include <string.h>

/*============================================================================
 * STM32 UART - internal struct (hidden from callers)
 *============================================================================*/

struct stm32_uart {
    UART_HandleTypeDef* huart;
};

/* Single static instance (only one UART used for ESP8266) */
static struct stm32_uart g_uart_instance;

/*============================================================================
 * Public API
 *============================================================================*/

stm32_uart_t* stm32_uart_init(UART_HandleTypeDef* huart)
{
    g_uart_instance.huart = huart;
    return &g_uart_instance;
}

int32_t stm32_uart_transmit(stm32_uart_t* self, const uint8_t* data, size_t len)
{
    HAL_StatusTypeDef status = HAL_UART_Transmit(
        self->huart,
        (uint8_t*)data,
        (uint16_t)len,
        1000);
    return status == HAL_OK ? (int32_t)len : -1;
}

int32_t stm32_uart_receive(stm32_uart_t* self, uint8_t* buffer, size_t len,
                            uint32_t timeout_ms)
{
    HAL_StatusTypeDef status = HAL_UART_Receive(
        self->huart,
        buffer,
        (uint16_t)len,
        timeout_ms);

    if (status == HAL_OK) {
        return (int32_t)len;
    }

    if (status == HAL_TIMEOUT) {
        /* Return however many bytes actually arrived before the timeout */
        uint16_t received = (uint16_t)len - self->huart->RxXferCount;
        return received > 0 ? (int32_t)received : -1;
    }

    return -1;
}

void stm32_uart_flush_rx(stm32_uart_t* self)
{
    uint8_t dummy;
    /* Drain any pending bytes from the hardware data register */
    while (HAL_UART_Receive(self->huart, &dummy, 1, 10) == HAL_OK) {
        /* discard */
    }
}

void stm32_uart_delay_ms(stm32_uart_t* self, uint32_t ms)
{
    (void)self;
    HAL_Delay(ms);
}
