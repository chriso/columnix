#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "column.h"

#include "helpers.h"

#define COUNT 1111

static void *setup_bit(const MunitParameter params[], void *data)
{
    struct cx_column *col = cx_column_new(CX_COLUMN_BIT, CX_ENCODING_NONE);
    assert_not_null(col);
    for (int32_t i = 0; i < COUNT; i++)
        assert_true(cx_column_put_bit(col, i % 5 == 0));
    assert_int(cx_column_type(col), ==, CX_COLUMN_BIT);
    assert_int(cx_column_encoding(col), ==, CX_ENCODING_NONE);
    return col;
}

static void *setup_i32(const MunitParameter params[], void *data)
{
    struct cx_column *col = cx_column_new(CX_COLUMN_I32, CX_ENCODING_NONE);
    assert_not_null(col);
    for (int32_t i = 0; i < COUNT; i++)
        assert_true(cx_column_put_i32(col, i));
    assert_int(cx_column_type(col), ==, CX_COLUMN_I32);
    assert_int(cx_column_encoding(col), ==, CX_ENCODING_NONE);
    return col;
}

static void *setup_i64(const MunitParameter params[], void *data)
{
    struct cx_column *col = cx_column_new(CX_COLUMN_I64, CX_ENCODING_NONE);
    assert_not_null(col);
    for (int32_t i = 0; i < COUNT; i++)
        assert_true(cx_column_put_i64(col, i));
    assert_int(cx_column_type(col), ==, CX_COLUMN_I64);
    assert_int(cx_column_encoding(col), ==, CX_ENCODING_NONE);
    return col;
}

static void *setup_str(const MunitParameter params[], void *data)
{
    struct cx_column *col = cx_column_new(CX_COLUMN_STR, CX_ENCODING_NONE);
    assert_not_null(col);
    char buffer[64];
    for (size_t i = 0; i < COUNT; i++) {
        sprintf(buffer, "cx %zu", i);
        assert_true(cx_column_put_str(col, buffer));
    }
    assert_int(cx_column_type(col), ==, CX_COLUMN_STR);
    assert_int(cx_column_encoding(col), ==, CX_ENCODING_NONE);
    return col;
}

static void teardown(void *fixture)
{
    cx_column_free((struct cx_column *)fixture);
}

static MunitResult test_export(const MunitParameter params[], void *fixture)
{
    struct cx_column *col = (struct cx_column *)fixture;
    size_t size;
    const int32_t *buf = cx_column_export(col, &size);
    assert_not_null(buf);
    assert_size(size, ==, sizeof(int32_t) * COUNT);
    for (int32_t i = 0; i < COUNT; i++)
        assert_int32(i, ==, buf[i]);
    return MUNIT_OK;
}

static void assert_i32_col_equal(const struct cx_column *a,
                                 const struct cx_column *b)
{
    assert_not_null(a);
    assert_not_null(b);
    size_t a_size, b_size;
    const void *a_ptr = cx_column_export(a, &a_size);
    const void *b_ptr = cx_column_export(b, &b_size);
    assert_not_null(a_ptr);
    assert_not_null(b_ptr);
    assert_size(a_size, ==, b_size);
    assert_memory_equal(a_size, a_ptr, b_ptr);
}

static MunitResult test_import_mmapped(const MunitParameter params[],
                                       void *fixture)
{
    struct cx_column *col = (struct cx_column *)fixture;
    size_t size;
    const void *ptr = cx_column_export(col, &size);
    assert_not_null(ptr);
    struct cx_column *copy = cx_column_new_mmapped(
        CX_COLUMN_I32, CX_ENCODING_NONE, ptr, size, cx_column_count(col));
    assert_i32_col_equal(col, copy);
    assert_false(cx_column_put_i32(copy, 0));  // mmapped
    cx_column_free(copy);
    return MUNIT_OK;
}

static MunitResult test_import_compressed(const MunitParameter params[],
                                          void *fixture)
{
    struct cx_column *col = (struct cx_column *)fixture;
    size_t size;
    const void *ptr = cx_column_export(col, &size);
    assert_not_null(ptr);
    void *dest;
    struct cx_column *copy = cx_column_new_compressed(
        CX_COLUMN_I32, CX_ENCODING_NONE, &dest, size, cx_column_count(col));
    assert_not_null(copy);
    memcpy(dest, ptr, size);
    assert_i32_col_equal(col, copy);
    assert_true(cx_column_put_i32(copy, 0));  // mutable
    cx_column_free(copy);
    return MUNIT_OK;
}

static MunitResult test_bit_put_mismatch(const MunitParameter params[],
                                         void *fixture)
{
    struct cx_column *col = (struct cx_column *)fixture;
    assert_false(cx_column_put_i64(col, 1));
    return MUNIT_OK;
}

static MunitResult test_bit_cursor(const MunitParameter params[], void *fixture)
{
    struct cx_column *col = (struct cx_column *)fixture;
    struct cx_column_cursor *cursor = cx_column_cursor_new(col);
    assert_not_null(cursor);

    size_t position, count;
    size_t starting_positions[] = {0, 64, 256, COUNT - (COUNT % 64)};

    CX_FOREACH(starting_positions, position)
    {
        assert_size(cx_column_cursor_skip_bit(cursor, position), ==, position);
        while (cx_column_cursor_valid(cursor)) {
            const uint64_t *bitset =
                cx_column_cursor_next_batch_bit(cursor, &count);
            for (size_t i = 0; i < count; i++) {
                bool bit = *bitset & ((uint64_t)1 << i);
                if ((i + position) % 5 == 0)
                    assert_true(bit);
                else
                    assert_false(bit);
            }
            position += count;
        }
        assert_size(position, ==, COUNT);
        cx_column_cursor_rewind(cursor);
    }

    cx_column_cursor_free(cursor);
    return MUNIT_OK;
}

