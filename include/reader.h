#ifndef ZCS_READER_
#define ZCS_READER_

#include "predicate.h"
#include "row_group.h"

struct zcs_reader;

struct zcs_reader *zcs_reader_new(const char *);

struct zcs_reader *zcs_reader_new_matching(const char *,
                                           struct zcs_predicate *);

void zcs_reader_free(struct zcs_reader *);

void zcs_reader_rewind(struct zcs_reader *);

bool zcs_reader_next(struct zcs_reader *);

bool zcs_reader_error(const struct zcs_reader *);

size_t zcs_reader_count(struct zcs_reader *);

size_t zcs_reader_column_count(const struct zcs_reader *);

enum zcs_column_type zcs_reader_column_type(const struct zcs_reader *, size_t);

enum zcs_encoding_type zcs_reader_column_encoding(const struct zcs_reader *,
                                                  size_t);

enum zcs_compression_type zcs_reader_column_compression(
    const struct zcs_reader *, size_t);

bool zcs_reader_get_bit(const struct zcs_reader *, size_t column_index,
                        bool *value);
bool zcs_reader_get_i32(const struct zcs_reader *, size_t column_index,
                        int32_t *value);
bool zcs_reader_get_i64(const struct zcs_reader *, size_t column_index,
                        int64_t *value);
bool zcs_reader_get_str(const struct zcs_reader *, size_t column_index,
                        const struct zcs_string **value);

struct zcs_row_group_reader;

struct zcs_row_group_reader *zcs_row_group_reader_new(const char *);

size_t zcs_row_group_reader_column_count(const struct zcs_row_group_reader *);

size_t zcs_row_group_reader_row_group_count(
    const struct zcs_row_group_reader *);

enum zcs_column_type zcs_row_group_reader_column_type(
    const struct zcs_row_group_reader *, size_t);

enum zcs_encoding_type zcs_row_group_reader_column_encoding(
    const struct zcs_row_group_reader *, size_t);

enum zcs_compression_type zcs_row_group_reader_column_compression(
    const struct zcs_row_group_reader *, size_t);

struct zcs_row_group *zcs_row_group_reader_get(
    const struct zcs_row_group_reader *, size_t);

void zcs_row_group_reader_free(struct zcs_row_group_reader *reader);

#endif
