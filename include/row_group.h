#ifndef ZCS_ROW_GROUP_
#define ZCS_ROW_GROUP_

#include "column.h"

struct zcs_row_group;

struct zcs_row_group_cursor;

struct zcs_row_group *zcs_row_group_new(void);

void zcs_row_group_free(struct zcs_row_group *);

bool zcs_row_group_add_column(struct zcs_row_group *, struct zcs_column *column,
                              struct zcs_column *nulls);

struct zcs_lazy_column {
    enum zcs_column_type type;
    enum zcs_encoding_type encoding;
    enum zcs_compression_type compression;
    const struct zcs_column_index *index;
    const void *ptr;
    size_t size;
    size_t decompressed_size;
};

bool zcs_row_group_add_lazy_column(struct zcs_row_group *,
                                   const struct zcs_lazy_column *column,
                                   const struct zcs_lazy_column *nulls);

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

const struct zcs_column_index *zcs_row_group_null_index(
    const struct zcs_row_group *, size_t);

const struct zcs_column *zcs_row_group_nulls(const struct zcs_row_group *,
                                             size_t);

struct zcs_row_group_cursor *zcs_row_group_cursor_new(struct zcs_row_group *);

void zcs_row_group_cursor_free(struct zcs_row_group_cursor *);

void zcs_row_group_cursor_rewind(struct zcs_row_group_cursor *);

bool zcs_row_group_cursor_next(struct zcs_row_group_cursor *);

size_t zcs_row_group_cursor_batch_count(const struct zcs_row_group_cursor *);

const uint64_t *zcs_row_group_cursor_batch_nulls(struct zcs_row_group_cursor *,
                                                 size_t column_index,
                                                 size_t *count);
const uint64_t *zcs_row_group_cursor_batch_bit(struct zcs_row_group_cursor *,
                                               size_t column_index,
                                               size_t *count);
const int32_t *zcs_row_group_cursor_batch_i32(struct zcs_row_group_cursor *,
                                              size_t column_index,
                                              size_t *count);
const int64_t *zcs_row_group_cursor_batch_i64(struct zcs_row_group_cursor *,
                                              size_t column_index,
                                              size_t *count);
const struct zcs_string *zcs_row_group_cursor_batch_str(
    struct zcs_row_group_cursor *, size_t column_index, size_t *count);

#endif
