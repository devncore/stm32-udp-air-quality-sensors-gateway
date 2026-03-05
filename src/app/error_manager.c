/**
 * @file error_manager.c
 * @brief Application-level error tracking via a 32-bit bitfield.
 */

#include "app/error_manager.h"

#include "FreeRTOS.h"  // IWYU pragma: keep — must precede all other FreeRTOS headers
#include "task.h"

/*============================================================================
 * Internal state
 *============================================================================*/

static uint32_t s_error_register = 0U;

/*============================================================================
 * API implementation
 *============================================================================*/

void error_set(error_id_t id)
{
    if (id >= ERROR_COUNT) {
        return;
    }
    taskENTER_CRITICAL();
    s_error_register |= (1U << (uint32_t)id);
    taskEXIT_CRITICAL();
}

void error_set_from_isr(error_id_t id)
{
    if (id >= ERROR_COUNT) {
        return;
    }
    const UBaseType_t mask = portSET_INTERRUPT_MASK_FROM_ISR();
    s_error_register |= (1U << (uint32_t)id);
    portCLEAR_INTERRUPT_MASK_FROM_ISR(mask);
}

void error_reset(error_id_t id)
{
    if (id >= ERROR_COUNT) {
        return;
    }
    taskENTER_CRITICAL();
    s_error_register &= ~(1U << (uint32_t)id);
    taskEXIT_CRITICAL();
}

void error_reset_from_isr(error_id_t id)
{
    if (id >= ERROR_COUNT) {
        return;
    }
    const UBaseType_t mask = portSET_INTERRUPT_MASK_FROM_ISR();
    s_error_register &= ~(1U << (uint32_t)id);
    portCLEAR_INTERRUPT_MASK_FROM_ISR(mask);
}

bool error_is_active(error_id_t id)
{
    if (id >= ERROR_COUNT) {
        return false;
    }
    taskENTER_CRITICAL();
    const bool active = (s_error_register & (1U << (uint32_t)id)) != 0U;
    taskEXIT_CRITICAL();
    return active;
}

uint32_t error_get_all(void)
{
    taskENTER_CRITICAL();
    const uint32_t snapshot = s_error_register;
    taskEXIT_CRITICAL();
    return snapshot;
}
