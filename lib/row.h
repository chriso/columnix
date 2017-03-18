#ifndef CX_ROW_H_
#define CX_ROW_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "predicate.h"

struct cx_row_cursor;

CX_EXPORT struct cx_row_cursor *cx_row_cursor_new(struct cx_row_group *,
                                                  const struct cx_predicate *);

CX_EXPORT void cx_row_cursor_free(struct cx_row_cursor *);

CX_EXPORT void cx_row_cursor_rewind(struct cx_row_cursor *);

CX_EXPORT bool cx_row_cursor_next(struct cx_row_cursor *);

CX_EXPORT bool cx_row_cursor_error(const struct cx_row_cursor *);

CX_EXPORT size_t cx_row_cursor_count(struct cx_row_cursor *);

CX_EXPORT bool cx_row_cursor_get_null(const struct cx_row_cursor *,
                                      size_t column_index, bool *value);
CX_EXPORT bool cx_row_cursor_get_bit(const struct cx_row_cursor *,
                                     size_t column_index, bool *value);
CX_EXPORT bool cx_row_cursor_get_i32(const struct cx_row_cursor *,
                                     size_t column_index, int32_t *value);
CX_EXPORT bool cx_row_cursor_get_i64(const struct cx_row_cursor *,
                                     size_t column_index, int64_t *value);
CX_EXPORT bool cx_row_cursor_get_flt(const struct cx_row_cursor *,
                                     size_t column_index, float *value);
CX_EXPORT bool cx_row_cursor_get_dbl(const struct cx_row_cursor *,
                                     size_t column_index, double *value);
CX_EXPORT bool cx_row_cursor_get_str(const struct cx_row_cursor *,
                                     size_t column_index,
                                     struct cx_string *value);

#ifdef __cplusplus
}
#endif

#endif
