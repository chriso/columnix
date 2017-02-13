#include <stdio.h>

#include "predicate.h"

#include "helpers.h"

#define COLUMN_COUNT 8
#define ROW_COUNT 10

static const uint64_t all_rows = (1 << ROW_COUNT) - 1;

struct zcs_predicate_fixture {
    struct zcs_row_group *row_group;
    struct zcs_column *columns[COLUMN_COUNT];
    struct zcs_row_group_cursor *cursor;
};

struct zcs_predicate_index_test_case {
    struct zcs_predicate *predicate;
    enum zcs_predicate_match expected;
};

struct zcs_predicate_row_test_case {
    struct zcs_predicate *predicate;
    uint64_t expected;
};

static void *setup(const MunitParameter params[], void *data)
{
    struct zcs_predicate_fixture *fixture = malloc(sizeof(*fixture));
    assert_not_null(fixture);

    fixture->row_group = zcs_row_group_new();
    assert_not_null(fixture->row_group);

    fixture->columns[0] = zcs_column_new(ZCS_COLUMN_I32, ZCS_ENCODE_NONE);
    fixture->columns[1] = zcs_column_new(ZCS_COLUMN_I64, ZCS_ENCODE_NONE);
    fixture->columns[2] = zcs_column_new(ZCS_COLUMN_BIT, ZCS_ENCODE_NONE);
    fixture->columns[3] = zcs_column_new(ZCS_COLUMN_STR, ZCS_ENCODE_NONE);

    fixture->columns[4] = zcs_column_new(ZCS_COLUMN_I32, ZCS_ENCODE_NONE);
    fixture->columns[5] = zcs_column_new(ZCS_COLUMN_I64, ZCS_ENCODE_NONE);
    fixture->columns[6] = zcs_column_new(ZCS_COLUMN_BIT, ZCS_ENCODE_NONE);
    fixture->columns[7] = zcs_column_new(ZCS_COLUMN_BIT, ZCS_ENCODE_NONE);

    for (size_t i = 0; i < COLUMN_COUNT; i++)
        assert_not_null(fixture->columns[i]);

    char buffer[64];
    for (size_t i = 0; i < ROW_COUNT; i++) {
        assert_true(zcs_column_put_i32(fixture->columns[0], i));
        assert_true(zcs_column_put_i64(fixture->columns[1], i));
        assert_true(zcs_column_put_bit(fixture->columns[2], i % 3 == 0));
        sprintf(buffer, "zcs %zu", i);
        assert_true(zcs_column_put_str(fixture->columns[3], buffer));

        assert_true(zcs_column_put_i32(fixture->columns[4], 5));
        assert_true(zcs_column_put_i64(fixture->columns[5], 5));

        assert_true(zcs_column_put_bit(fixture->columns[6], false));
        assert_true(zcs_column_put_bit(fixture->columns[7], true));
    }

    for (size_t i = 0; i < COLUMN_COUNT; i++)
        assert_true(
            zcs_row_group_add_column(fixture->row_group, fixture->columns[i]));

    fixture->cursor = zcs_row_group_cursor_new(fixture->row_group);
    assert_not_null(fixture->cursor);

    return fixture;
}

static void teardown(void *ptr)
{
    struct zcs_predicate_fixture *fixture = ptr;
    for (size_t i = 0; i < COLUMN_COUNT; i++)
        zcs_column_free(fixture->columns[i]);
    zcs_row_group_cursor_free(fixture->cursor);
    zcs_row_group_free(fixture->row_group);
    free(fixture);
}

static MunitResult test_indexes(
    const struct zcs_predicate_fixture *fixture,
    struct zcs_predicate_index_test_case *test_cases, size_t size)
{
    for (size_t i = 0; i < size / sizeof(*test_cases); i++) {
        struct zcs_predicate_index_test_case *test_case = &test_cases[i];
        assert_not_null(test_case->predicate);
        assert_int(zcs_predicate_match_indexes(test_case->predicate,
                                               fixture->row_group),
                   ==, test_case->expected);
        zcs_predicate_free(test_case->predicate);
    }
    return MUNIT_OK;
}

static MunitResult test_rows(const struct zcs_predicate_fixture *fixture,
                             struct zcs_predicate_row_test_case *test_cases,
                             size_t size)
{
    for (size_t i = 0; i < size / sizeof(*test_cases); i++) {
        struct zcs_predicate_row_test_case *test_case = &test_cases[i];
        assert_not_null(test_case->predicate);
        size_t count;
        uint64_t matches;
        assert_true(
            zcs_predicate_match_rows(test_case->predicate, fixture->row_group,
                                     fixture->cursor, &matches, &count));
        assert_size(count, ==, ROW_COUNT);
        assert_uint64(matches, ==, test_case->expected);
        zcs_predicate_free(test_case->predicate);
    }
    return MUNIT_OK;
}

