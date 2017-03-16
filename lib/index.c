#define __STDC_LIMIT_MACROS
#include <assert.h>
#include <float.h>
#include <stdint.h>
#include <stdlib.h>

#include "index.h"

static void cx_index_update_bit(struct cx_index *, struct cx_column_cursor *);
static void cx_index_update_i32(struct cx_index *, struct cx_column_cursor *);
static void cx_index_update_i64(struct cx_index *, struct cx_column_cursor *);
static void cx_index_update_flt(struct cx_index *, struct cx_column_cursor *);
static void cx_index_update_dbl(struct cx_index *, struct cx_column_cursor *);
static void cx_index_update_str(struct cx_index *, struct cx_column_cursor *);

struct cx_index *cx_index_new(const struct cx_column *column)
{
    struct cx_index *index = calloc(1, sizeof(*index));
    if (!index)
        return NULL;
    struct cx_column_cursor *cursor = cx_column_cursor_new(column);
    if (!cursor)
        goto error;
    switch (cx_column_type(column)) {
        case CX_COLUMN_BIT:
            index->min.bit = true;
            cx_index_update_bit(index, cursor);
            break;
        case CX_COLUMN_I32:
            index->min.i32 = INT32_MAX;
            cx_index_update_i32(index, cursor);
            break;
        case CX_COLUMN_I64:
            index->min.i64 = INT64_MAX;
            cx_index_update_i64(index, cursor);
            break;
        case CX_COLUMN_FLT:
            index->min.flt = FLT_MAX;
            cx_index_update_flt(index, cursor);
            break;
        case CX_COLUMN_DBL:
            index->min.dbl = DBL_MAX;
            cx_index_update_dbl(index, cursor);
            break;
        case CX_COLUMN_STR:
            index->min.len = UINT64_MAX;
            cx_index_update_str(index, cursor);
            break;
    }
    cx_column_cursor_free(cursor);
    return index;
error:
    free(index);
    return NULL;
}

void cx_index_free(struct cx_index *index)
{
    free(index);
}

static void cx_index_update_bit(struct cx_index *index,
                                struct cx_column_cursor *cursor)
{
    while (cx_column_cursor_valid(cursor)) {
        size_t count;
        const uint64_t *bitset =
            cx_column_cursor_next_batch_bit(cursor, &count);
        assert(count);
        index->count += count;
        for (size_t i = 0; i < count; i++) {
            bool value = *bitset & ((uint64_t)1 << i);
            index->min.bit = index->min.bit && value;
            index->max.bit = index->max.bit || value;
        }
    }
}

static void cx_index_update_i32(struct cx_index *index,
                                struct cx_column_cursor *cursor)
{
    while (cx_column_cursor_valid(cursor)) {
        size_t count;
        const int32_t *values = cx_column_cursor_next_batch_i32(cursor, &count);
        assert(count);
        index->count += count;
        for (size_t i = 0; i < count; i++) {
            int32_t value = values[i];
            if (value > index->max.i32)
                index->max.i32 = value;
            if (value < index->min.i32)
                index->min.i32 = value;
        }
    }
}

static void cx_index_update_i64(struct cx_index *index,
                                struct cx_column_cursor *cursor)
{
    while (cx_column_cursor_valid(cursor)) {
        size_t count;
        const int64_t *values = cx_column_cursor_next_batch_i64(cursor, &count);
        assert(count);
        index->count += count;
        for (size_t i = 0; i < count; i++) {
            int64_t value = values[i];
            if (value > index->max.i64)
                index->max.i64 = value;
            if (value < index->min.i64)
                index->min.i64 = value;
        }
    }
}

static void cx_index_update_flt(struct cx_index *index,
                                struct cx_column_cursor *cursor)
{
    while (cx_column_cursor_valid(cursor)) {
        size_t count;
        const float *values = cx_column_cursor_next_batch_flt(cursor, &count);
        assert(count);
        index->count += count;
        for (size_t i = 0; i < count; i++) {
            float value = values[i];
            if (value > index->max.flt)
                index->max.flt = value;
            if (value < index->min.flt)
                index->min.flt = value;
        }
    }
}

static void cx_index_update_dbl(struct cx_index *index,
                                struct cx_column_cursor *cursor)
{
    while (cx_column_cursor_valid(cursor)) {
        size_t count;
        const double *values = cx_column_cursor_next_batch_dbl(cursor, &count);
        assert(count);
        index->count += count;
        for (size_t i = 0; i < count; i++) {
            double value = values[i];
            if (value > index->max.dbl)
                index->max.dbl = value;
            if (value < index->min.dbl)
                index->min.dbl = value;
        }
    }
}

static void cx_index_update_str(struct cx_index *index,
                                struct cx_column_cursor *cursor)
{
    while (cx_column_cursor_valid(cursor)) {
        size_t count;
        const struct cx_string *values =
            cx_column_cursor_next_batch_str(cursor, &count);
        assert(count);
        index->count += count;
        for (size_t i = 0; i < count; i++) {
            uint64_t value = values[i].len;
            if (value > index->max.len)
                index->max.len = value;
            if (value < index->min.len)
                index->min.len = value;
        }
    }
}

