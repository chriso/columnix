#define _GNU_SOURCE
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "match.h"
#include "predicate.h"

enum zcs_predicate_type {
    ZCS_PREDICATE_TRUE,
    ZCS_PREDICATE_NULL,
    ZCS_PREDICATE_EQ,
    ZCS_PREDICATE_LT,
    ZCS_PREDICATE_GT,
    ZCS_PREDICATE_CONTAINS,
    ZCS_PREDICATE_AND,
    ZCS_PREDICATE_OR
};

struct zcs_predicate {
    enum zcs_predicate_type type;
    enum zcs_column_type column_type;
    size_t column;
    zcs_value_t value;
    size_t operand_count;
    struct zcs_predicate **operands;
    char *string;
    enum zcs_str_location location;
    bool case_sensitive;
    bool negate;
};

static const uint64_t zcs_full_mask = (uint64_t)-1;

static struct zcs_predicate *zcs_predicate_new()
{
    return calloc(1, sizeof(struct zcs_predicate));
}

void zcs_predicate_free(struct zcs_predicate *predicate)
{
    if (predicate->operands) {
        for (size_t i = 0; i < predicate->operand_count; i++)
            if (predicate->operands[i])
                zcs_predicate_free(predicate->operands[i]);
        free(predicate->operands);
    }
    if (predicate->string)
        free(predicate->string);
    free(predicate);
}

struct zcs_predicate *zcs_predicate_new_true()
{
    struct zcs_predicate *predicate = zcs_predicate_new();
    if (!predicate)
        return NULL;
    predicate->type = ZCS_PREDICATE_TRUE;
    return predicate;
}

struct zcs_predicate *zcs_predicate_new_null(size_t column)
{
    struct zcs_predicate *predicate = zcs_predicate_new();
    if (!predicate)
        return NULL;
    predicate->type = ZCS_PREDICATE_NULL;
    predicate->column = column;
    return predicate;
}

struct zcs_predicate *zcs_predicate_new_bit_eq(size_t column, bool value)
{
    struct zcs_predicate *predicate = zcs_predicate_new();
    if (!predicate)
        return NULL;
    predicate->column = column;
    predicate->type = ZCS_PREDICATE_EQ;
    predicate->column_type = ZCS_COLUMN_BIT;
    predicate->value.bit = value;
    return predicate;
}

static struct zcs_predicate *zcs_predicate_new_i32(size_t column, int32_t value,
                                                   enum zcs_predicate_type type)
{
    struct zcs_predicate *predicate = zcs_predicate_new();
    if (!predicate)
        return NULL;
    predicate->column = column;
    predicate->type = type;
    predicate->column_type = ZCS_COLUMN_I32;
    predicate->value.i32 = value;
    return predicate;
}

struct zcs_predicate *zcs_predicate_new_i32_eq(size_t column, int32_t value)
{
    return zcs_predicate_new_i32(column, value, ZCS_PREDICATE_EQ);
}

struct zcs_predicate *zcs_predicate_new_i32_lt(size_t column, int32_t value)
{
    return zcs_predicate_new_i32(column, value, ZCS_PREDICATE_LT);
}

struct zcs_predicate *zcs_predicate_new_i32_gt(size_t column, int32_t value)
{
    return zcs_predicate_new_i32(column, value, ZCS_PREDICATE_GT);
}

static struct zcs_predicate *zcs_predicate_new_i64(size_t column, int64_t value,
                                                   enum zcs_predicate_type type)
{
    struct zcs_predicate *predicate = zcs_predicate_new();
    if (!predicate)
        return NULL;
    predicate->column = column;
    predicate->type = type;
    predicate->column_type = ZCS_COLUMN_I64;
    predicate->value.i64 = value;
    return predicate;
}

struct zcs_predicate *zcs_predicate_new_i64_eq(size_t column, int64_t value)
{
    return zcs_predicate_new_i64(column, value, ZCS_PREDICATE_EQ);
}

struct zcs_predicate *zcs_predicate_new_i64_lt(size_t column, int64_t value)
{
    return zcs_predicate_new_i64(column, value, ZCS_PREDICATE_LT);
}