static MunitResult test_valid(const MunitParameter params[], void *ptr)
{
    struct zcs_predicate_fixture *fixture = ptr;

    struct {
        struct zcs_predicate *predicate;
        bool valid;
    } test_cases[] = {
        {zcs_predicate_new_true(), true},
        {zcs_predicate_new_i32_eq(0, 100), true},
        {zcs_predicate_new_i32_eq(1, 100), false},   // type mismatch
        {zcs_predicate_new_i32_eq(20, 100), false},  // column doesn't exist
        {zcs_predicate_new_and(2, zcs_predicate_new_true(),
                               zcs_predicate_new_true()),
         true},
        {zcs_predicate_new_and(2, zcs_predicate_new_true(),
                               zcs_predicate_new_i32_eq(0, 100)),
         true},
        {zcs_predicate_new_and(2, zcs_predicate_new_true(),
                               zcs_predicate_new_i32_eq(1, 100)),
         false},  // type mismatch
        {zcs_predicate_new_and(2, zcs_predicate_new_true(),
                               zcs_predicate_new_i32_eq(20, 100)),
         false},  // column doesn't exist
        {zcs_predicate_new_or(2, zcs_predicate_new_true(),
                              zcs_predicate_new_true()),
         true},
        {zcs_predicate_new_or(2, zcs_predicate_new_true(),
                              zcs_predicate_new_i32_eq(0, 100)),
         true},
        {zcs_predicate_new_or(2, zcs_predicate_new_true(),
                              zcs_predicate_new_i32_eq(1, 100)),
         false},  // type mismatch
        {zcs_predicate_new_or(2, zcs_predicate_new_true(),
                              zcs_predicate_new_i32_eq(20, 100)),
         false}  // column doesn't exist
    };

    for (size_t i = 0; i < sizeof(test_cases) / sizeof(*test_cases); i++) {
        bool valid =
            zcs_predicate_valid(test_cases[i].predicate, fixture->row_group);
        if (test_cases[i].valid)
            assert_true(valid);
        else
            assert_false(valid);
        zcs_predicate_free(test_cases[i].predicate);
    }

    return MUNIT_OK;
}

static MunitResult test_bit_match_index(const MunitParameter params[],
                                        void *fixture)
{
    struct zcs_predicate_index_test_case test_cases[] = {
        {zcs_predicate_new_true(), ZCS_PREDICATE_MATCH_ALL_ROWS},
        {zcs_predicate_new_bit_eq(2, true), ZCS_PREDICATE_MATCH_UNKNOWN},
        {zcs_predicate_new_bit_eq(2, false), ZCS_PREDICATE_MATCH_UNKNOWN},
        {zcs_predicate_new_bit_eq(6, false), ZCS_PREDICATE_MATCH_ALL_ROWS},
        {zcs_predicate_new_bit_eq(6, true), ZCS_PREDICATE_MATCH_NO_ROWS},
        {zcs_predicate_new_bit_eq(7, false), ZCS_PREDICATE_MATCH_NO_ROWS},
        {zcs_predicate_new_bit_eq(7, true), ZCS_PREDICATE_MATCH_ALL_ROWS}};

    return test_indexes(fixture, test_cases, sizeof(test_cases));
}

static MunitResult test_bit_match_rows(const MunitParameter params[],
                                       void *fixture)
{
    struct zcs_predicate_row_test_case test_cases[] = {
        {zcs_predicate_new_true(), all_rows},
        {zcs_predicate_new_bit_eq(2, true), 0x249},   // 0b1001001001
        {zcs_predicate_new_bit_eq(2, false), 0x1B6},  // 0b0110110110
        {zcs_predicate_new_bit_eq(6, false), all_rows},
        {zcs_predicate_new_bit_eq(6, true), 0},
        {zcs_predicate_new_bit_eq(7, false), 0},
        {zcs_predicate_new_bit_eq(7, true), all_rows}};

    return test_rows(fixture, test_cases, sizeof(test_cases));
}

