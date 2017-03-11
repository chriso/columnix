#ifndef CX_ROW_
#define CX_ROW_

#include "predicate.h"

struct cx_row_cursor;

struct cx_row_cursor *cx_row_cursor_new(struct cx_row_group *,
                                        const struct cx_predicate *);

void cx_row_cursor_free(struct cx_row_cursor *);

void cx_row_cursor_rewind(struct cx_row_cursor *);

bool cx_row_cursor_next(struct cx_row_cursor *);

bool cx_row_cursor_error(const struct cx_row_cursor *);

size_t cx_row_cursor_count(struct cx_row_cursor *);

bool cx_row_cursor_get_null(const struct cx_row_cursor *, size_t column_index,
                            bool *value);
bool cx_row_cursor_get_bit(const struct cx_row_cursor *, size_t column_index,
                           bool *value);
bool cx_row_cursor_get_i32(const struct cx_row_cursor *, size_t column_index,
                           int32_t *value);
bool cx_row_cursor_get_i64(const struct cx_row_cursor *, size_t column_index,
                           int64_t *value);
bool cx_row_cursor_get_str(const struct cx_row_cursor *, size_t column_index,
                           const struct cx_string **value);

#endif
