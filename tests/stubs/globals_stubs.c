/**
 * @file globals_stubs.c
 * @brief Stub definitions for shared application globals used in tests.
 *
 * network_data.h declares g_sensor_queue as extern; this file provides
 * a NULL definition so test executables that pull in display.c can link.
 */

#include <stddef.h>
#include "app/network_data.h"

osMessageQueueId_t g_sensor_queue = NULL;
