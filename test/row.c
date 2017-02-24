#include <stdio.h>

#include "row.h"

#include "helpers.h"

#define COLUMN_COUNT 4
#define ROW_COUNT 100

struct zcs_row_fixture {
    struct zcs_row_group *row_group;
    struct zcs_column *columns[COLUMN_COUNT];
    struct zcs_row_cursor *cursor;
    struct zcs_predicate *predicate;
};

static void *setup(const MunitParameter params[], void *data)
{
    struct zcs_row_fixture *fixture = malloc(sizeof(*fixture));
    assert_not_null(fixture);

    fixture->row_group = zcs_row_group_new();
    assert_not_null(fixture->row_group);

    fixture->columns[0] = zcs_column_new(ZCS_COLUMN_I32, ZCS_ENCODING_NONE);
    fixture->columns[1] = zcs_column_new(ZCS_COLUMN_I64, ZCS_ENCODING_NONE);
    fixture->columns[2] = zcs_column_new(ZCS_COLUMN_BIT, ZCS_ENCODING_NONE);
    fixture->columns[3] = zcs_column_new(ZCS_COLUMN_STR, ZCS_ENCODING_NONE);

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
        assert_true(zcs_row_group_add_column(fixture->row_group,
                                             fixture->columns[i], NULL));

    fixture->predicate = zcs_predicate_new_true();
    assert_not_null(fixture->predicate);

    fixture->cursor =
        zcs_row_cursor_new(fixture->row_group, fixture->predicate);
    assert_not_null(fixture->cursor);

    return fixture;
}

static void teardown(void *ptr)
{
    struct zcs_row_fixture *fixture = ptr;
    zcs_row_cursor_free(fixture->cursor);
    zcs_predicate_free(fixture->predicate);
    for (size_t i = 0; i < COLUMN_COUNT; i++)
        zcs_column_free(fixture->columns[i]);
    zcs_row_group_free(fixture->row_group);
    free(fixture);
}

static void test_cursor_position(struct zcs_row_cursor *cursor, size_t expected)
{
    int32_t i32;
    assert_true(zcs_row_cursor_get_i32(cursor, 0, &i32));
    assert_int32(i32, ==, expected);

    int64_t i64;
    assert_true(zcs_row_cursor_get_i64(cursor, 1, &i64));
    assert_int64(i64, ==, expected * 10);

    bool bit;
    assert_true(zcs_row_cursor_get_bit(cursor, 2, &bit));
    if (expected % 3 == 0)
        assert_true(bit);
    else
        assert_false(bit);

    const struct zcs_string *string;
    assert_true(zcs_row_cursor_get_str(cursor, 3, &string));
    char buffer[64];
    sprintf(buffer, "zcs %zu", expected);
    assert_int(string->len, ==, strlen(buffer));
    assert_string_equal(buffer, string->ptr);
}

static MunitResult test_count(const MunitParameter params[], void *ptr)
{
    struct zcs_row_fixture *fixture = ptr;
    size_t count = zcs_row_cursor_count(fixture->cursor);
    assert_size(count, ==, ROW_COUNT);
    assert_false(zcs_row_cursor_error(fixture->cursor));
    return MUNIT_OK;
}

static MunitResult test_cursor(const MunitParameter params[], void *ptr)
{
    struct zcs_row_fixture *fixture = ptr;
    size_t position = 0;
    while (zcs_row_cursor_next(fixture->cursor))
        position++;
    assert_size(position, ==, ROW_COUNT);
    assert_false(zcs_row_cursor_error(fixture->cursor));
    for (size_t repeats = 0; repeats < 10; repeats++) {
        zcs_row_cursor_rewind(fixture->cursor);
        for (position = 0; zcs_row_cursor_next(fixture->cursor); position++) {
            test_cursor_position(fixture->cursor, position);
            test_cursor_position(fixture->cursor, position);
        }
        assert_size(position, ==, ROW_COUNT);
        assert_false(zcs_row_cursor_error(fixture->cursor));
    }
    return MUNIT_OK;
}

static MunitResult test_cursor_matching(const MunitParameter params[],
                                        void *ptr)
{
    struct zcs_row_fixture *fixture = ptr;

    struct zcs_predicate *predicate = zcs_predicate_new_and(
        4, zcs_predicate_new_i32_gt(0, 20), zcs_predicate_new_i64_lt(1, 900),
        zcs_predicate_new_bit_eq(2, true),
        zcs_predicate_new_str_contains(3, "0", false, ZCS_STR_LOCATION_END));
    assert_not_null(predicate);

    struct zcs_row_cursor *cursor =
        zcs_row_cursor_new(fixture->row_group, predicate);
    assert_not_null(cursor);

    size_t position, expected[] = {30, 60};

    ZCS_FOREACH(expected, position)
    {
        assert_true(zcs_row_cursor_next(cursor));
        test_cursor_position(cursor, position);
    }
    assert_false(zcs_row_cursor_next(cursor));
    assert_false(zcs_row_cursor_error(cursor));

    zcs_row_cursor_free(cursor);
    zcs_predicate_free(predicate);

    return MUNIT_OK;
}

static MunitResult test_count_matching(const MunitParameter params[], void *ptr)
{
    struct zcs_row_fixture *fixture = ptr;

    struct zcs_predicate *predicate = zcs_predicate_new_and(
        4, zcs_predicate_new_i32_gt(0, 20), zcs_predicate_new_i64_lt(1, 900),
        zcs_predicate_new_bit_eq(2, true),
        zcs_predicate_new_str_contains(3, "0", false, ZCS_STR_LOCATION_END));
    assert_not_null(predicate);

    struct zcs_row_cursor *cursor =
        zcs_row_cursor_new(fixture->row_group, predicate);
    assert_not_null(cursor);

    size_t count = zcs_row_cursor_count(cursor);
    assert_size(count, ==, 2);

    assert_false(zcs_row_cursor_error(cursor));

    zcs_row_cursor_free(cursor);
    zcs_predicate_free(predicate);

    return MUNIT_OK;
}

static MunitResult test_empty_row_group(const MunitParameter params[],
                                        void *ptr)
{
    struct zcs_row_fixture *fixture = ptr;

    struct zcs_row_group *row_group = zcs_row_group_new();
    assert_not_null(row_group);
    struct zcs_row_cursor *cursor =
        zcs_row_cursor_new(row_group, fixture->predicate);
    assert_not_null(cursor);

    size_t count = zcs_row_cursor_count(cursor);
    assert_size(count, ==, 0);
    assert_false(zcs_row_cursor_error(cursor));

    zcs_row_cursor_rewind(cursor);
    assert_false(zcs_row_cursor_next(cursor));
    assert_false(zcs_row_cursor_error(cursor));

    zcs_row_cursor_free(cursor);
    zcs_row_group_free(row_group);
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
