/**
 * @file esp8266.h
 * @brief ESP8266 (ESP-01S) WiFi Module Driver — UDP variant (C, object-method pattern)
 *
 * Driver for ESP8266 module using AT command interface over UART.
 * This variant replaces TCP with UDP for connectionless datagram reception.
 *
 * Hardware connection:
 *   USART2 (PA2=TX, PA3=RX) -> ESP-01S
 *   Power: 3.3V, GND
 */

#ifndef ESP8266_H
#define ESP8266_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app/hal_interface.h"

/*============================================================================
 * Constants
 *============================================================================*/

#define ESP8266_RX_BUFFER_SIZE     1024U
#define ESP8266_DEFAULT_TIMEOUT_MS 5000U

/*============================================================================
 * Enumerations
 *============================================================================*/

typedef enum {
    ESP8266_WIFI_OPEN     = 0,
    ESP8266_WIFI_WEP      = 1,
    ESP8266_WIFI_WPA_PSK  = 2,
    ESP8266_WIFI_WPA2_PSK = 3,
    ESP8266_WIFI_WPA_WPA2 = 4
} esp8266_wifi_security_t;

typedef enum {
    ESP8266_STATE_DISCONNECTED,
    ESP8266_STATE_CONNECTING,
    ESP8266_STATE_CONNECTED,
    ESP8266_STATE_GOT_IP,
    ESP8266_STATE_ERROR
} esp8266_conn_state_t;

typedef enum {
    ESP8266_OK,
    ESP8266_ERR_TIMEOUT,
    ESP8266_ERR_INVALID_RESPONSE,
    ESP8266_ERR_NOT_CONNECTED,
    ESP8266_ERR_CONNECTION_FAILED,
    ESP8266_ERR_SEND_FAILED,
    ESP8266_ERR_BUFFER_OVERFLOW,
    ESP8266_ERR_COMMAND_ERROR,
    ESP8266_ERR_NOT_INITIALIZED
} esp8266_error_t;

typedef enum {
    ESP8266_WAIT_FOUND,
    ESP8266_WAIT_ERROR,
    ESP8266_WAIT_TIMEOUT
} esp8266_wait_result_t;

/*============================================================================
 * Data Structures
 *============================================================================*/

/** WiFi credentials */
typedef struct {
    const char* ssid;
    const char* password;
    esp8266_wifi_security_t security;
} esp8266_wifi_creds_t;

/** IP address information */
typedef struct {
    uint8_t ip[4];
    uint8_t gateway[4];
    uint8_t netmask[4];
} esp8266_ip_info_t;

/*============================================================================
 * ESP8266 Device Context (object-method pattern)
 *============================================================================*/

typedef struct {
    stm32_uart_t* uart;
    esp8266_conn_state_t state;
    bool initialized;
    bool udp_listening;

    uint8_t rx_buffer[ESP8266_RX_BUFFER_SIZE];
    size_t rx_index;
} esp8266_t;

/*============================================================================
 * Public API
 *============================================================================*/

/** Construct/initialise an ESP8266 context */
void esp8266_create(esp8266_t* dev, stm32_uart_t* uart);

/** Initialise the ESP8266 module (AT test, set station mode) */
esp8266_error_t esp8266_init(esp8266_t* dev);

/** Reset the module */
esp8266_error_t esp8266_reset(esp8266_t* dev);

/** Connect to a WiFi network */
esp8266_error_t esp8266_connect_wifi(esp8266_t* dev,
                                      const esp8266_wifi_creds_t* creds,
                                      uint32_t timeout_ms);

/** Disconnect from WiFi */
esp8266_error_t esp8266_disconnect_wifi(esp8266_t* dev);

/** Get current connection state */
esp8266_conn_state_t esp8266_get_state(const esp8266_t* dev);

/** Check if connected to WiFi */
bool esp8266_is_connected(const esp8266_t* dev);

/** Get IP address info. Returns error code, fills @p info on success. */
esp8266_error_t esp8266_get_ip_info(esp8266_t* dev, esp8266_ip_info_t* info);

/**
 * @brief Start listening for UDP datagrams.
 *
 * Issues AT+CIPSTART="UDP","0.0.0.0",0,<local_port>,2
 * Mode 2: destination changes to last received remote IP/port.
 *
 * @param dev         Device context
 * @param local_port  Local UDP port to bind
 * @return ESP8266_OK on success
 */
esp8266_error_t esp8266_udp_start(esp8266_t* dev, uint16_t local_port);

/** Stop the UDP listener */
esp8266_error_t esp8266_udp_stop(esp8266_t* dev);

/**
 * @brief Receive a UDP datagram.
 *
 * Blocks until a +IPD message arrives or the timeout expires.
 *
 * @param dev        Device context
 * @param buffer     Output buffer for received payload
 * @param len        Size of output buffer
 * @param timeout_ms Maximum time to wait (ms)
 * @return Bytes received (>0), 0 on timeout, -1 on error
 */
int32_t esp8266_udp_receive(esp8266_t* dev, uint8_t* buffer,
                             size_t len, uint32_t timeout_ms);

/** Check if module is initialised */
bool esp8266_is_initialized(const esp8266_t* dev);

#endif /* ESP8266_H */
