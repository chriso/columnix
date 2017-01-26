#ifndef ZCS_MATCH_H_
#define ZCS_MATCH_H_

#include "column.h"

/**
 * Numeric matching.
 *
 * The following match functions are available for i32, i64, flt and dbl
 * numeric types:
 *
 *   - eq: equals
 *   - lt: less than
 *   - gt: greater than
 *
 * The functions take a batch of <= 64 numbers and return a uint64_t bitset
 * containing matches. If the first number in the batch (batch[0]) matches
 * then the least significant bit of the bitset would be set (1 << 0), etc.
 */
uint64_t zcs_match_i32_eq(size_t size, const int32_t batch[], int32_t cmp);
uint64_t zcs_match_i32_lt(size_t size, const int32_t batch[], int32_t cmp);
uint64_t zcs_match_i32_gt(size_t size, const int32_t batch[], int32_t cmp);

uint64_t zcs_match_i64_eq(size_t size, const int64_t batch[], int64_t cmp);
uint64_t zcs_match_i64_lt(size_t size, const int64_t batch[], int64_t cmp);
uint64_t zcs_match_i64_gt(size_t size, const int64_t batch[], int64_t cmp);

uint64_t zcs_match_flt_eq(size_t size, const float batch[], float cmp);
uint64_t zcs_match_flt_lt(size_t size, const float batch[], float cmp);
uint64_t zcs_match_flt_gt(size_t size, const float batch[], float cmp);

uint64_t zcs_match_dbl_eq(size_t size, const double batch[], double cmp);
uint64_t zcs_match_dbl_lt(size_t size, const double batch[], double cmp);
uint64_t zcs_match_dbl_gt(size_t size, const double batch[], double cmp);

/**
 * String matching.
 */
enum zcs_str_location {
    ZCS_STR_LOCATION_START,
    ZCS_STR_LOCATION_END,
    ZCS_STR_LOCATION_ANY
};

uint64_t zcs_match_str_eq(size_t size, const struct zcs_string strings[],
                          const struct zcs_string *cmp, bool case_sensitive);
uint64_t zcs_match_str_lt(size_t size, const struct zcs_string strings[],
                          const struct zcs_string *cmp, bool case_sensitive);
uint64_t zcs_match_str_gt(size_t size, const struct zcs_string strings[],
                          const struct zcs_string *cmp, bool case_sensitive);
uint64_t zcs_match_str_contains(size_t size, const struct zcs_string strings[],
                                const struct zcs_string *cmp,
                                bool case_sensitive, enum zcs_str_location);

#endif
