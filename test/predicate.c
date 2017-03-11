#include <stdio.h>

#include "predicate.h"

#include "helpers.h"

#define COLUMN_COUNT 8
#define ROW_COUNT 10

static const uint64_t all_rows = (1 << ROW_COUNT) - 1;

struct cx_predicate_fixture {
    struct cx_row_group *row_group;
    struct cx_column *columns[COLUMN_COUNT];
    struct cx_column *nulls[COLUMN_COUNT];
    struct cx_row_group_cursor *cursor;
};

struct cx_predicate_index_test_case {
    struct cx_predicate *predicate;
    enum cx_predicate_match expected;
};

struct cx_predicate_row_test_case {
    struct cx_predicate *predicate;
    uint64_t expected;
};

static void *setup(const MunitParameter params[], void *data)
{
    struct cx_predicate_fixture *fixture = malloc(sizeof(*fixture));
    assert_not_null(fixture);

    fixture->row_group = cx_row_group_new();
    assert_not_null(fixture->row_group);

    enum cx_column_type types[] = {CX_COLUMN_I32, CX_COLUMN_I64, CX_COLUMN_BIT,
                                   CX_COLUMN_STR, CX_COLUMN_I32, CX_COLUMN_I64,
                                   CX_COLUMN_BIT, CX_COLUMN_BIT};

    for (size_t i = 0; i < COLUMN_COUNT; i++) {
        fixture->columns[i] = cx_column_new(types[i], CX_ENCODING_NONE);
        assert_not_null(fixture->columns[i]);
        fixture->nulls[i] = cx_column_new(CX_COLUMN_BIT, CX_ENCODING_NONE);
        assert_not_null(fixture->nulls[i]);
    }

    char buffer[64];
    for (size_t i = 0; i < ROW_COUNT; i++) {
        assert_true(cx_column_put_i32(fixture->columns[0], i));
        assert_true(cx_column_put_i64(fixture->columns[1], i));
        assert_true(cx_column_put_bit(fixture->columns[2], i % 3 == 0));
        sprintf(buffer, "cx %zu", i);
        assert_true(cx_column_put_str(fixture->columns[3], buffer));

        assert_true(cx_column_put_i32(fixture->columns[4], 5));
        assert_true(cx_column_put_i64(fixture->columns[5], 5));

        assert_true(cx_column_put_bit(fixture->columns[6], false));
        assert_true(cx_column_put_bit(fixture->columns[7], true));

        assert_true(cx_column_put_bit(fixture->nulls[0], i % 2 == 0));
        assert_true(cx_column_put_bit(fixture->nulls[1], i % 3 == 0));
        assert_true(cx_column_put_bit(fixture->nulls[2], true));
        for (size_t j = 3; j < COLUMN_COUNT; j++)
            assert_true(cx_column_put_bit(fixture->nulls[j], false));
    }

    for (size_t i = 0; i < COLUMN_COUNT; i++)
        assert_true(cx_row_group_add_column(
            fixture->row_group, fixture->columns[i], fixture->nulls[i]));

    fixture->cursor = cx_row_group_cursor_new(fixture->row_group);
    assert_not_null(fixture->cursor);

    return fixture;
}

static void teardown(void *ptr)
{
    struct cx_predicate_fixture *fixture = ptr;
    for (size_t i = 0; i < COLUMN_COUNT; i++) {
        cx_column_free(fixture->columns[i]);
        cx_column_free(fixture->nulls[i]);
    }
    cx_row_group_cursor_free(fixture->cursor);
    cx_row_group_free(fixture->row_group);
    free(fixture);
}

static MunitResult test_indexes(const struct cx_predicate_fixture *fixture,
                                struct cx_predicate_index_test_case *test_cases,
                                size_t size)
{
    for (size_t i = 0; i < size / sizeof(*test_cases); i++) {
        struct cx_predicate_index_test_case *test_case = &test_cases[i];
        assert_not_null(test_case->predicate);
        assert_int(cx_predicate_match_indexes(test_case->predicate,
                                              fixture->row_group),
                   ==, test_case->expected);
        cx_predicate_free(test_case->predicate);
    }
    return MUNIT_OK;
}

