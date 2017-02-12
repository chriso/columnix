#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "column.h"
#include "common.h"

static const size_t zcs_column_initial_size = 64;

struct zcs_column {
    union {
        void *mutable;
        const void *immutable;
    } buffer;
    size_t offset;
    size_t size;
    enum zcs_column_type type;
    enum zcs_encode_type encode;
    struct zcs_column_index index;
    bool immutable;
};

typedef union {
    bool bit;
    int32_t i32;
    int64_t i64;
    struct zcs_string str;
} zcs_value_t;

struct zcs_column_cursor {
    const struct zcs_column *column;
    const void *start;
    const void *end;
    const void *position;
    size_t offset;
    size_t bitset_offset;
    zcs_value_t buffer[ZCS_BATCH_SIZE];
};

static struct zcs_column *zcs_column_new_size(
    enum zcs_column_type type, enum zcs_encode_type encode, size_t size,
    const struct zcs_column_index *index)
{
    struct zcs_column *column = calloc(1, sizeof(*column));
    if (!column)
        return NULL;
    if (size) {
        column->buffer.mutable = malloc(size);
        if (!column->buffer.mutable)
            goto error;
        column->size = size;
    }
    column->type = type;
    column->encode = encode;
    if (index)
        memcpy(&column->index, index, sizeof(*index));
    return column;
error:
    free(column);
    return NULL;
}

struct zcs_column *zcs_column_new(enum zcs_column_type type,
                                  enum zcs_encode_type encode)
{
    return zcs_column_new_size(type, encode, zcs_column_initial_size, NULL);
}

struct zcs_column *zcs_column_new_immutable(
    enum zcs_column_type type, enum zcs_encode_type encode, const void *ptr,
    size_t size, const struct zcs_column_index *index)
{
    struct zcs_column *column = zcs_column_new_size(type, encode, 0, index);
    if (!column)
        return NULL;
    column->offset = size;
    column->size = size;
    column->immutable = true;
    column->buffer.immutable = ptr;
    return column;
}

struct zcs_column *zcs_column_new_compressed(
    enum zcs_column_type type, enum zcs_encode_type encode, void **ptr,
    size_t size, const struct zcs_column_index *index)
{
    if (!size)
        return NULL;
    struct zcs_column *column = zcs_column_new_size(type, encode, size, index);
    if (!column)
        return NULL;
    column->offset = size;
    *ptr = column->buffer.mutable;
    return column;
}

void zcs_column_free(struct zcs_column *column)
{
    if (!column->immutable)
        free(column->buffer.mutable);
    free(column);
}

static const void *zcs_column_head(const struct zcs_column *column)
{
    if (column->immutable)
        return column->buffer.immutable;
    else
        return column->buffer.mutable;
}

static const void *zcs_column_offset(const struct zcs_column *column,
                                     size_t offset)
{
    return (const void *)((uintptr_t)zcs_column_head(column) + offset);
}

static const void *zcs_column_tail(const struct zcs_column *column)
{
    return zcs_column_offset(column, column->offset);
}

const void *zcs_column_export(const struct zcs_column *column, size_t *size)
{
    *size = column->offset;
    return zcs_column_head(column);
}

enum zcs_column_type zcs_column_type(const struct zcs_column *column)
{
    return column->type;
}

enum zcs_encode_type zcs_column_encode(const struct zcs_column *column)
{
    return column->encode;
}

const struct zcs_column_index *zcs_column_index(const struct zcs_column *column)
{
    return &column->index;
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
    void *buffer = realloc(column->buffer.mutable, size);
    if (!buffer)
        return false;
    column->buffer.mutable = buffer;
    column->size = size;
    return true;
}

static void *zcs_column_alloc(struct zcs_column *column, size_t size)
{
    if (ZCS_UNLIKELY(column->immutable))
        return false;
    if (ZCS_UNLIKELY(column->offset + size > column->size))
        if (!zcs_column_resize(column, size))
            return NULL;
    void *ptr = (void *)zcs_column_tail(column);
    column->offset += size;
    return ptr;
}

