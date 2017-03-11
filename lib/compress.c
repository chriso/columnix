#include <limits.h>
#include <stdlib.h>

#include <lz4hc.h>
#include <zstd.h>

#include "compress.h"

static void *cx_compress_lz4(int level, const void *src, size_t src_size,
                             size_t *dest_size)
{
    if (src_size > INT_MAX)
        return NULL;
    int max_size = LZ4_compressBound(src_size);
    if (!max_size)
        return NULL;
    void *compressed = malloc(max_size);
    if (!compressed)
        return NULL;
    int result = LZ4_compress_fast(src, compressed, src_size, max_size, level);
    if (!result)
        goto error;
    *dest_size = result;
    return compressed;
error:
    free(compressed);
    return NULL;
}

static bool cx_decompress_lz4(const void *src, size_t src_size, void *dest,
                              size_t dest_size)
{
    int result = LZ4_decompress_safe(src, dest, src_size, dest_size);
    return result && (size_t)result == dest_size;
}

static void *cx_compress_lz4hc(int level, const void *src, size_t src_size,
                               size_t *dest_size)
{
    if (src_size > INT_MAX)
        return NULL;
    int max_size = LZ4_compressBound(src_size);
    if (!max_size)
        return NULL;
    void *compressed = malloc(max_size);
    if (!compressed)
        return NULL;
    int result = LZ4_compress_HC(src, compressed, src_size, max_size, level);
    if (!result)
        goto error;
    *dest_size = result;
    return compressed;
error:
    free(compressed);
    return NULL;
}

static void *cx_compress_zstd(int level, const void *src, size_t src_size,
                              size_t *dest_size)
{
    int max_size = ZSTD_compressBound(src_size);
    if (!max_size)
        return NULL;
    void *compressed = malloc(max_size);
    if (!compressed)
        return NULL;
    size_t result = ZSTD_compress(compressed, max_size, src, src_size, level);
    if (ZSTD_isError(result))
        goto error;
    *dest_size = result;
    return compressed;
error:
    free(compressed);
    return NULL;
}

static bool cx_decompress_zstd(const void *src, size_t src_size, void *dest,
                               size_t dest_size)
{
    size_t result = ZSTD_decompress(dest, dest_size, src, src_size);
    return !ZSTD_isError(result) && result == dest_size;
}

void *cx_compress(enum cx_compression_type type, int level, const void *src,
                  size_t src_size, size_t *dest_size)
{
    if (type == CX_COMPRESSION_LZ4)
        return cx_compress_lz4(level, src, src_size, dest_size);
    else if (type == CX_COMPRESSION_LZ4HC)
        return cx_compress_lz4hc(level, src, src_size, dest_size);
    else if (type == CX_COMPRESSION_ZSTD)
        return cx_compress_zstd(level, src, src_size, dest_size);
    else
        return NULL;
}

bool cx_decompress(enum cx_compression_type type, const void *src,
                   size_t src_size, void *dest, size_t dest_size)
{
    if (type == CX_COMPRESSION_LZ4 || type == CX_COMPRESSION_LZ4HC)
        return cx_decompress_lz4(src, src_size, dest, dest_size);
    else if (type == CX_COMPRESSION_ZSTD)
        return cx_decompress_zstd(src, src_size, dest, dest_size);
    else
        return false;
}
