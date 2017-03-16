#ifndef CX_INDEX_
#define CX_INDEX_

#include "types.h"

typedef union {
    bool bit;
    int32_t i32;
    int64_t i64;
    uint64_t len;
} cx_index_value_t;

struct cx_index {
    uint64_t count;
    cx_index_value_t min;
    cx_index_value_t max;
};

#endif
