/**
 * @file cmsis_os.h  [STUB]
 * @brief Minimal CMSIS-RTOS v2 stub for host-side unit tests.
 */

#ifndef CMSIS_OS_H
#define CMSIS_OS_H

#include <stdint.h>

typedef void    *osMessageQueueId_t;
typedef int32_t  osStatus_t;

#define osOK           0

#define osWaitForever  0xFFFFFFFFU

static inline osStatus_t osMessageQueueGet(osMessageQueueId_t mq_id,
                                            void               *msg_ptr,
                                            uint8_t            *msg_prio,
                                            uint32_t            timeout)
{
    (void)mq_id; (void)msg_ptr; (void)msg_prio; (void)timeout;
    return 0;
}

#endif /* CMSIS_OS_H */
