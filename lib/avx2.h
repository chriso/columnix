#include <immintrin.h>

typedef __m256i zcs_i32_vec_t;
typedef __m256i zcs_i64_vec_t;
typedef __m256 zcs_flt_vec_t;
typedef __m256d zcs_dbl_vec_t;

static inline __m256i zcs_avx2_set1_i32(int32_t value)
{
    return _mm256_set1_epi32(value);
}

static inline __m256i zcs_avx2_load_i32(const int32_t *ptr)
{
    return _mm256_loadu_si256((__m256i *)ptr);
}

static inline __m256i zcs_avx2_eq_i32(__m256i a, __m256i b)
{
    return _mm256_cmpeq_epi32(a, b);
}

static inline __m256i zcs_avx2_lt_i32(__m256i a, __m256i b)
{
    return _mm256_cmpgt_epi32(a, b);
}

static inline __m256i zcs_avx2_gt_i32(__m256i a, __m256i b)
{
    return _mm256_cmpgt_epi32(b, a);
}

static inline int zcs_avx2_mask_i32(__m256i vec)
{
    return _mm256_movemask_ps((__m256)vec);
}

static inline __m256i zcs_avx2_set1_i64(int64_t value)
{
    return _mm256_set1_epi64x(value);
}

static inline __m256i zcs_avx2_load_i64(const int64_t *ptr)
{
    return _mm256_loadu_si256((__m256i *)ptr);
}

static inline __m256i zcs_avx2_eq_i64(__m256i a, __m256i b)
{
    return _mm256_cmpeq_epi64(a, b);
}

static inline __m256i zcs_avx2_lt_i64(__m256i a, __m256i b)
{
    return _mm256_cmpgt_epi64(a, b);
}

static inline __m256i zcs_avx2_gt_i64(__m256i a, __m256i b)
{
    return _mm256_cmpgt_epi64(b, a);
}

static inline int zcs_avx2_mask_i64(__m256i vec)
{
    return _mm256_movemask_pd((__m256d)vec);
}

static inline __m256 zcs_avx2_set1_flt(float value)
{
    return _mm256_set1_ps(value);
}

static inline __m256 zcs_avx2_load_flt(const float *ptr)
{
    return _mm256_loadu_ps(ptr);
}

static inline __m256 zcs_avx2_eq_flt(__m256 a, __m256 b)
{
    return _mm256_cmp_ps(a, b, _CMP_EQ_OQ);
}

static inline __m256 zcs_avx2_lt_flt(__m256 a, __m256 b)
{
    return _mm256_cmp_ps(b, a, _CMP_LT_OQ);
}

static inline __m256 zcs_avx2_gt_flt(__m256 a, __m256 b)
{
    return _mm256_cmp_ps(b, a, _CMP_GT_OQ);
}

static inline int zcs_avx2_mask_flt(__m256 vec)
{
    return _mm256_movemask_ps(vec);
}

static inline __m256d zcs_avx2_set1_dbl(double value)
{
    return _mm256_set1_pd(value);
}

static inline __m256d zcs_avx2_load_dbl(const double *ptr)
{
    return _mm256_loadu_pd(ptr);
}

static inline __m256d zcs_avx2_eq_dbl(__m256d a, __m256d b)
{
    return _mm256_cmp_pd(a, b, _CMP_EQ_OQ);
}

static inline __m256d zcs_avx2_lt_dbl(__m256d a, __m256d b)
{
    return _mm256_cmp_pd(b, a, _CMP_LT_OQ);
}

static inline __m256d zcs_avx2_gt_dbl(__m256d a, __m256d b)
{
    return _mm256_cmp_pd(b, a, _CMP_GT_OQ);
}

static inline int zcs_avx2_mask_dbl(__m256d vec)
{
    return _mm256_movemask_pd(vec);
}
