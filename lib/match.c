#define _GNU_SOURCE
#include <assert.h>
#include <string.h>

#include "match.h"

#ifdef CX_AVX512
#include "avx512.h"
#define CX_SIMD_WIDTH 64
#elif defined(CX_AVX2)
#include "avx2.h"
#define CX_SIMD_WIDTH 32
#elif defined(CX_AVX)
#include "avx.h"
#define CX_SIMD_WIDTH 16
#endif

#ifdef CX_SSE42
#include <smmintrin.h>
#endif

#define CX_NAIVE_MATCH_DEFINITION(name, type, match, op) \
    static uint64_t cx_match_##name##_##match##_naive(   \
        size_t size, const type batch[], type cmp)       \
    {                                                    \
        assert(size <= 64);                              \
        uint64_t mask = 0;                               \
        for (size_t i = 0; i < size; i++)                \
            if (batch[i] op cmp)                         \
                mask |= (uint64_t)1 << i;                \
        return mask;                                     \
    }

#ifdef CX_SIMD_WIDTH

#define CX_SIMD_MATCH_DEFINITION(width, name, type, match)                   \
    static inline uint64_t cx_match_##name##_##match##_simd(                 \
        size_t size, const type batch[], type cmp)                           \
    {                                                                        \
        cx_##name##_vec_t v_cmp = cx_simd_##name##_set(cmp);                 \
        int partial_mask[64 * sizeof(type) / width];                         \
        for (size_t i = 0; i < 64 * sizeof(type) / width; i++) {             \
            cx_##name##_vec_t chunk =                                        \
                cx_simd_##name##_load(&batch[i * (width / sizeof(type))]);   \
            partial_mask[i] = cx_simd_##name##_##match(v_cmp, chunk);        \
        }                                                                    \
        uint64_t mask = 0;                                                   \
        for (size_t i = 0; i < 64 * sizeof(type) / width; i++)               \
            mask |=                                                          \
                ((uint64_t)partial_mask[i] << (i * (width / sizeof(type)))); \
        return mask;                                                         \
    }

#define CX_SIMD_MATCH_SET(name, type)                       \
    CX_SIMD_MATCH_DEFINITION(CX_SIMD_WIDTH, name, type, eq) \
    CX_SIMD_MATCH_DEFINITION(CX_SIMD_WIDTH, name, type, lt) \
    CX_SIMD_MATCH_DEFINITION(CX_SIMD_WIDTH, name, type, gt)

CX_SIMD_MATCH_SET(i32, int32_t)
CX_SIMD_MATCH_SET(i64, int64_t)
CX_SIMD_MATCH_SET(flt, float)
CX_SIMD_MATCH_SET(dbl, double)

#define CX_MATCH_DEFINITION(name, type, match)                          \
    uint64_t cx_match_##name##_##match(size_t size, const type batch[], \
                                       type cmp)                        \
    {                                                                   \
        if (size == 64)                                                 \
            return cx_match_##name##_##match##_simd(size, batch, cmp);  \
        return cx_match_##name##_##match##_naive(size, batch, cmp);     \
    }

#else

#define CX_MATCH_DEFINITION(name, type, match)                          \
    uint64_t cx_match_##name##_##match(size_t size, const type batch[], \
                                       type cmp)                        \
    {                                                                   \
        return cx_match_##name##_##match##_naive(size, batch, cmp);     \
    }

#endif  // simd

#define CX_MATCH_TYPE(name, type)                 \
    CX_NAIVE_MATCH_DEFINITION(name, type, eq, ==) \
    CX_MATCH_DEFINITION(name, type, eq)           \
    CX_NAIVE_MATCH_DEFINITION(name, type, lt, <)  \
    CX_MATCH_DEFINITION(name, type, lt)           \
    CX_NAIVE_MATCH_DEFINITION(name, type, gt, >)  \
    CX_MATCH_DEFINITION(name, type, gt)

CX_MATCH_TYPE(i32, int32_t)
CX_MATCH_TYPE(i64, int64_t)
CX_MATCH_TYPE(flt, float)
CX_MATCH_TYPE(dbl, double)

static inline bool cx_str_eq(const struct cx_string *str,
                             const struct cx_string *cmp)
{
    if (str->len != cmp->len)
        return false;
#if CX_SSE42
    if (str->len < 16) {
        __m128i v_str = _mm_loadu_si128((__m128i *)str->ptr);
        __m128i v_cmp = _mm_loadu_si128((__m128i *)cmp->ptr);
        return _mm_cmpistro(
            v_cmp, v_str,
            _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_EACH | _SIDD_BIT_MASK);
    }
#endif
    return !memcmp(str->ptr, cmp->ptr, str->len);
}

static inline bool cx_str_eq_ci(const struct cx_string *str,
                                const struct cx_string *cmp)
{
    return str->len == cmp->len && !strcasecmp(str->ptr, cmp->ptr);
}

