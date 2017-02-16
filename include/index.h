#ifndef ZCS_INDEX_
#define ZCS_INDEX_

#include "types.h"

typedef union {
    bool bit;
    int32_t i32;
    int64_t i64;
    uint64_t len;
} zcs_column_index_value_t;

struct zcs_column_index {
    uint64_t count;
    zcs_column_index_value_t min;
    zcs_column_index_value_t max;
};

#endif
