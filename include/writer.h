#ifndef ZCS_WRITER_
#define ZCS_WRITER_

#include "row_group.h"

struct zcs_writer;

struct zcs_writer *zcs_writer_new(const char *, size_t row_group_size);

void zcs_writer_free(struct zcs_writer *);

bool zcs_writer_add_column(struct zcs_writer *, const char *name,
                           enum zcs_column_type, enum zcs_encoding_type,
                           enum zcs_compression_type, int level);

bool zcs_writer_put_bit(struct zcs_writer *, size_t, bool);
bool zcs_writer_put_i32(struct zcs_writer *, size_t, int32_t);
bool zcs_writer_put_i64(struct zcs_writer *, size_t, int64_t);
bool zcs_writer_put_str(struct zcs_writer *, size_t, const char *);
bool zcs_writer_put_null(struct zcs_writer *, size_t);

bool zcs_writer_finish(struct zcs_writer *, bool sync);

struct zcs_row_group_writer;

struct zcs_row_group_writer *zcs_row_group_writer_new(const char *);

void zcs_row_group_writer_free(struct zcs_row_group_writer *);

bool zcs_row_group_writer_add_column(struct zcs_row_group_writer *,
                                     const char *name, enum zcs_column_type,
                                     enum zcs_encoding_type,
                                     enum zcs_compression_type, int level);

bool zcs_row_group_writer_put(struct zcs_row_group_writer *,
                              struct zcs_row_group *);

bool zcs_row_group_writer_finish(struct zcs_row_group_writer *, bool sync);

#endif