bool zcs_column_put_bit(struct zcs_column *column, bool value)
{
    if (ZCS_UNLIKELY(column->type != ZCS_COLUMN_BIT))
        return false;
    if (ZCS_UNLIKELY(column->immutable))
        return false;
    uint64_t *bitset;
    if (ZCS_UNLIKELY(column->index.count % 64 == 0)) {
        bitset = zcs_column_alloc(column, sizeof(uint64_t));
        if (!bitset)
            return false;
        *bitset = 0;
    } else {
        assert(column->offset >= sizeof(uint64_t));
        bitset = (uint64_t *)zcs_column_offset(
            column, column->offset - sizeof(uint64_t));
    }
    if (value)
        *bitset |= (uint64_t)1 << column->index.count;
    if (ZCS_UNLIKELY(!column->index.count)) {
        column->index.min.bit = column->index.max.bit = value;
    } else {
        column->index.min.bit = value && column->index.min.bit;
        column->index.max.bit = value || column->index.max.bit;
    }
    column->index.count++;
    return true;
}

bool zcs_column_put_i32(struct zcs_column *column, int32_t value)
{
    if (ZCS_UNLIKELY(column->type != ZCS_COLUMN_I32))
        return false;
    int32_t *slot = zcs_column_alloc(column, sizeof(int32_t));
    if (ZCS_UNLIKELY(!slot))
        return false;
    *slot = value;
    if (ZCS_UNLIKELY(!column->index.count)) {
        column->index.min.i32 = column->index.max.i32 = value;
    } else {
        if (value > column->index.max.i32)
            column->index.max.i32 = value;
        if (value < column->index.min.i32)
            column->index.min.i32 = value;
    }
    column->index.count++;
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
    if (ZCS_UNLIKELY(!column->index.count)) {
        column->index.min.i64 = column->index.max.i64 = value;
    } else {
        if (value > column->index.max.i64)
            column->index.max.i64 = value;
        if (value < column->index.min.i64)
            column->index.min.i64 = value;
    }
    column->index.count++;
    return true;
}

bool zcs_column_put_str(struct zcs_column *column, const char *value)
{
    if (ZCS_UNLIKELY(column->type != ZCS_COLUMN_STR))
        return false;
    size_t length = strlen(value);
    void *slot = zcs_column_alloc(column, length + 1);
    if (ZCS_UNLIKELY(!slot))
        return false;
    memcpy(slot, value, length + 1);
    if (ZCS_UNLIKELY(!column->index.count)) {
        column->index.min.len = column->index.max.len = length;
    } else {
        if (length < column->index.min.len)
            column->index.min.len = length;
        if (length > column->index.max.len)
            column->index.max.len = length;
    }
    column->index.count++;
    return true;
}

struct zcs_column_cursor *zcs_column_cursor_new(const struct zcs_column *column)
{
    struct zcs_column_cursor *cursor = malloc(sizeof(*cursor));
    if (!cursor)
        return NULL;
    cursor->column = column;
    cursor->start = zcs_column_head(column);
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
    cursor->bitset_offset = 0;
    cursor->offset = 0;
}

static size_t zcs_column_cursor_remaining(
    const struct zcs_column_cursor *cursor, size_t size)
{
    return ((uintptr_t)cursor->end - (uintptr_t)cursor->position) / size;
}

bool zcs_column_cursor_valid(const struct zcs_column_cursor *cursor)
{
    if (cursor->column->type == ZCS_COLUMN_BIT) {
        size_t trailing_bits = cursor->column->index.count % 64;
        if (trailing_bits != 0 &&
            zcs_column_cursor_remaining(cursor, sizeof(uint64_t)) == 1)
            return cursor->bitset_offset < trailing_bits;
    }
    return cursor->position < cursor->end;
}

