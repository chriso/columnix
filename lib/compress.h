#ifndef CX_COMPRESSION_
#define CX_COMPRESSION_

#include "types.h"

void *cx_compress(enum cx_compression_type, int level, const void *src,
                  size_t src_size, size_t *dest_size);

bool cx_decompress(enum cx_compression_type, const void *src, size_t src_size,
                   void *dest, size_t dest_size);

#endif