#define _GNU_SOURCE
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "match.h"
#include "predicate.h"

enum cx_predicate_type {
    CX_PREDICATE_TRUE,
    CX_PREDICATE_NULL,
    CX_PREDICATE_EQ,
    CX_PREDICATE_LT,
    CX_PREDICATE_GT,
    CX_PREDICATE_CONTAINS,
    CX_PREDICATE_AND,
    CX_PREDICATE_OR,
    CX_PREDICATE_CUSTOM
};

struct cx_predicate {
    enum cx_predicate_type type;
    enum cx_column_type column_type;
    size_t column;
    cx_value_t value;
    size_t operand_count;
    struct cx_predicate **operands;
    char *string;
    enum cx_str_location location;
    bool case_sensitive;
    bool negate;
    struct {
        cx_predicate_match_rows_t match_rows;
        cx_predicate_match_index_t match_index;
        int cost;
        void *data;
    } custom;
};

static const uint64_t cx_full_mask = (uint64_t)-1;

static struct cx_predicate *cx_predicate_new()
{
    return calloc(1, sizeof(struct cx_predicate));
}

void cx_predicate_free(struct cx_predicate *predicate)
{
    if (predicate->operands) {
        for (size_t i = 0; i < predicate->operand_count; i++)
            if (predicate->operands[i])
                cx_predicate_free(predicate->operands[i]);
        free(predicate->operands);
    }
    if (predicate->string)
        free(predicate->string);
    free(predicate);
}

struct cx_predicate *cx_predicate_new_true()
{
    struct cx_predicate *predicate = cx_predicate_new();
    if (!predicate)
        return NULL;
    predicate->type = CX_PREDICATE_TRUE;
    return predicate;
}

struct cx_predicate *cx_predicate_new_null(size_t column)
{
    struct cx_predicate *predicate = cx_predicate_new();
    if (!predicate)
        return NULL;
    predicate->type = CX_PREDICATE_NULL;
    predicate->column = column;
    return predicate;
}

struct cx_predicate *cx_predicate_new_bit_eq(size_t column, bool value)
{
    struct cx_predicate *predicate = cx_predicate_new();
    if (!predicate)
        return NULL;
    predicate->column = column;
    predicate->type = CX_PREDICATE_EQ;
    predicate->column_type = CX_COLUMN_BIT;
    predicate->value.bit = value;
    return predicate;
}

static struct cx_predicate *cx_predicate_new_i32(size_t column, int32_t value,
                                                 enum cx_predicate_type type)
{
    struct cx_predicate *predicate = cx_predicate_new();
    if (!predicate)
        return NULL;
    predicate->column = column;
    predicate->type = type;
    predicate->column_type = CX_COLUMN_I32;
    predicate->value.i32 = value;
    return predicate;
}

struct cx_predicate *cx_predicate_new_i32_eq(size_t column, int32_t value)
{
    return cx_predicate_new_i32(column, value, CX_PREDICATE_EQ);
}

struct cx_predicate *cx_predicate_new_i32_lt(size_t column, int32_t value)
{
    return cx_predicate_new_i32(column, value, CX_PREDICATE_LT);
}

struct cx_predicate *cx_predicate_new_i32_gt(size_t column, int32_t value)
{
    return cx_predicate_new_i32(column, value, CX_PREDICATE_GT);
}

static struct cx_predicate *cx_predicate_new_i64(size_t column, int64_t value,
                                                 enum cx_predicate_type type)
{
    struct cx_predicate *predicate = cx_predicate_new();
    if (!predicate)
        return NULL;
    predicate->column = column;
    predicate->type = type;
    predicate->column_type = CX_COLUMN_I64;
    predicate->value.i64 = value;
    return predicate;
}

struct cx_predicate *cx_predicate_new_i64_eq(size_t column, int64_t value)
{
    return cx_predicate_new_i64(column, value, CX_PREDICATE_EQ);
}

struct cx_predicate *cx_predicate_new_i64_lt(size_t column, int64_t value)
{
    return cx_predicate_new_i64(column, value, CX_PREDICATE_LT);
}

struct cx_predicate *cx_predicate_new_i64_gt(size_t column, int64_t value)
{
    return cx_predicate_new_i64(column, value, CX_PREDICATE_GT);
}

