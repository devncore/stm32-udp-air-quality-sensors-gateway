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
#include "toolbox/assert.h"

#include "cmsis_os.h"
#include "queue.h"
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

/* Task configurations (must outlive app_main) */
static network_task_config_t g_network_cfg;
static display_task_config_t g_display_cfg;

/*============================================================================
 * Shared synchronisation primitives
 *============================================================================*/

#define SENSOR_QUEUE_LENGTH 8U
static uint8_t       g_sensor_data_for_display_queue_storage[SENSOR_QUEUE_LENGTH * sizeof(sensor_data_t)];
static StaticQueue_t g_sensor_data_for_display_queue_struct;
static QueueHandle_t g_sensor_data_for_display_queue;
static MessageBufferHandle_t g_raw_data_buffer;

/*============================================================================
 * Application entry point
 *============================================================================*/

void app_main(void)
{
    /* NB: HAL and peripherals are already initialised by CubeMX main() */

    /* --------------------------------------------------------------------- */
    /* --------------- Initialize shared data and components --------------- */
    /* --------------------------------------------------------------------- */

    /* affect shared queue/buffer */
    g_raw_data_buffer = xMessageBufferCreate(RTOS_MESSAGE_BUFFER_LEN);
    g_sensor_data_for_display_queue = xQueueCreateStatic(SENSOR_QUEUE_LENGTH, sizeof(sensor_data_t),
                                        g_sensor_data_for_display_queue_storage, &g_sensor_data_for_display_queue_struct);
    ASSERT(g_raw_data_buffer!=NULL, "g_raw_data_buffer is NULL");
    ASSERT(g_sensor_data_for_display_queue!=NULL, "g_sensor_data_for_display_queue is NULL");

    /* initialization of uart */
    g_uart = stm32_uart_init(&huart2);
    ASSERT(g_uart!=NULL, "g_uart is NULL");
    uart_rx_init(&huart2, g_raw_data_buffer);

    /* external components intizialisation */
    esp8266_create(&g_esp8266, g_uart);


    /* ------------------------------------------ */
    /* --------------- RTOS tasks --------------- */
    /* ------------------------------------------ */

    /* Create Network Task (high priority) */
    g_network_cfg.esp          = &g_esp8266;
    g_network_cfg.msg_buf      = g_raw_data_buffer;
    g_network_cfg.sensor_queue = g_sensor_data_for_display_queue;
    const osThreadAttr_t network_attr = {
        .name       = "networkTask",
        .stack_size = CONFIG_NETWORK_TASK_STACK_SIZE * 4,
        .priority   = (osPriority_t)CONFIG_NETWORK_TASK_PRIORITY,
    };
    osThreadNew(network_task, &g_network_cfg, &network_attr);

    /* Create Display Task (low priority) */
    g_display_cfg.sensor_queue = g_sensor_data_for_display_queue;
    const osThreadAttr_t display_attr = {
        .name       = "displayTask",
        .stack_size = CONFIG_DISPLAY_TASK_STACK_SIZE * 4,
        .priority   = (osPriority_t)CONFIG_DISPLAY_TASK_PRIORITY,
    };
    osThreadNew(display_task, &g_display_cfg, &display_attr);

    /*
     * The calling thread (defaultTask) is no longer needed.
     * Terminate it to free its stack.
     */
    osThreadExit();
}