static MunitResult test_rows(const struct cx_predicate_fixture *fixture,
                             struct cx_predicate_row_test_case *test_cases,
                             size_t size)
{
    for (size_t i = 0; i < size / sizeof(*test_cases); i++) {
        struct cx_predicate_row_test_case *test_case = &test_cases[i];
        assert_not_null(test_case->predicate);
        size_t count;
        uint64_t matches;
        assert_true(cx_predicate_match_rows(test_case->predicate,
                                            fixture->row_group, fixture->cursor,
                                            &matches, &count));
        assert_size(count, ==, ROW_COUNT);
        assert_uint64(matches, ==, test_case->expected);
        cx_predicate_free(test_case->predicate);
    }
    return MUNIT_OK;
}

static MunitResult test_valid(const MunitParameter params[], void *ptr)
{
    struct cx_predicate_fixture *fixture = ptr;

    struct {
        struct cx_predicate *predicate;
        bool valid;
    } test_cases[] = {
        {cx_predicate_new_true(), true},
        {cx_predicate_new_i32_eq(0, 100), true},
        {cx_predicate_new_null(0), true},
        {cx_predicate_new_i32_eq(1, 100), false},   // type mismatch
        {cx_predicate_new_i32_eq(20, 100), false},  // column doesn't exist
        {cx_predicate_new_null(20), false},         // column doesn't exist
        {cx_predicate_new_and(2, cx_predicate_new_true(),
                              cx_predicate_new_true()),
         true},
        {cx_predicate_new_and(2, cx_predicate_new_true(),
                              cx_predicate_new_i32_eq(0, 100)),
         true},
        {cx_predicate_new_and(2, cx_predicate_new_true(),
                              cx_predicate_new_i32_eq(1, 100)),
         false},  // type mismatch
        {cx_predicate_new_and(2, cx_predicate_new_true(),
                              cx_predicate_new_i32_eq(20, 100)),
         false},  // column doesn't exist
        {cx_predicate_new_or(2, cx_predicate_new_true(),
                             cx_predicate_new_true()),
         true},
        {cx_predicate_new_or(2, cx_predicate_new_true(),
                             cx_predicate_new_i32_eq(0, 100)),
         true},
        {cx_predicate_new_or(2, cx_predicate_new_true(),
                             cx_predicate_new_i32_eq(1, 100)),
         false},  // type mismatch
        {cx_predicate_new_or(2, cx_predicate_new_true(),
                             cx_predicate_new_i32_eq(20, 100)),
         false}  // column doesn't exist
    };

    for (size_t i = 0; i < sizeof(test_cases) / sizeof(*test_cases); i++) {
        bool valid =
            cx_predicate_valid(test_cases[i].predicate, fixture->row_group);
        if (test_cases[i].valid)
            assert_true(valid);
        else
            assert_false(valid);
        cx_predicate_free(test_cases[i].predicate);
    }

    return MUNIT_OK;
}

static MunitResult test_bit_match_index(const MunitParameter params[],
                                        void *fixture)
{
    struct cx_predicate_index_test_case test_cases[] = {
        {cx_predicate_new_true(), CX_PREDICATE_MATCH_ALL_ROWS},
        {cx_predicate_new_bit_eq(2, true), CX_PREDICATE_MATCH_UNKNOWN},
        {cx_predicate_new_bit_eq(2, false), CX_PREDICATE_MATCH_UNKNOWN},
        {cx_predicate_new_bit_eq(6, false), CX_PREDICATE_MATCH_ALL_ROWS},
        {cx_predicate_new_bit_eq(6, true), CX_PREDICATE_MATCH_NO_ROWS},
        {cx_predicate_new_bit_eq(7, false), CX_PREDICATE_MATCH_NO_ROWS},
        {cx_predicate_new_bit_eq(7, true), CX_PREDICATE_MATCH_ALL_ROWS}};

    return test_indexes(fixture, test_cases, sizeof(test_cases));
}

