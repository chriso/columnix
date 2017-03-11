#include <stdio.h>

#include "row.h"

#include "helpers.h"

#define COLUMN_COUNT 4
#define ROW_COUNT 100

struct cx_row_fixture {
    struct cx_row_group *row_group;
    struct cx_column *columns[COLUMN_COUNT];
    struct cx_column *nulls[COLUMN_COUNT];
    struct cx_row_cursor *cursor;
    struct cx_predicate *predicate;
};

static void *setup(const MunitParameter params[], void *data)
{
    struct cx_row_fixture *fixture = malloc(sizeof(*fixture));
    assert_not_null(fixture);

    fixture->row_group = cx_row_group_new();
    assert_not_null(fixture->row_group);

    enum cx_column_type types[] = {CX_COLUMN_I32, CX_COLUMN_I64, CX_COLUMN_BIT,
                                   CX_COLUMN_STR};

    for (size_t i = 0; i < COLUMN_COUNT; i++) {
        fixture->columns[i] = cx_column_new(types[i], CX_ENCODING_NONE);
        assert_not_null(fixture->columns[i]);
        fixture->nulls[i] = cx_column_new(CX_COLUMN_BIT, CX_ENCODING_NONE);
        assert_not_null(fixture->nulls[i]);
    }

    char buffer[64];
    for (size_t i = 0; i < ROW_COUNT; i++) {
        assert_true(cx_column_put_i32(fixture->columns[0], i));
        assert_true(cx_column_put_i64(fixture->columns[1], i * 10));
        assert_true(cx_column_put_bit(fixture->columns[2], i % 3 == 0));
        sprintf(buffer, "cx %zu", i);
        assert_true(cx_column_put_str(fixture->columns[3], buffer));

        assert_true(cx_column_put_bit(fixture->nulls[0], i % 2 == 0));
        assert_true(cx_column_put_bit(fixture->nulls[1], i % 3 == 0));
        assert_true(cx_column_put_bit(fixture->nulls[2], true));
        assert_true(cx_column_put_bit(fixture->nulls[3], false));
    }

    for (size_t i = 0; i < COLUMN_COUNT; i++)
        assert_true(cx_row_group_add_column(
            fixture->row_group, fixture->columns[i], fixture->nulls[i]));

    fixture->predicate = cx_predicate_new_true();
    assert_not_null(fixture->predicate);

    fixture->cursor = cx_row_cursor_new(fixture->row_group, fixture->predicate);
    assert_not_null(fixture->cursor);

    return fixture;
}

static void teardown(void *ptr)
{
    struct cx_row_fixture *fixture = ptr;
    cx_row_cursor_free(fixture->cursor);
    cx_predicate_free(fixture->predicate);
    for (size_t i = 0; i < COLUMN_COUNT; i++) {
        cx_column_free(fixture->columns[i]);
        cx_column_free(fixture->nulls[i]);
    }
    cx_row_group_free(fixture->row_group);
    free(fixture);
}

static void test_cursor_position(struct cx_row_cursor *cursor, size_t expected)
{
    int32_t i32;
    assert_true(cx_row_cursor_get_i32(cursor, 0, &i32));
    assert_int32(i32, ==, expected);

    int64_t i64;
    assert_true(cx_row_cursor_get_i64(cursor, 1, &i64));
    assert_int64(i64, ==, expected * 10);

    bool bit;
    assert_true(cx_row_cursor_get_bit(cursor, 2, &bit));
    if (expected % 3 == 0)
        assert_true(bit);
    else
        assert_false(bit);

    bool null;
    assert_true(cx_row_cursor_get_null(cursor, 0, &null));
    if (expected % 2 == 0)
        assert_true(null);
    else
        assert_false(null);
    assert_true(cx_row_cursor_get_null(cursor, 1, &null));
    if (expected % 3 == 0)
        assert_true(null);
    else
        assert_false(null);
    assert_true(cx_row_cursor_get_null(cursor, 2, &null));
    assert_true(null);
    assert_true(cx_row_cursor_get_null(cursor, 3, &null));
    assert_false(null);

    const struct cx_string *string;
    assert_true(cx_row_cursor_get_str(cursor, 3, &string));
    char buffer[64];
    sprintf(buffer, "cx %zu", expected);
    assert_int(string->len, ==, strlen(buffer));
    assert_string_equal(buffer, string->ptr);
}