struct zcs_predicate *zcs_predicate_new_i64_gt(size_t column, int64_t value)
{
    return zcs_predicate_new_i64(column, value, ZCS_PREDICATE_GT);
}

static struct zcs_predicate *zcs_predicate_new_str(size_t column,
                                                   const char *value,
                                                   enum zcs_predicate_type type,
                                                   bool case_sensitive)
{
    struct zcs_predicate *predicate = zcs_predicate_new();
    if (!predicate)
        return NULL;
    predicate->column = column;
    predicate->type = type;
    predicate->column_type = ZCS_COLUMN_STR;
    size_t length = strlen(value);
#if ZCS_PCMPISTRM
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

struct zcs_predicate *zcs_predicate_new_str_eq(size_t column, const char *value,
                                               bool case_sensitive)
{
    return zcs_predicate_new_str(column, value, ZCS_PREDICATE_EQ,
                                 case_sensitive);
}

struct zcs_predicate *zcs_predicate_new_str_lt(size_t column, const char *value,
                                               bool case_sensitive)
{
    return zcs_predicate_new_str(column, value, ZCS_PREDICATE_LT,
                                 case_sensitive);
}

struct zcs_predicate *zcs_predicate_new_str_gt(size_t column, const char *value,
                                               bool case_sensitive)
{
    return zcs_predicate_new_str(column, value, ZCS_PREDICATE_GT,
                                 case_sensitive);
}

struct zcs_predicate *zcs_predicate_new_str_contains(
    size_t column, const char *value, bool case_sensitive,
    enum zcs_str_location location)
{
    struct zcs_predicate *predicate = zcs_predicate_new_str(
        column, value, ZCS_PREDICATE_CONTAINS, case_sensitive);
    if (!predicate)
        return NULL;
    predicate->location = location;
    return predicate;
}

static struct zcs_predicate *zcs_predicate_new_operator(
    enum zcs_predicate_type type, size_t count, va_list operands)
{
    if (!count)
        return NULL;
    struct zcs_predicate *predicate = zcs_predicate_new();
    if (!predicate)
        return NULL;
    predicate->operand_count = count;
    predicate->type = type;
    predicate->operands = calloc(count, sizeof(struct zcs_predicate *));
    if (!predicate->operands)
        goto error;
    for (size_t i = 0; i < count; i++) {
        predicate->operands[i] = va_arg(operands, struct zcs_predicate *);
        if (!predicate->operands[i])
            goto error;
    }
    return predicate;
error:
    zcs_predicate_free(predicate);
    return NULL;
}

static struct zcs_predicate *zcs_predicate_new_operator_array(
    enum zcs_predicate_type type, size_t count, struct zcs_predicate **operands)
{
    if (!count)
        return NULL;
    struct zcs_predicate *predicate = zcs_predicate_new();
    if (!predicate)
        return NULL;
    predicate->operand_count = count;
    predicate->type = type;
    predicate->operands = calloc(count, sizeof(struct zcs_predicate *));
    if (!predicate->operands)
        goto error;
    for (size_t i = 0; i < count; i++) {
        predicate->operands[i] = operands[i];
        if (!predicate->operands[i])
            goto error;
    }
    return predicate;
error:
    zcs_predicate_free(predicate);
    return NULL;
}

struct zcs_predicate *zcs_predicate_new_and(size_t count, ...)
{
    va_list operands;
    va_start(operands, count);
    struct zcs_predicate *predicate = zcs_predicate_new_vand(count, operands);
    va_end(operands);
    return predicate;
}

struct zcs_predicate *zcs_predicate_new_vand(size_t count, va_list operands)
{
    return zcs_predicate_new_operator(ZCS_PREDICATE_AND, count, operands);
}

struct zcs_predicate *zcs_predicate_new_aand(size_t count,
                                             struct zcs_predicate **operands)
{
    return zcs_predicate_new_operator_array(ZCS_PREDICATE_AND, count, operands);
}

struct zcs_predicate *zcs_predicate_new_or(size_t count, ...)
{
    va_list operands;
    va_start(operands, count);
    struct zcs_predicate *predicate = zcs_predicate_new_vor(count, operands);
    va_end(operands);
    return predicate;
}

struct zcs_predicate *zcs_predicate_new_vor(size_t count, va_list operands)
{
    return zcs_predicate_new_operator(ZCS_PREDICATE_OR, count, operands);
}

struct zcs_predicate *zcs_predicate_new_aor(size_t count,
                                            struct zcs_predicate **operands)
{
    return zcs_predicate_new_operator_array(ZCS_PREDICATE_OR, count, operands);
}

struct zcs_predicate *zcs_predicate_negate(struct zcs_predicate *predicate)
{
    if (predicate)
        predicate->negate ^= 1;
    return predicate;
}

static bool zcs_predicate_is_operator(const struct zcs_predicate *predicate)
{
    return predicate->type == ZCS_PREDICATE_AND ||
           predicate->type == ZCS_PREDICATE_OR;
}

bool zcs_predicate_valid(const struct zcs_predicate *predicate,
                         const struct zcs_row_group *row_group)
{
    if (predicate->column >= zcs_row_group_column_count(row_group))
        return false;

    enum zcs_column_type column_type =
        zcs_row_group_column_type(row_group, predicate->column);
    if (!zcs_predicate_is_operator(predicate) &&
        predicate->type != ZCS_PREDICATE_TRUE &&
        predicate->type != ZCS_PREDICATE_NULL &&
        predicate->column_type != column_type)
        return false;

    switch (predicate->type) {
        case ZCS_PREDICATE_TRUE:
        case ZCS_PREDICATE_NULL:
        case ZCS_PREDICATE_EQ:
            break;
        case ZCS_PREDICATE_LT:
        case ZCS_PREDICATE_GT:
            return column_type != ZCS_COLUMN_BIT;
        case ZCS_PREDICATE_CONTAINS:
            return column_type == ZCS_COLUMN_STR;
        case ZCS_PREDICATE_AND:
        case ZCS_PREDICATE_OR:
            for (size_t i = 0; i < predicate->operand_count; i++)
                if (!zcs_predicate_valid(predicate->operands[i], row_group))
                    return false;
            break;
    }

    return true;
}

static inline uint64_t zcs_mask_cap(uint64_t mask, size_t count)
{
    assert(count <= 64);
    if (count < 64)
        mask &= ((uint64_t)1 << count) - 1;
    return mask;
}

static bool zcs_predicate_match_rows_eq(const struct zcs_predicate *predicate,
                                        struct zcs_row_group_cursor *cursor,
                                        enum zcs_column_type type,
                                        uint64_t *matches, size_t *count)
{
    uint64_t mask = 0;
    switch (type) {
        case ZCS_COLUMN_BIT: {
            assert(predicate->column_type == ZCS_COLUMN_BIT);
            const uint64_t *values = zcs_row_group_cursor_batch_bit(
                cursor, predicate->column, count);
            if (!values)
                goto error;
            if (*count) {
                if (predicate->value.bit)
                    mask = *values;
                else
                    mask = zcs_mask_cap(~*values, *count);
            }
        } break;
        case ZCS_COLUMN_I32: {
            assert(predicate->column_type == ZCS_COLUMN_I32);
            const int32_t *values = zcs_row_group_cursor_batch_i32(
                cursor, predicate->column, count);
            if (!values)
                goto error;
            mask = zcs_match_i32_eq(*count, values, predicate->value.i32);
        } break;
        case ZCS_COLUMN_I64: {
            assert(predicate->column_type == ZCS_COLUMN_I64);
            const int64_t *values = zcs_row_group_cursor_batch_i64(
                cursor, predicate->column, count);
            if (!values)
                goto error;
            mask = zcs_match_i64_eq(*count, values, predicate->value.i64);
        } break;
        case ZCS_COLUMN_STR: {
            assert(predicate->column_type == ZCS_COLUMN_STR);
            const struct zcs_string *values = zcs_row_group_cursor_batch_str(
                cursor, predicate->column, count);
            if (!values)
                goto error;
            mask = zcs_match_str_eq(*count, values, &predicate->value.str,
                                    predicate->case_sensitive);
        } break;
    }
    *matches = mask;
    return true;
error:
    return false;
}

static enum zcs_predicate_match zcs_predicate_match_index_eq(
    const struct zcs_predicate *predicate, enum zcs_column_type type,
    const struct zcs_column_index *index)
{
    enum zcs_predicate_match result = ZCS_PREDICATE_MATCH_UNKNOWN;
    switch (type) {
        case ZCS_COLUMN_BIT:
            assert(predicate->column_type == ZCS_COLUMN_BIT);
            if (index->min.bit && index->max.bit)
                result = predicate->value.bit ? ZCS_PREDICATE_MATCH_ALL_ROWS
                                              : ZCS_PREDICATE_MATCH_NO_ROWS;
            else if (!index->min.bit && !index->max.bit)
                result = predicate->value.bit ? ZCS_PREDICATE_MATCH_NO_ROWS
                                              : ZCS_PREDICATE_MATCH_ALL_ROWS;
            break;
        case ZCS_COLUMN_I32:
            assert(predicate->column_type == ZCS_COLUMN_I32);
            if (index->min.i32 > predicate->value.i32 ||
                index->max.i32 < predicate->value.i32)
                result = ZCS_PREDICATE_MATCH_NO_ROWS;
            else if (index->min.i32 == predicate->value.i32 &&
                     index->max.i32 == predicate->value.i32)
                result = ZCS_PREDICATE_MATCH_ALL_ROWS;
            break;
        case ZCS_COLUMN_I64:
            assert(predicate->column_type == ZCS_COLUMN_I64);
            if (index->min.i64 > predicate->value.i64 ||
                index->max.i64 < predicate->value.i64)
                result = ZCS_PREDICATE_MATCH_NO_ROWS;
            else if (index->min.i64 == predicate->value.i64 &&
                     index->max.i64 == predicate->value.i64)
                result = ZCS_PREDICATE_MATCH_ALL_ROWS;
            break;
        case ZCS_COLUMN_STR:
            assert(predicate->column_type == ZCS_COLUMN_STR);
            if (index->min.len > predicate->value.str.len ||
                index->max.len < predicate->value.str.len)
                result = ZCS_PREDICATE_MATCH_NO_ROWS;
            break;
    }
    return result;
}

static bool zcs_predicate_match_rows_lt(const struct zcs_predicate *predicate,
                                        struct zcs_row_group_cursor *cursor,
                                        enum zcs_column_type type,
                                        uint64_t *matches, size_t *count)
{
    uint64_t mask = 0;
    switch (type) {
        case ZCS_COLUMN_BIT:
            goto error;  // unsupported
        case ZCS_COLUMN_I32: {
            assert(predicate->column_type == ZCS_COLUMN_I32);
            const int32_t *values = zcs_row_group_cursor_batch_i32(
                cursor, predicate->column, count);
            if (!values)
                goto error;
            mask = zcs_match_i32_lt(*count, values, predicate->value.i32);
        } break;
        case ZCS_COLUMN_I64: {
            assert(predicate->column_type == ZCS_COLUMN_I64);
            const int64_t *values = zcs_row_group_cursor_batch_i64(
                cursor, predicate->column, count);
            if (!values)
                goto error;
            mask = zcs_match_i64_lt(*count, values, predicate->value.i64);
        } break;
        case ZCS_COLUMN_STR: {
            assert(predicate->column_type == ZCS_COLUMN_STR);
            const struct zcs_string *values = zcs_row_group_cursor_batch_str(
                cursor, predicate->column, count);
            if (!values)
                goto error;
            mask = zcs_match_str_lt(*count, values, &predicate->value.str,
                                    predicate->case_sensitive);
        } break;
    }
    *matches = mask;
    return true;
error:
    return false;
}

static enum zcs_predicate_match zcs_predicate_match_index_lt(
    const struct zcs_predicate *predicate, enum zcs_column_type type,
    const struct zcs_column_index *index)
{
    enum zcs_predicate_match result = ZCS_PREDICATE_MATCH_UNKNOWN;
    switch (type) {
        case ZCS_COLUMN_BIT:
            break;
        case ZCS_COLUMN_I32:
            assert(predicate->column_type == ZCS_COLUMN_I32);
            if (index->min.i32 >= predicate->value.i32)
                result = ZCS_PREDICATE_MATCH_NO_ROWS;
            else if (index->max.i32 < predicate->value.i32)
                result = ZCS_PREDICATE_MATCH_ALL_ROWS;
            break;
        case ZCS_COLUMN_I64:
            assert(predicate->column_type == ZCS_COLUMN_I64);
            if (index->min.i64 >= predicate->value.i64)
                result = ZCS_PREDICATE_MATCH_NO_ROWS;
            else if (index->max.i64 < predicate->value.i64)
                result = ZCS_PREDICATE_MATCH_ALL_ROWS;
            break;
        case ZCS_COLUMN_STR:
            break;
    }
    return result;
}

static bool zcs_predicate_match_rows_gt(const struct zcs_predicate *predicate,
                                        struct zcs_row_group_cursor *cursor,
                                        enum zcs_column_type type,
                                        uint64_t *matches, size_t *count)
{
    uint64_t mask = 0;
    switch (type) {
        case ZCS_COLUMN_BIT:
            goto error;  // unsupported
        case ZCS_COLUMN_I32: {
            assert(predicate->column_type == ZCS_COLUMN_I32);
            const int32_t *values = zcs_row_group_cursor_batch_i32(
                cursor, predicate->column, count);
            if (!values)
                goto error;
            mask = zcs_match_i32_gt(*count, values, predicate->value.i32);
        } break;
        case ZCS_COLUMN_I64: {
            assert(predicate->column_type == ZCS_COLUMN_I64);
            const int64_t *values = zcs_row_group_cursor_batch_i64(
                cursor, predicate->column, count);
            if (!values)
                goto error;
            mask = zcs_match_i64_gt(*count, values, predicate->value.i64);
        } break;
        case ZCS_COLUMN_STR: {
            assert(predicate->column_type == ZCS_COLUMN_STR);
            const struct zcs_string *values = zcs_row_group_cursor_batch_str(
                cursor, predicate->column, count);
            if (!values)
                goto error;
            mask = zcs_match_str_gt(*count, values, &predicate->value.str,
                                    predicate->case_sensitive);
        } break;
    }
    *matches = mask;
    return true;
error:
    return false;
}

static enum zcs_predicate_match zcs_predicate_match_index_gt(
    const struct zcs_predicate *predicate, enum zcs_column_type type,
    const struct zcs_column_index *index)
{
    enum zcs_predicate_match result = ZCS_PREDICATE_MATCH_UNKNOWN;
    switch (type) {
        case ZCS_COLUMN_BIT:
            break;
        case ZCS_COLUMN_I32:
            assert(predicate->column_type == ZCS_COLUMN_I32);
            if (index->min.i32 > predicate->value.i32)
                result = ZCS_PREDICATE_MATCH_ALL_ROWS;
            else if (index->max.i32 <= predicate->value.i32)
                result = ZCS_PREDICATE_MATCH_NO_ROWS;
            break;
        case ZCS_COLUMN_I64:
            assert(predicate->column_type == ZCS_COLUMN_I64);
            if (index->min.i64 > predicate->value.i64)
                result = ZCS_PREDICATE_MATCH_ALL_ROWS;
            else if (index->max.i64 <= predicate->value.i64)
                result = ZCS_PREDICATE_MATCH_NO_ROWS;
            break;
        case ZCS_COLUMN_STR:
            break;
    }
    return result;
}

bool zcs_predicate_match_rows(const struct zcs_predicate *predicate,
                              const struct zcs_row_group *row_group,
                              struct zcs_row_group_cursor *cursor,
                              uint64_t *matches, size_t *count)
{
    enum zcs_column_type column_type =
        zcs_row_group_column_type(row_group, predicate->column);
    uint64_t mask = 0;
    switch (predicate->type) {
        case ZCS_PREDICATE_TRUE:
            *count = zcs_row_group_cursor_batch_count(cursor);
            mask = zcs_mask_cap(zcs_full_mask, *count);
            break;
        case ZCS_PREDICATE_NULL: {
            const uint64_t *nulls = zcs_row_group_cursor_batch_nulls(
                cursor, predicate->column, count);
            if (!nulls)
                goto error;
            mask = *count ? *nulls : 0;
        } break;
        case ZCS_PREDICATE_EQ:
            if (!zcs_predicate_match_rows_eq(predicate, cursor, column_type,
                                             &mask, count))
                goto error;
            break;
        case ZCS_PREDICATE_LT:
            if (!zcs_predicate_match_rows_lt(predicate, cursor, column_type,
                                             &mask, count))
                goto error;
            break;
        case ZCS_PREDICATE_GT:
            if (!zcs_predicate_match_rows_gt(predicate, cursor, column_type,
                                             &mask, count))
                goto error;
            break;
        case ZCS_PREDICATE_CONTAINS:
            assert(column_type == ZCS_COLUMN_STR);
            {
                const struct zcs_string *values =
                    zcs_row_group_cursor_batch_str(cursor, predicate->column,
                                                   count);
                if (!values)
                    goto error;
                mask = zcs_match_str_contains(
                    *count, values, &predicate->value.str,
                    predicate->case_sensitive, predicate->location);
            }
            break;
        case ZCS_PREDICATE_AND:
            mask = zcs_full_mask;
            // short-circuit the remaining predicates once the mask is empty
            for (size_t i = 0; mask && i < predicate->operand_count; i++) {
                uint64_t operand_mask;
                if (!zcs_predicate_match_rows(predicate->operands[i], row_group,
                                              cursor, &operand_mask, count))
                    goto error;
                mask &= operand_mask;
            }
            break;
        case ZCS_PREDICATE_OR:
            mask = 0;
            // short-circuit the remaining predicates once the mask is full
            for (size_t i = 0;
                 mask != zcs_full_mask && i < predicate->operand_count; i++) {
                uint64_t operand_mask;
                if (!zcs_predicate_match_rows(predicate->operands[i], row_group,
                                              cursor, &operand_mask, count))
                    goto error;
                mask |= operand_mask;
            }
            break;
    }
    *matches = predicate->negate ? zcs_mask_cap(~mask, *count) : mask;
    return true;
error:
    return false;
}

enum zcs_predicate_match zcs_predicate_match_indexes(
    const struct zcs_predicate *predicate,
    const struct zcs_row_group *row_group)
{
    if (!zcs_row_group_row_count(row_group))
        return ZCS_PREDICATE_MATCH_NO_ROWS;

    const struct zcs_column_index *index =
        zcs_row_group_column_index(row_group, predicate->column);
    enum zcs_column_type type =
        zcs_row_group_column_type(row_group, predicate->column);
    enum zcs_predicate_match result = ZCS_PREDICATE_MATCH_UNKNOWN;
    switch (predicate->type) {
        case ZCS_PREDICATE_TRUE:
            result = ZCS_PREDICATE_MATCH_ALL_ROWS;
            break;
        case ZCS_PREDICATE_NULL: {
            const struct zcs_column_index *index =
                zcs_row_group_null_index(row_group, predicate->column);
            if (index->min.bit && index->max.bit)
                result = ZCS_PREDICATE_MATCH_ALL_ROWS;
            else if (!index->min.bit && !index->max.bit)
                result = ZCS_PREDICATE_MATCH_NO_ROWS;
        } break;
        case ZCS_PREDICATE_EQ:
            result = zcs_predicate_match_index_eq(predicate, type, index);
            break;
        case ZCS_PREDICATE_LT:
            result = zcs_predicate_match_index_lt(predicate, type, index);
            break;
        case ZCS_PREDICATE_GT:
            result = zcs_predicate_match_index_gt(predicate, type, index);
            break;
        case ZCS_PREDICATE_CONTAINS:
            if (index->max.len < predicate->value.str.len)
                result = ZCS_PREDICATE_MATCH_NO_ROWS;
            break;
        case ZCS_PREDICATE_AND:
            result = ZCS_PREDICATE_MATCH_ALL_ROWS;
            for (size_t i = 0; result != ZCS_PREDICATE_MATCH_NO_ROWS &&
                               i < predicate->operand_count;
                 i++) {
                enum zcs_predicate_match operand_match =
                    zcs_predicate_match_indexes(predicate->operands[i],
                                                row_group);
                if (operand_match < result)
                    result = operand_match;
            }
            break;
        case ZCS_PREDICATE_OR:
            result = ZCS_PREDICATE_MATCH_NO_ROWS;
            for (size_t i = 0; result != ZCS_PREDICATE_MATCH_ALL_ROWS &&
                               i < predicate->operand_count;
                 i++) {
                enum zcs_predicate_match operand_match =
                    zcs_predicate_match_indexes(predicate->operands[i],
                                                row_group);
                if (operand_match > result)
                    result = operand_match;
            }
            break;
    }
    return predicate->negate ? -result : result;
}

static int zcs_column_cost(enum zcs_column_type type)
{
    int cost = 0;
    switch (type) {
        case ZCS_COLUMN_BIT:
            cost = 1;
            break;
        case ZCS_COLUMN_I32:
            cost = 8;
            break;
        case ZCS_COLUMN_I64:
            cost = 16;
            break;
        case ZCS_COLUMN_STR:
            cost = 128;
            break;
    }
    return cost;
}

static int zcs_predicate_cost(const struct zcs_predicate *predicate,
                              const struct zcs_row_group *row_group)
{
    int cost = 0;
    switch (predicate->type) {
        case ZCS_PREDICATE_TRUE:
            break;
        case ZCS_PREDICATE_NULL:
            cost = zcs_column_cost(ZCS_COLUMN_BIT);
            break;
        case ZCS_PREDICATE_EQ:
        case ZCS_PREDICATE_LT:
        case ZCS_PREDICATE_GT:
        case ZCS_PREDICATE_CONTAINS:
            cost = zcs_column_cost(
                zcs_row_group_column_type(row_group, predicate->column));
            break;
        case ZCS_PREDICATE_AND:
        case ZCS_PREDICATE_OR:
            for (size_t i = 0; i < predicate->operand_count; i++)
                cost += zcs_predicate_cost(predicate->operands[i], row_group);
            break;
    }
    return cost;
}

// qsort_r() differs on OS X and Linux..
#ifdef __APPLE__
static int zcs_predicate_cmp(void *ctx, const void *a, const void *b)
#else
static int zcs_predicate_cmp(const void *a, const void *b, void *ctx)
#endif
{
    const struct zcs_row_group *row_group = (const struct zcs_row_group *)ctx;
    // sort by cost asc
    return zcs_predicate_cost(*(struct zcs_predicate **)a, row_group) -
           zcs_predicate_cost(*(struct zcs_predicate **)b, row_group);
}

void zcs_predicate_optimize(struct zcs_predicate *predicate,
                            const struct zcs_row_group *row_group)
{
#ifdef __APPLE__
    qsort_r(predicate->operands, predicate->operand_count,
            sizeof(struct zcs_predicate *), (void *)row_group,
            zcs_predicate_cmp);
#else
    qsort_r(predicate->operands, predicate->operand_count,
            sizeof(struct zcs_predicate *), zcs_predicate_cmp,
            (void *)row_group);
#endif
    if (zcs_predicate_is_operator(predicate)) {
        for (size_t i = 0; i < predicate->operand_count; i++)
            zcs_predicate_optimize(predicate->operands[i], row_group);
        return;
    }
}

const struct zcs_predicate **zcs_predicate_operands(
    const struct zcs_predicate *predicate, size_t *size)
{
    // this is necessary to test zcs_predicate_optimize() operand reordering
    // the only other way would be to make the zcs_predicate struct public
    *size = predicate->operand_count;
    return (const struct zcs_predicate **)predicate->operands;
}