static MunitResult test_bit_match_rows(const MunitParameter params[],
                                       void *fixture)
{
    struct cx_predicate_row_test_case test_cases[] = {
        {cx_predicate_new_true(), all_rows},
        {cx_predicate_new_bit_eq(2, true), 0x249},   // 0b1001001001
        {cx_predicate_new_bit_eq(2, false), 0x1B6},  // 0b0110110110
        {cx_predicate_new_bit_eq(6, false), all_rows},
        {cx_predicate_new_bit_eq(6, true), 0},
        {cx_predicate_new_bit_eq(7, false), 0},
        {cx_predicate_new_bit_eq(7, true), all_rows}};

    return test_rows(fixture, test_cases, sizeof(test_cases));
}

static MunitResult test_i32_match_index(const MunitParameter params[],
                                        void *fixture)
{
    struct cx_predicate_index_test_case test_cases[] = {
        {cx_predicate_new_true(), CX_PREDICATE_MATCH_ALL_ROWS},
        {cx_predicate_new_i32_lt(0, 10), CX_PREDICATE_MATCH_ALL_ROWS},
        {cx_predicate_new_i32_lt(0, 0), CX_PREDICATE_MATCH_NO_ROWS},
        {cx_predicate_new_i32_lt(0, 5), CX_PREDICATE_MATCH_UNKNOWN},
        {cx_predicate_new_i32_gt(0, -1), CX_PREDICATE_MATCH_ALL_ROWS},
        {cx_predicate_new_i32_gt(0, 9), CX_PREDICATE_MATCH_NO_ROWS},
        {cx_predicate_new_i32_gt(0, 5), CX_PREDICATE_MATCH_UNKNOWN},
        {cx_predicate_new_i32_eq(0, -1), CX_PREDICATE_MATCH_NO_ROWS},
        {cx_predicate_new_i32_eq(0, 10), CX_PREDICATE_MATCH_NO_ROWS},
        {cx_predicate_new_i32_eq(0, 9), CX_PREDICATE_MATCH_UNKNOWN},
        {cx_predicate_new_i32_eq(4, 5), CX_PREDICATE_MATCH_ALL_ROWS},
    };

    return test_indexes(fixture, test_cases, sizeof(test_cases));
}

static MunitResult test_i32_match_rows(const MunitParameter params[],
                                       void *fixture)
{
    struct cx_predicate_row_test_case test_cases[] = {
        {cx_predicate_new_true(), all_rows},
        {cx_predicate_new_i32_lt(0, 10), all_rows},
        {cx_predicate_new_i32_lt(0, 0), 0},
        {cx_predicate_new_i32_lt(0, 4), 0xF},
        {cx_predicate_new_i32_gt(0, -1), all_rows},
        {cx_predicate_new_i32_gt(0, 9), 0},
        {cx_predicate_new_i32_eq(0, 0), 0x1},
        {cx_predicate_new_i32_eq(0, 1), 0x2},
        {cx_predicate_new_i32_eq(0, 2), 0x4},
        {cx_predicate_new_i32_eq(0, 3), 0x8},
        // (col != 3) => 0b1111110111
        {cx_predicate_negate(cx_predicate_new_i32_eq(0, 3)), 0x3F7},
        // (col > 2 && col < 8) => 0b0011111000
        {cx_predicate_new_and(2, cx_predicate_new_i32_gt(0, 2),
                              cx_predicate_new_i32_lt(0, 8)),
         0xF8},
        // (col < 2 || col > 8) => 0b1000000011
        {cx_predicate_new_or(2, cx_predicate_new_i32_lt(0, 2),
                             cx_predicate_new_i32_gt(0, 8)),
         0x203},
    };

    return test_rows(fixture, test_cases, sizeof(test_cases));
}

