/**
 * @file display.h
 * @brief OLED display module for the Air Quality Server.
 *
 * Screen layout (128×128 px, SH1107):
 *
 *   3 px margin on all sides → content area 122×122 px.
 *   4 sensor columns (each ≈ 1/4 of 122 px) separated by 1-px lines.
 *   Each column shows, top-to-bottom with equal vertical spacing:
 *     1. Room name       (static — drawn once via display_draw_room())
 *     2. Temperature     (°C, integer)
 *     3. Humidity        (%, integer)
 *     4. Air quality     (enum label)
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include "app/network_data.h"
#include "cmsis_os.h"

/*============================================================================
 * Air quality classification
 *============================================================================*/

/**
 * @brief Air quality categories derived from the BME680 IAQ index (0–500).
 */
typedef enum {
    AIR_QUALITY_PERFECT = 0,
    AIR_QUALITY_VERY_GOOD,
    AIR_QUALITY_GOOD,
    AIR_QUALITY_MEDIUM,
    AIR_QUALITY_BAD,
    AIR_QUALITY_VERY_BAD
} air_quality_t;

/**
 * @brief Convert a BME680 IAQ index (0–500) to an air_quality_t category.
 *
 * @param iaq  Raw IAQ value from the sensor.
 * @return     Corresponding air_quality_t enum value.
 */
air_quality_t display_iaq_classify(uint16_t iaq);

/*============================================================================
 * Display API
 *============================================================================*/

/**
 * @brief Initialize the display hardware and draw static structural elements.
 *
 * Clears the screen and draws the three 1-px vertical column separators.
 * Room names are NOT drawn here; call display_draw_room() once per column
 * when the room identity is first known.
 */
void display_init(void);

/**
 * @brief Draw the room name label for one sensor column.
 *
 * Intended to be called exactly once per column when the first sensor frame
 * for that room arrives.  The label is clipped to fit the column width and is
 * NOT overwritten by subsequent display_draw_sensor() calls.
 *
 * @param sensor_col  Sensor column index, 0…SENSOR_MAX_ROOMS-1 (left → right).
 * @param name        Null-terminated room name string.
 */
void display_draw_room(uint8_t sensor_col, const char *name);

/**
 * @brief Refresh the dynamic fields (temperature, humidity, air quality)
 *        for one sensor column.
 *
 * The room name field is left untouched.  A single ssd1306_UpdateScreen()
 * call is issued at the end so all three fields appear simultaneously.
 *
 * @param sensor_col  Sensor column index, 0…SENSOR_MAX_ROOMS-1.
 * @param data        Pointer to the latest sensor data; must be non-NULL and valid.
 */
void display_draw_sensor(uint8_t sensor_col, const sensor_data_t *data);

/**
 * @brief Erase all displayed data for one sensor column.
 *
 * Clears the room name and all dynamic fields (temperature, humidity,
 * air quality) by filling the relevant pixel rectangles with black.
 * A single ssd1306_UpdateScreen() call is issued at the end.
 *
 * Intended to be called when a sensor times out or is de-registered, so
 * the column is visually blank and ready to accept a new room.
 *
 * @param sensor_col  Sensor column index, 0…SENSOR_MAX_ROOMS-1.
 */
void display_remove_sensor(uint8_t sensor_col);

/**
 * @brief Configuration passed to display_task() via its pvParameters argument.
 */
typedef struct {
    osMessageQueueId_t sensor_queue; /**< Queue from which decoded sensor frames are consumed */
} display_task_config_t;

/**
 * @brief FreeRTOS task entry point (osThreadFunc_t).
 *
 * Initialises the display and loops, blocking on sensor_queue and driving
 * display_draw_room() / display_draw_sensor() as frames arrive.
 *
 * @param argument  Pointer to a display_task_config_t (must not be NULL).
 *                  The pointed-to struct must remain valid for the task lifetime.
 */
void display_task(void *argument);

#endif /* DISPLAY_H */
