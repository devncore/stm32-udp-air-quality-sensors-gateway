/**
 * @file error_manager.h
 * @brief Application-level error tracking via a 32-bit bitfield.
 *
 * Each error_id_t value maps to one bit in an internal uint32_t register.
 * Thread and ISR safety is provided internally via FreeRTOS critical sections.
 * Use error_set() / error_reset() from task context, and the _from_isr()
 * variants from interrupt handlers.
 */

#ifndef ERROR_MANAGER_H
#define ERROR_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

/*============================================================================
 * Error identifiers
 *============================================================================*/

/**
 * @brief Enumeration of all application error flags.
 *
 * Each entry represents one bit in the 32-bit error register.
 * Add concrete error names here as the project grows; keep ERROR_COUNT last.
 *
 * @note Maximum 32 entries (uint32_t bitfield).
 */
typedef enum {
    ERROR_MESSAGE_BUFFER_OVERFLOW = 0,  /* byte message buffer overflow */
    ERROR_SENSOR_FRAME_CRC_INVALID,     /* CRC has been wrong several times */
    ERROR_SENSOR_QUEUE_PUT_FAILED,      /* Error while trying to add a queue message (see network.c) */
    ERROR_ESP8266_INIT_FAILED,       
    ERROR_WIFI_CONNECT_TIMEOUT, 
    ERROR_UDP_START_FAILED,

    ERROR_COUNT              /**< Sentinel — must remain last */
} error_id_t;

/*============================================================================
 * API
 *============================================================================*/

/**
 * @brief Set (raise) an error flag.
 *
 * Must be called from task context only.
 *
 * @param id  Error to set. Ignored if id >= ERROR_COUNT.
 */
void error_set(error_id_t id);

/**
 * @brief Set (raise) an error flag from an ISR.
 *
 * ISR-safe variant of error_set(). Use this when calling from an
 * interrupt handler.
 *
 * @param id  Error to set. Ignored if id >= ERROR_COUNT.
 */
void error_set_from_isr(error_id_t id);

/**
 * @brief Clear (acknowledge) an error flag.
 *
 * @param id  Error to clear. Ignored if id >= ERROR_COUNT.
 */
void error_reset(error_id_t id);

/**
 * @brief Clear (acknowledge) an error flag from an ISR.
 *
 * ISR-safe variant of error_reset(). Use this when calling from an
 * interrupt handler.
 *
 * @param id  Error to clear. Ignored if id >= ERROR_COUNT.
 */
void error_reset_from_isr(error_id_t id);

/**
 * @brief Check whether a specific error flag is currently set.
 *
 * @param id  Error to test.
 * @return    true if the flag is set, false otherwise (or if id >= ERROR_COUNT).
 */
bool error_is_active(error_id_t id);

/**
 * @brief Return the raw 32-bit error register.
 *
 * Bit N corresponds to error_id_t value N.
 *
 * @return  Current bitfield snapshot.
 */
uint32_t error_get_all(void);

#endif /* ERROR_MANAGER_H */