static struct cx_predicate *cx_predicate_new_str(size_t column,
                                                 const char *value,
                                                 enum cx_predicate_type type,
                                                 bool case_sensitive)
{
    struct cx_predicate *predicate = cx_predicate_new();
    if (!predicate)
        return NULL;
    predicate->column = column;
    predicate->type = type;
    predicate->column_type = CX_COLUMN_STR;
    size_t length = strlen(value);
#if CX_PCMPISTRM
    predicate->string = calloc(1, length + 1 + 16);
#else
    predicate->string = malloc(length + 1);
#endif
    if (!predicate->string)
        goto error;
    memcpy(predicate->string, value, length + 1);
    predicate->value.str.ptr = predicate->string;
    predicate->value.str.len = length;
    predicate->case_sensitive = case_sensitive;
    return predicate;
error:
    free(predicate);
    return NULL;
}

struct cx_predicate *cx_predicate_new_str_eq(size_t column, const char *value,
                                             bool case_sensitive)
{
    return cx_predicate_new_str(column, value, CX_PREDICATE_EQ, case_sensitive);
}

struct cx_predicate *cx_predicate_new_str_lt(size_t column, const char *value,
                                             bool case_sensitive)
{
    return cx_predicate_new_str(column, value, CX_PREDICATE_LT, case_sensitive);
}

struct cx_predicate *cx_predicate_new_str_gt(size_t column, const char *value,
                                             bool case_sensitive)
{
    return cx_predicate_new_str(column, value, CX_PREDICATE_GT, case_sensitive);
}

struct cx_predicate *cx_predicate_new_str_contains(
    size_t column, const char *value, bool case_sensitive,
    enum cx_str_location location)
{
    struct cx_predicate *predicate = cx_predicate_new_str(
        column, value, CX_PREDICATE_CONTAINS, case_sensitive);
    if (!predicate)
        return NULL;
    predicate->location = location;
    return predicate;
}

struct cx_predicate *cx_predicate_new_custom(
    size_t column, enum cx_column_type type,
    cx_predicate_match_rows_t match_rows,
    cx_predicate_match_index_t match_index, int cost, void *data)
{
    struct cx_predicate *predicate = cx_predicate_new();
    if (!predicate)
        return NULL;
    predicate->column = column;
    predicate->type = CX_PREDICATE_CUSTOM;
    predicate->column_type = type;
    predicate->custom.match_rows = match_rows;
    predicate->custom.match_index = match_index;
    predicate->custom.data = data;
    predicate->custom.cost = cost;
    return predicate;
}

static struct cx_predicate *cx_predicate_new_operator(
    enum cx_predicate_type type, size_t count, va_list operands)
{
    if (!count)
        return NULL;
    struct cx_predicate *predicate = cx_predicate_new();
    if (!predicate)
        return NULL;
    predicate->operand_count = count;
    predicate->type = type;
    predicate->operands = calloc(count, sizeof(struct cx_predicate *));
    if (!predicate->operands)
        goto error;
    for (size_t i = 0; i < count; i++) {
        predicate->operands[i] = va_arg(operands, struct cx_predicate *);
        if (!predicate->operands[i])
            goto error;
    }
    return predicate;
error:
    cx_predicate_free(predicate);
    return NULL;
}

static struct cx_predicate *cx_predicate_new_operator_array(
    enum cx_predicate_type type, size_t count, struct cx_predicate **operands)
{
    if (!count)
        return NULL;
    struct cx_predicate *predicate = cx_predicate_new();
    if (!predicate)
        return NULL;
    predicate->operand_count = count;
    predicate->type = type;
    predicate->operands = calloc(count, sizeof(struct cx_predicate *));
    if (!predicate->operands)
        goto error;
    for (size_t i = 0; i < count; i++) {
        predicate->operands[i] = operands[i];
        if (!predicate->operands[i])
            goto error;
    }
    return predicate;
error:
    cx_predicate_free(predicate);
    return NULL;
}

struct cx_predicate *cx_predicate_new_and(size_t count, ...)
{
    va_list operands;
    va_start(operands, count);
    struct cx_predicate *predicate = cx_predicate_new_vand(count, operands);
    va_end(operands);
    return predicate;
}

struct cx_predicate *cx_predicate_new_vand(size_t count, va_list operands)
{
    return cx_predicate_new_operator(CX_PREDICATE_AND, count, operands);
}

