#include <immintrin.h>

typedef __m128i cx_i32_vec_t;
typedef __m128i cx_i64_vec_t;
typedef __m128 cx_flt_vec_t;
typedef __m128d cx_dbl_vec_t;

static inline __m128i cx_simd_i32_set(int32_t value)
{
    return _mm_set1_epi32(value);
}

static inline __m128i cx_simd_i32_load(const int32_t *ptr)
{
    return _mm_loadu_si128((__m128i *)ptr);
}

static inline __m128i cx_simd_i32_eq(__m128i a, __m128i b)
{
    return _mm_cmpeq_epi32(a, b);
}

static inline __m128i cx_simd_i32_lt(__m128i a, __m128i b)
{
    return _mm_cmpgt_epi32(a, b);
}

static inline __m128i cx_simd_i32_gt(__m128i a, __m128i b)
{
    return _mm_cmpgt_epi32(b, a);
}

static inline int cx_simd_i32_mask(__m128i vec)
{
    return _mm_movemask_ps((__m128)vec);
}

static inline __m128i cx_simd_i64_set(int64_t value)
{
    return _mm_set1_epi64x(value);
}

static inline __m128i cx_simd_i64_load(const int64_t *ptr)
{
    return _mm_loadu_si128((__m128i *)ptr);
}

static inline __m128i cx_simd_i64_eq(__m128i a, __m128i b)
{
    return _mm_cmpeq_epi64(a, b);
}

static inline __m128i cx_simd_i64_lt(__m128i a, __m128i b)
{
    return _mm_cmpgt_epi64(a, b);
}

static inline __m128i cx_simd_i64_gt(__m128i a, __m128i b)
{
    return _mm_cmpgt_epi64(b, a);
}

static inline int cx_simd_i64_mask(__m128i vec)
{
    return _mm_movemask_pd((__m128d)vec);
}

static inline __m128 cx_simd_flt_set(float value)
{
    return _mm_set1_ps(value);
}

static inline __m128 cx_simd_flt_load(const float *ptr)
{
    return _mm_loadu_ps(ptr);
}

static inline __m128 cx_simd_flt_eq(__m128 a, __m128 b)
{
    return _mm_cmp_ps(a, b, _CMP_EQ_OQ);
}

static inline __m128 cx_simd_flt_lt(__m128 a, __m128 b)
{
    return _mm_cmp_ps(b, a, _CMP_LT_OQ);
}

static inline __m128 cx_simd_flt_gt(__m128 a, __m128 b)
{
    return _mm_cmp_ps(b, a, _CMP_GT_OQ);
}

static inline int cx_simd_flt_mask(__m128 vec)
{
    return _mm_movemask_ps(vec);
}

static inline __m128d cx_simd_dbl_set(double value)
{
    return _mm_set1_pd(value);
}

static inline __m128d cx_simd_dbl_load(const double *ptr)
{
    return _mm_loadu_pd(ptr);
}

static inline __m128d cx_simd_dbl_eq(__m128d a, __m128d b)
{
    return _mm_cmp_pd(a, b, _CMP_EQ_OQ);
}

static inline __m128d cx_simd_dbl_lt(__m128d a, __m128d b)
{
    return _mm_cmp_pd(b, a, _CMP_LT_OQ);
}

static inline __m128d cx_simd_dbl_gt(__m128d a, __m128d b)
{
    return _mm_cmp_pd(b, a, _CMP_GT_OQ);
}

static inline int cx_simd_dbl_mask(__m128d vec)
{
    return _mm_movemask_pd(vec);
}
