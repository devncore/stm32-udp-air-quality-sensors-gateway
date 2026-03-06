/**
 * @file reset.h
 * @brief Reset cause detection from the RCC CSR register.
 *
 * Reads the RCC reset-source flags on boot, classifies the cause into one
 * of three categories, and clears the flags so the next boot starts clean.
 */

#ifndef APP_RESET_H
#define APP_RESET_H

/**
 * @brief Classification of the MCU reset cause.
 */
typedef enum {
    RESET_CAUSE_POWER_ON,  /**< Normal power cycle (POR), or external pin reset */
    RESET_CAUSE_WATCHDOG,  /**< IWDG counter reached zero (watchdog timeout) */
    RESET_CAUSE_OTHER,     /**< BOR (unstable supply), software reset, WWDG, low-power reset */
} reset_cause_t;

/**
 * @brief Read, classify, and clear the RCC reset-source flags.
 *
 * Must be called once at boot, before any other module that may trigger a
 * software reset. The RCC CSR flags are cleared after sampling so they do
 * not persist into the next reset cycle.
 *
 * @return  The classified reset cause.
 */
reset_cause_t get_reset_cause(void);

#endif /* APP_RESET_H */
