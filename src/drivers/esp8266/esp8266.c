/**
 * @file esp8266.c
 * @brief ESP8266 WiFi Module Driver Implementation — UDP variant (C, object-method)
 *
 * AT command driver for ESP8266/ESP-01S module adapted for UDP reception.
 * The server listens on a fixed local port and receives sensor datagrams.
 */

#include "drivers/esp8266/esp8266.h"
#include "stm32f4xx_hal_uart.h"

#include <stdio.h>
#include <string.h>

/*============================================================================
 * Private constants
 *============================================================================*/

static const char* const AT_OK         = "OK";
static const char* const AT_ERROR      = "ERROR";
static const char* const AT_READY      = "ready";
static const char* const AT_WIFI_GOT_IP = "WIFI GOT IP";
static const char* const AT_CLOSED     = "CLOSED";
static const char* const AT_IPD        = "+IPD,";

/*============================================================================
 * Private helpers
 *============================================================================*/

/** Flush hardware FIFO and reset the software rx buffer */
static void clear_rx_buffer(esp8266_t* dev)
{
    stm32_uart_flush_rx(dev->uart);
    dev->rx_index = 0;
    dev->rx_buffer[0] = '\0';
}

/** Search for a substring in the rx_buffer */
static const char* response_find(const esp8266_t* dev, const char* needle)
{
    if (dev->rx_index == 0) {
        return NULL;
    }
    return strstr((const char*)dev->rx_buffer, needle);
}

/** Read available UART bytes into rx_buffer using blocking 1-byte reads.
 *  At 115200 baud a full byte takes ~87 us. A 10 ms inter-byte timeout
 *  is generous enough to bridge any gap inside a single AT response. */
static void read_available(esp8266_t* dev)
{
    size_t space = ESP8266_RX_BUFFER_SIZE - 1 - dev->rx_index;
    if (space == 0) {
        return;
    }

    int32_t received;
    do {
        received = stm32_uart_receive(dev->uart,
                                       dev->rx_buffer + dev->rx_index,
                                       1, 10);
        if (received > 0) {
            dev->rx_index += 1;
            space--;
        }
    } while (received > 0 && space > 0);

    dev->rx_buffer[dev->rx_index] = '\0';
}

/** Wait for a specific response pattern */
static esp8266_wait_result_t wait_for_response(esp8266_t* dev,
                                                const char* expected,
                                                uint32_t timeout_ms)
{
    uint32_t start = HAL_GetTick();

    while ((HAL_GetTick() - start) < timeout_ms) {
        read_available(dev);

        if (response_find(dev, expected) != NULL) {
            return ESP8266_WAIT_FOUND;
        }
        if (response_find(dev, AT_ERROR) != NULL) {
            return ESP8266_WAIT_ERROR;
        }
        if (response_find(dev, AT_CLOSED) != NULL) {
            return ESP8266_WAIT_ERROR;
        }
    }

    return ESP8266_WAIT_TIMEOUT;
}

/** Flush rx, build command + CRLF, and transmit */
static esp8266_error_t transmit_raw(esp8266_t* dev, const char* command)
{
    clear_rx_buffer(dev);

    size_t cmd_len = strlen(command);
    uint8_t tx_buf[256];

    if (cmd_len + 2 > sizeof(tx_buf)) {
        return ESP8266_ERR_BUFFER_OVERFLOW;
    }

    memcpy(tx_buf, command, cmd_len);
    tx_buf[cmd_len++] = '\r';
    tx_buf[cmd_len++] = '\n';

    if (stm32_uart_transmit(dev->uart, tx_buf, cmd_len) < 0) {
        return ESP8266_ERR_SEND_FAILED;
    }

    return ESP8266_OK;
}

/** Send AT command and wait for expected response */
static esp8266_error_t send_command(esp8266_t* dev, const char* command,
                                     const char* expected_response,
                                     uint32_t timeout_ms)
{
    esp8266_error_t err = transmit_raw(dev, command);
    if (err != ESP8266_OK) {
        return err;
    }

    esp8266_wait_result_t wr = wait_for_response(dev, expected_response,
                                                   timeout_ms);
    if (wr == ESP8266_WAIT_FOUND) {
        return ESP8266_OK;
    }

    return wr == ESP8266_WAIT_ERROR ? ESP8266_ERR_COMMAND_ERROR
                                    : ESP8266_ERR_TIMEOUT;
}

/*============================================================================
 * Public API
 *============================================================================*/

void esp8266_create(esp8266_t* dev, stm32_uart_t* uart)
{
    memset(dev, 0, sizeof(*dev));
    dev->uart = uart;
}