static void zcs_column_cursor_advance(struct zcs_column_cursor *cursor,
                                      size_t amount)
{
    cursor->position = (void *)((uintptr_t)cursor->position + amount);
    assert(cursor->position <= cursor->end);
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

static size_t zcs_column_cursor_skip_bit(struct zcs_column_cursor *cursor,
                                  size_t count)
{
    assert(cursor->column->type == ZCS_COLUMN_BIT);
    assert(cursor->bitset_offset % 64 == 0);
    assert(count % 64 == 0);
    size_t skipped =
        zcs_column_cursor_skip(cursor, sizeof(uint64_t), count / 64);
    skipped *= 64;
    if (!zcs_column_cursor_valid(cursor)) {
        size_t trailing_bits = cursor->column->index.count % 64;
        if (trailing_bits)
            skipped -= 64 - trailing_bits;
    }
    cursor->offset += skipped;
    return skipped;
}

static size_t zcs_column_cursor_skip_i32(struct zcs_column_cursor *cursor,
                                  size_t count)
{
    assert(cursor->column->type == ZCS_COLUMN_I32);
    size_t skipped = zcs_column_cursor_skip(cursor, sizeof(int32_t), count);
    cursor->offset += skipped;
    return skipped;
}

static size_t zcs_column_cursor_skip_i64(struct zcs_column_cursor *cursor,
                                  size_t count)
{
    assert(cursor->column->type == ZCS_COLUMN_I64);
    size_t skipped = zcs_column_cursor_skip(cursor, sizeof(int64_t), count);
    cursor->offset += skipped;
    return skipped;
}

static size_t zcs_column_cursor_skip_str(struct zcs_column_cursor *cursor,
                                  size_t count)
{
    assert(cursor->column->type == ZCS_COLUMN_STR);
    size_t skipped = 0;
    for (; skipped < count && zcs_column_cursor_valid(cursor); skipped++) {
        const char *str = cursor->position;
        zcs_column_cursor_advance(cursor, strlen(str) + 1);
    }
    cursor->offset += skipped;
    return skipped;
}

bool zcs_column_cursor_jump_bit(struct zcs_column_cursor *cursor,
                                size_t position)
{
    if (cursor->offset > position)
        return false;
    if (cursor->offset == position)
        return true;
    size_t remaining = position - cursor->offset;
    return zcs_column_cursor_skip_bit(cursor, remaining) == remaining;
}

bool zcs_column_cursor_jump_i32(struct zcs_column_cursor *cursor,
                                size_t position)
{
    if (cursor->offset > position)
        return false;
    if (cursor->offset == position)
        return true;
    size_t remaining = position - cursor->offset;
    return zcs_column_cursor_skip_i32(cursor, remaining) == remaining;
}

bool zcs_column_cursor_jump_i64(struct zcs_column_cursor *cursor,
                                size_t position)
{
    if (cursor->offset > position)
        return false;
    if (cursor->offset == position)
        return true;
    size_t remaining = position - cursor->offset;
    return zcs_column_cursor_skip_i64(cursor, remaining) == remaining;
}

bool zcs_column_cursor_jump_str(struct zcs_column_cursor *cursor,
                                size_t position)
{
    if (cursor->offset > position)
        return false;
    if (cursor->offset == position)
        return true;
    size_t remaining = position - cursor->offset;
    return zcs_column_cursor_skip_str(cursor, remaining) == remaining;
}

static const void *zcs_column_cursor_next_batch(
    struct zcs_column_cursor *cursor, size_t size, size_t count,
    size_t *available)
{
    const void *values = cursor->position;
    *available = zcs_column_cursor_skip(cursor, size, count);
    return values;
}

const uint64_t *zcs_column_cursor_next_batch_bit(
    struct zcs_column_cursor *cursor, size_t *available)
{
    assert(cursor->column->type == ZCS_COLUMN_BIT);
    const uint64_t *values = cursor->position;
    *available = zcs_column_cursor_skip_bit(cursor, ZCS_BATCH_SIZE);
    return values;
}

const int32_t *zcs_column_cursor_next_batch_i32(
    struct zcs_column_cursor *cursor, size_t *available)
{
    assert(cursor->column->type == ZCS_COLUMN_I32);
    return zcs_column_cursor_next_batch(cursor, sizeof(int32_t), ZCS_BATCH_SIZE,
                                        available);
}

const int64_t *zcs_column_cursor_next_batch_i64(
    struct zcs_column_cursor *cursor, size_t *available)
{
    assert(cursor->column->type == ZCS_COLUMN_I64);
    return zcs_column_cursor_next_batch(cursor, sizeof(int64_t), ZCS_BATCH_SIZE,
                                        available);
}

const struct zcs_string *zcs_column_cursor_next_batch_str(
    struct zcs_column_cursor *cursor, size_t *available)
{
    assert(cursor->column->type == ZCS_COLUMN_STR);
    size_t i = 0;
    struct zcs_string *strings = (struct zcs_string *)cursor->buffer;
    for (; i < ZCS_BATCH_SIZE && zcs_column_cursor_valid(cursor); i++) {
        strings[i].ptr = cursor->position;
        strings[i].len = strlen(cursor->position);
        zcs_column_cursor_advance(cursor, strings[i].len + 1);
    }
    cursor->offset += i;
    *available = i;
    return (const struct zcs_string *)cursor->buffer;
}
