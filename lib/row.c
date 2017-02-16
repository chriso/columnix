#include <assert.h>
#include <stdlib.h>

#include "row.h"

struct zcs_row_cursor {
    struct zcs_row_group *row_group;
    struct zcs_row_group_cursor *cursor;
    const struct zcs_predicate *predicate;
    size_t column_count;
    uint64_t row_mask;
    size_t position;
    enum zcs_predicate_match index_match;
    bool implicit_predicate;
    bool error;
};

struct zcs_row_cursor *zcs_row_cursor_new(
    struct zcs_row_group *row_group, const struct zcs_predicate *predicate)
{
    struct zcs_row_cursor *cursor = calloc(1, sizeof(*cursor));
    if (!cursor)
        return NULL;
    cursor->row_group = row_group;
    cursor->cursor = zcs_row_group_cursor_new(row_group);
    if (!cursor->cursor)
        goto error;
    cursor->predicate = predicate;
    cursor->index_match =
        zcs_predicate_match_indexes(cursor->predicate, cursor->row_group);
    zcs_row_cursor_rewind(cursor);
    return cursor;
error:
    free(cursor);
    return NULL;
}

void zcs_row_cursor_free(struct zcs_row_cursor *cursor)
{
    zcs_row_group_cursor_free(cursor->cursor);
    free(cursor);
}

void zcs_row_cursor_rewind(struct zcs_row_cursor *cursor)
{
    cursor->row_mask = 0;
    cursor->position = 64;
    zcs_row_group_cursor_rewind(cursor->cursor);
    cursor->error = false;
}

static uint64_t zcs_row_cursor_load_row_mask(struct zcs_row_cursor *cursor)
{
    uint64_t row_mask = 0;
    while (!row_mask && zcs_row_group_cursor_next(cursor->cursor)) {
        size_t count;
        if (cursor->index_match == ZCS_PREDICATE_MATCH_ALL_ROWS) {
            count = zcs_row_group_cursor_batch_count(cursor->cursor);
            row_mask = (uint64_t)-1;
            if (count < 64)
                row_mask &= ((uint64_t)1 << count) - 1;
        } else if (!zcs_predicate_match_rows(cursor->predicate,
                                             cursor->row_group, cursor->cursor,
                                             &row_mask, &count))
            goto error;
    }
    return row_mask;
error:
    cursor->error = true;
    return 0;
}

bool zcs_row_cursor_next(struct zcs_row_cursor *cursor)
{
    if (cursor->position < 63) {
        uint64_t mask = cursor->row_mask >> (cursor->position + 1);
        if (mask) {
            cursor->position += __builtin_ctzll(mask) + 1;
            return true;
        }
    }
    cursor->row_mask = zcs_row_cursor_load_row_mask(cursor);
    if (!cursor->row_mask)
        return false;
    cursor->position = __builtin_ctzll(cursor->row_mask);
    return true;
}

bool zcs_row_cursor_error(const struct zcs_row_cursor *cursor)
{
    return cursor->error;
}

size_t zcs_row_cursor_count(struct zcs_row_cursor *cursor)
{
    if (cursor->index_match == ZCS_PREDICATE_MATCH_NO_ROWS)
        return 0;
    else if (cursor->index_match == ZCS_PREDICATE_MATCH_ALL_ROWS)
        return zcs_row_group_row_count(cursor->row_group);
    zcs_row_cursor_rewind(cursor);
    size_t count = 0;
    for (;;) {
        uint64_t row_mask = zcs_row_cursor_load_row_mask(cursor);
        if (!row_mask)
            break;
        count += __builtin_popcountll(row_mask);
    }
    return count;
}

bool zcs_row_cursor_get_bit(const struct zcs_row_cursor *cursor,
                            size_t column_index, bool *value)
{
    assert(cursor->row_mask);
    size_t count;
    const uint64_t *bitset =
        zcs_row_group_cursor_batch_bit(cursor->cursor, column_index, &count);
    uint64_t row_bit = (uint64_t)1 << cursor->position;
    if (!count || !bitset)
        return false;
    *value = *bitset & row_bit;
    return true;
}

bool zcs_row_cursor_get_i32(const struct zcs_row_cursor *cursor,
                            size_t column_index, int32_t *value)
{
    assert(cursor->row_mask);
    size_t count;
    const int32_t *batch =
        zcs_row_group_cursor_batch_i32(cursor->cursor, column_index, &count);
    if (!batch || !count)
        return false;
    *value = batch[cursor->position];
    return true;
}

bool zcs_row_cursor_get_i64(const struct zcs_row_cursor *cursor,
                            size_t column_index, int64_t *value)
{
    assert(cursor->row_mask);
    size_t count;
    const int64_t *batch =
        zcs_row_group_cursor_batch_i64(cursor->cursor, column_index, &count);
    if (!batch || !count)
        return false;
    *value = batch[cursor->position];
    return true;
}

bool zcs_row_cursor_get_str(const struct zcs_row_cursor *cursor,
                            size_t column_index,
                            const struct zcs_string **value)
{
    assert(cursor->row_mask);
    size_t count;
    const struct zcs_string *batch =
        zcs_row_group_cursor_batch_str(cursor->cursor, column_index, &count);
    if (!batch || !count)
        return false;
    *value = &batch[cursor->position];
    return true;
}
