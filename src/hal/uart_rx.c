/**
 * @file uart_rx.c
 * @brief UART RX interrupt-driven reception with +IPD frame detection
 *
 * Byte-level ISR reception via HAL_UART_Receive_IT. A state machine parses
 * the ESP8266 "+IPD,<len>:<payload>" framing and writes complete, null-
 * terminated payloads into the lwrb ring buffer g_uart_rx_rb.
 * A binary semaphore wakes the network task when a new frame is ready.
 *
 * The ring buffer is lock-free (single producer ISR, single consumer task).
 */

#include "app/uart_rx.h"
#include "app/config.h"

#include "FreeRTOS.h"
#include "message_buffer.h"
#include "projdefs.h"
#include "semphr.h"

/*============================================================================
 * Ring buffer (extern'd in uart_rx.h)
 *============================================================================*/

static uint8_t s_rb_data[CONFIG_UART_RX_BUF_SIZE];
MessageBufferHandle_t x_message_buffer;
lwrb_t g_uart_rx_rb;

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
    UART_HandleTypeDef* huart;
    uint8_t rx_byte;            /**< Single byte for HAL_UART_Receive_IT */

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
            /* Currently: only frame 1 currently supported */
            if (s_rx.payload_len != FRAME_1_PAYLOAD_LEN) {
                /* Invalid or oversized length: discard */
                s_rx.state = RX_STATE_IDLE;
            } else {
                s_rx.state = RX_STATE_RECV_PAYLOAD;
                s_rx.payload_idx = 0;
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
            // TODO(me): treat the case where buffer is full, currently it is silently dropped
            BaseType_t woken = pdTRUE;
            xMessageBufferSendFromISR(x_message_buffer,s_rx.payload_buf,FRAME_1_PAYLOAD_LEN,&woken);
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

void uart_rx_init(UART_HandleTypeDef* huart)
{
    s_rx.huart = huart;
    s_rx.state = RX_STATE_IDLE;
    s_rx.rx_byte = 0;
    s_rx.match_idx = 0;
    s_rx.payload_len = 0;
    s_rx.payload_idx = 0;

    x_message_buffer = xMessageBufferCreate(RTOS_MESSAGE_BUFFER_LEN);

    lwrb_init(&g_uart_rx_rb, s_rb_data, sizeof(s_rb_data));
}

void uart_rx_start(void)
{
    /* Flush any pending data in the UART data register */
    __HAL_UART_FLUSH_DRREGISTER(s_rx.huart);

    /* Arm the first interrupt-driven receive */
    HAL_UART_Receive_IT(s_rx.huart, &s_rx.rx_byte, 1);
}
