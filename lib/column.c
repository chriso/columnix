#include <assert.h>
#include <stdlib.h>

#include "column.h"
#include "common.h"

static const size_t zcs_column_initial_size = 64;

struct zcs_column {
    void *buffer;
    size_t offset;
    size_t size;
    enum zcs_column_type type;
    enum zcs_encode_type encode;
};

struct zcs_column *zcs_column_new(enum zcs_column_type type,
                                  enum zcs_encode_type encode)
{
    struct zcs_column *col = calloc(1, sizeof(*col));
    if (!col)
        return NULL;
    col->buffer = malloc(zcs_column_initial_size);
    if (!col->buffer) {
        free(col);
        return NULL;
    }
    col->size = zcs_column_initial_size;
    col->type = type;
    col->encode = encode;
    return col;
}

void zcs_column_free(struct zcs_column *col)
{
    free(col->buffer);
    free(col);
}

const void *zcs_column_export(const struct zcs_column *col, size_t *size)
{
    *size = col->offset;
    return col->buffer;
}

ZCS_NO_INLINE static bool zcs_column_resize(struct zcs_column *col,
                                            size_t alloc_size)
{
    size_t size = col->size;
    size_t required_size = col->offset + alloc_size;
    while (size < required_size) {
        assert(size * 2 > size);
        size *= 2;
    }
    void *buffer = realloc(col->buffer, size);
    if (!buffer)
        return false;
    col->buffer = buffer;
    col->size = size;
    return true;
}

static void *zcs_column_alloc(struct zcs_column *col, size_t size)
{
    if (ZCS_UNLIKELY(col->offset + size > col->size))
        if (!zcs_column_resize(col, size))
            return NULL;
    void *ptr = (void *)((uintptr_t)col->buffer + col->offset);
    col->offset += size;
    return ptr;
}

bool zcs_column_put_i32(struct zcs_column *col, int32_t value)
{
    if (ZCS_UNLIKELY(col->type != ZCS_COLUMN_I32))
        return false;
    int32_t *slot = zcs_column_alloc(col, sizeof(int32_t));
    if (ZCS_UNLIKELY(!slot))
        return false;
    *slot = value;
    return true;
}

bool zcs_column_put_i64(struct zcs_column *col, int64_t value)
{
    if (ZCS_UNLIKELY(col->type != ZCS_COLUMN_I64))
        return false;
    int64_t *slot = zcs_column_alloc(col, sizeof(int64_t));
    if (ZCS_UNLIKELY(!slot))
        return false;
    *slot = value;
    return true;
}
