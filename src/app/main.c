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

/* Task configurations (must outlive app_main) */
static network_task_config_t g_network_cfg;
static display_task_config_t g_display_cfg;

/*============================================================================
 * Shared synchronisation primitives
 *============================================================================*/

static osMessageQueueId_t g_sensor_queue;
static MessageBufferHandle_t g_msg_buffer;

/*============================================================================
 * Application entry point
 *============================================================================*/

void app_main(void)
{
    /* HAL and peripherals are already initialised by CubeMX main() */

    /* affect shared queue/buffer */
    g_sensor_queue = osMessageQueueNew(8, sizeof(sensor_data_t), NULL);
    g_msg_buffer = xMessageBufferCreate(RTOS_MESSAGE_BUFFER_LEN);

    /* initialization of uart */
    g_uart = stm32_uart_init(&huart2);
    uart_rx_init(&huart2, g_msg_buffer);

    /* external components intizialisation */
    esp8266_create(&g_esp8266, g_uart);


    /* ------------------------------------------ */
    /* --------------- RTOS tasks --------------- */
    /* ------------------------------------------ */

    /* Create Network Task (high priority) */
    g_network_cfg.esp          = &g_esp8266;
    g_network_cfg.msg_buf      = g_msg_buffer;
    g_network_cfg.sensor_queue = g_sensor_queue;
    const osThreadAttr_t network_attr = {
        .name       = "networkTask",
        .stack_size = CONFIG_NETWORK_TASK_STACK_SIZE * 4,
        .priority   = (osPriority_t)CONFIG_NETWORK_TASK_PRIORITY,
    };
    osThreadNew(network_task, &g_network_cfg, &network_attr);

    /* Create Display Task (low priority) */
    g_display_cfg.sensor_queue = g_sensor_queue;
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