static MunitResult test_i64_match_index(const MunitParameter params[],
                                        void *fixture)
{
    struct cx_predicate_index_test_case test_cases[] = {
        {cx_predicate_new_true(), CX_PREDICATE_MATCH_ALL_ROWS},
        {cx_predicate_new_i64_lt(1, 10), CX_PREDICATE_MATCH_ALL_ROWS},
        {cx_predicate_new_i64_lt(1, 0), CX_PREDICATE_MATCH_NO_ROWS},
        {cx_predicate_new_i64_lt(1, 5), CX_PREDICATE_MATCH_UNKNOWN},
        {cx_predicate_new_i64_gt(1, -1), CX_PREDICATE_MATCH_ALL_ROWS},
        {cx_predicate_new_i64_gt(1, 9), CX_PREDICATE_MATCH_NO_ROWS},
        {cx_predicate_new_i64_gt(1, 5), CX_PREDICATE_MATCH_UNKNOWN},
        {cx_predicate_new_i64_eq(1, -1), CX_PREDICATE_MATCH_NO_ROWS},
        {cx_predicate_new_i64_eq(1, 10), CX_PREDICATE_MATCH_NO_ROWS},
        {cx_predicate_new_i64_eq(1, 9), CX_PREDICATE_MATCH_UNKNOWN},
        {cx_predicate_new_i64_eq(5, 5), CX_PREDICATE_MATCH_ALL_ROWS},
    };

    return test_indexes(fixture, test_cases, sizeof(test_cases));
}

static MunitResult test_i64_match_rows(const MunitParameter params[],
                                       void *fixture)
{
    struct cx_predicate_row_test_case test_cases[] = {
        {cx_predicate_new_true(), all_rows},
        {cx_predicate_new_i64_lt(1, 10), all_rows},
        {cx_predicate_new_i64_lt(1, 0), 0},
        {cx_predicate_new_i64_lt(1, 4), 0xF},
        {cx_predicate_new_i64_gt(1, -1), all_rows},
        {cx_predicate_new_i64_gt(1, 9), 0},
        {cx_predicate_new_i64_eq(1, 0), 0x1},
        {cx_predicate_new_i64_eq(1, 1), 0x2},
        {cx_predicate_new_i64_eq(1, 2), 0x4},
        {cx_predicate_new_i64_eq(1, 3), 0x8},
        // (col != 3) => 0b1111110111
        {cx_predicate_negate(cx_predicate_new_i64_eq(1, 3)), 0x3F7},
        // (col > 2 && col < 8) => 0b0011111000
        {cx_predicate_new_and(2, cx_predicate_new_i64_gt(1, 2),
                              cx_predicate_new_i64_lt(1, 8)),
         0xF8},
        // (col < 2 || col > 8) => 0b1000000011
        {cx_predicate_new_or(2, cx_predicate_new_i64_lt(1, 2),
                             cx_predicate_new_i64_gt(1, 8)),
         0x203},
    };

    return test_rows(fixture, test_cases, sizeof(test_cases));
}

static MunitResult test_str_match_index(const MunitParameter params[],
                                        void *fixture)
{
    struct cx_predicate_index_test_case test_cases[] = {
        {cx_predicate_new_true(), CX_PREDICATE_MATCH_ALL_ROWS},
        {cx_predicate_new_str_eq(3, "foo", true), CX_PREDICATE_MATCH_NO_ROWS},
        {cx_predicate_new_str_eq(3, "cx 0", true), CX_PREDICATE_MATCH_UNKNOWN},
        {cx_predicate_new_str_eq(3, "cx 10", true), CX_PREDICATE_MATCH_NO_ROWS},
        {cx_predicate_new_str_lt(3, "foo", true), CX_PREDICATE_MATCH_UNKNOWN},
        {cx_predicate_new_str_lt(3, "cx 0", true), CX_PREDICATE_MATCH_UNKNOWN},
        {cx_predicate_new_str_lt(3, "cx 10", true), CX_PREDICATE_MATCH_UNKNOWN},
        {cx_predicate_new_str_gt(3, "foo", true), CX_PREDICATE_MATCH_UNKNOWN},
        {cx_predicate_new_str_gt(3, "cx 0", true), CX_PREDICATE_MATCH_UNKNOWN},
        {cx_predicate_new_str_gt(3, "cx 10", true), CX_PREDICATE_MATCH_UNKNOWN},
        {cx_predicate_new_str_contains(3, "foo", true, CX_STR_LOCATION_ANY),
         CX_PREDICATE_MATCH_UNKNOWN},
        {cx_predicate_new_str_contains(3, "cx 0", true, CX_STR_LOCATION_ANY),
         CX_PREDICATE_MATCH_UNKNOWN},
        {cx_predicate_new_str_contains(3, "cx 10", true, CX_STR_LOCATION_ANY),
         CX_PREDICATE_MATCH_NO_ROWS}};

    return test_indexes(fixture, test_cases, sizeof(test_cases));
}

