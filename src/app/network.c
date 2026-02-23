/**
 * @file network.c
 * @brief Network task — WiFi + UDP receive loop
 *
 * Connects to WiFi, opens a UDP listener, then switches to
 * interrupt-driven UART reception and decodes incoming sensor
 * frames into the shared g_sensor_data[] array.
 */

#include "app/network.h"

#include "app/config.h"
#include "app/network_data.h"
#include "message_buffer.h"
#include "app/uart_rx.h"
#include "drivers/esp8266/esp8266.h"

#include "cmsis_os.h"
#include "portmacro.h"
#include "stm32f4xx_hal_conf.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>


extern MessageBufferHandle_t x_message_buffer;

/*============================================================================
 * Network Task — helpers
 *============================================================================*/

/**
 * @brief CRC16-CCITT (poly 0x1021, init 0xFFFF) over @p len bytes.
 */
static uint16_t crc16_ccitt(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int j = 0; j < 8; j++) {
            crc = (crc & 0x8000u) ? (uint16_t)((crc << 1) ^ 0x1021u)
                                  : (uint16_t)(crc << 1);
        }
    }
    return crc;
}

/**
 * @brief Parse a binary sensor frame into a sensor_data_t.
 *
 * Frame layout (FRAME_1_PAYLOAD_LEN = 10 bytes):
 *   [0]     type    : uint8_t  — must be 0x01
 *   [1..4]  temp    : float    (little-endian)
 *   [5]     humidity: uint8_t  (0–100 %)
 *   [6..7]  iaq     : uint16_t (little-endian)
 *   [8..9]  crc16   : uint16_t — CRC16-CCITT over bytes [0..7]
 *
 * @return true if type == 0x01 and CRC matches, false otherwise.
 */
static bool parse_sensor_frame(const char* raw, sensor_data_t* out)
{
    const uint8_t *buf = (const uint8_t *)raw;

    /* Validate frame type */
    if (buf[0] != 0x01u) {
        return false;
    }

    /* Verify CRC16 over the data bytes (all but the trailing 2 CRC bytes) */
    uint16_t crc_calc = crc16_ccitt(buf, FRAME_1_PAYLOAD_LEN - 2u);
    uint16_t crc_recv = (uint16_t)buf[8] | ((uint16_t)buf[9] << 8);
    if (crc_calc != crc_recv) {
        return false;
    }

    /* affect parameters;*/
    memcpy(&out->temperature,(const void*)(buf+1),sizeof(float));
    out->humidity    = (float)buf[5];
    out->iaq         = (uint16_t)buf[6] | ((uint16_t)buf[7] << 8);
    out->valid       = true;

    return true;
}


/*============================================================================
 * Public API
 *============================================================================*/

void network_task(void* argument)
{
    esp8266_t* esp = argument;

    /* ── Phase 1: blocking AT commands ───────────────────────────── */

    /* Initialise ESP8266 */
    esp8266_error_t err = esp8266_init(esp);
    if (err != ESP8266_OK) {
        osDelay(5000);
        NVIC_SystemReset();
    }

    /* Connect to WiFi */
    esp8266_wifi_creds_t creds = {
        .ssid     = CONFIG_WIFI_SSID,
        .password = CONFIG_WIFI_PASSWORD,
        .security = ESP8266_WIFI_WPA2_PSK
    };

    while (esp8266_connect_wifi(esp, &creds,
                                 CONFIG_WIFI_CONNECT_TIMEOUT_MS) != ESP8266_OK) {
        osDelay(5000);
    }

    /* Start UDP listener on the configured local port */
    while (esp8266_udp_start(esp, CONFIG_UDP_LOCAL_PORT) != ESP8266_OK) {
        osDelay(2000);
    }

    /* ── Phase 2: switch to interrupt-driven UART reception ──────── */
    uart_rx_start();

    for (;;) {

        char frame[FRAME_1_PAYLOAD_LEN];
        const size_t received_length = xMessageBufferReceive(x_message_buffer,&frame,FRAME_1_PAYLOAD_LEN,portMAX_DELAY);

        if(received_length==FRAME_1_PAYLOAD_LEN)
        {
            sensor_data_t parsed;
            if (parse_sensor_frame(frame, &parsed)) {
                osMessageQueuePut(g_sensor_queue, &parsed, 0, 0);
            }
        }
    }
}
