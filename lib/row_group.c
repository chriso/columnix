#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "compress.h"
#include "row_group.h"

static const size_t zcs_row_group_column_initial_size = 8;

struct zcs_row_group_physical_column {
    const struct zcs_column_index *index;
    struct zcs_column *column;
    struct zcs_lazy_column lazy_column;
};

struct zcs_row_group_column {
    enum zcs_column_type type;
    enum zcs_encoding_type encoding;
    struct zcs_row_group_physical_column values;
    struct zcs_row_group_physical_column nulls;
    bool lazy;
};

struct zcs_row_group {
    struct zcs_row_group_column *columns;
    size_t count;
    size_t size;
};

struct zcs_row_group_cursor_physical_column {
    struct zcs_column_cursor *cursor;
    size_t position;
    const void *batch;
    size_t count;
};

struct zcs_row_group_cursor_column {
    struct zcs_row_group_cursor_physical_column values;
    struct zcs_row_group_cursor_physical_column nulls;
};

struct zcs_row_group_cursor {
    struct zcs_row_group *row_group;
    size_t column_count;
    size_t row_count;
    size_t position;
    bool initialized;
    struct zcs_row_group_cursor_column columns[];
};

struct zcs_row_group *zcs_row_group_new()
{
    struct zcs_row_group *row_group = malloc(sizeof(*row_group));
    if (!row_group)
        return NULL;
    row_group->columns =
        malloc(zcs_row_group_column_initial_size * sizeof(*row_group->columns));
    if (!row_group->columns)
        goto error;
    row_group->count = 0;
    row_group->size = zcs_row_group_column_initial_size;
    return row_group;
error:
    free(row_group);
    return NULL;
}

