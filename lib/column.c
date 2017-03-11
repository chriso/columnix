#define _BSD_SOURCE
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "column.h"

static const size_t cx_column_initial_size = 64;

struct cx_column {
    union {
        void *mutable;
        const void *mmapped;
    } buffer;
    size_t offset;
    size_t size;
    enum cx_column_type type;
    enum cx_encoding_type encoding;
    struct cx_column_index index;
    bool mmapped;
};

struct cx_column_cursor {
    const struct cx_column *column;
    const void *start;
    const void *end;
    const void *position;
    cx_value_t buffer[CX_BATCH_SIZE];
};

static struct cx_column *cx_column_new_size(enum cx_column_type type,
                                            enum cx_encoding_type encoding,
                                            size_t size,
                                            const struct cx_column_index *index)
{
    struct cx_column *column = calloc(1, sizeof(*column));
    if (!column)
        return NULL;
    if (size) {
#if CX_PCMPISTRM
        size += 16;
        column->buffer.mutable = calloc(1, size);
#else
        column->buffer.mutable = malloc(size);
#endif
        if (!column->buffer.mutable)
            goto error;
        column->size = size;
    }
    column->type = type;
    column->encoding = encoding;
    if (index)
        memcpy(&column->index, index, sizeof(*index));
    return column;
error:
    free(column);
    return NULL;
}

struct cx_column *cx_column_new(enum cx_column_type type,
                                enum cx_encoding_type encoding)
{
    return cx_column_new_size(type, encoding, cx_column_initial_size, NULL);
}

struct cx_column *cx_column_new_mmapped(enum cx_column_type type,
                                        enum cx_encoding_type encoding,
                                        const void *ptr, size_t size,
                                        const struct cx_column_index *index)
{
    struct cx_column *column = cx_column_new_size(type, encoding, 0, index);
    if (!column)
        return NULL;
    column->offset = size;
    column->size = size;
    column->mmapped = true;
    column->buffer.mmapped = ptr;
    return column;
}

struct cx_column *cx_column_new_compressed(enum cx_column_type type,
                                           enum cx_encoding_type encoding,
                                           void **ptr, size_t size,
                                           const struct cx_column_index *index)
{
    if (!size)
        return NULL;
    struct cx_column *column = cx_column_new_size(type, encoding, size, index);
    if (!column)
        return NULL;
    column->offset = size;
    *ptr = column->buffer.mutable;
    return column;
}

void cx_column_free(struct cx_column *column)
{
    if (!column->mmapped)
        free(column->buffer.mutable);
    free(column);
}

static const void *cx_column_head(const struct cx_column *column)
{
    if (column->mmapped)
        return column->buffer.mmapped;
    else
        return column->buffer.mutable;
}

static const void *cx_column_offset(const struct cx_column *column,
                                    size_t offset)
{
    const void *ptr = cx_column_head(column);
    return (const void *)((uintptr_t)ptr + offset);
}

static const void *cx_column_tail(const struct cx_column *column)
{
    return cx_column_offset(column, column->offset);
}

const void *cx_column_export(const struct cx_column *column, size_t *size)
{
    *size = column->offset;
    return cx_column_head(column);
}

enum cx_column_type cx_column_type(const struct cx_column *column)
{
    return column->type;
}

enum cx_encoding_type cx_column_encoding(const struct cx_column *column)
{
    return column->encoding;
}

const struct cx_column_index *cx_column_index(const struct cx_column *column)
{
    return &column->index;
}

size_t cx_column_count(const struct cx_column *column)
{
    return column->index.count;
}

__attribute__((noinline)) static bool cx_column_resize(struct cx_column *column,
                                                       size_t alloc_size)
{
    size_t size = column->size;
    size_t required_size = column->offset + alloc_size;
#if CX_PCMPISTRM
    required_size += 16;
#endif
    while (size < required_size) {
        assert(size * 2 > size);
        size *= 2;
    }
    void *buffer = realloc(column->buffer.mutable, size);
    if (!buffer)
        return false;
#if CX_PCMPISTRM
    memset((void *)((uintptr_t)buffer + column->offset), 0,
           size - column->offset);
#endif
    column->buffer.mutable = buffer;
    column->size = size;
    return true;
}