struct cx_predicate *cx_predicate_new_aand(size_t count,
                                           struct cx_predicate **operands)
{
    return cx_predicate_new_operator_array(CX_PREDICATE_AND, count, operands);
}

struct cx_predicate *cx_predicate_new_or(size_t count, ...)
{
    va_list operands;
    va_start(operands, count);
    struct cx_predicate *predicate = cx_predicate_new_vor(count, operands);
    va_end(operands);
    return predicate;
}

struct cx_predicate *cx_predicate_new_vor(size_t count, va_list operands)
{
    return cx_predicate_new_operator(CX_PREDICATE_OR, count, operands);
}

struct cx_predicate *cx_predicate_new_aor(size_t count,
                                          struct cx_predicate **operands)
{
    return cx_predicate_new_operator_array(CX_PREDICATE_OR, count, operands);
}

struct cx_predicate *cx_predicate_negate(struct cx_predicate *predicate)
{
    if (predicate)
        predicate->negate ^= 1;
    return predicate;
}

static bool cx_predicate_is_operator(const struct cx_predicate *predicate)
{
    return predicate->type == CX_PREDICATE_AND ||
           predicate->type == CX_PREDICATE_OR;
}

bool cx_predicate_valid(const struct cx_predicate *predicate,
                        const struct cx_row_group *row_group)
{
    if (predicate->column >= cx_row_group_column_count(row_group))
        return false;

    enum cx_column_type column_type =
        cx_row_group_column_type(row_group, predicate->column);
    if (!cx_predicate_is_operator(predicate) &&
        predicate->type != CX_PREDICATE_TRUE &&
        predicate->type != CX_PREDICATE_NULL &&
        predicate->column_type != column_type)
        return false;

    switch (predicate->type) {
        case CX_PREDICATE_TRUE:
        case CX_PREDICATE_NULL:
        case CX_PREDICATE_EQ:
        case CX_PREDICATE_CUSTOM:
            break;
        case CX_PREDICATE_LT:
        case CX_PREDICATE_GT:
            return column_type != CX_COLUMN_BIT;
        case CX_PREDICATE_CONTAINS:
            return column_type == CX_COLUMN_STR;
        case CX_PREDICATE_AND:
        case CX_PREDICATE_OR:
            for (size_t i = 0; i < predicate->operand_count; i++)
                if (!cx_predicate_valid(predicate->operands[i], row_group))
                    return false;
            break;
    }

    return true;
}

static inline uint64_t cx_mask_cap(uint64_t mask, size_t count)
{
    assert(count <= 64);
    if (count < 64)
        mask &= ((uint64_t)1 << count) - 1;
    return mask;
}

static bool cx_predicate_match_rows_eq(const struct cx_predicate *predicate,
                                       struct cx_row_group_cursor *cursor,
                                       enum cx_column_type type,
                                       uint64_t *matches, size_t *count)
{
    uint64_t mask = 0;
    switch (type) {
        case CX_COLUMN_BIT: {
            assert(predicate->column_type == CX_COLUMN_BIT);
            const uint64_t *values =
                cx_row_group_cursor_batch_bit(cursor, predicate->column, count);
            if (!values)
                goto error;
            if (*count) {
                if (predicate->value.bit)
                    mask = *values;
                else
                    mask = cx_mask_cap(~*values, *count);
            }
        } break;
        case CX_COLUMN_I32: {
            assert(predicate->column_type == CX_COLUMN_I32);
            const int32_t *values =
                cx_row_group_cursor_batch_i32(cursor, predicate->column, count);
            if (!values)
                goto error;
            mask = cx_match_i32_eq(*count, values, predicate->value.i32);
        } break;
        case CX_COLUMN_I64: {
            assert(predicate->column_type == CX_COLUMN_I64);
            const int64_t *values =
                cx_row_group_cursor_batch_i64(cursor, predicate->column, count);
            if (!values)
                goto error;
            mask = cx_match_i64_eq(*count, values, predicate->value.i64);
        } break;
        case CX_COLUMN_STR: {
            assert(predicate->column_type == CX_COLUMN_STR);
            const struct cx_string *values =
                cx_row_group_cursor_batch_str(cursor, predicate->column, count);
            if (!values)
                goto error;
            mask = cx_match_str_eq(*count, values, &predicate->value.str,
                                   predicate->case_sensitive);
        } break;
    }
    *matches = mask;
    return true;
error:
    return false;
}

