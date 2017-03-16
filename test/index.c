#define __STDC_LIMIT_MACROS
#include <stdint.h>
#include <stdio.h>

#include "index.h"

#include "helpers.h"

static MunitResult test_bit_index(const MunitParameter params[], void *fixture)
{
    struct cx_column *col = cx_column_new(CX_COLUMN_BIT, CX_ENCODING_NONE);
    assert_not_null(col);

    struct cx_index *index = cx_index_new(col);
    assert_uint64(index->count, ==, 0);
    cx_index_free(index);

    assert_true(cx_column_put_bit(col, false));
    index = cx_index_new(col);
    assert_uint64(index->count, ==, 1);
    assert_uint64(index->min.bit, ==, false);
    assert_uint64(index->max.bit, ==, false);
    cx_index_free(index);

    assert_true(cx_column_put_bit(col, false));
    index = cx_index_new(col);
    assert_uint64(index->count, ==, 2);
    assert_uint64(index->min.bit, ==, false);
    assert_uint64(index->max.bit, ==, false);
    cx_index_free(index);

    assert_true(cx_column_put_bit(col, true));
    index = cx_index_new(col);
    assert_uint64(index->count, ==, 3);
    assert_uint64(index->min.bit, ==, false);
    assert_uint64(index->max.bit, ==, true);
    cx_index_free(index);

    cx_column_free(col);

    col = cx_column_new(CX_COLUMN_BIT, CX_ENCODING_NONE);
    assert_not_null(col);

    index = cx_index_new(col);
    assert_uint64(index->count, ==, 0);
    cx_index_free(index);

    assert_true(cx_column_put_bit(col, true));
    index = cx_index_new(col);
    assert_uint64(index->count, ==, 1);
    assert_uint64(index->min.bit, ==, true);
    assert_uint64(index->max.bit, ==, true);
    cx_index_free(index);

    assert_true(cx_column_put_bit(col, true));
    index = cx_index_new(col);
    assert_uint64(index->count, ==, 2);
    assert_uint64(index->min.bit, ==, true);
    assert_uint64(index->max.bit, ==, true);
    cx_index_free(index);

    assert_true(cx_column_put_bit(col, false));
    index = cx_index_new(col);
    assert_uint64(index->count, ==, 3);
    assert_uint64(index->min.bit, ==, false);
    assert_uint64(index->max.bit, ==, true);
    cx_index_free(index);

    cx_column_free(col);
    return MUNIT_OK;
}

static MunitResult test_i32_index(const MunitParameter params[], void *fixture)
{
    struct cx_column *col = cx_column_new(CX_COLUMN_I32, CX_ENCODING_NONE);
    assert_not_null(col);

    struct cx_index *index = cx_index_new(col);
    assert_uint64(index->count, ==, 0);
    cx_index_free(index);

    assert_true(cx_column_put_i32(col, 10));
    index = cx_index_new(col);
    assert_uint64(index->count, ==, 1);
    assert_uint64(index->min.i32, ==, 10);
    assert_uint64(index->max.i32, ==, 10);
    cx_index_free(index);

    assert_true(cx_column_put_i32(col, 20));
    index = cx_index_new(col);
    assert_uint64(index->count, ==, 2);
    assert_uint64(index->min.i32, ==, 10);
    assert_uint64(index->max.i32, ==, 20);
    cx_index_free(index);

    assert_true(cx_column_put_i32(col, 15));
    index = cx_index_new(col);
    assert_uint64(index->count, ==, 3);
    assert_uint64(index->min.i32, ==, 10);
    assert_uint64(index->max.i32, ==, 20);
    cx_index_free(index);

    assert_true(cx_column_put_i32(col, INT32_MAX));
    index = cx_index_new(col);
    assert_uint64(index->count, ==, 4);
    assert_uint64(index->min.i32, ==, 10);
    assert_uint64(index->max.i32, ==, INT32_MAX);
    cx_index_free(index);

    assert_true(cx_column_put_i32(col, 0));
    index = cx_index_new(col);
    assert_uint64(index->count, ==, 5);
    assert_uint64(index->min.i32, ==, 0);
    assert_uint64(index->max.i32, ==, INT32_MAX);
    cx_index_free(index);

    assert_true(cx_column_put_i32(col, INT32_MIN));
    index = cx_index_new(col);
    assert_uint64(index->count, ==, 6);
    assert_uint64(index->min.i32, ==, INT32_MIN);
    assert_uint64(index->max.i32, ==, INT32_MAX);
    cx_index_free(index);

    cx_column_free(col);
    return MUNIT_OK;
}

