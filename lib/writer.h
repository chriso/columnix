#ifndef CX_WRITER_H_
#define CX_WRITER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "row_group.h"

struct cx_writer;

CX_EXPORT struct cx_writer *cx_writer_new(const char *, size_t row_group_size);

CX_EXPORT void cx_writer_free(struct cx_writer *);

CX_EXPORT bool cx_writer_metadata(struct cx_writer *, const char *);

CX_EXPORT bool cx_writer_add_column(struct cx_writer *, const char *name,
                                    enum cx_column_type, enum cx_encoding_type,
                                    enum cx_compression_type, int level);

CX_EXPORT bool cx_writer_put_bit(struct cx_writer *, size_t, bool);
CX_EXPORT bool cx_writer_put_i32(struct cx_writer *, size_t, int32_t);
CX_EXPORT bool cx_writer_put_i64(struct cx_writer *, size_t, int64_t);
CX_EXPORT bool cx_writer_put_flt(struct cx_writer *, size_t, float);
CX_EXPORT bool cx_writer_put_dbl(struct cx_writer *, size_t, double);
CX_EXPORT bool cx_writer_put_str(struct cx_writer *, size_t, const char *);
CX_EXPORT bool cx_writer_put_null(struct cx_writer *, size_t);

CX_EXPORT bool cx_writer_finish(struct cx_writer *, bool sync);

struct cx_row_group_writer;

CX_EXPORT struct cx_row_group_writer *cx_row_group_writer_new(const char *);

CX_EXPORT void cx_row_group_writer_free(struct cx_row_group_writer *);

CX_EXPORT bool cx_row_group_writer_metadata(struct cx_row_group_writer *,
                                            const char *);

CX_EXPORT bool cx_row_group_writer_add_column(
    struct cx_row_group_writer *, const char *name, enum cx_column_type,
    enum cx_encoding_type, enum cx_compression_type, int level);

CX_EXPORT bool cx_row_group_writer_put(struct cx_row_group_writer *,
                                       struct cx_row_group *);

CX_EXPORT bool cx_row_group_writer_finish(struct cx_row_group_writer *,
                                          bool sync);

#ifdef __cplusplus
}
#endif

#endif
