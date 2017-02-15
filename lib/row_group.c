#include <assert.h>
#include <stdlib.h>

#include "row_group.h"

struct zcs_row_group_column {
    enum zcs_column_type type;
    enum zcs_encoding_type encoding;
    const struct zcs_column_index *index;
    union {
        struct zcs_column *ptr;
        struct {
            const void *ptr;
            size_t size;
        } lazy;
    } column;
    bool lazy;
    bool initialized;
};

struct zcs_row_group {
    struct zcs_row_group_column *columns;
    size_t count;
    size_t size;
};

struct zcs_row_group_cursor_column {
    struct zcs_column_cursor *cursor;
    size_t position;
    const void *values;
    size_t count;
};

struct zcs_row_group_cursor {
    struct zcs_row_group *row_group;
    struct zcs_row_group_cursor_column *columns;
    size_t column_count;
    size_t row_count;
    size_t position;
    bool initialized;
};

struct zcs_row_group *zcs_row_group_new()
{
    struct zcs_row_group *row_group = malloc(sizeof(*row_group));
    if (!row_group)
        return NULL;
    row_group->columns = malloc(sizeof(*row_group->columns));
    if (!row_group->columns)
        goto error;
    row_group->count = 0;
    row_group->size = 1;
    return row_group;
error:
    free(row_group);
    return NULL;
}

void zcs_row_group_free(struct zcs_row_group *row_group)
{
    for (size_t i = 0; i < row_group->count; i++) {
        struct zcs_row_group_column *row_group_column = &row_group->columns[i];
        if (row_group_column->lazy && row_group_column->initialized)
            zcs_column_free(row_group_column->column.ptr);
    }
    free(row_group->columns);
    free(row_group);
}

static bool zcs_row_group_ensure_column_size(struct zcs_row_group *row_group)
{
    if (row_group->count == row_group->size) {
        size_t new_size = row_group->size * 2;
        assert(new_size && new_size > row_group->size);
        struct zcs_row_group_column *columns =
            realloc(row_group->columns, new_size * sizeof(*columns));
        if (!columns)
            return false;
        row_group->columns = columns;
        row_group->size = new_size;
    }
    return true;
}

static bool zcs_row_group_valid_column(struct zcs_row_group *row_group,
                                       const struct zcs_column_index *index)
{
    if (row_group->count) {
        const struct zcs_column_index *last_index =
            zcs_row_group_column_index(row_group, row_group->count - 1);
        if (index->count != last_index->count)
            return false;
    }
    return true;
}

bool zcs_row_group_add_column(struct zcs_row_group *row_group,
                              struct zcs_column *column)
{
    const struct zcs_column_index *index = zcs_column_index(column);
    if (!zcs_row_group_valid_column(row_group, index))
        return false;
    if (!zcs_row_group_ensure_column_size(row_group))
        return false;
    struct zcs_row_group_column *row_group_column =
        &row_group->columns[row_group->count++];
    row_group_column->column.ptr = column;
    row_group_column->type = zcs_column_type(column);
    row_group_column->encoding = zcs_column_encoding(column);
    row_group_column->index = index;
    row_group_column->lazy = false;
    return true;
}

bool zcs_row_group_add_column_lazy(struct zcs_row_group *row_group,
                                   enum zcs_column_type type,
                                   enum zcs_encoding_type encoding,
                                   const struct zcs_column_index *index,
                                   const void *ptr, size_t size)
{
    if (!zcs_row_group_valid_column(row_group, index))
        return false;
    if (!zcs_row_group_ensure_column_size(row_group))
        return false;
    struct zcs_row_group_column *row_group_column =
        &row_group->columns[row_group->count++];
    row_group_column->type = type;
    row_group_column->encoding = encoding;
    row_group_column->index = index;
    row_group_column->column.lazy.ptr = ptr;
    row_group_column->column.lazy.size = size;
    row_group_column->lazy = true;
    row_group_column->initialized = false;
    return true;
}

size_t zcs_row_group_column_count(const struct zcs_row_group *row_group)
{
    return row_group->count;
}

size_t zcs_row_group_row_count(const struct zcs_row_group *row_group)
{
    if (!row_group->count)
        return 0;
    const struct zcs_column_index *index =
        zcs_row_group_column_index(row_group, 0);
    return index->count;
}

enum zcs_column_type zcs_row_group_column_type(
    const struct zcs_row_group *row_group, size_t index)
{
    assert(index <= row_group->count);
    return row_group->columns[index].type;
}

enum zcs_encoding_type zcs_row_group_column_encoding(
    const struct zcs_row_group *row_group, size_t index)
{
    assert(index <= row_group->count);
    return row_group->columns[index].encoding;
}

const struct zcs_column_index *zcs_row_group_column_index(
    const struct zcs_row_group *row_group, size_t index)
{
    assert(index <= row_group->count);
    return row_group->columns[index].index;
}

const struct zcs_column *zcs_row_group_column(
    const struct zcs_row_group *row_group, size_t index)
{
    assert(index <= row_group->count);
    struct zcs_row_group_column *row_group_column = &row_group->columns[index];
    if (row_group_column->lazy && !row_group_column->initialized) {
        struct zcs_column *column = zcs_column_new_immutable(
            row_group_column->type, row_group_column->encoding,
            row_group_column->column.lazy.ptr,
            row_group_column->column.lazy.size, row_group_column->index);
        if (!column)
            return NULL;
        row_group_column->column.ptr = column;
        row_group_column->initialized = true;
    }
    return row_group_column->column.ptr;
}

