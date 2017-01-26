#define MUNIT_ENABLE_ASSERT_ALIASES
#include "munit.h"

#include "row_group.h"

static MunitResult test_add_column(const MunitParameter params[], void *fixture)
{
    struct zcs_row_group *row_group = zcs_row_group_new();
    assert_not_null(row_group);

    struct zcs_column *columns[3];
    size_t count = sizeof(columns) / sizeof(*columns);

    columns[0] = zcs_column_new(ZCS_COLUMN_I32, ZCS_ENCODE_NONE);
    assert_not_null(columns[0]);
    assert_true(zcs_column_put_i32(columns[0], 10));
    assert_true(zcs_column_put_i32(columns[0], 20));

    columns[1] = zcs_column_new(ZCS_COLUMN_I64, ZCS_ENCODE_NONE);
    assert_not_null(columns[1]);
    assert_true(zcs_column_put_i64(columns[1], 30));

    columns[2] = zcs_column_new(ZCS_COLUMN_BIT, ZCS_ENCODE_NONE);
    assert_not_null(columns[2]);
    assert_true(zcs_column_put_bit(columns[2], true));

    for (size_t i = 0; i < count; i++)
        assert_true(zcs_row_group_add_column(row_group, columns[i]));

    assert_size(zcs_row_group_column_count(row_group), ==, count);

    assert_int(zcs_row_group_column_type(row_group, 0), ==, ZCS_COLUMN_I32);
    assert_int(zcs_row_group_column_type(row_group, 1), ==, ZCS_COLUMN_I64);
    assert_int(zcs_row_group_column_type(row_group, 2), ==, ZCS_COLUMN_BIT);

    assert_int(zcs_row_group_column_encode(row_group, 0), ==, ZCS_ENCODE_NONE);
    assert_int(zcs_row_group_column_encode(row_group, 1), ==, ZCS_ENCODE_NONE);
    assert_int(zcs_row_group_column_encode(row_group, 2), ==, ZCS_ENCODE_NONE);

    assert_ptr_equal(zcs_row_group_column_index(row_group, 0),
                     zcs_column_index(columns[0]));
    assert_ptr_equal(zcs_row_group_column_index(row_group, 1),
                     zcs_column_index(columns[1]));
    assert_ptr_equal(zcs_row_group_column_index(row_group, 2),
                     zcs_column_index(columns[2]));

    assert_ptr_equal(zcs_row_group_column(row_group, 0), columns[0]);
    assert_ptr_equal(zcs_row_group_column(row_group, 1), columns[1]);
    assert_ptr_equal(zcs_row_group_column(row_group, 2), columns[2]);

    for (size_t i = 0; i < count; i++)
        zcs_column_free(columns[i]);

    zcs_row_group_free(row_group);
    return MUNIT_OK;
}

MunitTest row_group_tests[] = {
    {"/add-column", test_add_column, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL}};
