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
#include "app/uart_rx.h"
#include "drivers/esp8266/esp8266.h"

#include "cmsis_os.h"
#include "portmacro.h"
#include "stm32f4xx_hal_conf.h"

#include <stdint.h>

/*============================================================================
 * Public API
 *============================================================================*/

void network_task(void* argument)
{
    const network_task_config_t* cfg = argument;

    /* ── Phase 1: blocking AT commands ───────────────────────────── */

    /* Initialise ESP8266 */
    esp8266_error_t err = esp8266_init(cfg->esp);
    if (err != ESP8266_OK) {
        osDelay(5000);
        NVIC_SystemReset();
    }

    /* Connect to WiFi — exponential backoff, max CONFIG_WIFI_MAX_RETRIES */
    esp8266_wifi_creds_t creds = {
        .ssid     = CONFIG_WIFI_SSID,
        .password = CONFIG_WIFI_PASSWORD,
        .security = ESP8266_WIFI_WPA2_PSK
    };

    uint8_t  retries = 0U;
    uint32_t delay   = CONFIG_WIFI_RETRY_BASE_DELAY_MS;

    while (esp8266_connect_wifi(cfg->esp, &creds,
                                CONFIG_WIFI_CONNECT_TIMEOUT_MS) != ESP8266_OK) {
        retries++;
        if (retries >= CONFIG_WIFI_MAX_RETRIES) {
            osDelay(1000);
            NVIC_SystemReset();
        }
        osDelay(delay);
        delay = (delay * 2U > CONFIG_WIFI_RETRY_MAX_DELAY_MS)
                ? CONFIG_WIFI_RETRY_MAX_DELAY_MS
                : delay * 2U;
    }

    /* Start UDP listener — fixed retry limit */
    uint8_t udp_retries = 0U;
    while (esp8266_udp_start(cfg->esp, CONFIG_UDP_LOCAL_PORT) != ESP8266_OK) {
        udp_retries++;
        if (udp_retries >= CONFIG_UDP_MAX_RETRIES) {
            osDelay(1000);
            NVIC_SystemReset();
        }
        osDelay(CONFIG_UDP_RETRY_DELAY_MS);
    }

    /* ── Phase 2: switch to interrupt-driven UART reception ──────── */
    uart_rx_start();

    for (;;) {
        char frame[FRAME_1_PAYLOAD_LEN];
        const size_t received_length = xMessageBufferReceive(
            cfg->msg_buf, &frame, FRAME_1_PAYLOAD_LEN, portMAX_DELAY);

        if (received_length == FRAME_1_PAYLOAD_LEN) {
            sensor_data_t parsed;
            if (parse_sensor_frame((const uint8_t *)frame, &parsed)) {
                osMessageQueuePut(cfg->sensor_queue, &parsed, 0, 0);
            }
        }
    }
}
