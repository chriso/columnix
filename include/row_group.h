#ifndef CX_ROW_GROUP_
#define CX_ROW_GROUP_

#include "column.h"

struct cx_row_group;

struct cx_row_group_cursor;

struct cx_row_group *cx_row_group_new(void);

void cx_row_group_free(struct cx_row_group *);

bool cx_row_group_add_column(struct cx_row_group *, struct cx_column *column,
                             struct cx_column *nulls);

struct cx_lazy_column {
    enum cx_column_type type;
    enum cx_encoding_type encoding;
    enum cx_compression_type compression;
    const struct cx_index *index;
    const void *ptr;
    size_t size;
    size_t decompressed_size;
};

bool cx_row_group_add_lazy_column(struct cx_row_group *,
                                  const struct cx_lazy_column *column,
                                  const struct cx_lazy_column *nulls);

size_t cx_row_group_column_count(const struct cx_row_group *);

size_t cx_row_group_row_count(const struct cx_row_group *);

enum cx_column_type cx_row_group_column_type(const struct cx_row_group *,
                                             size_t);

enum cx_encoding_type cx_row_group_column_encoding(const struct cx_row_group *,
                                                   size_t);

const struct cx_index *cx_row_group_column_index(const struct cx_row_group *,
                                                 size_t);

const struct cx_column *cx_row_group_column(const struct cx_row_group *,
                                            size_t);

const struct cx_index *cx_row_group_null_index(const struct cx_row_group *,
                                               size_t);

const struct cx_column *cx_row_group_nulls(const struct cx_row_group *, size_t);

struct cx_row_group_cursor *cx_row_group_cursor_new(struct cx_row_group *);

void cx_row_group_cursor_free(struct cx_row_group_cursor *);

void cx_row_group_cursor_rewind(struct cx_row_group_cursor *);

bool cx_row_group_cursor_next(struct cx_row_group_cursor *);

size_t cx_row_group_cursor_batch_count(const struct cx_row_group_cursor *);

const uint64_t *cx_row_group_cursor_batch_nulls(struct cx_row_group_cursor *,
                                                size_t column_index,
                                                size_t *count);
const uint64_t *cx_row_group_cursor_batch_bit(struct cx_row_group_cursor *,
                                              size_t column_index,
                                              size_t *count);
const int32_t *cx_row_group_cursor_batch_i32(struct cx_row_group_cursor *,
                                             size_t column_index,
                                             size_t *count);
const int64_t *cx_row_group_cursor_batch_i64(struct cx_row_group_cursor *,
                                             size_t column_index,
                                             size_t *count);
const struct cx_string *cx_row_group_cursor_batch_str(
    struct cx_row_group_cursor *, size_t column_index, size_t *count);

#endif
