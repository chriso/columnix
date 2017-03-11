#include "match.h"

#include "helpers.h"

#define RAND_MOD 512
#define ITERATIONS 1000

static int32_t random_i32()
{
    return munit_rand_int_range(-RAND_MOD, RAND_MOD);
}

static float random_flt()
{
    return (float)random_i32() / 100.0;
}

static MunitResult test_i32(const MunitParameter params[], void *fixture)
{
    int32_t values[64];
    int32_t cmp = random_i32();
    for (size_t i = 0; i < ITERATIONS; i++) {
        uint64_t eq = 0, lt = 0, gt = 0;
        for (size_t j = 0; j < 64; j++) {
            values[j] = random_i32();
            if (values[j] == cmp)
                eq |= (uint64_t)1 << j;
            if (values[j] < cmp)
                lt |= (uint64_t)1 << j;
            if (values[j] > cmp)
                gt |= (uint64_t)1 << j;
        }
        assert_uint64(eq, ==, cx_match_i32_eq(64, values, cmp));
        assert_uint64(lt, ==, cx_match_i32_lt(64, values, cmp));
        assert_uint64(gt, ==, cx_match_i32_gt(64, values, cmp));
    }
    return MUNIT_OK;
}

static MunitResult test_i64(const MunitParameter params[], void *fixture)
{
    int64_t values[64];
    int64_t cmp = random_i32();
    for (size_t i = 0; i < ITERATIONS; i++) {
        uint64_t eq = 0, lt = 0, gt = 0;
        for (size_t j = 0; j < 64; j++) {
            values[j] = random_i32();
            if (values[j] == cmp)
                eq |= (uint64_t)1 << j;
            if (values[j] < cmp)
                lt |= (uint64_t)1 << j;
            if (values[j] > cmp)
                gt |= (uint64_t)1 << j;
        }
        assert_uint64(eq, ==, cx_match_i64_eq(64, values, cmp));
        assert_uint64(lt, ==, cx_match_i64_lt(64, values, cmp));
        assert_uint64(gt, ==, cx_match_i64_gt(64, values, cmp));
    }
    return MUNIT_OK;
}

static MunitResult test_flt(const MunitParameter params[], void *fixture)
{
    float values[64];
    float cmp = random_flt();
    for (size_t i = 0; i < ITERATIONS; i++) {
        uint64_t eq = 0, lt = 0, gt = 0;
        for (size_t j = 0; j < 64; j++) {
            values[j] = random_flt();
            if (values[j] == cmp)
                eq |= (uint64_t)1 << j;
            if (values[j] < cmp)
                lt |= (uint64_t)1 << j;
            if (values[j] > cmp)
                gt |= (uint64_t)1 << j;
        }
        assert_uint64(eq, ==, cx_match_flt_eq(64, values, cmp));
        assert_uint64(lt, ==, cx_match_flt_lt(64, values, cmp));
        assert_uint64(gt, ==, cx_match_flt_gt(64, values, cmp));
    }
    return MUNIT_OK;
}

static MunitResult test_dbl(const MunitParameter params[], void *fixture)
{
    double values[64];
    double cmp = random_flt();
    for (size_t i = 0; i < ITERATIONS; i++) {
        uint64_t eq = 0, lt = 0, gt = 0;
        for (size_t j = 0; j < 64; j++) {
            values[j] = random_flt();
            if (values[j] == cmp)
                eq |= (uint64_t)1 << j;
            if (values[j] < cmp)
                lt |= (uint64_t)1 << j;
            if (values[j] > cmp)
                gt |= (uint64_t)1 << j;
        }
        assert_uint64(eq, ==, cx_match_dbl_eq(64, values, cmp));
        assert_uint64(lt, ==, cx_match_dbl_lt(64, values, cmp));
        assert_uint64(gt, ==, cx_match_dbl_gt(64, values, cmp));
    }
    return MUNIT_OK;
}

static MunitResult test_str(const MunitParameter params[], void *fixture)
{
#define CX_STR(str)          \
    {                        \
        str, sizeof(str) - 1 \
    }
#define CX_CMP(str) &(struct cx_string)CX_STR(str)

    struct cx_string strings[] = {CX_STR("za"), CX_STR("ab"), CX_STR("ba"),
                                  CX_STR("AB")};
    size_t size = sizeof(strings) / sizeof(*strings);

    assert_uint(cx_match_str_eq(size, strings, CX_CMP("foo"), false), ==, 0);
    assert_uint(cx_match_str_eq(size, strings, CX_CMP(""), false), ==, 0);
    assert_uint(cx_match_str_eq(size, strings, CX_CMP("ab"), true), ==, 0x2);
    assert_uint(cx_match_str_eq(size, strings, CX_CMP("ab"), false), ==, 0xA);

    assert_uint(cx_match_str_gt(size, strings, CX_CMP("x"), false), ==, 0x1);
    assert_uint(cx_match_str_lt(size, strings, CX_CMP("x"), false), ==, 0xE);

    assert_uint(cx_match_str_contains(size, strings, CX_CMP(""), false,
                                      CX_STR_LOCATION_START),
                ==, 0xF);
    assert_uint(cx_match_str_contains(size, strings, CX_CMP("a"), false,
                                      CX_STR_LOCATION_START),
                ==, 0xA);
    assert_uint(cx_match_str_contains(size, strings, CX_CMP("A"), false,
                                      CX_STR_LOCATION_START),
                ==, 0xA);
    assert_uint(cx_match_str_contains(size, strings, CX_CMP("a"), true,
                                      CX_STR_LOCATION_START),
                ==, 0x2);
    assert_uint(cx_match_str_contains(size, strings, CX_CMP("x"), false,
                                      CX_STR_LOCATION_START),
                ==, 0);

    assert_uint(cx_match_str_contains(size, strings, CX_CMP("z"), false,
                                      CX_STR_LOCATION_END),
                ==, 0);
    assert_uint(cx_match_str_contains(size, strings, CX_CMP("a"), true,
                                      CX_STR_LOCATION_END),
                ==, 0x5);
    assert_uint(cx_match_str_contains(size, strings, CX_CMP("a"), false,
                                      CX_STR_LOCATION_END),
                ==, 0x5);
    assert_uint(cx_match_str_contains(size, strings, CX_CMP("b"), false,
                                      CX_STR_LOCATION_END),
                ==, 0xA);
    assert_uint(cx_match_str_contains(size, strings, CX_CMP("b"), true,
                                      CX_STR_LOCATION_END),
                ==, 0x2);

    assert_uint(cx_match_str_contains(size, strings, CX_CMP("x"), false,
                                      CX_STR_LOCATION_ANY),
                ==, 0);
    assert_uint(cx_match_str_contains(size, strings, CX_CMP("a"), true,
                                      CX_STR_LOCATION_ANY),
                ==, 0x7);
    assert_uint(cx_match_str_contains(size, strings, CX_CMP("a"), false,
                                      CX_STR_LOCATION_ANY),
                ==, 0xF);

    return MUNIT_OK;
}

MunitTest match_tests[] = {
    {"/i32", test_i32, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/i64", test_i64, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/flt", test_flt, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/dbl", test_dbl, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/str", test_str, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL}};
