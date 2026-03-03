/**
 * @file network_data.h
 * @brief Decoded sensor data shared between network and display tasks
 *
 * The network task parses raw UDP payloads ("room:temp,hum,iaq") and
 * posts each decoded sensor_data_t to a sensor queue. The display task
 * blocks on the queue and processes updates as they arrive.
 */

#ifndef NETWORK_DATA_H
#define NETWORK_DATA_H

#include <stdbool.h>
#include <stdint.h>

/*============================================================================
 * Constants
 *============================================================================*/

#define SENSOR_MAX_ROOMS      4
#define SENSOR_ROOM_NAME_LEN  16

/*============================================================================
 * Sensor data structure
 *============================================================================*/

typedef struct {
    char     room[SENSOR_ROOM_NAME_LEN];
    float    temperature;
    float    humidity;
    uint16_t iaq;
    bool     valid;
} sensor_data_t;

#endif /* NETWORK_DATA_H */