static MunitResult test_i32_match_index(const MunitParameter params[],
                                        void *fixture)
{
    struct zcs_predicate_index_test_case test_cases[] = {
        {zcs_predicate_new_true(), ZCS_PREDICATE_MATCH_ALL_ROWS},
        {zcs_predicate_new_i32_lt(0, 10), ZCS_PREDICATE_MATCH_ALL_ROWS},
        {zcs_predicate_new_i32_lt(0, 0), ZCS_PREDICATE_MATCH_NO_ROWS},
        {zcs_predicate_new_i32_lt(0, 5), ZCS_PREDICATE_MATCH_UNKNOWN},
        {zcs_predicate_new_i32_gt(0, -1), ZCS_PREDICATE_MATCH_ALL_ROWS},
        {zcs_predicate_new_i32_gt(0, 9), ZCS_PREDICATE_MATCH_NO_ROWS},
        {zcs_predicate_new_i32_gt(0, 5), ZCS_PREDICATE_MATCH_UNKNOWN},
        {zcs_predicate_new_i32_eq(0, -1), ZCS_PREDICATE_MATCH_NO_ROWS},
        {zcs_predicate_new_i32_eq(0, 10), ZCS_PREDICATE_MATCH_NO_ROWS},
        {zcs_predicate_new_i32_eq(0, 9), ZCS_PREDICATE_MATCH_UNKNOWN},
        {zcs_predicate_new_i32_eq(4, 5), ZCS_PREDICATE_MATCH_ALL_ROWS},
    };

    return test_indexes(fixture, test_cases, sizeof(test_cases));
}

static MunitResult test_i32_match_rows(const MunitParameter params[],
                                       void *fixture)
{
    struct zcs_predicate_row_test_case test_cases[] = {
        {zcs_predicate_new_true(), all_rows},
        {zcs_predicate_new_i32_lt(0, 10), all_rows},
        {zcs_predicate_new_i32_lt(0, 0), 0},
        {zcs_predicate_new_i32_lt(0, 4), 0xF},
        {zcs_predicate_new_i32_gt(0, -1), all_rows},
        {zcs_predicate_new_i32_gt(0, 9), 0},
        {zcs_predicate_new_i32_eq(0, 0), 0x1},
        {zcs_predicate_new_i32_eq(0, 1), 0x2},
        {zcs_predicate_new_i32_eq(0, 2), 0x4},
        {zcs_predicate_new_i32_eq(0, 3), 0x8},
        // (col != 3) => 0b1111110111
        {zcs_predicate_negate(zcs_predicate_new_i32_eq(0, 3)), 0x3F7},
        // (col > 2 && col < 8) => 0b0011111000
        {zcs_predicate_new_and(2, zcs_predicate_new_i32_gt(0, 2),
                               zcs_predicate_new_i32_lt(0, 8)),
         0xF8},
        // (col < 2 || col > 8) => 0b1000000011
        {zcs_predicate_new_or(2, zcs_predicate_new_i32_lt(0, 2),
                              zcs_predicate_new_i32_gt(0, 8)),
         0x203},
    };

    return test_rows(fixture, test_cases, sizeof(test_cases));
}

static MunitResult test_i64_match_index(const MunitParameter params[],
                                        void *fixture)
{
    struct zcs_predicate_index_test_case test_cases[] = {
        {zcs_predicate_new_true(), ZCS_PREDICATE_MATCH_ALL_ROWS},
        {zcs_predicate_new_i64_lt(1, 10), ZCS_PREDICATE_MATCH_ALL_ROWS},
        {zcs_predicate_new_i64_lt(1, 0), ZCS_PREDICATE_MATCH_NO_ROWS},
        {zcs_predicate_new_i64_lt(1, 5), ZCS_PREDICATE_MATCH_UNKNOWN},
        {zcs_predicate_new_i64_gt(1, -1), ZCS_PREDICATE_MATCH_ALL_ROWS},
        {zcs_predicate_new_i64_gt(1, 9), ZCS_PREDICATE_MATCH_NO_ROWS},
        {zcs_predicate_new_i64_gt(1, 5), ZCS_PREDICATE_MATCH_UNKNOWN},
        {zcs_predicate_new_i64_eq(1, -1), ZCS_PREDICATE_MATCH_NO_ROWS},
        {zcs_predicate_new_i64_eq(1, 10), ZCS_PREDICATE_MATCH_NO_ROWS},
        {zcs_predicate_new_i64_eq(1, 9), ZCS_PREDICATE_MATCH_UNKNOWN},
        {zcs_predicate_new_i64_eq(5, 5), ZCS_PREDICATE_MATCH_ALL_ROWS},
    };

    return test_indexes(fixture, test_cases, sizeof(test_cases));
}