static enum cx_predicate_match cx_predicate_match_index_eq(
    const struct cx_predicate *predicate, enum cx_column_type type,
    const struct cx_column_index *index)
{
    enum cx_predicate_match result = CX_PREDICATE_MATCH_UNKNOWN;
    switch (type) {
        case CX_COLUMN_BIT:
            assert(predicate->column_type == CX_COLUMN_BIT);
            if (index->min.bit && index->max.bit)
                result = predicate->value.bit ? CX_PREDICATE_MATCH_ALL_ROWS
                                              : CX_PREDICATE_MATCH_NO_ROWS;
            else if (!index->min.bit && !index->max.bit)
                result = predicate->value.bit ? CX_PREDICATE_MATCH_NO_ROWS
                                              : CX_PREDICATE_MATCH_ALL_ROWS;
            break;
        case CX_COLUMN_I32:
            assert(predicate->column_type == CX_COLUMN_I32);
            if (index->min.i32 > predicate->value.i32 ||
                index->max.i32 < predicate->value.i32)
                result = CX_PREDICATE_MATCH_NO_ROWS;
            else if (index->min.i32 == predicate->value.i32 &&
                     index->max.i32 == predicate->value.i32)
                result = CX_PREDICATE_MATCH_ALL_ROWS;
            break;
        case CX_COLUMN_I64:
            assert(predicate->column_type == CX_COLUMN_I64);
            if (index->min.i64 > predicate->value.i64 ||
                index->max.i64 < predicate->value.i64)
                result = CX_PREDICATE_MATCH_NO_ROWS;
            else if (index->min.i64 == predicate->value.i64 &&
                     index->max.i64 == predicate->value.i64)
                result = CX_PREDICATE_MATCH_ALL_ROWS;
            break;
        case CX_COLUMN_STR:
            assert(predicate->column_type == CX_COLUMN_STR);
            if (index->min.len > predicate->value.str.len ||
                index->max.len < predicate->value.str.len)
                result = CX_PREDICATE_MATCH_NO_ROWS;
            break;
    }
    return result;
}

static bool cx_predicate_match_rows_lt(const struct cx_predicate *predicate,
                                       struct cx_row_group_cursor *cursor,
                                       enum cx_column_type type,
                                       uint64_t *matches, size_t *count)
{
    uint64_t mask = 0;
    switch (type) {
        case CX_COLUMN_BIT:
            goto error;  // unsupported
        case CX_COLUMN_I32: {
            assert(predicate->column_type == CX_COLUMN_I32);
            const int32_t *values =
                cx_row_group_cursor_batch_i32(cursor, predicate->column, count);
            if (!values)
                goto error;
            mask = cx_match_i32_lt(*count, values, predicate->value.i32);
        } break;
        case CX_COLUMN_I64: {
            assert(predicate->column_type == CX_COLUMN_I64);
            const int64_t *values =
                cx_row_group_cursor_batch_i64(cursor, predicate->column, count);
            if (!values)
                goto error;
            mask = cx_match_i64_lt(*count, values, predicate->value.i64);
        } break;
        case CX_COLUMN_STR: {
            assert(predicate->column_type == CX_COLUMN_STR);
            const struct cx_string *values =
                cx_row_group_cursor_batch_str(cursor, predicate->column, count);
            if (!values)
                goto error;
            mask = cx_match_str_lt(*count, values, &predicate->value.str,
                                   predicate->case_sensitive);
        } break;
    }
    *matches = mask;
    return true;
error:
    return false;
}

static enum cx_predicate_match cx_predicate_match_index_lt(
    const struct cx_predicate *predicate, enum cx_column_type type,
    const struct cx_column_index *index)
{
    enum cx_predicate_match result = CX_PREDICATE_MATCH_UNKNOWN;
    switch (type) {
        case CX_COLUMN_BIT:
            break;
        case CX_COLUMN_I32:
            assert(predicate->column_type == CX_COLUMN_I32);
            if (index->min.i32 >= predicate->value.i32)
                result = CX_PREDICATE_MATCH_NO_ROWS;
            else if (index->max.i32 < predicate->value.i32)
                result = CX_PREDICATE_MATCH_ALL_ROWS;
            break;
        case CX_COLUMN_I64:
            assert(predicate->column_type == CX_COLUMN_I64);
            if (index->min.i64 >= predicate->value.i64)
                result = CX_PREDICATE_MATCH_NO_ROWS;
            else if (index->max.i64 < predicate->value.i64)
                result = CX_PREDICATE_MATCH_ALL_ROWS;
            break;
        case CX_COLUMN_STR:
            break;
    }
    return result;
}

