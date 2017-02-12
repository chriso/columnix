#include <stdio.h>

#include "row_group.h"

#define MUNIT_ENABLE_ASSERT_ALIASES
#include "munit.h"

#define COLUMN_COUNT 4
#define ROW_COUNT 111

struct zcs_row_group_fixture {
    struct zcs_row_group *row_group;
    struct zcs_column *columns[COLUMN_COUNT];
};

static void *setup(const MunitParameter params[], void *data)
{
    struct zcs_row_group_fixture *fixture = malloc(sizeof(*fixture));
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

    return fixture;
}

static void teardown(void *ptr)
{
    struct zcs_row_group_fixture *fixture = ptr;
    for (size_t i = 0; i < COLUMN_COUNT; i++)
        zcs_column_free(fixture->columns[i]);
    zcs_row_group_free(fixture->row_group);
    free(fixture);
}

static MunitResult test_add_column(const MunitParameter params[], void *ptr)
{
    struct zcs_row_group_fixture *fixture = ptr;
    struct zcs_row_group *row_group = fixture->row_group;

    assert_size(zcs_row_group_column_count(row_group), ==, 0);
    assert_size(zcs_row_group_row_count(row_group), ==, 0);

    for (size_t i = 0; i < COLUMN_COUNT; i++)
        assert_true(zcs_row_group_add_column(row_group, fixture->columns[i]));

    assert_size(zcs_row_group_column_count(row_group), ==, COLUMN_COUNT);
    assert_size(zcs_row_group_row_count(row_group), ==, ROW_COUNT);

    for (size_t i = 0; i < COLUMN_COUNT; i++) {
        struct zcs_column *column = fixture->columns[i];
        assert_ptr_equal(zcs_row_group_column(row_group, i), column);
        assert_int(zcs_row_group_column_type(row_group, i), ==,
                   zcs_column_type(column));
        assert_int(zcs_row_group_column_encode(row_group, i), ==,
                   zcs_column_encode(column));
        assert_ptr_equal(zcs_row_group_column_index(row_group, i),
                         zcs_column_index(column));
    }

    return MUNIT_OK;
}

static MunitResult test_row_count_mismatch(const MunitParameter params[],
                                           void *ptr)
{
    struct zcs_row_group_fixture *fixture = ptr;
    struct zcs_row_group *row_group = fixture->row_group;
    assert_true(zcs_row_group_add_column(row_group, fixture->columns[0]));
    assert_true(zcs_column_put_i64(fixture->columns[1], 30));
    assert_false(zcs_row_group_add_column(row_group, fixture->columns[1]));
    return MUNIT_OK;
}

MunitTest row_group_tests[] = {
    {"/add-column", test_add_column, setup, teardown, MUNIT_TEST_OPTION_NONE,
     NULL},
    {"/row-count-mismatch", test_row_count_mismatch, setup, teardown,
     MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL}};
