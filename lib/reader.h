#ifndef CX_READER_H_
#define CX_READER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>

#include "row.h"

struct cx_reader;

CX_EXPORT struct cx_reader *cx_reader_new(const char *);

CX_EXPORT struct cx_reader *cx_reader_new_matching(const char *,
                                                   struct cx_predicate *);

CX_EXPORT bool cx_reader_metadata(const struct cx_reader *, const char **);

CX_EXPORT void cx_reader_free(struct cx_reader *);

CX_EXPORT void cx_reader_rewind(struct cx_reader *);

CX_EXPORT bool cx_reader_next(struct cx_reader *);

CX_EXPORT bool cx_reader_error(const struct cx_reader *);

CX_EXPORT size_t cx_reader_column_count(const struct cx_reader *);

CX_EXPORT size_t cx_reader_row_count(struct cx_reader *);

CX_EXPORT bool cx_reader_query(struct cx_reader *, int thread_count, void *data,
                               void (*iter)(struct cx_row_cursor *,
                                            pthread_mutex_t *, void *));

CX_EXPORT const char *cx_reader_column_name(const struct cx_reader *, size_t);

CX_EXPORT enum cx_column_type cx_reader_column_type(const struct cx_reader *,
                                                    size_t);

CX_EXPORT enum cx_encoding_type cx_reader_column_encoding(
    const struct cx_reader *, size_t);

CX_EXPORT enum cx_compression_type cx_reader_column_compression(
    const struct cx_reader *, size_t, int *level);

CX_EXPORT bool cx_reader_get_null(const struct cx_reader *, size_t column_index,
                                  bool *value);
CX_EXPORT bool cx_reader_get_bit(const struct cx_reader *, size_t column_index,
                                 bool *value);
CX_EXPORT bool cx_reader_get_i32(const struct cx_reader *, size_t column_index,
                                 int32_t *value);
CX_EXPORT bool cx_reader_get_i64(const struct cx_reader *, size_t column_index,
                                 int64_t *value);
CX_EXPORT bool cx_reader_get_flt(const struct cx_reader *, size_t column_index,
                                 float *value);
CX_EXPORT bool cx_reader_get_dbl(const struct cx_reader *, size_t column_index,
                                 double *value);
CX_EXPORT bool cx_reader_get_str(const struct cx_reader *, size_t column_index,
                                 struct cx_string *value);

struct cx_row_group_reader;

struct cx_row_group_reader *cx_row_group_reader_new(const char *);

bool cx_row_group_reader_metadata(const struct cx_row_group_reader *,
                                  const char **);

size_t cx_row_group_reader_column_count(const struct cx_row_group_reader *);

size_t cx_row_group_reader_row_count(const struct cx_row_group_reader *);

size_t cx_row_group_reader_row_group_count(const struct cx_row_group_reader *);

const char *cx_row_group_reader_column_name(const struct cx_row_group_reader *,
                                            size_t);

enum cx_column_type cx_row_group_reader_column_type(
    const struct cx_row_group_reader *, size_t);

enum cx_encoding_type cx_row_group_reader_column_encoding(
    const struct cx_row_group_reader *, size_t);

enum cx_compression_type cx_row_group_reader_column_compression(
    const struct cx_row_group_reader *, size_t, int *level);

struct cx_row_group *cx_row_group_reader_get(const struct cx_row_group_reader *,
                                             size_t);

void cx_row_group_reader_free(struct cx_row_group_reader *reader);

#ifdef __cplusplus
}
#endif

#endif
