#ifndef ZCS_COMPRESSION_
#define ZCS_COMPRESSION_

#include "types.h"

void *zcs_compress(enum zcs_compression_type, int level, const void *src,
                   size_t src_size, size_t *dest_size);

bool zcs_decompress(enum zcs_compression_type, const void *src, size_t src_size,
                    void *dest, size_t dest_size);

#endif