static MunitResult test_count(const MunitParameter params[], void *ptr)
{
    struct cx_row_fixture *fixture = ptr;
    size_t count = cx_row_cursor_count(fixture->cursor);
    assert_size(count, ==, ROW_COUNT);
    assert_false(cx_row_cursor_error(fixture->cursor));
    return MUNIT_OK;
}

static MunitResult test_cursor(const MunitParameter params[], void *ptr)
{
    struct cx_row_fixture *fixture = ptr;
    size_t position = 0;
    while (cx_row_cursor_next(fixture->cursor))
        position++;
    assert_size(position, ==, ROW_COUNT);
    assert_false(cx_row_cursor_error(fixture->cursor));
    for (size_t repeats = 0; repeats < 10; repeats++) {
        cx_row_cursor_rewind(fixture->cursor);
        for (position = 0; cx_row_cursor_next(fixture->cursor); position++) {
            test_cursor_position(fixture->cursor, position);
            test_cursor_position(fixture->cursor, position);
        }
        assert_size(position, ==, ROW_COUNT);
        assert_false(cx_row_cursor_error(fixture->cursor));
    }
    return MUNIT_OK;
}

static MunitResult test_cursor_matching(const MunitParameter params[],
                                        void *ptr)
{
    struct cx_row_fixture *fixture = ptr;

    struct cx_predicate *predicate = cx_predicate_new_and(
        4, cx_predicate_new_i32_gt(0, 20), cx_predicate_new_i64_lt(1, 900),
        cx_predicate_new_bit_eq(2, true),
        cx_predicate_new_str_contains(3, "0", false, CX_STR_LOCATION_END));
    assert_not_null(predicate);

    struct cx_row_cursor *cursor =
        cx_row_cursor_new(fixture->row_group, predicate);
    assert_not_null(cursor);

    size_t position, expected[] = {30, 60};

    CX_FOREACH(expected, position)
    {
        assert_true(cx_row_cursor_next(cursor));
        test_cursor_position(cursor, position);
    }
    assert_false(cx_row_cursor_next(cursor));
    assert_false(cx_row_cursor_error(cursor));

    cx_row_cursor_free(cursor);
    cx_predicate_free(predicate);

    return MUNIT_OK;
}

static MunitResult test_count_matching(const MunitParameter params[], void *ptr)
{
    struct cx_row_fixture *fixture = ptr;

    struct cx_predicate *predicate = cx_predicate_new_and(
        4, cx_predicate_new_i32_gt(0, 20), cx_predicate_new_i64_lt(1, 900),
        cx_predicate_new_bit_eq(2, true),
        cx_predicate_new_str_contains(3, "0", false, CX_STR_LOCATION_END));
    assert_not_null(predicate);

    struct cx_row_cursor *cursor =
        cx_row_cursor_new(fixture->row_group, predicate);
    assert_not_null(cursor);

    size_t count = cx_row_cursor_count(cursor);
    assert_size(count, ==, 2);

    assert_false(cx_row_cursor_error(cursor));

    cx_row_cursor_free(cursor);
    cx_predicate_free(predicate);

    return MUNIT_OK;
}

static MunitResult test_empty_row_group(const MunitParameter params[],
                                        void *ptr)
{
    struct cx_row_fixture *fixture = ptr;

    struct cx_row_group *row_group = cx_row_group_new();
    assert_not_null(row_group);
    struct cx_row_cursor *cursor =
        cx_row_cursor_new(row_group, fixture->predicate);
    assert_not_null(cursor);

    size_t count = cx_row_cursor_count(cursor);
    assert_size(count, ==, 0);
    assert_false(cx_row_cursor_error(cursor));

    cx_row_cursor_rewind(cursor);
    assert_false(cx_row_cursor_next(cursor));
    assert_false(cx_row_cursor_error(cursor));

    cx_row_cursor_free(cursor);
    cx_row_group_free(row_group);
    return MUNIT_OK;
}

MunitTest row_tests[] = {
    {"/cursor", test_cursor, setup, teardown, MUNIT_TEST_OPTION_NONE, NULL},
    {"/count", test_count, setup, teardown, MUNIT_TEST_OPTION_NONE, NULL},
    {"/cursor-matching", test_cursor_matching, setup, teardown,
     MUNIT_TEST_OPTION_NONE, NULL},
    {"/count-matching", test_count_matching, setup, teardown,
     MUNIT_TEST_OPTION_NONE, NULL},
    {"/empty-row-group", test_empty_row_group, setup, teardown,
     MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL}};
