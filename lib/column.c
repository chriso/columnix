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

struct zcs_column_cursor {
    const void *start;
    const void *end;
    const void *position;
};

struct zcs_column *zcs_column_new(enum zcs_column_type type,
                                  enum zcs_encode_type encode)
{
    struct zcs_column *column = calloc(1, sizeof(*column));
    if (!column)
        return NULL;
    column->buffer = malloc(zcs_column_initial_size);
    if (!column->buffer)
        goto error;
    column->size = zcs_column_initial_size;
    column->type = type;
    column->encode = encode;
    return column;
error:
    free(column);
    return NULL;
}

void zcs_column_free(struct zcs_column *column)
{
    free(column->buffer);
    free(column);
}

const void *zcs_column_export(const struct zcs_column *column, size_t *size)
{
    *size = column->offset;
    return column->buffer;
}

ZCS_NO_INLINE static bool zcs_column_resize(struct zcs_column *column,
                                            size_t alloc_size)
{
    size_t size = column->size;
    size_t required_size = column->offset + alloc_size;
    while (size < required_size) {
        assert(size * 2 > size);
        size *= 2;
    }
    void *buffer = realloc(column->buffer, size);
    if (!buffer)
        return false;
    column->buffer = buffer;
    column->size = size;
    return true;
}

static void *zcs_column_tail(const struct zcs_column *column)
{
    return (void *)((uintptr_t)column->buffer + column->offset);
}

static void *zcs_column_alloc(struct zcs_column *column, size_t size)
{
    if (ZCS_UNLIKELY(column->offset + size > column->size))
        if (!zcs_column_resize(column, size))
            return NULL;
    void *ptr = zcs_column_tail(column);
    column->offset += size;
    return ptr;
}

bool zcs_column_put_i32(struct zcs_column *column, int32_t value)
{
    if (ZCS_UNLIKELY(column->type != ZCS_COLUMN_I32))
        return false;
    int32_t *slot = zcs_column_alloc(column, sizeof(int32_t));
    if (ZCS_UNLIKELY(!slot))
        return false;
    *slot = value;
    return true;
}

bool zcs_column_put_i64(struct zcs_column *column, int64_t value)
{
    if (ZCS_UNLIKELY(column->type != ZCS_COLUMN_I64))
        return false;
    int64_t *slot = zcs_column_alloc(column, sizeof(int64_t));
    if (ZCS_UNLIKELY(!slot))
        return false;
    *slot = value;
    return true;
}

struct zcs_column_cursor *zcs_column_cursor_new(const struct zcs_column *column)
{
    struct zcs_column_cursor *cursor = malloc(sizeof(*cursor));
    if (!cursor)
        return NULL;
    cursor->start = column->buffer;
    cursor->end = zcs_column_tail(column);
    zcs_column_cursor_rewind(cursor);
    return cursor;
}

void zcs_column_cursor_free(struct zcs_column_cursor *cursor)
{
    free(cursor);
}

void zcs_column_cursor_rewind(struct zcs_column_cursor *cursor)
{
    cursor->position = cursor->start;
}

bool zcs_column_cursor_valid(const struct zcs_column_cursor *cursor)
{
    return cursor->position < cursor->end;
}

static void zcs_column_cursor_advance(struct zcs_column_cursor *cursor,
                                      size_t amount)
{
    cursor->position = (void *)((uintptr_t)cursor->position + amount);
    assert(cursor->position <= cursor->end);
}

static const void *zcs_column_cursor_next(struct zcs_column_cursor *cursor,
                                          size_t size)
{
    const void *value = cursor->position;
    zcs_column_cursor_advance(cursor, size);
    return value;
}

int32_t zcs_column_cursor_next_i32(struct zcs_column_cursor *cursor)
{
    return *(const int32_t *)zcs_column_cursor_next(cursor, sizeof(int32_t));
}

int64_t zcs_column_cursor_next_i64(struct zcs_column_cursor *cursor)
{
    return *(const int64_t *)zcs_column_cursor_next(cursor, sizeof(int64_t));
}

static size_t zcs_column_cursor_remaining(struct zcs_column_cursor *cursor,
                                          size_t size)
{
    return ((uintptr_t)cursor->end - (uintptr_t)cursor->position) / size;
}

static size_t zcs_column_cursor_skip(struct zcs_column_cursor *cursor,
                                     size_t size, size_t count)
{
    size_t remaining = zcs_column_cursor_remaining(cursor, size);
    if (remaining < count)
        count = remaining;
    zcs_column_cursor_advance(cursor, size * count);
    return count;
}

size_t zcs_column_cursor_skip_i32(struct zcs_column_cursor *cursor,
                                  size_t count)
{
    return zcs_column_cursor_skip(cursor, sizeof(int32_t), count);
}

size_t zcs_column_cursor_skip_i64(struct zcs_column_cursor *cursor,
                                  size_t count)
{
    return zcs_column_cursor_skip(cursor, sizeof(int64_t), count);
}

static const void *zcs_column_cursor_next_batch(
    struct zcs_column_cursor *cursor, size_t size, size_t count,
    size_t *available)
{
    const void *values = cursor->position;
    *available = zcs_column_cursor_skip(cursor, size, count);
    return values;
}

const int32_t *zcs_column_cursor_next_batch_i32(
    struct zcs_column_cursor *cursor, size_t count, size_t *available)
{
    return (const int32_t *)zcs_column_cursor_next_batch(
        cursor, sizeof(int32_t), count, available);
}

const int64_t *zcs_column_cursor_next_batch_i64(
    struct zcs_column_cursor *cursor, size_t count, size_t *available)
{
    return (const int64_t *)zcs_column_cursor_next_batch(
        cursor, sizeof(int64_t), count, available);
}