static bool cx_predicate_match_rows_gt(const struct cx_predicate *predicate,
                                       struct cx_row_group_cursor *cursor,
                                       enum cx_column_type type,
                                       uint64_t *matches, size_t *count)
{
    uint64_t mask = 0;
    switch (type) {
        case CX_COLUMN_BIT:
            goto error;  // unsupported
        case CX_COLUMN_I32: {
            assert(predicate->column_type == CX_COLUMN_I32);
            const int32_t *values =
                cx_row_group_cursor_batch_i32(cursor, predicate->column, count);
            if (!values)
                goto error;
            mask = cx_match_i32_gt(*count, values, predicate->value.i32);
        } break;
        case CX_COLUMN_I64: {
            assert(predicate->column_type == CX_COLUMN_I64);
            const int64_t *values =
                cx_row_group_cursor_batch_i64(cursor, predicate->column, count);
            if (!values)
                goto error;
            mask = cx_match_i64_gt(*count, values, predicate->value.i64);
        } break;
        case CX_COLUMN_STR: {
            assert(predicate->column_type == CX_COLUMN_STR);
            const struct cx_string *values =
                cx_row_group_cursor_batch_str(cursor, predicate->column, count);
            if (!values)
                goto error;
            mask = cx_match_str_gt(*count, values, &predicate->value.str,
                                   predicate->case_sensitive);
        } break;
    }
    *matches = mask;
    return true;
error:
    return false;
}

static enum cx_predicate_match cx_predicate_match_index_gt(
    const struct cx_predicate *predicate, enum cx_column_type type,
    const struct cx_column_index *index)
{
    enum cx_predicate_match result = CX_PREDICATE_MATCH_UNKNOWN;
    switch (type) {
        case CX_COLUMN_BIT:
            break;
        case CX_COLUMN_I32:
            assert(predicate->column_type == CX_COLUMN_I32);
            if (index->min.i32 > predicate->value.i32)
                result = CX_PREDICATE_MATCH_ALL_ROWS;
            else if (index->max.i32 <= predicate->value.i32)
                result = CX_PREDICATE_MATCH_NO_ROWS;
            break;
        case CX_COLUMN_I64:
            assert(predicate->column_type == CX_COLUMN_I64);
            if (index->min.i64 > predicate->value.i64)
                result = CX_PREDICATE_MATCH_ALL_ROWS;
            else if (index->max.i64 <= predicate->value.i64)
                result = CX_PREDICATE_MATCH_NO_ROWS;
            break;
        case CX_COLUMN_STR:
            break;
    }
    return result;
}

static bool cx_predicate_match_rows_custom(const struct cx_predicate *predicate,
                                           struct cx_row_group_cursor *cursor,
                                           enum cx_column_type type,
                                           uint64_t *matches, size_t *count)
{
    size_t column = predicate->column;
    const void *values = NULL;
    switch (type) {
        case CX_COLUMN_BIT:
            assert(predicate->column_type == CX_COLUMN_BIT);
            values = cx_row_group_cursor_batch_bit(cursor, column, count);
            break;
        case CX_COLUMN_I32:
            assert(predicate->column_type == CX_COLUMN_I32);
            values = cx_row_group_cursor_batch_i32(cursor, column, count);
            break;
        case CX_COLUMN_I64:
            assert(predicate->column_type == CX_COLUMN_I64);
            values = cx_row_group_cursor_batch_i64(cursor, column, count);
            break;
        case CX_COLUMN_STR:
            assert(predicate->column_type == CX_COLUMN_STR);
            values = cx_row_group_cursor_batch_str(cursor, column, count);
            break;
    }
    if (!values)
        return false;
    if (!predicate->custom.match_rows)
        return true;
    return predicate->custom.match_rows(type, *count, values, matches,
                                        predicate->custom.data);
}

