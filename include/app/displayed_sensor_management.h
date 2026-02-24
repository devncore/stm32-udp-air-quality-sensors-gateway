#include <stdint.h>

#ifndef DISPLAYED_SENSOR_MANAGEMENT_H
#define DISPLAYED_SENSOR_MANAGEMENT_H

#define NO_ACTIVE_INDEX_AVAILABLE 0xFF

/** 
    @brief: run the internal active sensor strategy
    @param room_name:  
    @return: return the active index matching the room_name if any, otherwise return 0xFF (NO_ACTIVE_INDEX_AVAILABLE)
**/
uint8_t displayed_sensor_update(const char* room_name);

#endif