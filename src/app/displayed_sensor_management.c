#include "app/displayed_sensor_management.h"
#include "app/config.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "portmacro.h"
#include "task.h"

// constants
#define ROOM_NAME_MAX_SIZE 16U
#define NUMBER_OF_SUPPORTED_SENSORS 4

// internal types
typedef struct
{
    char room_name[ROOM_NAME_MAX_SIZE];
    uint8_t current_index;
    uint32_t last_timestamp_milliseconds;
    bool is_active;
} sensor_slot_t;

// keep some static data which represents 'active' sensors
static sensor_slot_t active_sensors[NUMBER_OF_SUPPORTED_SENSORS];

// helpers
static inline uint32_t current_time_stamp_ms()
{
    const TickType_t now = xTaskGetTickCount();
    return portTICK_PERIOD_MS * now;
}

#ifdef UNIT_TEST
void displayed_sensor_management_reset(void)
{
    memset(active_sensors, 0, sizeof(active_sensors));
}
#endif

// public functions
uint8_t displayed_sensor_update(const char* room_name)
{
    uint8_t free_slot = NO_ACTIVE_INDEX_AVAILABLE;

    for(uint8_t index = 0u; index < NUMBER_OF_SUPPORTED_SENSORS; index++)
    {
        if(active_sensors[index].is_active)
        {
            if(strcmp(room_name, active_sensors[index].room_name) == 0)
            {
                active_sensors[index].last_timestamp_milliseconds = current_time_stamp_ms();
                return index;
            }
        }
        else if(free_slot == NO_ACTIVE_INDEX_AVAILABLE)
        {
            free_slot = index;
        }
    }

    // sensor not yet active — take the first free slot if available
    if(free_slot == NO_ACTIVE_INDEX_AVAILABLE)
    {
        return NO_ACTIVE_INDEX_AVAILABLE;
    }

    snprintf(active_sensors[free_slot].room_name, ROOM_NAME_MAX_SIZE, "%s", room_name);
    active_sensors[free_slot].current_index = free_slot;
    active_sensors[free_slot].last_timestamp_milliseconds = current_time_stamp_ms();
    active_sensors[free_slot].is_active = true;

    return free_slot;
}

uint8_t displayed_sensor_evaluate_timeout(void)
{
    const uint32_t now = current_time_stamp_ms();

    for(uint8_t i = 0u; i < NUMBER_OF_SUPPORTED_SENSORS; i++)
    {
        if(!active_sensors[i].is_active)
        {
            continue;
        }

        if((now - active_sensors[i].last_timestamp_milliseconds) >= DISPLAY_SENSOR_TIMEOUT_MS)
        {
            active_sensors[i].is_active = false;
            return i;
        }
    }

    return NO_TIMEOUT;
}
