/**
 * @file uart_rx.h
 * @brief UART RX interrupt-driven reception for ESP8266 +IPD frames
 *
 * Receives bytes via UART interrupt, parses +IPD,<len>:<payload> frames
 * from the ESP8266, and sends complete payloads into a FreeRTOS MessageBuffer
 * provided by the caller. The network task blocks on the MessageBuffer and is
 * woken automatically when a new frame is ready.
 *
 * Usage:
 *   1. Create a MessageBuffer (xMessageBufferCreate) in main.
 *   2. Call uart_rx_init() with the UART handle and MessageBuffer handle.
 *   3. Use blocking UART for AT commands (init, WiFi, UDP start).
 *   4. Call uart_rx_start() to switch to interrupt-driven reception.
 */

#ifndef UART_RX_H
#define UART_RX_H

#include <stdint.h>

#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"          /* must precede message_buffer.h */
#include "message_buffer.h"

/*============================================================================
 * Diagnostics
 *============================================================================*/

/**
 * @brief Count of ISR frames dropped because the MessageBuffer was full.
 *
 * Incremented from ISR context; read from task context. Declared volatile
 * so the compiler does not cache it across accesses.
 */
extern volatile uint32_t uart_rx_overflow_count;

/*============================================================================
 * Public API
 *============================================================================*/

/**
 * @brief Initialise the UART RX module.
 *
 * Stores the UART handle and MessageBuffer handle, then resets internal
 * state. Does NOT enable interrupts yet.
 *
 * @param huart    HAL UART handle (USART2)
 * @param msg_buf  FreeRTOS MessageBuffer to post complete payloads into.
 *                 Must be created by the caller before invoking this function.
 */
void uart_rx_init(UART_HandleTypeDef* huart, MessageBufferHandle_t msg_buf);

/**
 * @brief Start interrupt-driven UART reception.
 *
 * Arms the first HAL_UART_Receive_IT call. From this point on, incoming
 * bytes are handled by the ISR and the +IPD state machine. Do NOT use
 * blocking UART receive after calling this.
 */
void uart_rx_start(void);

#endif /* UART_RX_H */
