#ifndef ZCS_READER_
#define ZCS_READER_

#include "row_group.h"

struct zcs_reader *zcs_reader_new(const char *);

size_t zcs_reader_column_count(const struct zcs_reader *);

size_t zcs_reader_row_group_count(const struct zcs_reader *);

enum zcs_column_type zcs_reader_column_type(const struct zcs_reader *, size_t);

enum zcs_encoding_type zcs_reader_column_encoding(const struct zcs_reader *,
                                                  size_t);

enum zcs_compression_type zcs_reader_column_compression(
    const struct zcs_reader *, size_t);

struct zcs_row_group *zcs_reader_row_group(struct zcs_reader *, size_t);

void zcs_reader_free(struct zcs_reader *reader);

#endif
