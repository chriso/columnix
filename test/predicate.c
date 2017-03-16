#include <stdio.h>

#include "predicate.h"

#include "helpers.h"

#define COLUMN_COUNT 10
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
                                   CX_COLUMN_BIT, CX_COLUMN_BIT, CX_COLUMN_I32,
                                   CX_COLUMN_I32};

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

        assert_true(cx_column_put_i32(fixture->columns[8], -10));
        assert_true(cx_column_put_i32(fixture->columns[9], (i & 0x1) ? -1 : 1));

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

enum cx_predicate_match cx_custom_i32_polarity_match_index(
    enum cx_column_type type, const struct cx_index *index, void *data)
{
    bool negative = *(bool *)data;
    assert_int(type, ==, CX_COLUMN_I32);
    if (index->max.i32 <= 0)
        return negative ? CX_PREDICATE_MATCH_ALL_ROWS
                        : CX_PREDICATE_MATCH_NO_ROWS;
    if (index->min.i32 >= 0)
        return negative ? CX_PREDICATE_MATCH_NO_ROWS
                        : CX_PREDICATE_MATCH_ALL_ROWS;
    return CX_PREDICATE_MATCH_UNKNOWN;
}

static MunitResult test_custom_match_index(const MunitParameter params[],
                                           void *fixture)
{
    bool negative = true, positive = false;
    cx_predicate_match_index_t match = cx_custom_i32_polarity_match_index;

    struct cx_predicate_index_test_case test_cases[] = {
        {cx_predicate_new_custom(0, CX_COLUMN_I32, NULL, match, 0, &negative),
         CX_PREDICATE_MATCH_NO_ROWS},
        {cx_predicate_new_custom(0, CX_COLUMN_I32, NULL, match, 0, &positive),
         CX_PREDICATE_MATCH_ALL_ROWS},
        {cx_predicate_new_custom(8, CX_COLUMN_I32, NULL, match, 0, &negative),
         CX_PREDICATE_MATCH_ALL_ROWS},
        {cx_predicate_new_custom(8, CX_COLUMN_I32, NULL, match, 0, &positive),
         CX_PREDICATE_MATCH_NO_ROWS},
        {cx_predicate_new_custom(9, CX_COLUMN_I32, NULL, match, 0, &negative),
         CX_PREDICATE_MATCH_UNKNOWN},
        {cx_predicate_new_custom(9, CX_COLUMN_I32, NULL, match, 0, &positive),
         CX_PREDICATE_MATCH_UNKNOWN},
    };

    return test_indexes(fixture, test_cases, sizeof(test_cases));
}

bool cx_custom_i32_polarity_match_rows(enum cx_column_type type, size_t count,
                                       const void *raw_values,
                                       uint64_t *matches, void *data)
{
    assert_int(type, ==, CX_COLUMN_I32);
    bool negative = *(bool *)data;
    const int32_t *values = raw_values;
    uint64_t mask = 0;
    for (size_t i = 0; i < count; i++)
        if (negative ^ (values[i] >= 0))
            mask |= (uint64_t)1 << i;
    *matches = mask;
    return true;
}

static MunitResult test_custom_match_rows(const MunitParameter params[],
                                          void *fixture)
{
    bool negative = true, positive = false;
    cx_predicate_match_rows_t match = cx_custom_i32_polarity_match_rows;

    struct cx_predicate_row_test_case test_cases[] = {
        {cx_predicate_new_custom(0, CX_COLUMN_I32, match, NULL, 0, &negative),
         0},
        {cx_predicate_new_custom(0, CX_COLUMN_I32, match, NULL, 0, &positive),
         all_rows},
        {cx_predicate_new_custom(8, CX_COLUMN_I32, match, NULL, 0, &negative),
         all_rows},
        {cx_predicate_new_custom(8, CX_COLUMN_I32, match, NULL, 0, &positive),
         0},
        {cx_predicate_new_custom(9, CX_COLUMN_I32, match, NULL, 0, &negative),
         0x2aa},  // 0b1010101010
        {cx_predicate_new_custom(9, CX_COLUMN_I32, match, NULL, 0, &positive),
         0x155},  // 0b0101010101
    };

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

    struct cx_predicate *p_custom_i32 =
        cx_predicate_new_custom(0, CX_COLUMN_I32, NULL, NULL, -1, NULL);
    assert_not_null(p_custom_i32);

    struct cx_predicate *p_custom_high_cost =
        cx_predicate_new_custom(0, CX_COLUMN_STR, NULL, NULL, 999, NULL);
    assert_not_null(p_custom_high_cost);

    struct cx_predicate *p_or = cx_predicate_new_or(
        6, p_custom_i32, p_custom_high_cost, p_i64, p_str, p_true, p_and);
    assert_not_null(p_or);

    cx_predicate_optimize(p_or, fixture->row_group);

    size_t operand_count;
    const struct cx_predicate **operands =
        cx_predicate_operands(p_or, &operand_count);
    assert_not_null(operands);
    assert_size(operand_count, ==, 6);
    assert_ptr_equal(operands[0], p_true);
    assert_ptr_equal(operands[1], p_custom_i32);
    assert_ptr_equal(operands[2], p_and);
    assert_ptr_equal(operands[3], p_i64);
    assert_ptr_equal(operands[4], p_str);
    assert_ptr_equal(operands[5], p_custom_high_cost);

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
    {"/custom-match-index", test_custom_match_index, setup, teardown,
     MUNIT_TEST_OPTION_NONE, NULL},
    {"/custom-match-rows", test_custom_match_rows, setup, teardown,
     MUNIT_TEST_OPTION_NONE, NULL},
    {"/optimize", test_optimize, setup, teardown, MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL}};
