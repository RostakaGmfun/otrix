#ifndef OTRIX_ASSERT_H
#define OTRIX_ASSERT_H

#include "arch/common.h"

#define kASSERT(expr) \
    do { \
        arch_assert_panic(__FILE__, __LINE__, #expr); }    \
    while (!(expr));

#endif // OTRIX_ASSERT_H