static MunitResult test_i64_match_rows(const MunitParameter params[],
                                       void *fixture)
{
    struct zcs_predicate_row_test_case test_cases[] = {
        {zcs_predicate_new_true(), all_rows},
        {zcs_predicate_new_i64_lt(1, 10), all_rows},
        {zcs_predicate_new_i64_lt(1, 0), 0},
        {zcs_predicate_new_i64_lt(1, 4), 0xF},
        {zcs_predicate_new_i64_gt(1, -1), all_rows},
        {zcs_predicate_new_i64_gt(1, 9), 0},
        {zcs_predicate_new_i64_eq(1, 0), 0x1},
        {zcs_predicate_new_i64_eq(1, 1), 0x2},
        {zcs_predicate_new_i64_eq(1, 2), 0x4},
        {zcs_predicate_new_i64_eq(1, 3), 0x8},
        // (col != 3) => 0b1111110111
        {zcs_predicate_negate(zcs_predicate_new_i64_eq(1, 3)), 0x3F7},
        // (col > 2 && col < 8) => 0b0011111000
        {zcs_predicate_new_and(2, zcs_predicate_new_i64_gt(1, 2),
                               zcs_predicate_new_i64_lt(1, 8)),
         0xF8},
        // (col < 2 || col > 8) => 0b1000000011
        {zcs_predicate_new_or(2, zcs_predicate_new_i64_lt(1, 2),
                              zcs_predicate_new_i64_gt(1, 8)),
         0x203},
    };

    return test_rows(fixture, test_cases, sizeof(test_cases));
}

static MunitResult test_str_match_index(const MunitParameter params[],
                                        void *fixture)
{
    struct zcs_predicate_index_test_case test_cases[] = {
        {zcs_predicate_new_true(), ZCS_PREDICATE_MATCH_ALL_ROWS},
        {zcs_predicate_new_str_eq(3, "foo", true), ZCS_PREDICATE_MATCH_NO_ROWS},
        {zcs_predicate_new_str_eq(3, "zcs 0", true),
         ZCS_PREDICATE_MATCH_UNKNOWN},
        {zcs_predicate_new_str_eq(3, "zcs 10", true),
         ZCS_PREDICATE_MATCH_NO_ROWS},
        {zcs_predicate_new_str_lt(3, "foo", true), ZCS_PREDICATE_MATCH_UNKNOWN},
        {zcs_predicate_new_str_lt(3, "zcs 0", true),
         ZCS_PREDICATE_MATCH_UNKNOWN},
        {zcs_predicate_new_str_lt(3, "zcs 10", true),
         ZCS_PREDICATE_MATCH_UNKNOWN},
        {zcs_predicate_new_str_gt(3, "foo", true), ZCS_PREDICATE_MATCH_UNKNOWN},
        {zcs_predicate_new_str_gt(3, "zcs 0", true),
         ZCS_PREDICATE_MATCH_UNKNOWN},
        {zcs_predicate_new_str_gt(3, "zcs 10", true),
         ZCS_PREDICATE_MATCH_UNKNOWN},
        {zcs_predicate_new_str_contains(3, "foo", true, ZCS_STR_LOCATION_ANY),
         ZCS_PREDICATE_MATCH_UNKNOWN},
        {zcs_predicate_new_str_contains(3, "zcs 0", true, ZCS_STR_LOCATION_ANY),
         ZCS_PREDICATE_MATCH_UNKNOWN},
        {zcs_predicate_new_str_contains(3, "zcs 10", true,
                                        ZCS_STR_LOCATION_ANY),
         ZCS_PREDICATE_MATCH_NO_ROWS}};

    return test_indexes(fixture, test_cases, sizeof(test_cases));
}

