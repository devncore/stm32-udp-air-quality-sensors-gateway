#include "app/displayed_sensor_management.h"
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
} active_sensor_data_t;

// keep some static data which represents 'active' sensors
static active_sensor_data_t active_sensors[NUMBER_OF_SUPPORTED_SENSORS];
static uint8_t next_free_active_sensor_index;

// helpers
static inline uint32_t current_time_stamp_ms()
{
    const TickType_t now = xTaskGetTickCount();
    return portTICK_PERIOD_MS * now;
}

// public functions
uint8_t displayed_sensor_update(const char* room_name)
{
    for(uint8_t index=0u;index<NUMBER_OF_SUPPORTED_SENSORS;index++)
    {
        // find if current sensor data is matching an active sensor
        if(strcmp(room_name,(const char*)&active_sensors[index].room_name) == 0)
        {
            // already refered as active sensor, then update timestamp and leave
            active_sensors[index].last_timestamp_milliseconds = current_time_stamp_ms();
            return index;
        }
    }

    // sensor is not yet refered as active
    // if any room for a new sensor, take it, otherwise data would be dropped
    if(next_free_active_sensor_index<NUMBER_OF_SUPPORTED_SENSORS)
    {
        // at least one index is free, then, take the seat
        snprintf((char *)&active_sensors[next_free_active_sensor_index].room_name, ROOM_NAME_MAX_SIZE, "%s", room_name);
        active_sensors[next_free_active_sensor_index].current_index = next_free_active_sensor_index;
        active_sensors[next_free_active_sensor_index].last_timestamp_milliseconds = current_time_stamp_ms();
        
        return next_free_active_sensor_index;
    }
    else {
        // no index free
        return NO_ACTIVE_INDEX_AVAILABLE;
    }
        
}
