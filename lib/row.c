#include <assert.h>
#include <stdlib.h>

#include "row.h"

struct cx_row_cursor {
    struct cx_row_group *row_group;
    struct cx_row_group_cursor *cursor;
    const struct cx_predicate *predicate;
    size_t column_count;
    uint64_t row_mask;
    size_t position;
    enum cx_index_match index_match;
    bool implicit_predicate;
    bool error;
};

struct cx_row_cursor *cx_row_cursor_new(struct cx_row_group *row_group,
                                        const struct cx_predicate *predicate)
{
    struct cx_row_cursor *cursor = calloc(1, sizeof(*cursor));
    if (!cursor)
        return NULL;
    cursor->row_group = row_group;
    cursor->cursor = cx_row_group_cursor_new(row_group);
    if (!cursor->cursor)
        goto error;
    cursor->predicate = predicate;
    cursor->index_match =
        cx_index_match_indexes(cursor->predicate, cursor->row_group);
    cx_row_cursor_rewind(cursor);
    return cursor;
error:
    free(cursor);
    return NULL;
}

void cx_row_cursor_free(struct cx_row_cursor *cursor)
{
    cx_row_group_cursor_free(cursor->cursor);
    free(cursor);
}

void cx_row_cursor_rewind(struct cx_row_cursor *cursor)
{
    cursor->row_mask = 0;
    cursor->position = 64;
    cx_row_group_cursor_rewind(cursor->cursor);
    cursor->error = false;
}

static uint64_t cx_row_cursor_load_row_mask(struct cx_row_cursor *cursor)
{
    uint64_t row_mask = 0;
    while (!row_mask && cx_row_group_cursor_next(cursor->cursor)) {
        size_t count;
        if (cursor->index_match == CX_INDEX_MATCH_ALL) {
            count = cx_row_group_cursor_batch_count(cursor->cursor);
            row_mask = (uint64_t)-1;
            if (count < 64)
                row_mask &= ((uint64_t)1 << count) - 1;
        } else if (!cx_index_match_rows(cursor->predicate, cursor->row_group,
                                        cursor->cursor, &row_mask, &count))
            goto error;
    }
    return row_mask;
error:
    cursor->error = true;
    return 0;
}

bool cx_row_cursor_next(struct cx_row_cursor *cursor)
{
    if (cursor->position < 63) {
        uint64_t mask = cursor->row_mask >> (cursor->position + 1);
        if (mask) {
            cursor->position += __builtin_ctzll(mask) + 1;
            return true;
        }
    }
    cursor->row_mask = cx_row_cursor_load_row_mask(cursor);
    if (!cursor->row_mask)
        return false;
    cursor->position = __builtin_ctzll(cursor->row_mask);
    return true;
}

bool cx_row_cursor_error(const struct cx_row_cursor *cursor)
{
    return cursor->error;
}

size_t cx_row_cursor_count(struct cx_row_cursor *cursor)
{
    if (cursor->index_match == CX_INDEX_MATCH_NONE)
        return 0;
    else if (cursor->index_match == CX_INDEX_MATCH_ALL)
        return cx_row_group_row_count(cursor->row_group);
    cx_row_cursor_rewind(cursor);
    size_t count = 0;
    for (;;) {
        uint64_t row_mask = cx_row_cursor_load_row_mask(cursor);
        if (!row_mask)
            break;
        count += __builtin_popcountll(row_mask);
    }
    return count;
}

bool cx_row_cursor_get_null(const struct cx_row_cursor *cursor,
                            size_t column_index, bool *value)
{
    assert(cursor->row_mask);
    size_t count;
    const uint64_t *nulls =
        cx_row_group_cursor_batch_nulls(cursor->cursor, column_index, &count);
    uint64_t row_bit = (uint64_t)1 << cursor->position;
    if (!count || !nulls)
        return false;
    *value = *nulls & row_bit;
    return true;
}

bool cx_row_cursor_get_bit(const struct cx_row_cursor *cursor,
                           size_t column_index, bool *value)
{
    assert(cursor->row_mask);
    size_t count;
    const uint64_t *bitset =
        cx_row_group_cursor_batch_bit(cursor->cursor, column_index, &count);
    uint64_t row_bit = (uint64_t)1 << cursor->position;
    if (!count || !bitset)
        return false;
    *value = *bitset & row_bit;
    return true;
}

bool cx_row_cursor_get_i32(const struct cx_row_cursor *cursor,
                           size_t column_index, int32_t *value)
{
    assert(cursor->row_mask);
    size_t count;
    const int32_t *batch =
        cx_row_group_cursor_batch_i32(cursor->cursor, column_index, &count);
    if (!batch || !count)
        return false;
    *value = batch[cursor->position];
    return true;
}

bool cx_row_cursor_get_i64(const struct cx_row_cursor *cursor,
                           size_t column_index, int64_t *value)
{
    assert(cursor->row_mask);
    size_t count;
    const int64_t *batch =
        cx_row_group_cursor_batch_i64(cursor->cursor, column_index, &count);
    if (!batch || !count)
        return false;
    *value = batch[cursor->position];
    return true;
}

bool cx_row_cursor_get_flt(const struct cx_row_cursor *cursor,
                           size_t column_index, float *value)
{
    assert(cursor->row_mask);
    size_t count;
    const float *batch =
        cx_row_group_cursor_batch_flt(cursor->cursor, column_index, &count);
    if (!batch || !count)
        return false;
    *value = batch[cursor->position];
    return true;
}

bool cx_row_cursor_get_dbl(const struct cx_row_cursor *cursor,
                           size_t column_index, double *value)
{
    assert(cursor->row_mask);
    size_t count;
    const double *batch =
        cx_row_group_cursor_batch_dbl(cursor->cursor, column_index, &count);
    if (!batch || !count)
        return false;
    *value = batch[cursor->position];
    return true;
}

bool cx_row_cursor_get_str(const struct cx_row_cursor *cursor,
                           size_t column_index, const struct cx_string **value)
{
    assert(cursor->row_mask);
    size_t count;
    const struct cx_string *batch =
        cx_row_group_cursor_batch_str(cursor->cursor, column_index, &count);
    if (!batch || !count)
        return false;
    *value = &batch[cursor->position];
    return true;
}
