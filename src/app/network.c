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
#include "app/frame_parser.h"
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
            if (parse_sensor_frame((const uint8_t *)frame, &parsed)) {
                osMessageQueuePut(g_sensor_queue, &parsed, 0, 0);
            }
        }
    }
}