enum cx_index_match cx_index_match_bit_eq(const struct cx_index *index,
                                          bool value)
{
    if (index->min.bit && index->max.bit)
        return value ? CX_INDEX_MATCH_ALL : CX_INDEX_MATCH_NONE;
    if (!index->min.bit && !index->max.bit)
        return value ? CX_INDEX_MATCH_NONE : CX_INDEX_MATCH_ALL;
    return CX_INDEX_MATCH_UNKNOWN;
}

enum cx_index_match cx_index_match_i32_eq(const struct cx_index *index,
                                          int32_t value)
{
    if (index->min.i32 > value || index->max.i32 < value)
        return CX_INDEX_MATCH_NONE;
    if (index->min.i32 == value && index->max.i32 == value)
        return CX_INDEX_MATCH_ALL;
    return CX_INDEX_MATCH_UNKNOWN;
}

enum cx_index_match cx_index_match_i32_lt(const struct cx_index *index,
                                          int32_t value)
{
    if (index->min.i32 >= value)
        return CX_INDEX_MATCH_NONE;
    if (index->max.i32 < value)
        return CX_INDEX_MATCH_ALL;
    return CX_INDEX_MATCH_UNKNOWN;
}

enum cx_index_match cx_index_match_i32_gt(const struct cx_index *index,
                                          int32_t value)
{
    if (index->min.i32 > value)
        return CX_INDEX_MATCH_ALL;
    if (index->max.i32 <= value)
        return CX_INDEX_MATCH_NONE;
    return CX_INDEX_MATCH_UNKNOWN;
}

enum cx_index_match cx_index_match_i64_eq(const struct cx_index *index,
                                          int64_t value)
{
    if (index->min.i64 > value || index->max.i64 < value)
        return CX_INDEX_MATCH_NONE;
    if (index->min.i64 == value && index->max.i64 == value)
        return CX_INDEX_MATCH_ALL;
    return CX_INDEX_MATCH_UNKNOWN;
}

enum cx_index_match cx_index_match_i64_lt(const struct cx_index *index,
                                          int64_t value)
{
    if (index->min.i64 >= value)
        return CX_INDEX_MATCH_NONE;
    if (index->max.i64 < value)
        return CX_INDEX_MATCH_ALL;
    return CX_INDEX_MATCH_UNKNOWN;
}

enum cx_index_match cx_index_match_i64_gt(const struct cx_index *index,
                                          int64_t value)
{
    if (index->min.i64 > value)
        return CX_INDEX_MATCH_ALL;
    if (index->max.i64 <= value)
        return CX_INDEX_MATCH_NONE;
    return CX_INDEX_MATCH_UNKNOWN;
}

enum cx_index_match cx_index_match_flt_eq(const struct cx_index *index,
                                          float value)
{
    if (index->min.flt > value || index->max.flt < value)
        return CX_INDEX_MATCH_NONE;
    if (index->min.flt == value && index->max.flt == value)
        return CX_INDEX_MATCH_ALL;
    return CX_INDEX_MATCH_UNKNOWN;
}

enum cx_index_match cx_index_match_flt_lt(const struct cx_index *index,
                                          float value)
{
    if (index->min.flt >= value)
        return CX_INDEX_MATCH_NONE;
    if (index->max.flt < value)
        return CX_INDEX_MATCH_ALL;
    return CX_INDEX_MATCH_UNKNOWN;
}

enum cx_index_match cx_index_match_flt_gt(const struct cx_index *index,
                                          float value)
{
    if (index->min.flt > value)
        return CX_INDEX_MATCH_ALL;
    if (index->max.flt <= value)
        return CX_INDEX_MATCH_NONE;
    return CX_INDEX_MATCH_UNKNOWN;
}

enum cx_index_match cx_index_match_dbl_eq(const struct cx_index *index,
                                          double value)
{
    if (index->min.dbl > value || index->max.dbl < value)
        return CX_INDEX_MATCH_NONE;
    if (index->min.dbl == value && index->max.dbl == value)
        return CX_INDEX_MATCH_ALL;
    return CX_INDEX_MATCH_UNKNOWN;
}

enum cx_index_match cx_index_match_dbl_lt(const struct cx_index *index,
                                          double value)
{
    if (index->min.dbl >= value)
        return CX_INDEX_MATCH_NONE;
    if (index->max.dbl < value)
        return CX_INDEX_MATCH_ALL;
    return CX_INDEX_MATCH_UNKNOWN;
}

enum cx_index_match cx_index_match_dbl_gt(const struct cx_index *index,
                                          double value)
{
    if (index->min.dbl > value)
        return CX_INDEX_MATCH_ALL;
    if (index->max.dbl <= value)
        return CX_INDEX_MATCH_NONE;
    return CX_INDEX_MATCH_UNKNOWN;
}

enum cx_index_match cx_index_match_str_eq(const struct cx_index *index,
                                          const struct cx_string *string)
{
    if (index->min.len > string->len || index->max.len < string->len)
        return CX_INDEX_MATCH_NONE;
    return CX_INDEX_MATCH_UNKNOWN;
}

enum cx_index_match cx_index_match_str_contains(const struct cx_index *index,
                                                const struct cx_string *string)
{
    if (index->max.len < string->len)
        return CX_INDEX_MATCH_NONE;
    return CX_INDEX_MATCH_UNKNOWN;
}
