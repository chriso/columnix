#ifndef CX_READER_
#define CX_READER_

#include <pthread.h>

#include "row.h"

struct cx_reader;

struct cx_reader *cx_reader_new(const char *);

struct cx_reader *cx_reader_new_matching(const char *, struct cx_predicate *);

void cx_reader_free(struct cx_reader *);

void cx_reader_rewind(struct cx_reader *);

bool cx_reader_next(struct cx_reader *);

bool cx_reader_error(const struct cx_reader *);

size_t cx_reader_column_count(const struct cx_reader *);

size_t cx_reader_row_count(struct cx_reader *);

bool cx_reader_query(struct cx_reader *, int thread_count, void *data,
                     void (*iter)(struct cx_row_cursor *, pthread_mutex_t *,
                                  void *));

const char *cx_reader_column_name(const struct cx_reader *, size_t);

enum cx_column_type cx_reader_column_type(const struct cx_reader *, size_t);

enum cx_encoding_type cx_reader_column_encoding(const struct cx_reader *,
                                                size_t);

enum cx_compression_type cx_reader_column_compression(const struct cx_reader *,
                                                      size_t);

bool cx_reader_get_null(const struct cx_reader *, size_t column_index,
                        bool *value);
bool cx_reader_get_bit(const struct cx_reader *, size_t column_index,
                       bool *value);
bool cx_reader_get_i32(const struct cx_reader *, size_t column_index,
                       int32_t *value);
bool cx_reader_get_i64(const struct cx_reader *, size_t column_index,
                       int64_t *value);
bool cx_reader_get_str(const struct cx_reader *, size_t column_index,
                       const struct cx_string **value);

struct cx_row_group_reader;

struct cx_row_group_reader *cx_row_group_reader_new(const char *);

size_t cx_row_group_reader_column_count(const struct cx_row_group_reader *);

size_t cx_row_group_reader_row_count(const struct cx_row_group_reader *);

size_t cx_row_group_reader_row_group_count(const struct cx_row_group_reader *);

const char *cx_row_group_reader_column_name(struct cx_row_group_reader *,
                                            size_t);

enum cx_column_type cx_row_group_reader_column_type(
    const struct cx_row_group_reader *, size_t);

enum cx_encoding_type cx_row_group_reader_column_encoding(
    const struct cx_row_group_reader *, size_t);

enum cx_compression_type cx_row_group_reader_column_compression(
    const struct cx_row_group_reader *, size_t);

struct cx_row_group *cx_row_group_reader_get(const struct cx_row_group_reader *,
                                             size_t);

void cx_row_group_reader_free(struct cx_row_group_reader *reader);

#endif