bool cx_predicate_match_rows(const struct cx_predicate *predicate,
                             const struct cx_row_group *row_group,
                             struct cx_row_group_cursor *cursor,
                             uint64_t *matches, size_t *count)
{
    enum cx_column_type column_type =
        cx_row_group_column_type(row_group, predicate->column);
    uint64_t mask = 0;
    switch (predicate->type) {
        case CX_PREDICATE_TRUE:
            *count = cx_row_group_cursor_batch_count(cursor);
            mask = cx_mask_cap(cx_full_mask, *count);
            break;
        case CX_PREDICATE_NULL: {
            const uint64_t *nulls = cx_row_group_cursor_batch_nulls(
                cursor, predicate->column, count);
            if (!nulls)
                goto error;
            mask = *count ? *nulls : 0;
        } break;
        case CX_PREDICATE_EQ:
            if (!cx_predicate_match_rows_eq(predicate, cursor, column_type,
                                            &mask, count))
                goto error;
            break;
        case CX_PREDICATE_LT:
            if (!cx_predicate_match_rows_lt(predicate, cursor, column_type,
                                            &mask, count))
                goto error;
            break;
        case CX_PREDICATE_GT:
            if (!cx_predicate_match_rows_gt(predicate, cursor, column_type,
                                            &mask, count))
                goto error;
            break;
        case CX_PREDICATE_CONTAINS:
            assert(column_type == CX_COLUMN_STR);
            {
                const struct cx_string *values = cx_row_group_cursor_batch_str(
                    cursor, predicate->column, count);
                if (!values)
                    goto error;
                mask = cx_match_str_contains(
                    *count, values, &predicate->value.str,
                    predicate->case_sensitive, predicate->location);
            }
            break;
        case CX_PREDICATE_CUSTOM:
            if (!cx_predicate_match_rows_custom(predicate, cursor, column_type,
                                                &mask, count))
                goto error;
            break;
        case CX_PREDICATE_AND:
            mask = cx_full_mask;
            // short-circuit the remaining predicates once the mask is empty
            for (size_t i = 0; mask && i < predicate->operand_count; i++) {
                uint64_t operand_mask;
                if (!cx_predicate_match_rows(predicate->operands[i], row_group,
                                             cursor, &operand_mask, count))
                    goto error;
                mask &= operand_mask;
            }
            break;
        case CX_PREDICATE_OR:
            mask = 0;
            // short-circuit the remaining predicates once the mask is full
            for (size_t i = 0;
                 mask != cx_full_mask && i < predicate->operand_count; i++) {
                uint64_t operand_mask;
                if (!cx_predicate_match_rows(predicate->operands[i], row_group,
                                             cursor, &operand_mask, count))
                    goto error;
                mask |= operand_mask;
            }
            break;
    }
    *matches = predicate->negate ? cx_mask_cap(~mask, *count) : mask;
    return true;
error:
    return false;
}

enum cx_predicate_match cx_predicate_match_indexes(
    const struct cx_predicate *predicate, const struct cx_row_group *row_group)
{
    if (!cx_row_group_row_count(row_group))
        return CX_PREDICATE_MATCH_NO_ROWS;

    const struct cx_column_index *index =
        cx_row_group_column_index(row_group, predicate->column);
    enum cx_column_type type =
        cx_row_group_column_type(row_group, predicate->column);
    enum cx_predicate_match result = CX_PREDICATE_MATCH_UNKNOWN;
    switch (predicate->type) {
        case CX_PREDICATE_TRUE:
            result = CX_PREDICATE_MATCH_ALL_ROWS;
            break;
        case CX_PREDICATE_NULL: {
            const struct cx_column_index *index =
                cx_row_group_null_index(row_group, predicate->column);
            if (index->min.bit && index->max.bit)
                result = CX_PREDICATE_MATCH_ALL_ROWS;
            else if (!index->min.bit && !index->max.bit)
                result = CX_PREDICATE_MATCH_NO_ROWS;
        } break;
        case CX_PREDICATE_EQ:
            result = cx_predicate_match_index_eq(predicate, type, index);
            break;
        case CX_PREDICATE_LT:
            result = cx_predicate_match_index_lt(predicate, type, index);
            break;
        case CX_PREDICATE_GT:
            result = cx_predicate_match_index_gt(predicate, type, index);
            break;
        case CX_PREDICATE_CONTAINS:
            if (index->max.len < predicate->value.str.len)
                result = CX_PREDICATE_MATCH_NO_ROWS;
            break;
        case CX_PREDICATE_AND:
            result = CX_PREDICATE_MATCH_ALL_ROWS;
            for (size_t i = 0; result != CX_PREDICATE_MATCH_NO_ROWS &&
                               i < predicate->operand_count;
                 i++) {
                enum cx_predicate_match operand_match =
                    cx_predicate_match_indexes(predicate->operands[i],
                                               row_group);
                if (operand_match < result)
                    result = operand_match;
            }
            break;
        case CX_PREDICATE_OR:
            result = CX_PREDICATE_MATCH_NO_ROWS;
            for (size_t i = 0; result != CX_PREDICATE_MATCH_ALL_ROWS &&
                               i < predicate->operand_count;
                 i++) {
                enum cx_predicate_match operand_match =
                    cx_predicate_match_indexes(predicate->operands[i],
                                               row_group);
                if (operand_match > result)
                    result = operand_match;
            }
            break;
        case CX_PREDICATE_CUSTOM:
            if (predicate->custom.match_index) {
                const struct cx_column_index *index =
                    cx_row_group_column_index(row_group, predicate->column);
                result = predicate->custom.match_index(type, index,
                                                       predicate->custom.data);
            }
            break;
    }
    return predicate->negate ? -result : result;
}

