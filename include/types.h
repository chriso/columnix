#ifndef ZCS_TYPES_
#define ZCS_TYPES_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum zcs_column_type {
    ZCS_COLUMN_BIT,
    ZCS_COLUMN_I32,
    ZCS_COLUMN_I64,
    ZCS_COLUMN_STR
};

enum zcs_encoding_type { ZCS_ENCODING_NONE };

enum zcs_compression_type {
    ZCS_COMPRESSION_NONE,
    ZCS_COMPRESSION_LZ4,
    ZCS_COMPRESSION_LZ4HC,
    ZCS_COMPRESSION_ZSTD
};

struct zcs_string {
    const char *ptr;
    size_t len;
};

typedef union {
    bool bit;
    int32_t i32;
    int64_t i64;
    struct zcs_string str;
} zcs_value_t;

#endif
