#define MUNIT_ENABLE_ASSERT_ALIASES
#include "munit.h"

#include "column.h"

#define COUNT 1024

static void *setup_i32(const MunitParameter params[], void *data)
{
    struct zcs_column *col = zcs_column_new(ZCS_COLUMN_I32, ZCS_ENCODE_NONE);
    assert_not_null(col);
    for (int32_t i = 0; i < COUNT; i++)
        assert_true(zcs_column_put_i32(col, i));
    return col;
}

static void teardown(void *fixture)
{
    zcs_column_free((struct zcs_column *)fixture);
}

static MunitResult test_i32_put_mismatch(const MunitParameter params[],
                                         void *fixture)
{
    struct zcs_column *col = (struct zcs_column *)fixture;
    assert_false(zcs_column_put_i64(col, 1));
    return MUNIT_OK;
}

static MunitResult test_i32_export(const MunitParameter params[], void *fixture)
{
    struct zcs_column *col = (struct zcs_column *)fixture;
    size_t size;
    const int32_t *buf = zcs_column_export(col, &size);
    assert_size(size, ==, sizeof(int32_t) * COUNT);
    for (int32_t i = 0; i < COUNT; i++)
        assert_int32(i, ==, buf[i]);
    return MUNIT_OK;
}

static MunitResult test_i32_cursor(const MunitParameter params[], void *fixture)
{
    struct zcs_column *col = (struct zcs_column *)fixture;
    struct zcs_column_cursor *cursor = zcs_column_cursor_new(col);
    assert_not_null(cursor);

    size_t position = 0;
    for (; zcs_column_cursor_valid(cursor); position++)
        assert_int32(zcs_column_cursor_next_i32(cursor), ==, position);
    assert_size(position, ==, COUNT);

    zcs_column_cursor_free(cursor);
    return MUNIT_OK;
}

static MunitResult test_i32_cursor_skipping(const MunitParameter params[],
                                            void *fixture)
{
    struct zcs_column *col = (struct zcs_column *)fixture;
    struct zcs_column_cursor *cursor = zcs_column_cursor_new(col);
    assert_not_null(cursor);

    size_t skip_by[] = {1, 8, 13, 64, 100, 234, 10000};
    for (size_t i = 0; i < sizeof(skip_by) / sizeof(*skip_by); i++) {
        size_t position = 0;
        while (zcs_column_cursor_valid(cursor)) {
            position += zcs_column_cursor_skip_i32(cursor, skip_by[i]);
            if (!zcs_column_cursor_valid(cursor))
                break;
            assert_int32(zcs_column_cursor_next_i32(cursor), ==, position);
            position++;
        }
        assert_size(position, ==, COUNT);
        zcs_column_cursor_rewind(cursor);
    }

    zcs_column_cursor_free(cursor);
    return MUNIT_OK;
}

MunitTest column_tests[] = {
    {"/i32-put-mismatch", test_i32_put_mismatch, setup_i32, teardown,
     MUNIT_TEST_OPTION_NONE, NULL},
    {"/i32-export", test_i32_export, setup_i32, teardown,
     MUNIT_TEST_OPTION_NONE, NULL},
    {"/i32-cursor", test_i32_cursor, setup_i32, teardown,
     MUNIT_TEST_OPTION_NONE, NULL},
    {"/i32-cursor-skipping", test_i32_cursor_skipping, setup_i32, teardown,
     MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL}};
