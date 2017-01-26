#define _GNU_SOURCE
#include <assert.h>
#include <string.h>

#include "match.h"

#ifdef ZCS_AVX2
#include "avx2.h"
#endif

#ifdef ZCS_PCMPISTRM
#include <smmintrin.h>
#endif

#define ZCS_NAIVE_MATCH_DEFINITION(name, type, match, op) \
    static uint64_t zcs_match_##name##_##match##_naive(   \
        size_t size, const type batch[], type cmp)        \
    {                                                     \
        assert(size <= 64);                               \
        uint64_t mask = 0;                                \
        for (size_t i = 0; i < size; i++)                 \
            if (batch[i] op cmp)                          \
                mask |= (uint64_t)1 << i;                 \
        return mask;                                      \
    }

#define ZCS_AVX2_MATCH_DEFINITION(name, type, match)                          \
    static inline uint64_t zcs_match_##name##_##match##_avx2(                 \
        size_t size, const type batch[], type cmp)                            \
    {                                                                         \
        zcs_##name##_vec_t cmp_vec = zcs_avx2_set1_##name(cmp);               \
        uint32_t partial_mask[2 * sizeof(type)];                              \
        for (size_t i = 0; i < 2 * sizeof(type); i++) {                       \
            zcs_##name##_vec_t chunk =                                        \
                zcs_avx2_load_##name(&batch[i * (32 / sizeof(type))]);        \
            zcs_##name##_vec_t result =                                       \
                zcs_avx2_##match##_##name(cmp_vec, chunk);                    \
            partial_mask[i] = zcs_avx2_mask_##name(result);                   \
        }                                                                     \
        uint64_t mask = 0;                                                    \
        for (size_t i = 0; i < 2 * sizeof(type); i++)                         \
            mask |= ((uint64_t)partial_mask[i] << (i * (32 / sizeof(type)))); \
        return mask;                                                          \
    }

#if ZCS_AVX2

#define ZCS_AVX2_MATCH_SET(name, type)        \
    ZCS_AVX2_MATCH_DEFINITION(name, type, eq) \
    ZCS_AVX2_MATCH_DEFINITION(name, type, lt) \
    ZCS_AVX2_MATCH_DEFINITION(name, type, gt)

ZCS_AVX2_MATCH_SET(i32, int32_t)
ZCS_AVX2_MATCH_SET(i64, int64_t)
ZCS_AVX2_MATCH_SET(flt, float)
ZCS_AVX2_MATCH_SET(dbl, double)

#define ZCS_MATCH_DEFINITION(name, type, match)                          \
    uint64_t zcs_match_##name##_##match(size_t size, const type batch[], \
                                        type cmp)                        \
    {                                                                    \
        if (size == 64)                                                  \
            return zcs_match_##name##_##match##_avx2(size, batch, cmp);  \
        return zcs_match_##name##_##match##_naive(size, batch, cmp);     \
    }

#else

#define ZCS_MATCH_DEFINITION(name, type, match)                          \
    uint64_t zcs_match_##name##_##match(size_t size, const type batch[], \
                                        type cmp)                        \
    {                                                                    \
        return zcs_match_##name##_##match##_naive(size, batch, cmp);     \
    }

#endif  // if ZCS_AVX2

#define ZCS_MATCH_TYPE(name, type)                 \
    ZCS_NAIVE_MATCH_DEFINITION(name, type, eq, ==) \
    ZCS_MATCH_DEFINITION(name, type, eq)           \
    ZCS_NAIVE_MATCH_DEFINITION(name, type, lt, <)  \
    ZCS_MATCH_DEFINITION(name, type, lt)           \
    ZCS_NAIVE_MATCH_DEFINITION(name, type, gt, >)  \
    ZCS_MATCH_DEFINITION(name, type, gt)

ZCS_MATCH_TYPE(i32, int32_t)
ZCS_MATCH_TYPE(i64, int64_t)
ZCS_MATCH_TYPE(flt, float)
ZCS_MATCH_TYPE(dbl, double)

static inline bool zcs_str_eq(const struct zcs_string *a,
                              const struct zcs_string *b)
{
    if (a->len != b->len)
        return false;
#if ZCS_PCMPISTRM
    if (a->len < 16) {
        __m128i a_vec = _mm_loadu_si128((__m128i *)b->ptr);
        __m128i b_vec = _mm_loadu_si128((__m128i *)a->ptr);
        return _mm_cmpistro(a_vec, b_vec, _SIDD_UBYTE_OPS |
                                              _SIDD_CMP_EQUAL_EACH |
                                              _SIDD_BIT_MASK);
    }
#endif
    return !memcmp(a->ptr, b->ptr, a->len);
}

static inline bool zcs_str_eq_ci(const struct zcs_string *a,
                                 const struct zcs_string *b)
{
    return a->len == b->len && !strcasecmp(a->ptr, b->ptr);
}