static MunitResult test_str_match_rows(const MunitParameter params[],
                                       void *fixture)
{
    struct zcs_predicate_row_test_case test_cases[] = {
        {zcs_predicate_new_true(), all_rows},

        {zcs_predicate_new_str_eq(3, "zcs", false), 0},
        {zcs_predicate_new_str_eq(3, "zcs 0", false), 0x1},
        {zcs_predicate_new_str_eq(3, "ZCS 1", false), 0x2},
        {zcs_predicate_new_str_eq(3, "ZCS 1", true), 0},

        {zcs_predicate_new_str_lt(3, "zcs", false), 0},
        {zcs_predicate_new_str_gt(3, "zcs", false), all_rows},
        {zcs_predicate_new_str_lt(3, "foo", false), 0},
        {zcs_predicate_new_str_gt(3, "foo", false), all_rows},
        {zcs_predicate_new_str_lt(3, "zzz", false), all_rows},
        {zcs_predicate_new_str_gt(3, "zzz", false), 0},

        // contains @ start
        {zcs_predicate_new_str_contains(3, "zcs", true, ZCS_STR_LOCATION_START),
         all_rows},
        {zcs_predicate_new_str_contains(3, "zcs", false,
                                        ZCS_STR_LOCATION_START),
         all_rows},
        {zcs_predicate_new_str_contains(3, "ZCS", false,
                                        ZCS_STR_LOCATION_START),
         all_rows},
        {zcs_predicate_new_str_contains(3, "ZCS", true, ZCS_STR_LOCATION_START),
         0},
        {zcs_predicate_new_str_contains(3, "foo", false,
                                        ZCS_STR_LOCATION_START),
         0},
        {zcs_predicate_new_str_contains(3, "zcs 0", false,
                                        ZCS_STR_LOCATION_START),
         0x1},
        {zcs_predicate_new_str_contains(3, "zcs 0", true,
                                        ZCS_STR_LOCATION_START),
         0x1},
        {zcs_predicate_new_str_contains(3, "zcs 1", false,
                                        ZCS_STR_LOCATION_START),
         0x2},
        {zcs_predicate_new_str_contains(3, "zcs 2", false,
                                        ZCS_STR_LOCATION_START),
         0x4},

        // contains @ any
        {zcs_predicate_new_str_contains(3, "zcs", true, ZCS_STR_LOCATION_ANY),
         all_rows},
        {zcs_predicate_new_str_contains(3, "zcs", false, ZCS_STR_LOCATION_ANY),
         all_rows},
        {zcs_predicate_new_str_contains(3, "ZCS", false, ZCS_STR_LOCATION_ANY),
         all_rows},
        {zcs_predicate_new_str_contains(3, "ZCS", true, ZCS_STR_LOCATION_ANY),
         0},
        {zcs_predicate_new_str_contains(3, "foo", false, ZCS_STR_LOCATION_ANY),
         0},
        {zcs_predicate_new_str_contains(3, "zcs 0", false,
                                        ZCS_STR_LOCATION_ANY),
         0x1},
        {zcs_predicate_new_str_contains(3, "zcs 1", false,
                                        ZCS_STR_LOCATION_ANY),
         0x2},
        {zcs_predicate_new_str_contains(3, "zcs 2", false,
                                        ZCS_STR_LOCATION_ANY),
         0x4},

        // contains @ end
        {zcs_predicate_new_str_contains(3, "zcs", true, ZCS_STR_LOCATION_END),
         0},
        {zcs_predicate_new_str_contains(3, "zcs", false, ZCS_STR_LOCATION_END),
         0},
        {zcs_predicate_new_str_contains(3, "ZCS", false, ZCS_STR_LOCATION_END),
         0},
        {zcs_predicate_new_str_contains(3, "ZCS", true, ZCS_STR_LOCATION_END),
         0},
        {zcs_predicate_new_str_contains(3, "foo", false, ZCS_STR_LOCATION_END),
         0},
        {zcs_predicate_new_str_contains(3, "0", false, ZCS_STR_LOCATION_END),
         0x1},
        {zcs_predicate_new_str_contains(3, "ZCS 0", false,
                                        ZCS_STR_LOCATION_END),
         0x1},
        {zcs_predicate_new_str_contains(3, "1", false, ZCS_STR_LOCATION_END),
         0x2},
        {zcs_predicate_new_str_contains(3, "2", false, ZCS_STR_LOCATION_END),
         0x4},
    };

    return test_rows(fixture, test_cases, sizeof(test_cases));
}

MunitTest predicate_tests[] = {
    {"/valid", test_valid, setup, teardown, MUNIT_TEST_OPTION_NONE, NULL},
    {"/bit-match-index", test_bit_match_index, setup, teardown,
     MUNIT_TEST_OPTION_NONE, NULL},
    {"/bit-match-rows", test_bit_match_rows, setup, teardown,
     MUNIT_TEST_OPTION_NONE, NULL},
    {"/i32-match-index", test_i32_match_index, setup, teardown,
     MUNIT_TEST_OPTION_NONE, NULL},
    {"/i32-match-rows", test_i32_match_rows, setup, teardown,
     MUNIT_TEST_OPTION_NONE, NULL},
    {"/i64-match-index", test_i64_match_index, setup, teardown,
     MUNIT_TEST_OPTION_NONE, NULL},
    {"/i64-match-rows", test_i64_match_rows, setup, teardown,
     MUNIT_TEST_OPTION_NONE, NULL},
    {"/str-match-index", test_str_match_index, setup, teardown,
     MUNIT_TEST_OPTION_NONE, NULL},
    {"/str-match-rows", test_str_match_rows, setup, teardown,
     MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL}};
