#ifndef CX_PREDICATE_H_
#define CX_PREDICATE_H_

#include <stdarg.h>

#include "match.h"
#include "row_group.h"

struct cx_predicate;

void cx_predicate_free(struct cx_predicate *);

struct cx_predicate *cx_predicate_new_true(void);

struct cx_predicate *cx_predicate_new_null(size_t);

struct cx_predicate *cx_predicate_negate(struct cx_predicate *);

struct cx_predicate *cx_predicate_new_bit_eq(size_t, bool);

struct cx_predicate *cx_predicate_new_i32_eq(size_t, int32_t);
struct cx_predicate *cx_predicate_new_i32_lt(size_t, int32_t);
struct cx_predicate *cx_predicate_new_i32_gt(size_t, int32_t);

struct cx_predicate *cx_predicate_new_i64_eq(size_t, int64_t);
struct cx_predicate *cx_predicate_new_i64_lt(size_t, int64_t);
struct cx_predicate *cx_predicate_new_i64_gt(size_t, int64_t);

struct cx_predicate *cx_predicate_new_str_eq(size_t, const char *, bool);
struct cx_predicate *cx_predicate_new_str_lt(size_t, const char *, bool);
struct cx_predicate *cx_predicate_new_str_gt(size_t, const char *, bool);
struct cx_predicate *cx_predicate_new_str_contains(size_t, const char *, bool,
                                                   enum cx_str_location);

struct cx_predicate *cx_predicate_new_and(size_t, ...);
struct cx_predicate *cx_predicate_new_vand(size_t, va_list);
struct cx_predicate *cx_predicate_new_aand(size_t, struct cx_predicate **);
struct cx_predicate *cx_predicate_new_or(size_t, ...);
struct cx_predicate *cx_predicate_new_vor(size_t, va_list);
struct cx_predicate *cx_predicate_new_aor(size_t, struct cx_predicate **);

bool cx_predicate_valid(const struct cx_predicate *,
                        const struct cx_row_group *);

void cx_predicate_optimize(struct cx_predicate *, const struct cx_row_group *);

const struct cx_predicate **cx_predicate_operands(const struct cx_predicate *,
                                                  size_t *);

enum cx_predicate_match {
    CX_PREDICATE_MATCH_NO_ROWS = -1,
    CX_PREDICATE_MATCH_UNKNOWN = 0,
    CX_PREDICATE_MATCH_ALL_ROWS = 1
};

enum cx_predicate_match cx_predicate_match_indexes(const struct cx_predicate *,
                                                   const struct cx_row_group *);

bool cx_predicate_match_rows(const struct cx_predicate *predicate,
                             const struct cx_row_group *row_group,
                             struct cx_row_group_cursor *cursor,
                             uint64_t *matches, size_t *count);

#endif
