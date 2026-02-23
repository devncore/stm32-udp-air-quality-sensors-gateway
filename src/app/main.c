/**
 * @file main.c
 * @brief Air Quality Server — Application entry point and FreeRTOS tasks
 *
 * STM32F401RE + ESP-01S (UDP) + SH1107 OLED
 *
 * Hardware connections:
 *   - USART2 (PA2=TX, PA3=RX) -> ESP-01S WiFi module
 *   - SPI1 (PA5=SCK, PA7=MOSI) + GPIO (PB6=CS, PA8=DC, PA9=RST) -> SH1107
 *
 * This file provides the application entry point called from the CubeMX
 * default task. It owns hardware instances, synchronisation primitives,
 * and creates all application tasks.
 */

#include "app/main.h"

#include "app/config.h"
#include "app/hal_interface.h"
#include "app/network.h"
#include "app/network_data.h"
#include "app/display.h"
#include "app/uart_rx.h"
#include "drivers/esp8266/esp8266.h"

#include "cmsis_os.h"
#include "main.h"

/* External HAL handles (defined in CubeMX-generated code) */
extern UART_HandleTypeDef huart2;
extern SPI_HandleTypeDef  hspi1;  /* SPI1 — OLED (PA5=SCK, PA7=MOSI) */

/*============================================================================
 * Static module instances (no heap allocation)
 *============================================================================*/

/* HAL interfaces */
static stm32_uart_t* g_uart;

/* Driver instances */
static esp8266_t g_esp8266;

/*============================================================================
 * Shared synchronisation primitives
 *============================================================================*/

osMessageQueueId_t g_sensor_queue;

/*============================================================================
 * Application entry point
 *============================================================================*/

void app_main(void)
{
    /* HAL and peripherals are already initialised by CubeMX main() */

    /* Create HAL interface */
    g_uart = stm32_uart_init(&huart2);

    /* Create driver instance */
    esp8266_create(&g_esp8266, g_uart);

    /* Initialise UART RX module (does NOT enable interrupts yet) */
    uart_rx_init(&huart2);

    /* Create synchronisation primitives */

    // shared queue between network and display tasks
    // network task: produce new sensor data
    // display task: consume those data
    g_sensor_queue = osMessageQueueNew(8, sizeof(sensor_data_t), NULL);

    /* Create Network Task (high priority, per SOFTWARE.md) */
    const osThreadAttr_t network_attr = {
        .name       = "networkTask",
        .stack_size = CONFIG_NETWORK_TASK_STACK_SIZE * 4,
        .priority   = (osPriority_t)CONFIG_NETWORK_TASK_PRIORITY,
    };
    osThreadNew(network_task, &g_esp8266, &network_attr);

    /* Create Display Task (low priority, per SOFTWARE.md) */
    const osThreadAttr_t display_attr = {
        .name       = "displayTask",
        .stack_size = CONFIG_NETWORK_TASK_STACK_SIZE * 4,
        .priority   = (osPriority_t)CONFIG_NETWORK_TASK_PRIORITY,
    };
    osThreadNew(display_task, NULL, &display_attr);

    /*
     * The calling thread (defaultTask) is no longer needed.
     * Terminate it to free its stack.
     */
    osThreadExit();
}