struct zcs_row_group_cursor *zcs_row_group_cursor_new(
    struct zcs_row_group *row_group)
{
    struct zcs_row_group_cursor *cursor = calloc(1, sizeof(*cursor));
    if (!cursor)
        return NULL;
    cursor->row_group = row_group;
    cursor->column_count = zcs_row_group_column_count(row_group);
    cursor->row_count = zcs_row_group_row_count(row_group);
    if (cursor->column_count) {
        cursor->columns =
            calloc(cursor->column_count, sizeof(*cursor->columns));
        if (!cursor->columns)
            goto error;
    }
    return cursor;
error:
    free(cursor);
    return NULL;
}

void zcs_row_group_cursor_free(struct zcs_row_group_cursor *cursor)
{
    zcs_row_group_cursor_rewind(cursor);
    if (cursor->column_count)
        free(cursor->columns);
    free(cursor);
}

void zcs_row_group_cursor_rewind(struct zcs_row_group_cursor *cursor)
{
    cursor->initialized = false;
    cursor->position = 0;
    for (size_t i = 0; i < cursor->column_count; i++) {
        if (cursor->columns[i].cursor)
            zcs_column_cursor_free(cursor->columns[i].cursor);
        cursor->columns[i].cursor = NULL;
        cursor->columns[i].position = 0;
    }
}

bool zcs_row_group_cursor_next(struct zcs_row_group_cursor *cursor)
{
    if (!cursor->initialized)
        cursor->initialized = true;
    else
        cursor->position += ZCS_BATCH_SIZE;
    return cursor->position < cursor->row_count;
}

size_t zcs_row_group_cursor_batch_count(
    const struct zcs_row_group_cursor *cursor)
{
    if (cursor->position >= cursor->row_count)
        return 0;
    size_t remaining = cursor->row_count - cursor->position;
    return remaining < ZCS_BATCH_SIZE ? remaining : ZCS_BATCH_SIZE;
}

static bool zcs_row_group_cursor_lazy_init(struct zcs_row_group_cursor *cursor,
                                           size_t column_index)
{
    if (column_index >= cursor->column_count)
        return false;
    if (cursor->columns[column_index].cursor)
        return true;
    const struct zcs_column *column =
        zcs_row_group_column(cursor->row_group, column_index);
    if (!column)
        return false;
    cursor->columns[column_index].cursor = zcs_column_cursor_new(column);
    return cursor->columns[column_index].cursor != NULL;
}

const uint64_t *zcs_row_group_cursor_next_bit(
    struct zcs_row_group_cursor *cursor, size_t column_index, size_t *count)
{
    if (!zcs_row_group_cursor_lazy_init(cursor, column_index))
        return NULL;
    struct zcs_row_group_cursor_column *column = &cursor->columns[column_index];
    if (column->position <= cursor->position) {
        size_t skipped = zcs_column_cursor_skip_bit(
            column->cursor, cursor->position - column->position);
        column->position += skipped;
        column->values =
            zcs_column_cursor_next_batch_bit(column->cursor, &column->count);
        column->position += column->count;
    }
    *count = column->count;
    return column->values;
}

const int32_t *zcs_row_group_cursor_next_i32(
    struct zcs_row_group_cursor *cursor, size_t column_index, size_t *count)
{
    if (!zcs_row_group_cursor_lazy_init(cursor, column_index))
        return NULL;
    struct zcs_row_group_cursor_column *column = &cursor->columns[column_index];
    if (column->position <= cursor->position) {
        size_t skipped = zcs_column_cursor_skip_i32(
            column->cursor, cursor->position - column->position);
        column->position += skipped;
        column->values =
            zcs_column_cursor_next_batch_i32(column->cursor, &column->count);
        column->position += column->count;
    }
    *count = column->count;
    return column->values;
}

const int64_t *zcs_row_group_cursor_next_i64(
    struct zcs_row_group_cursor *cursor, size_t column_index, size_t *count)
{
    if (!zcs_row_group_cursor_lazy_init(cursor, column_index))
        return NULL;
    struct zcs_row_group_cursor_column *column = &cursor->columns[column_index];
    if (column->position <= cursor->position) {
        size_t skipped = zcs_column_cursor_skip_i64(
            column->cursor, cursor->position - column->position);
        column->position += skipped;
        column->values =
            zcs_column_cursor_next_batch_i64(column->cursor, &column->count);
        column->position += column->count;
    }
    *count = column->count;
    return column->values;
}

const struct zcs_string *zcs_row_group_cursor_next_str(
    struct zcs_row_group_cursor *cursor, size_t column_index, size_t *count)
{
    if (!zcs_row_group_cursor_lazy_init(cursor, column_index))
        return NULL;
    struct zcs_row_group_cursor_column *column = &cursor->columns[column_index];
    if (column->position <= cursor->position) {
        size_t skipped = zcs_column_cursor_skip_str(
            column->cursor, cursor->position - column->position);
        column->position += skipped;
        column->values =
            zcs_column_cursor_next_batch_str(column->cursor, &column->count);
        column->position += column->count;
    }
    *count = column->count;
    return column->values;
}