static MunitResult test_i64_index(const MunitParameter params[], void *fixture)
{
    struct cx_column *col = cx_column_new(CX_COLUMN_I64, CX_ENCODING_NONE);
    assert_not_null(col);

    struct cx_index *index = cx_index_new(col);
    assert_uint64(index->count, ==, 0);
    cx_index_free(index);

    assert_true(cx_column_put_i64(col, 10));
    index = cx_index_new(col);
    assert_uint64(index->count, ==, 1);
    assert_uint64(index->min.i64, ==, 10);
    assert_uint64(index->max.i64, ==, 10);
    cx_index_free(index);

    assert_true(cx_column_put_i64(col, 20));
    index = cx_index_new(col);
    assert_uint64(index->count, ==, 2);
    assert_uint64(index->min.i64, ==, 10);
    assert_uint64(index->max.i64, ==, 20);
    cx_index_free(index);

    assert_true(cx_column_put_i64(col, 15));
    index = cx_index_new(col);
    assert_uint64(index->count, ==, 3);
    assert_uint64(index->min.i64, ==, 10);
    assert_uint64(index->max.i64, ==, 20);
    cx_index_free(index);

    assert_true(cx_column_put_i64(col, INT32_MAX));
    index = cx_index_new(col);
    assert_uint64(index->count, ==, 4);
    assert_uint64(index->min.i64, ==, 10);
    assert_uint64(index->max.i64, ==, INT32_MAX);
    cx_index_free(index);

    assert_true(cx_column_put_i64(col, 0));
    index = cx_index_new(col);
    assert_uint64(index->count, ==, 5);
    assert_uint64(index->min.i64, ==, 0);
    assert_uint64(index->max.i64, ==, INT32_MAX);
    cx_index_free(index);

    assert_true(cx_column_put_i64(col, INT32_MIN));
    index = cx_index_new(col);
    assert_uint64(index->count, ==, 6);
    assert_uint64(index->min.i64, ==, INT32_MIN);
    assert_uint64(index->max.i64, ==, INT32_MAX);
    cx_index_free(index);

    assert_true(cx_column_put_i64(col, INT64_MAX));
    index = cx_index_new(col);
    assert_uint64(index->count, ==, 7);
    assert_uint64(index->min.i64, ==, INT32_MIN);
    assert_uint64(index->max.i64, ==, INT64_MAX);
    cx_index_free(index);

    assert_true(cx_column_put_i64(col, INT64_MIN));
    index = cx_index_new(col);
    assert_uint64(index->count, ==, 8);
    assert_uint64(index->min.i64, ==, INT64_MIN);
    assert_uint64(index->max.i64, ==, INT64_MAX);
    cx_index_free(index);

    cx_column_free(col);
    return MUNIT_OK;
}

static MunitResult test_str_index(const MunitParameter params[], void *fixture)
{
    struct cx_column *col = cx_column_new(CX_COLUMN_STR, CX_ENCODING_NONE);
    assert_not_null(col);

    struct cx_index *index = cx_index_new(col);
    assert_uint64(index->count, ==, 0);
    cx_index_free(index);

    assert_true(cx_column_put_str(col, "foo"));
    index = cx_index_new(col);
    assert_uint64(index->count, ==, 1);
    assert_uint64(index->min.len, ==, 3);
    assert_uint64(index->max.len, ==, 3);
    cx_index_free(index);

    assert_true(cx_column_put_str(col, "foobar"));
    index = cx_index_new(col);
    assert_uint64(index->count, ==, 2);
    assert_uint64(index->min.len, ==, 3);
    assert_uint64(index->max.len, ==, 6);
    cx_index_free(index);

    assert_true(cx_column_put_str(col, "yeah"));
    index = cx_index_new(col);
    assert_uint64(index->count, ==, 3);
    assert_uint64(index->min.len, ==, 3);
    assert_uint64(index->max.len, ==, 6);
    cx_index_free(index);

    assert_true(cx_column_put_str(col, "foobarbaz"));
    index = cx_index_new(col);
    assert_uint64(index->count, ==, 4);
    assert_uint64(index->min.len, ==, 3);
    assert_uint64(index->max.len, ==, 9);
    cx_index_free(index);

    assert_true(cx_column_put_str(col, ""));
    index = cx_index_new(col);
    assert_uint64(index->count, ==, 5);
    assert_uint64(index->min.len, ==, 0);
    assert_uint64(index->max.len, ==, 9);
    cx_index_free(index);

    cx_column_free(col);
    return MUNIT_OK;
}

MunitTest index_tests[] = {
    {"/bit-index", test_bit_index, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/i32-index", test_i32_index, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/i64-index", test_i64_index, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/str-index", test_str_index, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL}};
