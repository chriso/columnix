#define MUNIT_ENABLE_ASSERT_ALIASES
#include "munit.h"

#include "column.h"

static MunitResult test_i32(const MunitParameter params[], void* data)
{
    struct zcs_column *col = zcs_column_new(ZCS_COLUMN_I32, 0);
    assert_not_null(col);

    size_t count = 1024;
    for (size_t i = 0; i < count; i++)
        assert_true(zcs_column_put_i32(col, (int32_t)i));

    assert_false(zcs_column_put_i64(col, 1)); // type mismatch

    size_t size;
    const int32_t *buf = zcs_column_export(col, &size);
    assert_size(size, ==, sizeof(int32_t) * count);
    for (size_t i = 0; i < count; i++)
        assert_int32((int32_t)i, ==, buf[i]);

    struct zcs_column_cursor *cursor = zcs_column_cursor_new(col);
    assert_not_null(cursor);

    size_t position = 0;
    for (; zcs_column_cursor_valid(cursor); position++)
        assert_int32(zcs_column_cursor_next_i32(cursor), ==, position);
    assert_size(position, ==, count);

    zcs_column_cursor_free(cursor);
    zcs_column_free(col);
    return MUNIT_OK;
}

MunitTest column_tests[] = {
    {"/i32", test_i32, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL}};
