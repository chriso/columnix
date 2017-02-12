#define MUNIT_ENABLE_ASSERT_ALIASES
#include "munit.h"

#include "match.h"

#define RAND_MOD 512

#define ITERATIONS 10000

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
        assert_uint64(eq, ==, zcs_match_i32_eq(64, values, cmp));
        assert_uint64(lt, ==, zcs_match_i32_lt(64, values, cmp));
        assert_uint64(gt, ==, zcs_match_i32_gt(64, values, cmp));
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
        assert_uint64(eq, ==, zcs_match_i64_eq(64, values, cmp));
        assert_uint64(lt, ==, zcs_match_i64_lt(64, values, cmp));
        assert_uint64(gt, ==, zcs_match_i64_gt(64, values, cmp));
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
        assert_uint64(eq, ==, zcs_match_flt_eq(64, values, cmp));
        assert_uint64(lt, ==, zcs_match_flt_lt(64, values, cmp));
        assert_uint64(gt, ==, zcs_match_flt_gt(64, values, cmp));
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
        assert_uint64(eq, ==, zcs_match_dbl_eq(64, values, cmp));
        assert_uint64(lt, ==, zcs_match_dbl_lt(64, values, cmp));
        assert_uint64(gt, ==, zcs_match_dbl_gt(64, values, cmp));
    }
    return MUNIT_OK;
}

static MunitResult test_str(const MunitParameter params[], void *fixture)
{
#define ZCS_STR(str)         \
    {                        \
        str, sizeof(str) - 1 \
    }
#define ZCS_CMP(str) &(struct zcs_string)ZCS_STR(str)

    struct zcs_string strings[] = {ZCS_STR("za"), ZCS_STR("ab"), ZCS_STR("ba"),
                                   ZCS_STR("AB")};
    size_t size = sizeof(strings) / sizeof(*strings);

    assert_uint(zcs_match_str_eq(size, strings, ZCS_CMP("foo"), false), ==, 0);
    assert_uint(zcs_match_str_eq(size, strings, ZCS_CMP(""), false), ==, 0);
    assert_uint(zcs_match_str_eq(size, strings, ZCS_CMP("ab"), true), ==, 0x2);
    assert_uint(zcs_match_str_eq(size, strings, ZCS_CMP("ab"), false), ==, 0xA);

    assert_uint(zcs_match_str_gt(size, strings, ZCS_CMP("x"), false), ==, 0x1);
    assert_uint(zcs_match_str_lt(size, strings, ZCS_CMP("x"), false), ==, 0xE);

    assert_uint(zcs_match_str_contains(size, strings, ZCS_CMP(""), false,
                                       ZCS_STR_LOCATION_START),
                ==, 0xF);
    assert_uint(zcs_match_str_contains(size, strings, ZCS_CMP("a"), false,
                                       ZCS_STR_LOCATION_START),
                ==, 0xA);
    assert_uint(zcs_match_str_contains(size, strings, ZCS_CMP("A"), false,
                                       ZCS_STR_LOCATION_START),
                ==, 0xA);
    assert_uint(zcs_match_str_contains(size, strings, ZCS_CMP("a"), true,
                                       ZCS_STR_LOCATION_START),
                ==, 0x2);
    assert_uint(zcs_match_str_contains(size, strings, ZCS_CMP("x"), false,
                                       ZCS_STR_LOCATION_START),
                ==, 0);

    assert_uint(zcs_match_str_contains(size, strings, ZCS_CMP("z"), false,
                                       ZCS_STR_LOCATION_END),
                ==, 0);
    assert_uint(zcs_match_str_contains(size, strings, ZCS_CMP("a"), true,
                                       ZCS_STR_LOCATION_END),
                ==, 0x5);
    assert_uint(zcs_match_str_contains(size, strings, ZCS_CMP("a"), false,
                                       ZCS_STR_LOCATION_END),
                ==, 0x5);
    assert_uint(zcs_match_str_contains(size, strings, ZCS_CMP("b"), false,
                                       ZCS_STR_LOCATION_END),
                ==, 0xA);
    assert_uint(zcs_match_str_contains(size, strings, ZCS_CMP("b"), true,
                                       ZCS_STR_LOCATION_END),
                ==, 0x2);

    assert_uint(zcs_match_str_contains(size, strings, ZCS_CMP("x"), false,
                                       ZCS_STR_LOCATION_ANY),
                ==, 0);
    assert_uint(zcs_match_str_contains(size, strings, ZCS_CMP("a"), true,
                                       ZCS_STR_LOCATION_ANY),
                ==, 0x7);
    assert_uint(zcs_match_str_contains(size, strings, ZCS_CMP("a"), false,
                                       ZCS_STR_LOCATION_ANY),
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