static inline bool cx_str_contains_any(const struct cx_string *str,
                                       const struct cx_string *cmp)
{
    if (str->len < cmp->len)
        return false;
#if CX_SSE42
    if (str->len < 16 && cmp->len < 16) {
        __m128i v_str = _mm_loadu_si128((__m128i *)str->ptr);
        __m128i v_cmp = _mm_loadu_si128((__m128i *)cmp->ptr);
        return _mm_cmpistrc(
            v_cmp, v_str,
            _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_ORDERED | _SIDD_BIT_MASK);
    }
#endif
    return !!strstr(str->ptr, cmp->ptr);
}

static inline bool cx_str_contains_any_ci(const struct cx_string *str,
                                          const struct cx_string *cmp)
{
    return str->len >= cmp->len && !!strcasestr(str->ptr, cmp->ptr);
}

static inline bool cx_str_contains_start(const struct cx_string *str,
                                         const struct cx_string *cmp)
{
    if (str->len < cmp->len)
        return false;
#if CX_SSE42
    if (cmp->len < 16) {
        __m128i v_str = _mm_loadu_si128((__m128i *)str->ptr);
        __m128i v_cmp = _mm_loadu_si128((__m128i *)cmp->ptr);
        return _mm_cmpistro(
            v_cmp, v_str,
            _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_ORDERED | _SIDD_BIT_MASK);
    }
#endif
    return !memcmp(str->ptr, cmp->ptr, cmp->len);
}

static inline bool cx_str_contains_start_ci(const struct cx_string *str,
                                            const struct cx_string *cmp)
{
    return str->len >= cmp->len && !strncasecmp(str->ptr, cmp->ptr, cmp->len);
}

static inline bool cx_str_contains_end(const struct cx_string *str,
                                       const struct cx_string *cmp)
{
    if (str->len < cmp->len)
        return false;
#if CX_SSE42
    if (cmp->len < 16) {
        __m128i v_str =
            _mm_loadu_si128((__m128i *)(str->ptr + str->len - cmp->len));
        __m128i v_cmp = _mm_loadu_si128((__m128i *)cmp->ptr);
        return _mm_cmpistro(
            v_cmp, v_str,
            _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_ORDERED | _SIDD_BIT_MASK);
    }
#endif
    return !memcmp(str->ptr + str->len - cmp->len, cmp->ptr, cmp->len);
}

static inline bool cx_str_contains_end_ci(const struct cx_string *str,
                                          const struct cx_string *cmp)
{
    return str->len >= cmp->len &&
           !strncasecmp(str->ptr + str->len - cmp->len, cmp->ptr, cmp->len);
}

static inline bool cx_str_lt(const struct cx_string *str,
                             const struct cx_string *cmp)
{
    return strcmp(str->ptr, cmp->ptr) < 0;
}

static inline bool cx_str_gt(const struct cx_string *str,
                             const struct cx_string *cmp)
{
    return strcmp(str->ptr, cmp->ptr) > 0;
}

static inline bool cx_str_lt_ci(const struct cx_string *str,
                                const struct cx_string *cmp)
{
    return strcasecmp(str->ptr, cmp->ptr) < 0;
}

static inline bool cx_str_gt_ci(const struct cx_string *str,
                                const struct cx_string *cmp)
{
    return strcasecmp(str->ptr, cmp->ptr) > 0;
}

#define CX_STR_MATCH(name, prefix)                        \
    prefix uint64_t cx_match_str_##name(                  \
        size_t size, const struct cx_string strings[],    \
        const struct cx_string *cmp, bool case_sensitive) \
    {                                                     \
        assert(size <= 64);                               \
        uint64_t mask = 0;                                \
        if (case_sensitive) {                             \
            for (size_t i = 0; i < size; i++)             \
                if (cx_str_##name(&strings[i], cmp))      \
                    mask |= (uint64_t)1 << i;             \
        } else {                                          \
            for (size_t i = 0; i < size; i++)             \
                if (cx_str_##name##_ci(&strings[i], cmp)) \
                    mask |= (uint64_t)1 << i;             \
        }                                                 \
        return mask;                                      \
    }

CX_STR_MATCH(eq, )
CX_STR_MATCH(lt, )
CX_STR_MATCH(gt, )

CX_STR_MATCH(contains_any, static)
CX_STR_MATCH(contains_start, static)
CX_STR_MATCH(contains_end, static)

uint64_t cx_match_str_contains(size_t size, const struct cx_string strings[],
                               const struct cx_string *cmp, bool case_sensitive,
                               enum cx_str_location location)
{
    uint64_t matches = 0;
    switch (location) {
        case CX_STR_LOCATION_START:
            matches =
                cx_match_str_contains_start(size, strings, cmp, case_sensitive);
            break;
        case CX_STR_LOCATION_END:
            matches =
                cx_match_str_contains_end(size, strings, cmp, case_sensitive);
            break;
        case CX_STR_LOCATION_ANY:
            matches =
                cx_match_str_contains_any(size, strings, cmp, case_sensitive);
            break;
    }
    return matches;
}
