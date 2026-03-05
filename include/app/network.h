/**
 * @file network.h
 * @brief Network task — WiFi + UDP receive loop
 *
 * Connects to WiFi, opens a UDP listener, then switches to
 * interrupt-driven UART reception and decodes incoming sensor
 * frames into the shared g_sensor_data[] array.
 */

#ifndef APP_NETWORK_H
#define APP_NETWORK_H

#include "FreeRTOS.h"
#include "message_buffer.h"
#include "queue.h"
#include "drivers/esp8266/esp8266.h"

/*============================================================================
 * Task configuration
 *============================================================================*/

/**
 * @brief Configuration passed to network_task() via its pvParameters argument.
 *
 * Bundles all resources the task needs so that ownership is explicit and
 * no module-level globals are required.
 */
typedef struct {
    esp8266_t             *esp;          /**< ESP8266 driver instance (owned by main) */
    MessageBufferHandle_t  msg_buf;      /**< MessageBuffer for ISR → task frame delivery */
    QueueHandle_t          sensor_queue; /**< Queue for pushing decoded sensor frames */
} network_task_config_t;

/*============================================================================
 * Task entry point
 *============================================================================*/

/**
 * @brief Network task entry point (osThreadFunc_t).
 *
 * Phase 1 (blocking UART): initialises ESP8266, connects WiFi, opens UDP.
 * Phase 2 (interrupt UART): switches to ISR-driven reception and posts
 * decoded sensor frames to g_sensor_data_for_display_queue for the display task.
 *
 * @param argument  Pointer to a network_task_config_t (must not be NULL).
 *                  The pointed-to struct must remain valid for the task lifetime.
 */
void network_task(void* argument);

#endif /* APP_NETWORK_H */
