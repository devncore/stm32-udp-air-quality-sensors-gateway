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
#include "app/error_manager.h"
#include "app/frame_parser.h"
#include "app/network_data.h"
#include "app/uart_rx.h"
#include "cmsis_os2.h"
#include "drivers/esp8266/esp8266.h"

#include "cmsis_os.h"
#include "portmacro.h"
#include "projdefs.h"
#include "stm32f4xx_hal_conf.h"

#include <stdint.h>


static bool s_network_task_init_successful = false;


/*============================================================================
 * Private functions
 *============================================================================*/

static bool network_task_init(const network_task_config_t* cfg)
{
    // get initial error status
    const bool esp8266_init_failed_active = error_is_active(ERROR_ESP8266_INIT_FAILED);
    const bool wifi_connection_failed_active = error_is_active(ERROR_WIFI_CONNECT_TIMEOUT);
    const bool udp_start_failed_active = error_is_active(ERROR_UDP_START_FAILED);
    const bool first_init = !(esp8266_init_failed_active || wifi_connection_failed_active || udp_start_failed_active);

    /* ── Phase 1: blocking AT commands ───────────────────────────── */

    /* Initialise ESP8266 */
    if(first_init || esp8266_init_failed_active)
    {
        esp8266_error_t err = esp8266_init(cfg->esp);
        if (err != ESP8266_OK) {
            error_set(ERROR_ESP8266_INIT_FAILED);
            return false;
        }
        //reset error flag
        if(esp8266_init_failed_active)
        {
            error_reset(ERROR_ESP8266_INIT_FAILED);
        }
    }

    /* Connect to WiFi — exponential backoff, max CONFIG_WIFI_MAX_RETRIES */
    if(first_init || wifi_connection_failed_active)
    {
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
                error_set(ERROR_WIFI_CONNECT_TIMEOUT);
                return false;
            }
            osDelay(delay);
            delay = (delay * 2U > CONFIG_WIFI_RETRY_MAX_DELAY_MS)
                    ? CONFIG_WIFI_RETRY_MAX_DELAY_MS
                    : delay * 2U;
        }

        //reset error flag
        if(wifi_connection_failed_active)
        {
            error_reset(ERROR_WIFI_CONNECT_TIMEOUT);
        }
    }

    /* Start UDP listener — fixed retry limit */
    if(first_init || udp_start_failed_active)
    {
        uint8_t udp_retries = 0U;
        while (esp8266_udp_start(cfg->esp, CONFIG_UDP_LOCAL_PORT) != ESP8266_OK) {
            udp_retries++;
            if (udp_retries >= CONFIG_UDP_MAX_RETRIES) {
                error_set(ERROR_UDP_START_FAILED);
                return false;
            }
            osDelay(CONFIG_UDP_RETRY_DELAY_MS);
        }

        //reset error flag
        if(udp_start_failed_active)
        {
            error_reset(ERROR_UDP_START_FAILED);
        }
    }

    /* ── Phase 2: switch to interrupt-driven UART reception ──────── */
    uart_rx_start();

    return true;
}

/*============================================================================
 * Public API
 *============================================================================*/

void network_task(void* argument)
{
    const network_task_config_t* cfg = argument;

    for (;;) {
        // INIT
        if(!s_network_task_init_successful) {
            s_network_task_init_successful = network_task_init(cfg);
            if(!s_network_task_init_successful){
                // if initialization has failed, set some delay before retrying
                vTaskDelay(pdMS_TO_TICKS(RETRY_NETWORK_INIT_AFTER_FAIL_MS));
            }
        }
        // INFINITE LOOP
        else {
            char frame[FRAME_1_PAYLOAD_LEN];
            const size_t received_length = xMessageBufferReceive(
                cfg->msg_buf, &frame, FRAME_1_PAYLOAD_LEN, portMAX_DELAY);

            if (received_length == FRAME_1_PAYLOAD_LEN) {
                const uint8_t type = frame[0];
                if(validate_type(type) && validate_crc((const uint8_t *)frame))
                {
                    sensor_data_t parsed;
                    parse_sensor_frame((const uint8_t *)frame, &parsed);
                    if(xQueueSendToBack(cfg->sensor_queue, &parsed, 0) != pdPASS)
                    {
                        error_set(ERROR_SENSOR_QUEUE_PUT_FAILED);
                    }
                }
            }
        }
    }
}
