/**
 * @file config.h
 * @brief Application configuration for UDP Air Quality Server
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

/*============================================================================
 * Hardware Configuration
 *============================================================================*/

#define CONFIG_USART2_BAUDRATE      115200U

/*============================================================================
 * WiFi Configuration
 *============================================================================*/

#define CONFIG_WIFI_SSID                "YOUR_SSID"
#define CONFIG_WIFI_PASSWORD            "YOUR_PASSWORD"
#define CONFIG_WIFI_CONNECT_TIMEOUT_MS  30000U

/** Maximum WiFi connection attempts before triggering a system reset. */
#define CONFIG_WIFI_MAX_RETRIES          3U
/** Initial retry delay (ms); doubles on each attempt up to the cap. */
#define CONFIG_WIFI_RETRY_BASE_DELAY_MS  5000U
/** Maximum retry delay (ms) after exponential backoff. */
#define CONFIG_WIFI_RETRY_MAX_DELAY_MS   30000U

/* ── ESP8266 AT command timeouts ─────────────────────────────────────────── */

/** Timeout (ms) for quick AT commands: ATE0, CWMODE, CIPMUX, RST send. */
#define CONFIG_ESP8266_CMD_TIMEOUT_MS         1000U
/** Timeout (ms) for slow operations: AT test, reset-ready, post-join OK, CIPSTART. */
#define CONFIG_ESP8266_INIT_TIMEOUT_MS        5000U
/** Timeout (ms) for disconnect/query commands: CWQAP, CIPCLOSE, CIFSR. */
#define CONFIG_ESP8266_DISCONNECT_TIMEOUT_MS  2000U
/** Inter-byte polling timeout (ms) during blocking UART receive. */
#define CONFIG_ESP8266_BYTE_RX_TIMEOUT_MS       10U
/** Extra wait (ms) for a complete UDP payload after the +IPD header. */
#define CONFIG_ESP8266_PAYLOAD_TIMEOUT_MS     4000U

/*============================================================================
 * UDP Server Configuration
 *============================================================================*/

#define CONFIG_UDP_LOCAL_PORT            4210U

/** Maximum UDP start attempts before triggering a system reset. */
#define CONFIG_UDP_MAX_RETRIES           3U
/** Delay (ms) between UDP start retry attempts. */
#define CONFIG_UDP_RETRY_DELAY_MS        2000U

/*============================================================================
 * UART RX / Frame Format
 *============================================================================*/

/*
 * Frame 1: [type(0x01), float temperature, uint8 humidity %, uint16 air quality, CRC16]
 * Total payload size: 1 + 4 + 1 + 2 + 2 = 10 bytes
 */
#define FRAME_1_PAYLOAD_LEN 10U

/** MessageBuffer capacity in bytes. Must be > FRAME_1_PAYLOAD_LEN + 4 (FreeRTOS length prefix)
 *  to hold at least one message. Sized for up to ~14 queued frames. */
#define RTOS_MESSAGE_BUFFER_LEN (FRAME_1_PAYLOAD_LEN * 20U)

/*============================================================================
 * FreeRTOS Task Configuration
 *============================================================================*/

#define CONFIG_NETWORK_TASK_STACK_SIZE   512U   /* words */
#define CONFIG_NETWORK_TASK_PRIORITY     (osPriorityAboveNormal)

#define CONFIG_DISPLAY_TASK_STACK_SIZE   512U   /* words */
#define CONFIG_DISPLAY_TASK_PRIORITY     (osPriorityBelowNormal)


/*============================================================================
 * Sensors display Configuration
 *============================================================================*/
 
#define DISPLAY_SENSOR_TIMEOUT_MS 5000U

#endif /* CONFIG_H */
