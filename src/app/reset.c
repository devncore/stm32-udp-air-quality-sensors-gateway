/**
 * @file reset.c
 * @brief Reset cause detection from the RCC CSR register.
 */

#include "app/reset.h"

#include "stm32f4xx_hal.h"

#include <stdbool.h>

reset_cause_t get_reset_cause(void)
{
    /* Sample all relevant flags before clearing them. Flags persist across
     * soft resets and are cleared only when RMVF is written (done below).
     * PINRST is excluded: it fires alongside PORRST on normal power-up. */
    const bool iwdg_rst  = __HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST)  != 0U;
    const bool por_rst   = __HAL_RCC_GET_FLAG(RCC_FLAG_PORRST)   != 0U;
    const bool other_rst = (__HAL_RCC_GET_FLAG(RCC_FLAG_BORRST)  != 0U) ||
                           (__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST)  != 0U) ||
                           (__HAL_RCC_GET_FLAG(RCC_FLAG_WWDGRST) != 0U) ||
                           (__HAL_RCC_GET_FLAG(RCC_FLAG_LPWRRST) != 0U);
    __HAL_RCC_CLEAR_RESET_FLAGS();

    if (iwdg_rst) {
        return RESET_CAUSE_WATCHDOG;
    }
    if (por_rst || !other_rst) {
        /* PORRST set, or only PINRST was set (debugger / reset button) */
        return RESET_CAUSE_POWER_ON;
    }
    return RESET_CAUSE_OTHER;
}
