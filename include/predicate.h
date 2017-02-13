#ifndef ZCS_PREDICATE_H_
#define ZCS_PREDICATE_H_

#include "match.h"
#include "row_group.h"

struct zcs_predicate;

void zcs_predicate_free(struct zcs_predicate *);

struct zcs_predicate *zcs_predicate_new_true(void);

struct zcs_predicate *zcs_predicate_negate(struct zcs_predicate *);

struct zcs_predicate *zcs_predicate_new_bit_eq(size_t, bool);

struct zcs_predicate *zcs_predicate_new_i32_eq(size_t, int32_t);
struct zcs_predicate *zcs_predicate_new_i32_lt(size_t, int32_t);
struct zcs_predicate *zcs_predicate_new_i32_gt(size_t, int32_t);

struct zcs_predicate *zcs_predicate_new_i64_eq(size_t, int64_t);
struct zcs_predicate *zcs_predicate_new_i64_lt(size_t, int64_t);
struct zcs_predicate *zcs_predicate_new_i64_gt(size_t, int64_t);

struct zcs_predicate *zcs_predicate_new_str_eq(size_t, const char *, bool);
struct zcs_predicate *zcs_predicate_new_str_lt(size_t, const char *, bool);
struct zcs_predicate *zcs_predicate_new_str_gt(size_t, const char *, bool);
struct zcs_predicate *zcs_predicate_new_str_contains(size_t, const char *, bool,
                                                     enum zcs_str_location);

struct zcs_predicate *zcs_predicate_new_and(size_t, ...);
struct zcs_predicate *zcs_predicate_new_or(size_t, ...);

bool zcs_predicate_valid(const struct zcs_predicate *,
                         const struct zcs_row_group *);

void zcs_predicate_optimize(struct zcs_predicate *,
                            const struct zcs_row_group *);

enum zcs_predicate_match {
    ZCS_PREDICATE_MATCH_NO_ROWS = -1,
    ZCS_PREDICATE_MATCH_UNKNOWN = 0,
    ZCS_PREDICATE_MATCH_ALL_ROWS = 1
};

enum zcs_predicate_match zcs_predicate_match_indexes(
    const struct zcs_predicate *, const struct zcs_row_group *);

bool zcs_predicate_match_rows(const struct zcs_predicate *predicate,
                              const struct zcs_row_group *row_group,
                              struct zcs_row_group_cursor *cursor,
                              uint64_t *matches, size_t *count);

#endif