static inline bool zcs_str_contains_any(const struct zcs_string *a,
                                        const struct zcs_string *b)
{
    if (a->len < b->len)
        return false;
#if ZCS_PCMPISTRM
    if (a->len < 16 && b->len < 16) {
        __m128i a_vec = _mm_loadu_si128((__m128i *)b->ptr);
        __m128i b_vec = _mm_loadu_si128((__m128i *)a->ptr);
        return _mm_cmpistrc(a_vec, b_vec, _SIDD_UBYTE_OPS |
                                              _SIDD_CMP_EQUAL_ORDERED |
                                              _SIDD_BIT_MASK);
    }
#endif
    return !!strstr(a->ptr, b->ptr);
}

static inline bool zcs_str_contains_any_ci(const struct zcs_string *a,
                                           const struct zcs_string *b)
{
    return a->len >= b->len && !!strcasestr(a->ptr, b->ptr);
}

static inline bool zcs_str_contains_start(const struct zcs_string *a,
                                          const struct zcs_string *b)
{
    if (a->len < b->len)
        return false;
#if ZCS_PCMPISTRM
    if (b->len < 16) {
        __m128i a_vec = _mm_loadu_si128((__m128i *)b->ptr);
        __m128i b_vec = _mm_loadu_si128((__m128i *)a->ptr);
        return _mm_cmpistro(a_vec, b_vec, _SIDD_UBYTE_OPS |
                                              _SIDD_CMP_EQUAL_ORDERED |
                                              _SIDD_BIT_MASK);
    }
#endif
    return !memcmp(a->ptr, b->ptr, b->len);
}

static inline bool zcs_str_contains_start_ci(const struct zcs_string *a,
                                             const struct zcs_string *b)
{
    return a->len >= b->len && !strncasecmp(a->ptr, b->ptr, b->len);
}

static inline bool zcs_str_contains_end(const struct zcs_string *a,
                                        const struct zcs_string *b)
{
    if (a->len < b->len)
        return false;
#if ZCS_PCMPISTRM
    if (b->len < 16) {
        __m128i a_vec = _mm_loadu_si128((__m128i *)b->ptr);
        __m128i b_vec = _mm_loadu_si128((__m128i *)(a->ptr + a->len - b->len));
        return _mm_cmpistro(a_vec, b_vec, _SIDD_UBYTE_OPS |
                                              _SIDD_CMP_EQUAL_ORDERED |
                                              _SIDD_BIT_MASK);
    }
#endif
    return !memcmp(a->ptr + a->len - b->len, b->ptr, b->len);
}

static inline bool zcs_str_contains_end_ci(const struct zcs_string *a,
                                           const struct zcs_string *b)
{
    return a->len >= b->len &&
           !strncasecmp(a->ptr + a->len - b->len, b->ptr, b->len);
}

static inline bool zcs_str_lt(const struct zcs_string *a,
                              const struct zcs_string *b)
{
    return strcmp(a->ptr, b->ptr) < 0;
}

static inline bool zcs_str_gt(const struct zcs_string *a,
                              const struct zcs_string *b)
{
    return strcmp(a->ptr, b->ptr) > 0;
}

static inline bool zcs_str_lt_ci(const struct zcs_string *a,
                                 const struct zcs_string *b)
{
    return strcasecmp(a->ptr, b->ptr) < 0;
}

static inline bool zcs_str_gt_ci(const struct zcs_string *a,
                                 const struct zcs_string *b)
{
    return strcasecmp(a->ptr, b->ptr) > 0;
}

#define ZCS_STR_MATCH(name, prefix)                        \
    prefix uint64_t zcs_match_str_##name(                  \
        size_t size, const struct zcs_string strings[],    \
        const struct zcs_string *cmp, bool case_sensitive) \
    {                                                      \
        assert(size <= 64);                                \
        uint64_t mask = 0;                                 \
        if (case_sensitive) {                              \
            for (size_t i = 0; i < size; i++)              \
                if (zcs_str_##name(&strings[i], cmp))      \
                    mask |= (uint64_t)1 << i;              \
        } else {                                           \
            for (size_t i = 0; i < size; i++)              \
                if (zcs_str_##name##_ci(&strings[i], cmp)) \
                    mask |= (uint64_t)1 << i;              \
        }                                                  \
        return mask;                                       \
    }

ZCS_STR_MATCH(eq, )
ZCS_STR_MATCH(lt, )
ZCS_STR_MATCH(gt, )

ZCS_STR_MATCH(contains_any, static)
ZCS_STR_MATCH(contains_start, static)
ZCS_STR_MATCH(contains_end, static)

uint64_t zcs_match_str_contains(size_t size, const struct zcs_string strings[],
                                const struct zcs_string *cmp,
                                bool case_sensitive,
                                enum zcs_str_location location)
{
    uint64_t matches = 0;
    switch (location) {
        case ZCS_STR_LOCATION_START:
            matches = zcs_match_str_contains_start(size, strings, cmp,
                                                   case_sensitive);
            break;
        case ZCS_STR_LOCATION_END:
            matches =
                zcs_match_str_contains_end(size, strings, cmp, case_sensitive);
            break;
        case ZCS_STR_LOCATION_ANY:
            matches =
                zcs_match_str_contains_any(size, strings, cmp, case_sensitive);
            break;
    }
    return matches;
}
