#ifndef ZCS_FILE_
#define ZCS_FILE_

#include "index.h"

#define ZCS_FILE_MAGIC 0x65726f7473637a1dLLU

#define ZCS_WRITE_ALIGN 8

struct zcs_header {
    uint64_t magic;
};

struct zcs_footer {
    uint32_t row_group_count;
    uint32_t column_count;
    uint64_t magic;
};

struct zcs_column_descriptor {
    uint32_t type;
    uint32_t encoding;
    uint32_t compression;
    int32_t level;
};

struct zcs_row_group_header {
    uint64_t size;
    uint64_t offset;
};

struct zcs_column_header {
    uint64_t offset;
    uint64_t size;
    uint64_t decompressed_size;
    struct zcs_column_index index;
};

#endif