esp8266_error_t esp8266_init(esp8266_t* dev)
{
    clear_rx_buffer(dev);

    /* Test communication */
    if (send_command(dev, "AT", AT_OK, 5000) != ESP8266_OK) {
        return ESP8266_ERR_TIMEOUT;
    }

    /* Disable echo */
    if (send_command(dev, "ATE0", AT_OK, 1000) != ESP8266_OK) {
        return ESP8266_ERR_COMMAND_ERROR;
    }

    /* Station mode */
    if (send_command(dev, "AT+CWMODE=1", AT_OK, 1000) != ESP8266_OK) {
        return ESP8266_ERR_COMMAND_ERROR;
    }

    /* Single connection mode */
    if (send_command(dev, "AT+CIPMUX=0", AT_OK, 1000) != ESP8266_OK) {
        return ESP8266_ERR_COMMAND_ERROR;
    }

    dev->initialized = true;
    dev->state = ESP8266_STATE_DISCONNECTED;

    return ESP8266_OK;
}

esp8266_error_t esp8266_reset(esp8266_t* dev)
{
    dev->initialized = false;
    dev->state = ESP8266_STATE_DISCONNECTED;
    dev->udp_listening = false;

    if (send_command(dev, "AT+RST", AT_OK, 1000) != ESP8266_OK) {
        return ESP8266_ERR_COMMAND_ERROR;
    }

    if (wait_for_response(dev, AT_READY, 5000) != ESP8266_WAIT_FOUND) {
        return ESP8266_ERR_TIMEOUT;
    }

    stm32_uart_delay_ms(dev->uart, 500);
    stm32_uart_flush_rx(dev->uart);

    return ESP8266_OK;
}

esp8266_error_t esp8266_connect_wifi(esp8266_t* dev,
                                      const esp8266_wifi_creds_t* creds,
                                      uint32_t timeout_ms)
{
    if (!dev->initialized) {
        return ESP8266_ERR_NOT_INITIALIZED;
    }

    dev->state = ESP8266_STATE_CONNECTING;

    char cmd[128];
    int len = snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"",
                       creds->ssid, creds->password);

    if (len < 0 || (size_t)len >= sizeof(cmd)) {
        dev->state = ESP8266_STATE_ERROR;
        return ESP8266_ERR_BUFFER_OVERFLOW;
    }

    esp8266_error_t err = transmit_raw(dev, cmd);
    if (err != ESP8266_OK) {
        dev->state = ESP8266_STATE_ERROR;
        return err;
    }

    /* Wait for WIFI GOT IP then final OK */
    esp8266_wait_result_t wr = wait_for_response(dev, AT_WIFI_GOT_IP,
                                                   timeout_ms);
    if (wr != ESP8266_WAIT_FOUND) {
        dev->state = ESP8266_STATE_ERROR;
        return wr == ESP8266_WAIT_ERROR ? ESP8266_ERR_CONNECTION_FAILED
                                        : ESP8266_ERR_TIMEOUT;
    }

    wr = wait_for_response(dev, AT_OK, 5000);
    if (wr != ESP8266_WAIT_FOUND) {
        dev->state = ESP8266_STATE_ERROR;
        return ESP8266_ERR_TIMEOUT;
    }

    dev->state = ESP8266_STATE_GOT_IP;
    return ESP8266_OK;
}

esp8266_error_t esp8266_disconnect_wifi(esp8266_t* dev)
{
    if (!dev->initialized) {
        return ESP8266_ERR_NOT_INITIALIZED;
    }

    dev->udp_listening = false;
    esp8266_error_t err = send_command(dev, "AT+CWQAP", AT_OK, 2000);
    dev->state = ESP8266_STATE_DISCONNECTED;
    return err;
}

esp8266_conn_state_t esp8266_get_state(const esp8266_t* dev)
{
    return dev->state;
}

bool esp8266_is_connected(const esp8266_t* dev)
{
    return dev->state == ESP8266_STATE_CONNECTED ||
           dev->state == ESP8266_STATE_GOT_IP;
}