void zcs_row_group_free(struct zcs_row_group *row_group)
{
    for (size_t i = 0; i < row_group->count; i++) {
        struct zcs_row_group_column *row_group_column = &row_group->columns[i];
        if (row_group_column->lazy) {
            if (row_group_column->values.column)
                zcs_column_free(row_group_column->values.column);
            if (row_group_column->nulls.column)
                zcs_column_free(row_group_column->nulls.column);
        }
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
                              struct zcs_column *column,
                              struct zcs_column *nulls)
{
    if (!column)
        return false;
    const struct zcs_column_index *index = zcs_column_index(column);
    if (!zcs_row_group_valid_column(row_group, index))
        return false;
    if (zcs_column_type(nulls) != ZCS_COLUMN_BIT)
        return false;
    const struct zcs_column_index *null_index = zcs_column_index(nulls);
    if (!zcs_row_group_valid_column(row_group, null_index))
        return false;
    if (!zcs_row_group_ensure_column_size(row_group))
        return false;
    struct zcs_row_group_column *row_group_column =
        &row_group->columns[row_group->count++];
    row_group_column->type = zcs_column_type(column);
    row_group_column->encoding = zcs_column_encoding(column);
    row_group_column->values.column = column;
    row_group_column->values.index = index;
    row_group_column->lazy = false;
    row_group_column->nulls.column = nulls;
    row_group_column->nulls.index = zcs_column_index(nulls);
    return true;
}

bool zcs_row_group_add_lazy_column(struct zcs_row_group *row_group,
                                   const struct zcs_lazy_column *column,
                                   const struct zcs_lazy_column *nulls)
{
    if (!zcs_row_group_valid_column(row_group, column->index))
        return false;
    if (nulls->type != ZCS_COLUMN_BIT)
        return false;
    if (!zcs_row_group_valid_column(row_group, nulls->index))
        return false;
    if (!zcs_row_group_ensure_column_size(row_group))
        return false;
    struct zcs_row_group_column *row_group_column =
        &row_group->columns[row_group->count++];
    row_group_column->type = column->type;
    row_group_column->encoding = column->encoding;
    row_group_column->values.index = column->index;
    row_group_column->values.column = NULL;
    memcpy(&row_group_column->values.lazy_column, column, sizeof(*column));
    row_group_column->lazy = true;
    row_group_column->nulls.column = NULL;
    row_group_column->nulls.index = nulls->index;
    memcpy(&row_group_column->nulls.lazy_column, nulls, sizeof(*nulls));
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
    assert(index < row_group->count);
    return row_group->columns[index].type;
}

enum zcs_encoding_type zcs_row_group_column_encoding(
    const struct zcs_row_group *row_group, size_t index)
{
    assert(index < row_group->count);
    return row_group->columns[index].encoding;
}

const struct zcs_column_index *zcs_row_group_column_index(
    const struct zcs_row_group *row_group, size_t index)
{
    assert(index < row_group->count);
    return row_group->columns[index].values.index;
}

const struct zcs_column_index *zcs_row_group_null_index(
    const struct zcs_row_group *row_group, size_t index)
{
    assert(index < row_group->count);
    return row_group->columns[index].nulls.index;
}

static bool zcs_row_group_lazy_column_init(
    struct zcs_row_group_physical_column *row_group_column)
{
    struct zcs_lazy_column *lazy = &row_group_column->lazy_column;
    struct zcs_column *column = NULL;
    if (lazy->compression && lazy->size) {
        void *dest;
        column = zcs_column_new_compressed(lazy->type, lazy->encoding, &dest,
                                           lazy->decompressed_size,
                                           row_group_column->index);
        if (!column)
            goto error;
        if (!zcs_decompress(lazy->compression, lazy->ptr, lazy->size, dest,
                            lazy->decompressed_size))
            goto error;
    } else {
        column = zcs_column_new_mmapped(lazy->type, lazy->encoding, lazy->ptr,
                                        lazy->size, row_group_column->index);
        if (!column)
            goto error;
    }
    row_group_column->column = column;
    return true;
error:
    if (column)
        zcs_column_free(column);
    return false;
}

const struct zcs_column *zcs_row_group_column(
    const struct zcs_row_group *row_group, size_t index)
{
    assert(index < row_group->count);
    struct zcs_row_group_column *row_group_column = &row_group->columns[index];
    if (row_group_column->lazy && !row_group_column->values.column)
        if (!zcs_row_group_lazy_column_init(&row_group_column->values))
            return NULL;
    return row_group_column->values.column;
}

const struct zcs_column *zcs_row_group_nulls(
    const struct zcs_row_group *row_group, size_t index)
{
    assert(index < row_group->count);
    struct zcs_row_group_column *row_group_column = &row_group->columns[index];
    if (row_group_column->lazy && !row_group_column->nulls.column)
        if (!zcs_row_group_lazy_column_init(&row_group_column->nulls))
            return NULL;
    return row_group_column->nulls.column;
}

struct zcs_row_group_cursor *zcs_row_group_cursor_new(
    struct zcs_row_group *row_group)
{
    size_t column_count = zcs_row_group_column_count(row_group);
    size_t size = sizeof(struct zcs_row_group_cursor) +
                  column_count * sizeof(struct zcs_row_group_cursor_column);
    struct zcs_row_group_cursor *cursor = calloc(1, size);
    if (!cursor)
        return NULL;
    cursor->row_group = row_group;
    cursor->column_count = column_count;
    cursor->row_count = zcs_row_group_row_count(row_group);
    return cursor;
}

void zcs_row_group_cursor_free(struct zcs_row_group_cursor *cursor)
{
    zcs_row_group_cursor_rewind(cursor);
    free(cursor);
}

static void zcs_row_group_cursor_rewind_columns(
    struct zcs_row_group_cursor_physical_column *column)
{
    if (column->cursor)
        zcs_column_cursor_free(column->cursor);
    column->cursor = NULL;
    column->position = 0;
}

void zcs_row_group_cursor_rewind(struct zcs_row_group_cursor *cursor)
{
    cursor->initialized = false;
    cursor->position = 0;
    for (size_t i = 0; i < cursor->column_count; i++) {
        zcs_row_group_cursor_rewind_columns(&cursor->columns[i].values);
        zcs_row_group_cursor_rewind_columns(&cursor->columns[i].nulls);
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

static bool zcs_row_group_cursor_lazy_column_init(
    struct zcs_row_group_cursor *cursor, size_t column_index)
{
    if (column_index >= cursor->column_count)
        return false;
    if (cursor->columns[column_index].values.cursor)
        return true;
    const struct zcs_column *column =
        zcs_row_group_column(cursor->row_group, column_index);
    if (!column)
        return false;
    cursor->columns[column_index].values.cursor = zcs_column_cursor_new(column);
    return cursor->columns[column_index].values.cursor != NULL;
}

static bool zcs_row_group_cursor_lazy_nulls_init(
    struct zcs_row_group_cursor *cursor, size_t column_index)
{
    if (column_index >= cursor->column_count)
        return false;
    if (cursor->columns[column_index].nulls.cursor)
        return true;
    const struct zcs_column *column =
        zcs_row_group_nulls(cursor->row_group, column_index);
    if (!column)
        return false;
    cursor->columns[column_index].nulls.cursor = zcs_column_cursor_new(column);
    return cursor->columns[column_index].nulls.cursor != NULL;
}

const uint64_t *zcs_row_group_cursor_batch_nulls(
    struct zcs_row_group_cursor *cursor, size_t column_index, size_t *count)
{
    if (!zcs_row_group_cursor_lazy_nulls_init(cursor, column_index))
        return NULL;
    struct zcs_row_group_cursor_column *column = &cursor->columns[column_index];
    if (column->nulls.position <= cursor->position) {
        size_t skipped = zcs_column_cursor_skip_bit(
            column->nulls.cursor, cursor->position - column->nulls.position);
        column->nulls.position += skipped;
        column->nulls.batch = zcs_column_cursor_next_batch_bit(
            column->nulls.cursor, &column->nulls.count);
        column->nulls.position += column->nulls.count;
    }
    *count = column->nulls.count;
    return column->nulls.batch;
}

const uint64_t *zcs_row_group_cursor_batch_bit(
    struct zcs_row_group_cursor *cursor, size_t column_index, size_t *count)
{
    if (!zcs_row_group_cursor_lazy_column_init(cursor, column_index))
        return NULL;
    struct zcs_row_group_cursor_column *column = &cursor->columns[column_index];
    if (column->values.position <= cursor->position) {
        size_t skipped = zcs_column_cursor_skip_bit(
            column->values.cursor, cursor->position - column->values.position);
        column->values.position += skipped;
        column->values.batch = zcs_column_cursor_next_batch_bit(
            column->values.cursor, &column->values.count);
        column->values.position += column->values.count;
    }
    *count = column->values.count;
    return column->values.batch;
}

const int32_t *zcs_row_group_cursor_batch_i32(
    struct zcs_row_group_cursor *cursor, size_t column_index, size_t *count)
{
    if (!zcs_row_group_cursor_lazy_column_init(cursor, column_index))
        return NULL;
    struct zcs_row_group_cursor_column *column = &cursor->columns[column_index];
    if (column->values.position <= cursor->position) {
        size_t skipped = zcs_column_cursor_skip_i32(
            column->values.cursor, cursor->position - column->values.position);
        column->values.position += skipped;
        column->values.batch = zcs_column_cursor_next_batch_i32(
            column->values.cursor, &column->values.count);
        column->values.position += column->values.count;
    }
    *count = column->values.count;
    return column->values.batch;
}

const int64_t *zcs_row_group_cursor_batch_i64(
    struct zcs_row_group_cursor *cursor, size_t column_index, size_t *count)
{
    if (!zcs_row_group_cursor_lazy_column_init(cursor, column_index))
        return NULL;
    struct zcs_row_group_cursor_column *column = &cursor->columns[column_index];
    if (column->values.position <= cursor->position) {
        size_t skipped = zcs_column_cursor_skip_i64(
            column->values.cursor, cursor->position - column->values.position);
        column->values.position += skipped;
        column->values.batch = zcs_column_cursor_next_batch_i64(
            column->values.cursor, &column->values.count);
        column->values.position += column->values.count;
    }
    *count = column->values.count;
    return column->values.batch;
}

const struct zcs_string *zcs_row_group_cursor_batch_str(
    struct zcs_row_group_cursor *cursor, size_t column_index, size_t *count)
{
    if (!zcs_row_group_cursor_lazy_column_init(cursor, column_index))
        return NULL;
    struct zcs_row_group_cursor_column *column = &cursor->columns[column_index];
    if (column->values.position <= cursor->position) {
        size_t skipped = zcs_column_cursor_skip_str(
            column->values.cursor, cursor->position - column->values.position);
        column->values.position += skipped;
        column->values.batch = zcs_column_cursor_next_batch_str(
            column->values.cursor, &column->values.count);
        column->values.position += column->values.count;
    }
    *count = column->values.count;
    return column->values.batch;
}