static MunitResult test_str_match_rows(const MunitParameter params[],
                                       void *fixture)
{
    struct cx_predicate_row_test_case test_cases[] = {
        {cx_predicate_new_true(), all_rows},

        {cx_predicate_new_str_eq(3, "cx", false), 0},
        {cx_predicate_new_str_eq(3, "cx 0", false), 0x1},
        {cx_predicate_new_str_eq(3, "CX 1", false), 0x2},
        {cx_predicate_new_str_eq(3, "CX 1", true), 0},

        {cx_predicate_new_str_lt(3, "cx", false), 0},
        {cx_predicate_new_str_gt(3, "cx", false), all_rows},
        {cx_predicate_new_str_lt(3, "abc", false), 0},
        {cx_predicate_new_str_gt(3, "abc", false), all_rows},
        {cx_predicate_new_str_lt(3, "zzz", false), all_rows},
        {cx_predicate_new_str_gt(3, "zzz", false), 0},

        // contains @ start
        {cx_predicate_new_str_contains(3, "cx", true, CX_STR_LOCATION_START),
         all_rows},
        {cx_predicate_new_str_contains(3, "cx", false, CX_STR_LOCATION_START),
         all_rows},
        {cx_predicate_new_str_contains(3, "CX", false, CX_STR_LOCATION_START),
         all_rows},
        {cx_predicate_new_str_contains(3, "CX", true, CX_STR_LOCATION_START),
         0},
        {cx_predicate_new_str_contains(3, "foo", false, CX_STR_LOCATION_START),
         0},
        {cx_predicate_new_str_contains(3, "cx 0", false, CX_STR_LOCATION_START),
         0x1},
        {cx_predicate_new_str_contains(3, "cx 0", true, CX_STR_LOCATION_START),
         0x1},
        {cx_predicate_new_str_contains(3, "cx 1", false, CX_STR_LOCATION_START),
         0x2},
        {cx_predicate_new_str_contains(3, "cx 2", false, CX_STR_LOCATION_START),
         0x4},

        // contains @ any
        {cx_predicate_new_str_contains(3, "cx", true, CX_STR_LOCATION_ANY),
         all_rows},
        {cx_predicate_new_str_contains(3, "cx", false, CX_STR_LOCATION_ANY),
         all_rows},
        {cx_predicate_new_str_contains(3, "CX", false, CX_STR_LOCATION_ANY),
         all_rows},
        {cx_predicate_new_str_contains(3, "CX", true, CX_STR_LOCATION_ANY), 0},
        {cx_predicate_new_str_contains(3, "foo", false, CX_STR_LOCATION_ANY),
         0},
        {cx_predicate_new_str_contains(3, "cx 0", false, CX_STR_LOCATION_ANY),
         0x1},
        {cx_predicate_new_str_contains(3, "cx 1", false, CX_STR_LOCATION_ANY),
         0x2},
        {cx_predicate_new_str_contains(3, "cx 2", false, CX_STR_LOCATION_ANY),
         0x4},

        // contains @ end
        {cx_predicate_new_str_contains(3, "cx", true, CX_STR_LOCATION_END), 0},
        {cx_predicate_new_str_contains(3, "cx", false, CX_STR_LOCATION_END), 0},
        {cx_predicate_new_str_contains(3, "CX", false, CX_STR_LOCATION_END), 0},
        {cx_predicate_new_str_contains(3, "CX", true, CX_STR_LOCATION_END), 0},
        {cx_predicate_new_str_contains(3, "foo", false, CX_STR_LOCATION_END),
         0},
        {cx_predicate_new_str_contains(3, "0", false, CX_STR_LOCATION_END),
         0x1},
        {cx_predicate_new_str_contains(3, "CX 0", false, CX_STR_LOCATION_END),
         0x1},
        {cx_predicate_new_str_contains(3, "1", false, CX_STR_LOCATION_END),
         0x2},
        {cx_predicate_new_str_contains(3, "2", false, CX_STR_LOCATION_END),
         0x4},
    };

    return test_rows(fixture, test_cases, sizeof(test_cases));
}

