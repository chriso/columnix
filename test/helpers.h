#define MUNIT_ENABLE_ASSERT_ALIASES
#include "munit.h"

#define ZCS_FOREACH(array, item)                                               \
    for (size_t __i = 0;                                                       \
         __i < sizeof(array) / sizeof(*(array)) ? (((item) = (array)[__i]), 1) \
                                                : 0;                           \
         __i++)
