#include <stdio.h>

#include "predicate.h"

#include "helpers.h"

#define COLUMN_COUNT 4
#define ROW_COUNT 10

static const uint64_t all_rows = (1 << ROW_COUNT) - 1;

struct zcs_predicate_fixture {
    struct zcs_row_group *row_group;
    struct zcs_column *columns[COLUMN_COUNT];
    struct zcs_row_group_cursor *cursor;
};

struct zcs_predicate_test_case {
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

    for (size_t i = 0; i < COLUMN_COUNT; i++)
        assert_not_null(fixture->columns[i]);

    char buffer[64];
    for (size_t i = 0; i < ROW_COUNT; i++) {
        assert_true(zcs_column_put_i32(fixture->columns[0], i));
        assert_true(zcs_column_put_i64(fixture->columns[1], i * 10));
        assert_true(zcs_column_put_bit(fixture->columns[2], i % 3 == 0));
        sprintf(buffer, "zcs %zu", i);
        assert_true(zcs_column_put_str(fixture->columns[3], buffer));
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

static MunitResult test_i32_match_rows(const MunitParameter params[], void *ptr)
{
    struct zcs_predicate_fixture *fixture = ptr;

    // the i32 column has values [0..9]
    struct zcs_predicate_test_case test_case;
    struct zcs_predicate_test_case test_cases[] = {
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
        // (col != 3) => hex(0b1111110111)
        {zcs_predicate_negate(zcs_predicate_new_i32_eq(0, 3)), 0x3F7},
        // (col > 2 && col < 8) => hex(0b0011111000)
        {zcs_predicate_new_and(2, zcs_predicate_new_i32_gt(0, 2),
                               zcs_predicate_new_i32_lt(0, 8)),
         0xF8},
        // (col < 2 || col > 8) => hex(0b1000000011)
        {zcs_predicate_new_or(2, zcs_predicate_new_i32_lt(0, 2),
                              zcs_predicate_new_i32_gt(0, 8)),
         0x203},
    };

    size_t count;
    uint64_t matches;

    ZCS_FOREACH(test_cases, test_case)
    {
        assert_not_null(test_case.predicate);
        assert_true(
            zcs_predicate_match_rows(test_case.predicate, fixture->row_group,
                                     fixture->cursor, &matches, &count));
        assert_size(count, ==, ROW_COUNT);
        assert_uint64(matches, ==, test_case.expected);
        zcs_predicate_free(test_case.predicate);
    }

    return MUNIT_OK;
}

MunitTest predicate_tests[] = {
    {"/i32-match-rows", test_i32_match_rows, setup, teardown,
     MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL}};
