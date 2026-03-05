#ifndef ASSERT_H
#define ASSERT_H

#define ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            __disable_irq(); \
            while (1) { __BKPT(0); } \
        } \
    } while (0)

#endif
