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

/*============================================================================
 * UDP Server Configuration
 *============================================================================*/

#define CONFIG_UDP_LOCAL_PORT            4210U

/*============================================================================
 * UART RX Ring Buffer
 *============================================================================*/

/** lwrb byte ring buffer: holds 5 × max-frame (38 B each = payload + '\0') */
#define CONFIG_UART_RX_BUF_SIZE   256U

/* 
    frame 1: [type(0x01), float temperature, uint8 humidity %, uint16 air quality, CRC16]
    total payload size: 1 + 4 + 1 + 2 + 2 = 10 bytes
*/
#define FRAME_1_PAYLOAD_LEN 10U

#define RTOS_MESSAGE_BUFFER_LEN 10U

/*============================================================================
 * FreeRTOS Task Configuration
 *============================================================================*/

#define CONFIG_NETWORK_TASK_STACK_SIZE   512U   /* words */
#define CONFIG_NETWORK_TASK_PRIORITY     (osPriorityAboveNormal)

#define CONFIG_DISPLAY_TASK_STACK_SIZE   512U   /* words */
#define CONFIG_DISPLAY_TASK_PRIORITY     (osPriorityBelowNormal)

#endif /* CONFIG_H */
