/**
 * @file freertos_test_helpers.h
 * @brief Test-only helpers for controlling the FreeRTOS tick stub.
 */

#ifndef FREERTOS_TEST_HELPERS_H
#define FREERTOS_TEST_HELPERS_H

#include "FreeRTOS.h"

/** Override the value returned by xTaskGetTickCount(). */
void freertos_stub_set_tick(TickType_t ticks);

#endif /* FREERTOS_TEST_HELPERS_H */
