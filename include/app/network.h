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

/**
 * @brief Network task entry point (osThreadFunc_t).
 *
 * Phase 1 (blocking UART): initialises ESP8266, connects WiFi, opens UDP.
 * Phase 2 (interrupt UART): switches to ISR-driven reception and posts
 * decoded sensor frames to g_sensor_queue for the display task.
 *
 * @param argument  Pointer to an esp8266_t instance (must not be NULL)
 */
void network_task(void* argument);

#endif /* APP_NETWORK_H */
