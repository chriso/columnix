#ifndef CX_TYPES_H_
#define CX_TYPES_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CX_EXPORT __attribute__((visibility("default")))

enum cx_column_type {
    CX_COLUMN_BIT,
    CX_COLUMN_I32,
    CX_COLUMN_I64,
    CX_COLUMN_FLT,
    CX_COLUMN_DBL,
    CX_COLUMN_STR
};

enum cx_encoding_type { CX_ENCODING_NONE };

enum cx_compression_type {
    CX_COMPRESSION_NONE,
    CX_COMPRESSION_LZ4,
    CX_COMPRESSION_LZ4HC,
    CX_COMPRESSION_ZSTD
};

struct cx_string {
    const char *ptr;
    size_t len;
};

typedef union {
    bool bit;
    int32_t i32;
    int64_t i64;
    float flt;
    double dbl;
    struct cx_string str;
} cx_value_t;

enum cx_str_location {
    CX_STR_LOCATION_START,
    CX_STR_LOCATION_END,
    CX_STR_LOCATION_ANY
};

#endif