static MunitResult test_i32_put_mismatch(const MunitParameter params[],
                                         void *fixture)
{
    struct cx_column *col = (struct cx_column *)fixture;
    assert_false(cx_column_put_i64(col, 1));
    return MUNIT_OK;
}

static MunitResult test_i32_cursor(const MunitParameter params[], void *fixture)
{
    struct cx_column *col = (struct cx_column *)fixture;
    struct cx_column_cursor *cursor = cx_column_cursor_new(col);
    assert_not_null(cursor);

    size_t position, count;
    size_t starting_positions[] = {0, 1, 8, 13, 64, 234, COUNT / 2 + 1, COUNT};

    CX_FOREACH(starting_positions, position)
    {
        assert_size(cx_column_cursor_skip_i32(cursor, position), ==, position);
        while (cx_column_cursor_valid(cursor)) {
            const int32_t *values =
                cx_column_cursor_next_batch_i32(cursor, &count);
            for (int32_t j = 0; j < count; j++)
                assert_int32(values[j], ==, j + position);
            position += count;
        }
        assert_size(position, ==, COUNT);
        cx_column_cursor_rewind(cursor);
    }

    cx_column_cursor_free(cursor);
    return MUNIT_OK;
}

static MunitResult test_i64_put_mismatch(const MunitParameter params[],
                                         void *fixture)
{
    struct cx_column *col = (struct cx_column *)fixture;
    assert_false(cx_column_put_i32(col, 1));
    return MUNIT_OK;
}

static MunitResult test_i64_cursor(const MunitParameter params[], void *fixture)
{
    struct cx_column *col = (struct cx_column *)fixture;
    struct cx_column_cursor *cursor = cx_column_cursor_new(col);
    assert_not_null(cursor);

    size_t position, count;
    size_t starting_positions[] = {0, 1, 8, 13, 64, 234, COUNT / 2 + 1, COUNT};

    CX_FOREACH(starting_positions, position)
    {
        assert_size(cx_column_cursor_skip_i64(cursor, position), ==, position);
        while (cx_column_cursor_valid(cursor)) {
            const int64_t *values =
                cx_column_cursor_next_batch_i64(cursor, &count);
            for (int64_t j = 0; j < count; j++)
                assert_int64(values[j], ==, j + position);
            position += count;
        }
        assert_size(position, ==, COUNT);
        cx_column_cursor_rewind(cursor);
    }

    cx_column_cursor_free(cursor);
    return MUNIT_OK;
}

static MunitResult test_str_put_mismatch(const MunitParameter params[],
                                         void *fixture)
{
    struct cx_column *col = (struct cx_column *)fixture;
    assert_false(cx_column_put_i32(col, 1));
    return MUNIT_OK;
}

static MunitResult test_str_cursor(const MunitParameter params[], void *fixture)
{
    struct cx_column *col = (struct cx_column *)fixture;
    struct cx_column_cursor *cursor = cx_column_cursor_new(col);
    assert_not_null(cursor);

    char expected[64];
    size_t position, count;
    size_t starting_positions[] = {0, 1, 8, 13, 64, COUNT - 1, COUNT};

    CX_FOREACH(starting_positions, position)
    {
        assert_size(cx_column_cursor_skip_str(cursor, position), ==, position);
        while (cx_column_cursor_valid(cursor)) {
            const struct cx_string *strings =
                cx_column_cursor_next_batch_str(cursor, &count);
            for (size_t j = 0; j < count; j++) {
                sprintf(expected, "cx %zu", j + position);
                assert_int(strings[j].len, ==, strlen(expected));
                assert_string_equal(expected, strings[j].ptr);
            }
            position += count;
        }
        assert_size(position, ==, COUNT);
        cx_column_cursor_rewind(cursor);
    }

    cx_column_cursor_free(cursor);
    return MUNIT_OK;
}

MunitTest column_tests[] = {
    {"/export", test_export, setup_i32, teardown, MUNIT_TEST_OPTION_NONE, NULL},
    {"/import-mmapped", test_import_mmapped, setup_i32, teardown,
     MUNIT_TEST_OPTION_NONE, NULL},
    {"/import-compressed", test_import_compressed, setup_i32, teardown,
     MUNIT_TEST_OPTION_NONE, NULL},
    {"/bit-put-mismatch", test_bit_put_mismatch, setup_bit, teardown,
     MUNIT_TEST_OPTION_NONE, NULL},
    {"/bit-cursor", test_bit_cursor, setup_bit, teardown,
     MUNIT_TEST_OPTION_NONE, NULL},
    {"/i32-put-mismatch", test_i32_put_mismatch, setup_i32, teardown,
     MUNIT_TEST_OPTION_NONE, NULL},
    {"/i32-cursor", test_i32_cursor, setup_i32, teardown,
     MUNIT_TEST_OPTION_NONE, NULL},
    {"/i64-put-mismatch", test_i64_put_mismatch, setup_i64, teardown,
     MUNIT_TEST_OPTION_NONE, NULL},
    {"/i64-cursor", test_i64_cursor, setup_i64, teardown,
     MUNIT_TEST_OPTION_NONE, NULL},
    {"/str-put-mismatch", test_str_put_mismatch, setup_str, teardown,
     MUNIT_TEST_OPTION_NONE, NULL},
    {"/str-cursor", test_str_cursor, setup_str, teardown,
     MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL}};