esp8266_error_t esp8266_get_ip_info(esp8266_t* dev, esp8266_ip_info_t* info)
{
    if (!dev->initialized) {
        return ESP8266_ERR_NOT_INITIALIZED;
    }

    if (!esp8266_is_connected(dev)) {
        return ESP8266_ERR_NOT_CONNECTED;
    }

    esp8266_error_t err = transmit_raw(dev, "AT+CIFSR");
    if (err != ESP8266_OK) {
        return err;
    }

    if (wait_for_response(dev, AT_OK, 2000) != ESP8266_WAIT_FOUND) {
        return ESP8266_ERR_TIMEOUT;
    }

    memset(info, 0, sizeof(*info));

    /* Parse STAIP */
    const char* pos = response_find(dev, "+CIFSR:STAIP,\"");
    if (pos != NULL) {
        pos += 14;  /* Skip prefix */
        unsigned int a = 0;
        unsigned int b = 0;
        unsigned int c = 0;
        unsigned int d = 0;
        if (sscanf(pos, "%u.%u.%u.%u", &a, &b, &c, &d) == 4) {
            info->ip[0] = (uint8_t)a;
            info->ip[1] = (uint8_t)b;
            info->ip[2] = (uint8_t)c;
            info->ip[3] = (uint8_t)d;
        }
    }

    return ESP8266_OK;
}

esp8266_error_t esp8266_udp_start(esp8266_t* dev, uint16_t local_port)
{
    if (!dev->initialized) {
        return ESP8266_ERR_NOT_INITIALIZED;
    }

    if (!esp8266_is_connected(dev)) {
        return ESP8266_ERR_NOT_CONNECTED;
    }

    /*
     * AT+CIPSTART="UDP","0.0.0.0",0,<local_port>,2
     *
     * "0.0.0.0" = no fixed remote (accept from any sender)
     * 0         = remote port placeholder (mode 2 overrides it)
     * mode 2    = destination changes to last received remote IP/port
     */
    char cmd[64];
    int len = snprintf(cmd, sizeof(cmd),
                       "AT+CIPSTART=\"UDP\",\"0.0.0.0\",0,%u,2",
                       (unsigned)local_port);

    if (len < 0 || (size_t)len >= sizeof(cmd)) {
        return ESP8266_ERR_BUFFER_OVERFLOW;
    }

    esp8266_error_t err = send_command(dev, cmd, AT_OK, 5000);
    if (err == ESP8266_OK) {
        dev->udp_listening = true;
    }

    return err;
}

esp8266_error_t esp8266_udp_stop(esp8266_t* dev)
{
    if (!dev->initialized) {
        return ESP8266_ERR_NOT_INITIALIZED;
    }

    dev->udp_listening = false;
    return send_command(dev, "AT+CIPCLOSE", AT_OK, 2000);
}

int32_t esp8266_udp_receive(esp8266_t* dev, uint8_t* buffer,
                             size_t len, uint32_t timeout_ms)
{
    if (!dev->initialized) {
        return -1;
    }

    if (!dev->udp_listening) {
        return -1;
    }

    /* Wait for +IPD header */
    esp8266_wait_result_t wr = wait_for_response(dev, AT_IPD, timeout_ms);
    if (wr != ESP8266_WAIT_FOUND) {
        return wr == ESP8266_WAIT_TIMEOUT ? 0 : -1;
    }

    /* Parse +IPD,<len>:<data> */
    const char* ipd_pos = response_find(dev, AT_IPD);
    if (ipd_pos == NULL) {
        return -1;
    }

    const char* colon_pos = strchr(ipd_pos, ':');
    if (colon_pos == NULL) {
        return -1;
    }

    int data_len = 0;
    if (sscanf(ipd_pos + 5, "%d", &data_len) != 1) {
        return -1;
    }

    size_t data_start = (size_t)(colon_pos + 1 - (const char*)dev->rx_buffer);

    /* Wait for complete payload */
    uint32_t extra_start = HAL_GetTick();
    while (dev->rx_index - data_start < (size_t)data_len &&
           (HAL_GetTick() - extra_start) < 4000) {
        read_available(dev);
    }

    if (dev->rx_index - data_start < (size_t)data_len) {
        return -1;  /* Timeout waiting for full payload */
    }

    /* Copy data to output buffer */
    size_t to_copy = (size_t)data_len;
    if (to_copy > len) {
        to_copy = len;
    }
    memcpy(buffer, dev->rx_buffer + data_start, to_copy);

    /* Shift remaining data */
    size_t consumed = data_start + (size_t)data_len;
    if (consumed < dev->rx_index) {
        memmove(dev->rx_buffer, dev->rx_buffer + consumed,
                dev->rx_index - consumed);
        dev->rx_index -= consumed;
    } else {
        dev->rx_index = 0;
    }
    dev->rx_buffer[dev->rx_index] = '\0';

    return (int32_t)to_copy;
}

bool esp8266_is_initialized(const esp8266_t* dev)
{
    return dev->initialized;
}
