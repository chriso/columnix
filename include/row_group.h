#ifndef ZCS_ROW_GROUP_
#define ZCS_ROW_GROUP_

#include "column.h"

struct zcs_row_group;

struct zcs_row_group_cursor;

struct zcs_row_group *zcs_row_group_new(void);

void zcs_row_group_free(struct zcs_row_group *);

bool zcs_row_group_add_column(struct zcs_row_group *, struct zcs_column *);

bool zcs_row_group_add_column_lazy(struct zcs_row_group *, enum zcs_column_type,
                                   enum zcs_encoding_type,
                                   const struct zcs_column_index *,
                                   const void *, size_t);

size_t zcs_row_group_column_count(const struct zcs_row_group *);

size_t zcs_row_group_row_count(const struct zcs_row_group *);

enum zcs_column_type zcs_row_group_column_type(const struct zcs_row_group *,
                                               size_t);

enum zcs_encoding_type zcs_row_group_column_encoding(
    const struct zcs_row_group *, size_t);

const struct zcs_column_index *zcs_row_group_column_index(
    const struct zcs_row_group *, size_t);

const struct zcs_column *zcs_row_group_column(const struct zcs_row_group *,
                                              size_t);

struct zcs_row_group_cursor *zcs_row_group_cursor_new(struct zcs_row_group *);

void zcs_row_group_cursor_free(struct zcs_row_group_cursor *);

void zcs_row_group_cursor_rewind(struct zcs_row_group_cursor *);

bool zcs_row_group_cursor_next(struct zcs_row_group_cursor *);

size_t zcs_row_group_cursor_batch_count(const struct zcs_row_group_cursor *);

const uint64_t *zcs_row_group_cursor_next_bit(struct zcs_row_group_cursor *,
                                              size_t column_index,
                                              size_t *count);
const int32_t *zcs_row_group_cursor_next_i32(struct zcs_row_group_cursor *,
                                             size_t column_index,
                                             size_t *count);
const int64_t *zcs_row_group_cursor_next_i64(struct zcs_row_group_cursor *,
                                             size_t column_index,
                                             size_t *count);
const struct zcs_string *zcs_row_group_cursor_next_str(
    struct zcs_row_group_cursor *, size_t column_index, size_t *count);

#endif