static int cx_column_cost(enum cx_column_type type)
{
    int cost = 0;
    switch (type) {
        case CX_COLUMN_BIT:
            cost = 1;
            break;
        case CX_COLUMN_I32:
            cost = 8;
            break;
        case CX_COLUMN_I64:
            cost = 16;
            break;
        case CX_COLUMN_STR:
            cost = 128;
            break;
    }
    return cost;
}

static int cx_predicate_cost(const struct cx_predicate *predicate,
                             const struct cx_row_group *row_group)
{
    // in future, this function should look at more than just the
    // column type to determine the cost of evaluating the predicate.
    // for example, it could check the size of each column, whether
    // it's compressed and/or encoded, and the compression ratio
    int cost = 0;
    switch (predicate->type) {
        case CX_PREDICATE_TRUE:
            break;
        case CX_PREDICATE_NULL:
            cost = cx_column_cost(CX_COLUMN_BIT);
            break;
        case CX_PREDICATE_EQ:
        case CX_PREDICATE_LT:
        case CX_PREDICATE_GT:
        case CX_PREDICATE_CONTAINS:
            cost = cx_column_cost(
                cx_row_group_column_type(row_group, predicate->column));
            break;
        case CX_PREDICATE_AND:
        case CX_PREDICATE_OR:
            for (size_t i = 0; i < predicate->operand_count; i++)
                cost += cx_predicate_cost(predicate->operands[i], row_group);
            break;
        case CX_PREDICATE_CUSTOM:
            if (predicate->custom.cost >= 0)
                cost = predicate->custom.cost;
            else
                cost = cx_column_cost(
                    cx_row_group_column_type(row_group, predicate->column));
            break;
    }
    return cost;
}

// qsort_r() differs on OS X and Linux..
#ifdef __APPLE__
static int cx_predicate_cmp(void *ctx, const void *a, const void *b)
#else
static int cx_predicate_cmp(const void *a, const void *b, void *ctx)
#endif
{
    const struct cx_row_group *row_group = (const struct cx_row_group *)ctx;
    // sort by cost asc
    return cx_predicate_cost(*(struct cx_predicate **)a, row_group) -
           cx_predicate_cost(*(struct cx_predicate **)b, row_group);
}

void cx_predicate_optimize(struct cx_predicate *predicate,
                           const struct cx_row_group *row_group)
{
#ifdef __APPLE__
    qsort_r(predicate->operands, predicate->operand_count,
            sizeof(struct cx_predicate *), (void *)row_group, cx_predicate_cmp);
#else
    qsort_r(predicate->operands, predicate->operand_count,
            sizeof(struct cx_predicate *), cx_predicate_cmp, (void *)row_group);
#endif
    if (cx_predicate_is_operator(predicate)) {
        for (size_t i = 0; i < predicate->operand_count; i++)
            cx_predicate_optimize(predicate->operands[i], row_group);
        return;
    }
}

const struct cx_predicate **cx_predicate_operands(
    const struct cx_predicate *predicate, size_t *size)
{
    // this is necessary to test cx_predicate_optimize() operand reordering
    // the only other way would be to make the cx_predicate struct public
    *size = predicate->operand_count;
    return (const struct cx_predicate **)predicate->operands;
}
