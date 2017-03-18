#ifndef CX_WRITER_H_
#define CX_WRITER_H_

#include "row_group.h"

struct cx_writer;

struct cx_writer *cx_writer_new(const char *, size_t row_group_size);

void cx_writer_free(struct cx_writer *);

bool cx_writer_metadata(struct cx_writer *, const char *);

bool cx_writer_add_column(struct cx_writer *, const char *name,
                          enum cx_column_type, enum cx_encoding_type,
                          enum cx_compression_type, int level);

bool cx_writer_put_bit(struct cx_writer *, size_t, bool);
bool cx_writer_put_i32(struct cx_writer *, size_t, int32_t);
bool cx_writer_put_i64(struct cx_writer *, size_t, int64_t);
bool cx_writer_put_flt(struct cx_writer *, size_t, float);
bool cx_writer_put_dbl(struct cx_writer *, size_t, double);
bool cx_writer_put_str(struct cx_writer *, size_t, const char *);
bool cx_writer_put_null(struct cx_writer *, size_t);

bool cx_writer_finish(struct cx_writer *, bool sync);

struct cx_row_group_writer;

struct cx_row_group_writer *cx_row_group_writer_new(const char *);

void cx_row_group_writer_free(struct cx_row_group_writer *);

bool cx_row_group_writer_metadata(struct cx_row_group_writer *, const char *);

bool cx_row_group_writer_add_column(struct cx_row_group_writer *,
                                    const char *name, enum cx_column_type,
                                    enum cx_encoding_type,
                                    enum cx_compression_type, int level);

bool cx_row_group_writer_put(struct cx_row_group_writer *,
                             struct cx_row_group *);

bool cx_row_group_writer_finish(struct cx_row_group_writer *, bool sync);

#endif
