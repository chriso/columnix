#ifndef CX_MATCH_H_
#define CX_MATCH_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "column.h"

uint64_t cx_match_i32_eq(size_t, const int32_t[], int32_t);
uint64_t cx_match_i32_lt(size_t, const int32_t[], int32_t);
uint64_t cx_match_i32_gt(size_t, const int32_t[], int32_t);

uint64_t cx_match_i64_eq(size_t, const int64_t[], int64_t);
uint64_t cx_match_i64_lt(size_t, const int64_t[], int64_t);
uint64_t cx_match_i64_gt(size_t, const int64_t[], int64_t);

uint64_t cx_match_flt_eq(size_t, const float[], float);
uint64_t cx_match_flt_lt(size_t, const float[], float);
uint64_t cx_match_flt_gt(size_t, const float[], float);

uint64_t cx_match_dbl_eq(size_t, const double[], double);
uint64_t cx_match_dbl_lt(size_t, const double[], double);
uint64_t cx_match_dbl_gt(size_t, const double[], double);

uint64_t cx_match_str_eq(size_t, const struct cx_string[],
                         const struct cx_string *, bool);
uint64_t cx_match_str_lt(size_t, const struct cx_string[],
                         const struct cx_string *, bool);
uint64_t cx_match_str_gt(size_t, const struct cx_string[],
                         const struct cx_string *, bool);
uint64_t cx_match_str_contains(size_t, const struct cx_string[],
                               const struct cx_string *, bool,
                               enum cx_str_location);

#ifdef __cplusplus
}
#endif

#endif