static void *cx_column_alloc(struct cx_column *column, size_t size)
{
    if (column->mmapped)
        return false;
    if (column->offset + size > column->size)
        if (!cx_column_resize(column, size))
            return NULL;
    void *ptr = (void *)cx_column_tail(column);
    column->offset += size;
    return ptr;
}

bool cx_column_put_bit(struct cx_column *column, bool value)
{
    if (column->type != CX_COLUMN_BIT)
        return false;
    if (column->mmapped)
        return false;
    uint64_t *bitset;
    if (column->index.count % 64 == 0) {
        bitset = cx_column_alloc(column, sizeof(uint64_t));
        if (!bitset)
            return false;
        *bitset = 0;
    } else {
        assert(column->offset >= sizeof(uint64_t));
        bitset = (uint64_t *)cx_column_offset(
            column, column->offset - sizeof(uint64_t));
    }
    if (value)
        *bitset |= (uint64_t)1 << column->index.count;
    if (!column->index.count) {
        column->index.min.bit = column->index.max.bit = value;
    } else {
        column->index.min.bit = value && column->index.min.bit;
        column->index.max.bit = value || column->index.max.bit;
    }
    column->index.count++;
    return true;
}

bool cx_column_put_i32(struct cx_column *column, int32_t value)
{
    if (column->type != CX_COLUMN_I32)
        return false;
    int32_t *slot = cx_column_alloc(column, sizeof(int32_t));
    if (!slot)
        return false;
    *slot = value;
    if (!column->index.count) {
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

bool cx_column_put_i64(struct cx_column *column, int64_t value)
{
    if (column->type != CX_COLUMN_I64)
        return false;
    int64_t *slot = cx_column_alloc(column, sizeof(int64_t));
    if (!slot)
        return false;
    *slot = value;
    if (!column->index.count) {
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

bool cx_column_put_str(struct cx_column *column, const char *value)
{
    if (column->type != CX_COLUMN_STR)
        return false;
    size_t length = strlen(value);
    void *slot = cx_column_alloc(column, length + 1);
    if (!slot)
        return false;
    memcpy(slot, value, length + 1);
    if (!column->index.count) {
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

bool cx_column_put_unit(struct cx_column *column)
{
    switch (column->type) {
        case CX_COLUMN_I32:
            return cx_column_put_i32(column, 0);
        case CX_COLUMN_I64:
            return cx_column_put_i64(column, 0);
        case CX_COLUMN_BIT:
            return cx_column_put_bit(column, false);
        case CX_COLUMN_STR:
            return cx_column_put_str(column, "");
    }
    return false;
}

static bool cx_column_madvise(const struct cx_column *column, int advice)
{
    if (!column->mmapped || !column->size)
        return true;
    size_t page_size = getpagesize();
    uintptr_t addr = (uintptr_t)column->buffer.mmapped;
    size_t offset = addr % page_size;
    return !madvise((void *)(addr - offset), (column->size + offset), advice);
}

struct cx_column_cursor *cx_column_cursor_new(const struct cx_column *column)
{
    struct cx_column_cursor *cursor = malloc(sizeof(*cursor));
    if (!cursor)
        return NULL;
    cursor->column = column;
    cursor->start = cx_column_head(column);
    cursor->end = cx_column_tail(column);
    if (!cx_column_madvise(column, MADV_SEQUENTIAL))
        goto error;
    cx_column_cursor_rewind(cursor);
    return cursor;
error:
    free(cursor);
    return NULL;
}

void cx_column_cursor_free(struct cx_column_cursor *cursor)
{
    free(cursor);
}

void cx_column_cursor_rewind(struct cx_column_cursor *cursor)
{
    cursor->position = cursor->start;
}

bool cx_column_cursor_valid(const struct cx_column_cursor *cursor)
{
    return cursor->position < cursor->end;
}

static void cx_column_cursor_advance(struct cx_column_cursor *cursor,
                                     size_t amount)
{
    cursor->position = (void *)((uintptr_t)cursor->position + amount);
    assert(cursor->position <= cursor->end);
}

static size_t cx_column_cursor_skip(struct cx_column_cursor *cursor,
                                    size_t size, size_t count)
{
    size_t remaining =
        ((uintptr_t)cursor->end - (uintptr_t)cursor->position) / size;
    if (remaining < count)
        count = remaining;
    cx_column_cursor_advance(cursor, size * count);
    return count;
}

size_t cx_column_cursor_skip_bit(struct cx_column_cursor *cursor, size_t count)
{
    assert(cursor->column->type == CX_COLUMN_BIT);
    assert(count % 64 == 0);
    size_t skipped =
        cx_column_cursor_skip(cursor, sizeof(uint64_t), count / 64);
    skipped *= 64;
    if (!cx_column_cursor_valid(cursor)) {
        size_t trailing_bits = cursor->column->index.count % 64;
        if (trailing_bits)
            skipped -= 64 - trailing_bits;
    }
    return skipped;
}

size_t cx_column_cursor_skip_i32(struct cx_column_cursor *cursor, size_t count)
{
    assert(cursor->column->type == CX_COLUMN_I32);
    return cx_column_cursor_skip(cursor, sizeof(int32_t), count);
}

size_t cx_column_cursor_skip_i64(struct cx_column_cursor *cursor, size_t count)
{
    assert(cursor->column->type == CX_COLUMN_I64);
    return cx_column_cursor_skip(cursor, sizeof(int64_t), count);
}

size_t cx_column_cursor_skip_str(struct cx_column_cursor *cursor, size_t count)
{
    assert(cursor->column->type == CX_COLUMN_STR);
    size_t skipped = 0;
    for (; skipped < count && cx_column_cursor_valid(cursor); skipped++) {
        const char *str = cursor->position;
        cx_column_cursor_advance(cursor, strlen(str) + 1);
    }
    return skipped;
}

static const void *cx_column_cursor_next_batch(struct cx_column_cursor *cursor,
                                               size_t size, size_t count,
                                               size_t *available)
{
    const void *values = cursor->position;
    *available = cx_column_cursor_skip(cursor, size, count);
    return values;
}

const uint64_t *cx_column_cursor_next_batch_bit(struct cx_column_cursor *cursor,
                                                size_t *available)
{
    assert(cursor->column->type == CX_COLUMN_BIT);
    const uint64_t *values = cursor->position;
    *available = cx_column_cursor_skip_bit(cursor, CX_BATCH_SIZE);
    return values;
}

const int32_t *cx_column_cursor_next_batch_i32(struct cx_column_cursor *cursor,
                                               size_t *available)
{
    assert(cursor->column->type == CX_COLUMN_I32);
    return cx_column_cursor_next_batch(cursor, sizeof(int32_t), CX_BATCH_SIZE,
                                       available);
}

const int64_t *cx_column_cursor_next_batch_i64(struct cx_column_cursor *cursor,
                                               size_t *available)
{
    assert(cursor->column->type == CX_COLUMN_I64);
    return cx_column_cursor_next_batch(cursor, sizeof(int64_t), CX_BATCH_SIZE,
                                       available);
}

const struct cx_string *cx_column_cursor_next_batch_str(
    struct cx_column_cursor *cursor, size_t *available)
{
    assert(cursor->column->type == CX_COLUMN_STR);
    size_t i = 0;
    struct cx_string *strings = (struct cx_string *)cursor->buffer;
    for (; i < CX_BATCH_SIZE && cx_column_cursor_valid(cursor); i++) {
        strings[i].ptr = cursor->position;
        strings[i].len = strlen(cursor->position);
        cx_column_cursor_advance(cursor, strings[i].len + 1);
    }
    *available = i;
    return (const struct cx_string *)cursor->buffer;
}

const struct cx_string *cx_column_cursor_get_str(
    const struct cx_column_cursor *cursor)
{
    assert(cursor->column->type == CX_COLUMN_STR);
    if (!cx_column_cursor_valid(cursor))
        return NULL;
    struct cx_string *string = (struct cx_string *)cursor->buffer;
    string->ptr = cursor->position;
    string->len = strlen(cursor->position);
    return string;
}
