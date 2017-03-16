#ifndef CX_INDEX_
#define CX_INDEX_

#include "column.h"

typedef union {
    bool bit;
    int32_t i32;
    int64_t i64;
    float flt;
    double dbl;
    uint64_t len;
} cx_index_value_t;

struct cx_index {
    uint64_t count;
    cx_index_value_t min;
    cx_index_value_t max;
};

struct cx_index *cx_index_new(const struct cx_column *);

void cx_index_free(struct cx_index *);

enum cx_index_match {
    CX_INDEX_MATCH_NONE = -1,
    CX_INDEX_MATCH_UNKNOWN = 0,
    CX_INDEX_MATCH_ALL = 1
};

enum cx_index_match cx_index_match_bit_eq(const struct cx_index *, bool);

enum cx_index_match cx_index_match_i32_eq(const struct cx_index *, int32_t);
enum cx_index_match cx_index_match_i32_lt(const struct cx_index *, int32_t);
enum cx_index_match cx_index_match_i32_gt(const struct cx_index *, int32_t);

enum cx_index_match cx_index_match_i64_eq(const struct cx_index *, int64_t);
enum cx_index_match cx_index_match_i64_lt(const struct cx_index *, int64_t);
enum cx_index_match cx_index_match_i64_gt(const struct cx_index *, int64_t);

enum cx_index_match cx_index_match_flt_eq(const struct cx_index *, float);
enum cx_index_match cx_index_match_flt_lt(const struct cx_index *, float);
enum cx_index_match cx_index_match_flt_gt(const struct cx_index *, float);

enum cx_index_match cx_index_match_dbl_eq(const struct cx_index *, double);
enum cx_index_match cx_index_match_dbl_lt(const struct cx_index *, double);
enum cx_index_match cx_index_match_dbl_gt(const struct cx_index *, double);

enum cx_index_match cx_index_match_str_eq(const struct cx_index *,
                                          const struct cx_string *);
enum cx_index_match cx_index_match_str_contains(const struct cx_index *,
                                                const struct cx_string *);

#endif
