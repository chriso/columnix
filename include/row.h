#ifndef ZCS_ROW_
#define ZCS_ROW_

#include "predicate.h"

struct zcs_row_cursor;

struct zcs_row_cursor *zcs_row_cursor_new(struct zcs_row_group *);

struct zcs_row_cursor *zcs_row_cursor_new_matching(struct zcs_row_group *,
                                                   struct zcs_predicate *);

void zcs_row_cursor_free(struct zcs_row_cursor *);

void zcs_row_cursor_rewind(struct zcs_row_cursor *);

bool zcs_row_cursor_next(struct zcs_row_cursor *);

bool zcs_row_cursor_error(const struct zcs_row_cursor *);

size_t zcs_row_cursor_count(struct zcs_row_cursor *);

bool zcs_row_cursor_get_bit(const struct zcs_row_cursor *, size_t column_index,
                            bool *value);
bool zcs_row_cursor_get_i32(const struct zcs_row_cursor *, size_t column_index,
                            int32_t *value);
bool zcs_row_cursor_get_i64(const struct zcs_row_cursor *, size_t column_index,
                            int64_t *value);
const struct zcs_string *zcs_row_cursor_get_str(const struct zcs_row_cursor *,
                                                size_t column_index);

#endif
