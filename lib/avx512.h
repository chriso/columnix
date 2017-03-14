#include <immintrin.h>

typedef __m512i cx_i32_vec_t;
typedef __m512i cx_i64_vec_t;
typedef __m512 cx_flt_vec_t;
typedef __m512d cx_dbl_vec_t;

static inline __m512i cx_simd_i32_set(int32_t value)
{
    return _mm512_set1_epi32(value);
}

static inline __m512i cx_simd_i32_load(const int32_t *ptr)
{
    return _mm512_loadu_si512((const void *)ptr);
}

static inline int cx_simd_i32_eq(__m512i a, __m512i b)
{
    return (int)_mm512_cmpeq_epi32_mask(a, b);
}

static inline int cx_simd_i32_lt(__m512i a, __m512i b)
{
    return (int)_mm512_cmplt_epi32_mask(a, b);
}

static inline int cx_simd_i32_gt(__m512i a, __m512i b)
{
    return (int)_mm512_cmpgt_epi32_mask(b, a);
}

static inline __m512i cx_simd_i64_set(int64_t value)
{
    return _mm512_set1_epi64(value);
}

static inline __m512i cx_simd_i64_load(const int64_t *ptr)
{
    return _mm512_loadu_si512((const void *)ptr);
}

static inline int cx_simd_i64_eq(__m512i a, __m512i b)
{
    return (int)_mm512_cmpeq_epi64_mask(a, b);
}

static inline int cx_simd_i64_lt(__m512i a, __m512i b)
{
    return (int)_mm512_cmplt_epi64_mask(a, b);
}

static inline int cx_simd_i64_gt(__m512i a, __m512i b)
{
    return (int)_mm512_cmpgt_epi64_mask(b, a);
}

static inline __m512 cx_simd_flt_set(float value)
{
    return _mm512_set1_ps(value);
}

static inline __m512 cx_simd_flt_load(const float *ptr)
{
    return _mm512_loadu_ps(ptr);
}

static inline int cx_simd_flt_eq(__m512 a, __m512 b)
{
    return (int)_mm512_cmp_ps_mask(a, b, _CMP_EQ_OQ);
}

static inline int cx_simd_flt_lt(__m512 a, __m512 b)
{
    return (int)_mm512_cmp_ps_mask(b, a, _CMP_LT_OQ);
}

static inline int cx_simd_flt_gt(__m512 a, __m512 b)
{
    return (int)_mm512_cmp_ps_mask(b, a, _CMP_GT_OQ);
}

static inline __m512d cx_simd_dbl_set(double value)
{
    return _mm512_set1_pd(value);
}

static inline __m512d cx_simd_dbl_load(const double *ptr)
{
    return _mm512_loadu_pd(ptr);
}

static inline int cx_simd_dbl_eq(__m512d a, __m512d b)
{
    return (int)_mm512_cmp_pd_mask(a, b, _CMP_EQ_OQ);
}

static inline int cx_simd_dbl_lt(__m512d a, __m512d b)
{
    return (int)_mm512_cmp_pd_mask(b, a, _CMP_LT_OQ);
}

static inline int cx_simd_dbl_gt(__m512d a, __m512d b)
{
    return (int)_mm512_cmp_pd_mask(b, a, _CMP_GT_OQ);
}
