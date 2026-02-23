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
 * NOT overwritten by subsequent display_update_sensor() calls.
 *
 * @param col   Column index, 0…SENSOR_MAX_ROOMS-1 (left → right).
 * @param name  Null-terminated room name string.
 */
void display_draw_room(uint8_t col, const char *name);

/**
 * @brief Refresh the dynamic fields (temperature, humidity, air quality)
 *        for one sensor column.
 *
 * The room name field is left untouched.  A single ssd1306_UpdateScreen()
 * call is issued at the end so all three fields appear simultaneously.
 *
 * @param col   Column index, 0…SENSOR_MAX_ROOMS-1.
 * @param data  Pointer to the latest sensor data; must be non-NULL and valid.
 */
void display_update_sensor(uint8_t col, const sensor_data_t *data);

/**
 * @brief FreeRTOS task entry point (osThreadFunc_t).
 *
 * Initialises the display and loops.  When g_sensor_queue is wired up,
 * this task will block on the queue and drive display_draw_room() /
 * display_update_sensor() as frames arrive.
 *
 * @param argument  Unused (reserved for future use).
 */
void display_task(void *argument);

#endif /* DISPLAY_H */
