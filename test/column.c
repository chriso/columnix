#include <limits.h>

#define MUNIT_ENABLE_ASSERT_ALIASES
#include "munit.h"

#include "column.h"

#define COUNT 4

static void *setup_i32(const MunitParameter params[], void *data)
{
    struct zcs_column *col = zcs_column_new(ZCS_COLUMN_I32, ZCS_ENCODE_NONE);
    assert_not_null(col);
    for (int32_t i = 0; i < COUNT; i++)
        assert_true(zcs_column_put_i32(col, i));
    assert_int(zcs_column_type(col), ==, ZCS_COLUMN_I32);
    assert_int(zcs_column_encode(col), ==, ZCS_ENCODE_NONE);
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

static MunitResult test_i32_index(const MunitParameter params[], void *fixture)
{
    struct zcs_column *col = zcs_column_new(ZCS_COLUMN_I32, ZCS_ENCODE_NONE);
    assert_not_null(col);

    const struct zcs_column_index *index = zcs_column_index(col);
    assert_uint64(index->count, ==, 0);

    assert_true(zcs_column_put_i32(col, 10));
    assert_uint64(index->count, ==, 1);
    assert_uint64(index->min.i32, ==, 10);
    assert_uint64(index->max.i32, ==, 10);

    assert_true(zcs_column_put_i32(col, 20));
    assert_uint64(index->count, ==, 2);
    assert_uint64(index->min.i32, ==, 10);
    assert_uint64(index->max.i32, ==, 20);

    assert_true(zcs_column_put_i32(col, 15));
    assert_uint64(index->count, ==, 3);
    assert_uint64(index->min.i32, ==, 10);
    assert_uint64(index->max.i32, ==, 20);

    assert_true(zcs_column_put_i32(col, INT_MAX));
    assert_uint64(index->count, ==, 4);
    assert_uint64(index->min.i32, ==, 10);
    assert_uint64(index->max.i32, ==, INT_MAX);

    assert_true(zcs_column_put_i32(col, 0));
    assert_uint64(index->count, ==, 5);
    assert_uint64(index->min.i32, ==, 0);
    assert_uint64(index->max.i32, ==, INT_MAX);

    zcs_column_free(col);
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

static MunitResult test_i32_cursor_batching(const MunitParameter params[],
                                            void *fixture)
{
    struct zcs_column *col = (struct zcs_column *)fixture;
    struct zcs_column_cursor *cursor = zcs_column_cursor_new(col);
    assert_not_null(cursor);

    size_t batch_size[] = {1, 8, 13, 64, 234, COUNT / 2, COUNT, COUNT + 1};
    for (size_t i = 0; i < sizeof(batch_size) / sizeof(*batch_size); i++) {
        size_t position = 0, count;
        while (zcs_column_cursor_valid(cursor)) {
            const int32_t *values =
                zcs_column_cursor_next_batch_i32(cursor, batch_size[i], &count);
            for (int32_t j = 0; j < count; j++)
                assert_int32(values[j], ==, j + position);
            position += count;
            if (!zcs_column_cursor_valid(cursor))
                break;
        }
        assert_size(position, ==, COUNT);
        zcs_column_cursor_rewind(cursor);
    }

    zcs_column_cursor_free(cursor);
    return MUNIT_OK;
}

static MunitResult test_i32_cursor_skipping(const MunitParameter params[],
                                            void *fixture)
{
    struct zcs_column *col = (struct zcs_column *)fixture;
    struct zcs_column_cursor *cursor = zcs_column_cursor_new(col);
    assert_not_null(cursor);

    size_t skip_size[] = {1, 8, 13, 64, 234, COUNT / 2, COUNT, COUNT + 1};
    for (size_t i = 0; i < sizeof(skip_size) / sizeof(*skip_size); i++) {
        size_t position = 0;
        while (zcs_column_cursor_valid(cursor)) {
            position += zcs_column_cursor_skip_i32(cursor, skip_size[i]);
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
    {"/i32-index", test_i32_index, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/i32-cursor", test_i32_cursor, setup_i32, teardown,
     MUNIT_TEST_OPTION_NONE, NULL},
    {"/i32-cursor-batching", test_i32_cursor_batching, setup_i32, teardown,
     MUNIT_TEST_OPTION_NONE, NULL},
    {"/i32-cursor-skipping", test_i32_cursor_skipping, setup_i32, teardown,
     MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL}};
