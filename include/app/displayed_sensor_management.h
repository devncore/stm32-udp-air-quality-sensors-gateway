#include <stdint.h>

#ifndef DISPLAYED_SENSOR_MANAGEMENT_H
#define DISPLAYED_SENSOR_MANAGEMENT_H

#define NO_ACTIVE_INDEX_AVAILABLE 0xFF
#define NO_TIMEOUT 0xFF

/**
    @brief: run the internal active sensor strategy
    @param room_name:
    @return: return the active index matching the room_name if any, otherwise return 0xFF (NO_ACTIVE_INDEX_AVAILABLE)
**/
uint8_t displayed_sensor_update(const char* room_name);

/**
    @brief: update timestamps and evaluate if any sensor is in timeout
    @return: return index of the sensor which is in timeout, otherwise if no timeout return 0xFF (NO_ACTIVE_INDEX_AVAILABLE)
**/
uint8_t displayed_sensor_evaluate_timeout(void);

#ifdef UNIT_TEST
/** @brief Reset internal state between unit tests. Not for production use. */
void displayed_sensor_management_reset(void);
#endif

#endif
