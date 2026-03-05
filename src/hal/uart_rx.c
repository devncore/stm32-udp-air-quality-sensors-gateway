/**
 * @file uart_rx.c
 * @brief UART RX interrupt-driven reception with +IPD frame detection
 *
 * Byte-level ISR reception via HAL_UART_Receive_IT. A state machine parses
 * the ESP8266 "+IPD,<len>:<payload>" framing and sends complete payloads
 * into the FreeRTOS MessageBuffer provided at init time, which wakes the
 * network task via its built-in task notification.
 *
 * The MessageBuffer is lock-free (single producer ISR, single consumer task).
 */

#include "app/uart_rx.h"
#include "app/config.h"

#include "message_buffer.h"
#include "projdefs.h"
#include "app/error_manager.h"

/*============================================================================
 * Diagnostics
 *============================================================================*/

volatile uint32_t uart_rx_overflow_count = 0U;

/*============================================================================
 * +IPD state machine
 *============================================================================*/

/** States for parsing "+IPD,<len>:<payload>" */
typedef enum {
    RX_STATE_IDLE,          /**< Waiting for '+' */
    RX_STATE_MATCH_IPD,     /**< Matching "IPD," after '+' */
    RX_STATE_PARSE_LEN,     /**< Accumulating ASCII length digits until ':' */
    RX_STATE_RECV_PAYLOAD   /**< Receiving exactly payload_len bytes */
} rx_state_t;

static const char IPD_PATTERN[] = "IPD,";  /* After '+', match these 4 chars */
#define IPD_PATTERN_LEN  4

/** Private state */
static struct {
    UART_HandleTypeDef*   huart;
    MessageBufferHandle_t msg_buf;      /**< MessageBuffer owned by main.c */
    uint8_t rx_byte;                    /**< Single byte for HAL_UART_Receive_IT */

    rx_state_t state;
    uint8_t match_idx;          /**< Index into IPD_PATTERN during MATCH_IPD */
    uint16_t payload_len;       /**< Parsed +IPD length */
    uint16_t payload_idx;       /**< Bytes received so far in current payload */
    char payload_buf[FRAME_1_PAYLOAD_LEN]; /* frame 1 payload length */
} s_rx;

/*============================================================================
 * State machine — process one byte (called from ISR context)
 *============================================================================*/

static void rx_process_byte(uint8_t byte)
{
    switch (s_rx.state) {

    case RX_STATE_IDLE:
        if (byte == '+') {
            s_rx.state = RX_STATE_MATCH_IPD;
            s_rx.match_idx = 0;
        }
        break;

    case RX_STATE_MATCH_IPD:
        if (byte == (uint8_t)IPD_PATTERN[s_rx.match_idx]) {
            s_rx.match_idx++;
            if (s_rx.match_idx == IPD_PATTERN_LEN) {
                /* "+IPD," fully matched — now parse length */
                s_rx.state = RX_STATE_PARSE_LEN;
                s_rx.payload_len = 0;
            }
        } else {
            /* Mismatch — back to idle */
            s_rx.state = RX_STATE_IDLE;
        }
        break;

    case RX_STATE_PARSE_LEN:
        if (byte >= '0' && byte <= '9') {
            s_rx.payload_len = s_rx.payload_len * 10u + (byte - '0');
        } else if (byte == ':') {
            switch (s_rx.payload_len) {
            case FRAME_1_PAYLOAD_LEN:
                s_rx.state = RX_STATE_RECV_PAYLOAD;
                s_rx.payload_idx = 0;
                break;
            default:
                /* Unsupported frame length: discard */
                s_rx.state = RX_STATE_IDLE;
                break;
            }
        } else {
            /* Unexpected character — discard */
            s_rx.state = RX_STATE_IDLE;
        }
        break;

    case RX_STATE_RECV_PAYLOAD:
        s_rx.payload_buf[s_rx.payload_idx] = (char)byte;
        s_rx.payload_idx++;

        if (s_rx.payload_idx == s_rx.payload_len) {
            BaseType_t woken = pdFALSE;
            const size_t sent = xMessageBufferSendFromISR(
                s_rx.msg_buf, s_rx.payload_buf, FRAME_1_PAYLOAD_LEN, &woken);
            if (sent == 0U) {
                uart_rx_overflow_count++;
                error_set_from_isr(ERROR_MESSAGE_BUFFER_OVERFLOW);
            }
            s_rx.state = RX_STATE_IDLE;
            portYIELD_FROM_ISR(woken);
        }
        break;
    }
}

/*============================================================================
 * HAL UART RX complete callback (called from ISR by HAL)
 *============================================================================*/

void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart)
{
    if (huart != s_rx.huart) {
        return;
    }

    rx_process_byte(s_rx.rx_byte);

    /* Re-arm for next byte */
    HAL_UART_Receive_IT(s_rx.huart, &s_rx.rx_byte, 1);
}

/*============================================================================
 * Public API
 *============================================================================*/

void uart_rx_init(UART_HandleTypeDef* huart, MessageBufferHandle_t msg_buf)
{
    s_rx.huart       = huart;
    s_rx.msg_buf     = msg_buf;
    s_rx.state       = RX_STATE_IDLE;
    s_rx.rx_byte     = 0;
    s_rx.match_idx   = 0;
    s_rx.payload_len = 0;
    s_rx.payload_idx = 0;
}

void uart_rx_start(void)
{
    /* Flush any pending data in the UART data register */
    __HAL_UART_FLUSH_DRREGISTER(s_rx.huart);

    /* Arm the first interrupt-driven receive */
    HAL_UART_Receive_IT(s_rx.huart, &s_rx.rx_byte, 1);
}
