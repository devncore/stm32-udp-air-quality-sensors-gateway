/**
 * @file hal_interface.h
 * @brief HAL Interface for drivers (opaque pointer pattern)
 *
 * Provides UART abstraction bridging to the STM32 HAL.
 * Internal state is hidden behind an opaque pointer.
 *
 * UART RX uses blocking mode: HAL_UART_Receive polls the RXNE flag
 * and returns when the requested bytes arrive or the timeout expires.
 */

#ifndef HAL_INTERFACE_H
#define HAL_INTERFACE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "stm32f4xx_hal.h"

/*============================================================================
 * STM32 UART with blocking RX (opaque pointer)
 *============================================================================*/

/** Opaque UART handle */
typedef struct stm32_uart stm32_uart_t;

/**
 * @brief Initialise the STM32 UART wrapper.
 * @param huart  HAL UART handle from CubeMX
 * @return Pointer to the internal context (NULL on failure)
 */
stm32_uart_t* stm32_uart_init(UART_HandleTypeDef* huart);

/** Blocking transmit. Returns bytes sent or -1 on error. */
int32_t stm32_uart_transmit(stm32_uart_t* self, const uint8_t* data, size_t len);

/** Blocking receive of up to @p len bytes.
 *  Returns bytes actually received, or -1 on timeout (no bytes at all). */
int32_t stm32_uart_receive(stm32_uart_t* self, uint8_t* buffer, size_t len,
                            uint32_t timeout_ms);

/** Drain any pending bytes from the hardware (discard). */
void stm32_uart_flush_rx(stm32_uart_t* self);

/** Blocking millisecond delay. */
void stm32_uart_delay_ms(stm32_uart_t* self, uint32_t ms);

#endif /* HAL_INTERFACE_H */