static MunitResult test_null_match_index(const MunitParameter params[],
                                         void *fixture)
{
    struct cx_predicate_index_test_case test_cases[] = {
        {cx_predicate_new_null(0), CX_PREDICATE_MATCH_UNKNOWN},
        {cx_predicate_new_null(1), CX_PREDICATE_MATCH_UNKNOWN},
        {cx_predicate_new_null(2), CX_PREDICATE_MATCH_ALL_ROWS},
        {cx_predicate_negate(cx_predicate_new_null(2)),
         CX_PREDICATE_MATCH_NO_ROWS},
        {cx_predicate_new_null(3), CX_PREDICATE_MATCH_NO_ROWS},
        {cx_predicate_negate(cx_predicate_new_null(3)),
         CX_PREDICATE_MATCH_ALL_ROWS},
    };

    return test_indexes(fixture, test_cases, sizeof(test_cases));
}

static MunitResult test_null_match_rows(const MunitParameter params[],
                                        void *fixture)
{
    struct cx_predicate_row_test_case test_cases[] = {
        {cx_predicate_new_true(), all_rows},
        {cx_predicate_new_null(0), 0x155},  // 0b0101010101
        {cx_predicate_new_null(1), 0x249},  // 0b1001001001
        {cx_predicate_new_null(2), all_rows},
        {cx_predicate_new_null(3), 0}};

    return test_rows(fixture, test_cases, sizeof(test_cases));
}

static MunitResult test_optimize(const MunitParameter params[], void *ptr)
{
    struct cx_predicate_fixture *fixture = ptr;

    struct cx_predicate *p_true = cx_predicate_new_true();
    assert_not_null(p_true);
    struct cx_predicate *p_i32 = cx_predicate_new_i32_eq(0, 10);
    assert_not_null(p_i32);
    struct cx_predicate *p_i64 = cx_predicate_new_i64_eq(1, 100);
    assert_not_null(p_i64);
    struct cx_predicate *p_bit = cx_predicate_new_bit_eq(2, false);
    assert_not_null(p_bit);
    struct cx_predicate *p_str = cx_predicate_new_str_eq(3, "foo", true);
    assert_not_null(p_str);

    struct cx_predicate *p_and = cx_predicate_new_and(2, p_i32, p_bit);
    assert_not_null(p_and);

    struct cx_predicate *p_or =
        cx_predicate_new_or(4, p_i64, p_str, p_true, p_and);
    assert_not_null(p_or);

    cx_predicate_optimize(p_or, fixture->row_group);

    size_t operand_count;
    const struct cx_predicate **operands =
        cx_predicate_operands(p_or, &operand_count);
    assert_not_null(operands);
    assert_size(operand_count, ==, 4);
    assert_ptr_equal(operands[0], p_true);
    assert_ptr_equal(operands[1], p_and);
    assert_ptr_equal(operands[2], p_i64);
    assert_ptr_equal(operands[3], p_str);

    operands = cx_predicate_operands(p_and, &operand_count);
    assert_not_null(operands);
    assert_size(operand_count, ==, 2);
    assert_ptr_equal(operands[0], p_bit);
    assert_ptr_equal(operands[1], p_i32);

    // noops:
    cx_predicate_optimize(p_true, fixture->row_group);
    cx_predicate_optimize(p_i32, fixture->row_group);
    cx_predicate_optimize(p_i64, fixture->row_group);

    cx_predicate_free(p_or);

    return MUNIT_OK;
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
    {"/null-match-index", test_null_match_index, setup, teardown,
     MUNIT_TEST_OPTION_NONE, NULL},
    {"/null-match-rows", test_null_match_rows, setup, teardown,
     MUNIT_TEST_OPTION_NONE, NULL},
    {"/optimize", test_optimize, setup, teardown, MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL}};
