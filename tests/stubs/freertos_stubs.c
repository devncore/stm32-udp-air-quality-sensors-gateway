/**
 * @file freertos_stubs.c
 * @brief FreeRTOS stub implementations for host-side unit tests.
 *
 * Provides a controllable tick counter so tests can exercise
 * timestamp-dependent logic deterministically.
 */

#include "task.h"
#include "freertos_test_helpers.h"

static TickType_t s_tick_count = 0U;

void freertos_stub_set_tick(TickType_t ticks)
{
    s_tick_count = ticks;
}

TickType_t xTaskGetTickCount(void)
{
    return s_tick_count;
}
