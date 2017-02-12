#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "column.h"

#define MUNIT_ENABLE_ASSERT_ALIASES
#include "helpers.h"
#include "munit.h"

#define COUNT 1111

static void *setup_bit(const MunitParameter params[], void *data)
{
    struct zcs_column *col = zcs_column_new(ZCS_COLUMN_BIT, ZCS_ENCODE_NONE);
    assert_not_null(col);
    for (int32_t i = 0; i < COUNT; i++)
        assert_true(zcs_column_put_bit(col, i % 5 == 0));
    assert_int(zcs_column_type(col), ==, ZCS_COLUMN_BIT);
    assert_int(zcs_column_encode(col), ==, ZCS_ENCODE_NONE);
    return col;
}

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

static void *setup_i64(const MunitParameter params[], void *data)
{
    struct zcs_column *col = zcs_column_new(ZCS_COLUMN_I64, ZCS_ENCODE_NONE);
    assert_not_null(col);
    for (int32_t i = 0; i < COUNT; i++)
        assert_true(zcs_column_put_i64(col, i));
    assert_int(zcs_column_type(col), ==, ZCS_COLUMN_I64);
    assert_int(zcs_column_encode(col), ==, ZCS_ENCODE_NONE);
    return col;
}

static void *setup_str(const MunitParameter params[], void *data)
{
    struct zcs_column *col = zcs_column_new(ZCS_COLUMN_STR, ZCS_ENCODE_NONE);
    assert_not_null(col);
    char buffer[64];
    for (size_t i = 0; i < COUNT; i++) {
        sprintf(buffer, "zcs %zu", i);
        assert_true(zcs_column_put_str(col, buffer));
    }
    assert_int(zcs_column_type(col), ==, ZCS_COLUMN_STR);
    assert_int(zcs_column_encode(col), ==, ZCS_ENCODE_NONE);
    return col;
}

static void teardown(void *fixture)
{
    zcs_column_free((struct zcs_column *)fixture);
}

static MunitResult test_export(const MunitParameter params[], void *fixture)
{
    struct zcs_column *col = (struct zcs_column *)fixture;
    size_t size;
    const int32_t *buf = zcs_column_export(col, &size);
    assert_not_null(buf);
    assert_size(size, ==, sizeof(int32_t) * COUNT);
    for (int32_t i = 0; i < COUNT; i++)
        assert_int32(i, ==, buf[i]);
    return MUNIT_OK;
}

static void assert_i32_col_equal(const struct zcs_column *a,
                                 const struct zcs_column *b)
{
    assert_not_null(a);
    assert_not_null(b);
    size_t a_size, b_size;
    const void *a_ptr = zcs_column_export(a, &a_size);
    const void *b_ptr = zcs_column_export(b, &b_size);
    assert_not_null(a_ptr);
    assert_not_null(b_ptr);
    assert_size(a_size, ==, b_size);
    assert_memory_equal(a_size, a_ptr, b_ptr);
    const struct zcs_column_index *a_index = zcs_column_index(a);
    const struct zcs_column_index *b_index = zcs_column_index(b);
    assert_ptr_not_equal(a_index, b_index);
    assert_memory_equal(sizeof(*a_index), a_index, b_index);
}

static MunitResult test_import_immutable(const MunitParameter params[],
                                         void *fixture)
{
    struct zcs_column *col = (struct zcs_column *)fixture;
    size_t size;
    const void *ptr = zcs_column_export(col, &size);
    assert_not_null(ptr);
    struct zcs_column *copy = zcs_column_new_immutable(
        ZCS_COLUMN_I32, ZCS_ENCODE_NONE, ptr, size, zcs_column_index(col));
    assert_i32_col_equal(col, copy);
    assert_false(zcs_column_put_i32(copy, 0));  // immutable
    zcs_column_free(copy);
    return MUNIT_OK;
}

static MunitResult test_import_compressed(const MunitParameter params[],
                                          void *fixture)
{
    struct zcs_column *col = (struct zcs_column *)fixture;
    size_t size;
    const void *ptr = zcs_column_export(col, &size);
    assert_not_null(ptr);
    void *dest;
    struct zcs_column *copy = zcs_column_new_compressed(
        ZCS_COLUMN_I32, ZCS_ENCODE_NONE, &dest, size, zcs_column_index(col));
    assert_not_null(copy);
    memcpy(dest, ptr, size);
    assert_i32_col_equal(col, copy);
    assert_true(zcs_column_put_i32(copy, 0));  // mutable
    zcs_column_free(copy);
    return MUNIT_OK;
}

static MunitResult test_bit_put_mismatch(const MunitParameter params[],
                                         void *fixture)
{
    struct zcs_column *col = (struct zcs_column *)fixture;
    assert_false(zcs_column_put_i64(col, 1));
    return MUNIT_OK;
}

static MunitResult test_bit_index(const MunitParameter params[], void *fixture)
{
    struct zcs_column *col = zcs_column_new(ZCS_COLUMN_BIT, ZCS_ENCODE_NONE);
    assert_not_null(col);

    const struct zcs_column_index *index = zcs_column_index(col);
    assert_uint64(index->count, ==, 0);

    assert_true(zcs_column_put_bit(col, false));
    assert_uint64(index->count, ==, 1);
    assert_uint64(index->min.bit, ==, false);
    assert_uint64(index->max.bit, ==, false);

    assert_true(zcs_column_put_bit(col, false));
    assert_uint64(index->count, ==, 2);
    assert_uint64(index->min.bit, ==, false);
    assert_uint64(index->max.bit, ==, false);

    assert_true(zcs_column_put_bit(col, true));
    assert_uint64(index->count, ==, 3);
    assert_uint64(index->min.bit, ==, false);
    assert_uint64(index->max.bit, ==, true);

    zcs_column_free(col);
    col = zcs_column_new(ZCS_COLUMN_BIT, ZCS_ENCODE_NONE);
    assert_not_null(col);

    index = zcs_column_index(col);
    assert_uint64(index->count, ==, 0);

    assert_true(zcs_column_put_bit(col, true));
    assert_uint64(index->count, ==, 1);
    assert_uint64(index->min.bit, ==, true);
    assert_uint64(index->max.bit, ==, true);

    assert_true(zcs_column_put_bit(col, true));
    assert_uint64(index->count, ==, 2);
    assert_uint64(index->min.bit, ==, true);
    assert_uint64(index->max.bit, ==, true);

    assert_true(zcs_column_put_bit(col, false));
    assert_uint64(index->count, ==, 3);
    assert_uint64(index->min.bit, ==, false);
    assert_uint64(index->max.bit, ==, true);

    zcs_column_free(col);
    return MUNIT_OK;
}

static MunitResult test_bit_cursor(const MunitParameter params[], void *fixture)
{
    struct zcs_column *col = (struct zcs_column *)fixture;
    struct zcs_column_cursor *cursor = zcs_column_cursor_new(col);
    assert_not_null(cursor);

    size_t position, count;
    size_t starting_positions[] = {0, 64, 256, COUNT - (COUNT %  64)};

    ZCS_FOREACH(starting_positions, position) {
        assert_true(zcs_column_cursor_jump_bit(cursor, position));
        while (zcs_column_cursor_valid(cursor)) {
            const uint64_t *bitset =
                zcs_column_cursor_next_batch_bit(cursor, &count);
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
        zcs_column_cursor_rewind(cursor);
    }

    zcs_column_cursor_free(cursor);
    return MUNIT_OK;
}

static MunitResult test_i32_put_mismatch(const MunitParameter params[],
                                         void *fixture)
{
    struct zcs_column *col = (struct zcs_column *)fixture;
    assert_false(zcs_column_put_i64(col, 1));
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

static MunitResult test_i32_cursor(const MunitParameter params[],
                                   void *fixture)
{
    struct zcs_column *col = (struct zcs_column *)fixture;
    struct zcs_column_cursor *cursor = zcs_column_cursor_new(col);
    assert_not_null(cursor);

    size_t position, count;
    size_t starting_positions[] = {0, 1, 8, 13, 64, 234, COUNT / 2 + 1, COUNT};

    ZCS_FOREACH(starting_positions, position) {
        assert_true(zcs_column_cursor_jump_i32(cursor, position));
        while (zcs_column_cursor_valid(cursor)) {
            const int32_t *values =
                zcs_column_cursor_next_batch_i32(cursor, &count);
            for (int32_t j = 0; j < count; j++)
                assert_int32(values[j], ==, j + position);
            position += count;
        }
        assert_size(position, ==, COUNT);
        zcs_column_cursor_rewind(cursor);
    }

    zcs_column_cursor_free(cursor);
    return MUNIT_OK;
}

static MunitResult test_i64_put_mismatch(const MunitParameter params[],
                                         void *fixture)
{
    struct zcs_column *col = (struct zcs_column *)fixture;
    assert_false(zcs_column_put_i32(col, 1));
    return MUNIT_OK;
}

static MunitResult test_i64_index(const MunitParameter params[], void *fixture)
{
    struct zcs_column *col = zcs_column_new(ZCS_COLUMN_I64, ZCS_ENCODE_NONE);
    assert_not_null(col);

    const struct zcs_column_index *index = zcs_column_index(col);
    assert_uint64(index->count, ==, 0);

    assert_true(zcs_column_put_i64(col, 10));
    assert_uint64(index->count, ==, 1);
    assert_uint64(index->min.i64, ==, 10);
    assert_uint64(index->max.i64, ==, 10);

    assert_true(zcs_column_put_i64(col, 20));
    assert_uint64(index->count, ==, 2);
    assert_uint64(index->min.i64, ==, 10);
    assert_uint64(index->max.i64, ==, 20);

    assert_true(zcs_column_put_i64(col, 15));
    assert_uint64(index->count, ==, 3);
    assert_uint64(index->min.i64, ==, 10);
    assert_uint64(index->max.i64, ==, 20);

    assert_true(zcs_column_put_i64(col, INT_MAX));
    assert_uint64(index->count, ==, 4);
    assert_uint64(index->min.i64, ==, 10);
    assert_uint64(index->max.i64, ==, INT_MAX);

    assert_true(zcs_column_put_i64(col, 0));
    assert_uint64(index->count, ==, 5);
    assert_uint64(index->min.i64, ==, 0);
    assert_uint64(index->max.i64, ==, INT_MAX);

    assert_true(zcs_column_put_i64(col, LLONG_MAX));
    assert_uint64(index->count, ==, 6);
    assert_uint64(index->min.i64, ==, 0);
    assert_uint64(index->max.i64, ==, LLONG_MAX);

    zcs_column_free(col);
    return MUNIT_OK;
}

static MunitResult test_i64_cursor(const MunitParameter params[],
                                   void *fixture)
{
    struct zcs_column *col = (struct zcs_column *)fixture;
    struct zcs_column_cursor *cursor = zcs_column_cursor_new(col);
    assert_not_null(cursor);

    size_t position, count;
    size_t starting_positions[] = {0, 1, 8, 13, 64, 234, COUNT / 2 + 1, COUNT};

    ZCS_FOREACH(starting_positions, position) {
        assert_true(zcs_column_cursor_jump_i64(cursor, position));
        while (zcs_column_cursor_valid(cursor)) {
            const int64_t *values =
                zcs_column_cursor_next_batch_i64(cursor, &count);
            for (int64_t j = 0; j < count; j++)
                assert_int64(values[j], ==, j + position);
            position += count;
        }
        assert_size(position, ==, COUNT);
        zcs_column_cursor_rewind(cursor);
    }

    zcs_column_cursor_free(cursor);
    return MUNIT_OK;
}

static MunitResult test_str_put_mismatch(const MunitParameter params[],
                                         void *fixture)
{
    struct zcs_column *col = (struct zcs_column *)fixture;
    assert_false(zcs_column_put_i32(col, 1));
    return MUNIT_OK;
}

static MunitResult test_str_index(const MunitParameter params[], void *fixture)
{
    struct zcs_column *col = zcs_column_new(ZCS_COLUMN_STR, ZCS_ENCODE_NONE);
    assert_not_null(col);

    const struct zcs_column_index *index = zcs_column_index(col);
    assert_uint64(index->count, ==, 0);

    assert_true(zcs_column_put_str(col, "foo"));
    assert_uint64(index->count, ==, 1);
    assert_uint64(index->min.len, ==, 3);
    assert_uint64(index->max.len, ==, 3);

    assert_true(zcs_column_put_str(col, "foobar"));
    assert_uint64(index->count, ==, 2);
    assert_uint64(index->min.len, ==, 3);
    assert_uint64(index->max.len, ==, 6);

    assert_true(zcs_column_put_str(col, "yeah"));
    assert_uint64(index->count, ==, 3);
    assert_uint64(index->min.len, ==, 3);
    assert_uint64(index->max.len, ==, 6);

    assert_true(zcs_column_put_str(col, "foobarbaz"));
    assert_uint64(index->count, ==, 4);
    assert_uint64(index->min.len, ==, 3);
    assert_uint64(index->max.len, ==, 9);

    assert_true(zcs_column_put_str(col, ""));
    assert_uint64(index->count, ==, 5);
    assert_uint64(index->min.len, ==, 0);
    assert_uint64(index->max.len, ==, 9);

    zcs_column_free(col);
    return MUNIT_OK;
}

static MunitResult test_str_cursor(const MunitParameter params[],
                                            void *fixture)
{
    struct zcs_column *col = (struct zcs_column *)fixture;
    struct zcs_column_cursor *cursor = zcs_column_cursor_new(col);
    assert_not_null(cursor);

    char expected[64];
    size_t position, count;
    size_t starting_positions[] = {0, 1, 8, 13, 64, COUNT - 1, COUNT};

    ZCS_FOREACH(starting_positions, position) {
        assert_true(zcs_column_cursor_jump_str(cursor, position));
        while (zcs_column_cursor_valid(cursor)) {
            const struct zcs_string *strings =
                zcs_column_cursor_next_batch_str(cursor, &count);
            for (size_t j = 0; j < count; j++) {
                sprintf(expected, "zcs %zu", j + position);
                assert_int(strings[j].len, ==, strlen(expected));
                assert_string_equal(expected, strings[j].ptr);
            }
            position += count;
        }
        assert_size(position, ==, COUNT);
        zcs_column_cursor_rewind(cursor);
    }

    zcs_column_cursor_free(cursor);
    return MUNIT_OK;
}

MunitTest column_tests[] = {
    {"/export", test_export, setup_i32, teardown, MUNIT_TEST_OPTION_NONE, NULL},
    {"/import-immutable", test_import_immutable, setup_i32, teardown,
     MUNIT_TEST_OPTION_NONE, NULL},
    {"/import-compressed", test_import_compressed, setup_i32, teardown,
     MUNIT_TEST_OPTION_NONE, NULL},
    {"/bit-put-mismatch", test_bit_put_mismatch, setup_bit, teardown,
     MUNIT_TEST_OPTION_NONE, NULL},
    {"/bit-index", test_bit_index, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/bit-cursor", test_bit_cursor, setup_bit, teardown,
     MUNIT_TEST_OPTION_NONE, NULL},
    {"/i32-put-mismatch", test_i32_put_mismatch, setup_i32, teardown,
     MUNIT_TEST_OPTION_NONE, NULL},
    {"/i32-index", test_i32_index, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/i32-cursor", test_i32_cursor, setup_i32, teardown,
     MUNIT_TEST_OPTION_NONE, NULL},
    {"/i64-put-mismatch", test_i64_put_mismatch, setup_i64, teardown,
     MUNIT_TEST_OPTION_NONE, NULL},
    {"/i64-index", test_i64_index, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/i64-cursor", test_i64_cursor, setup_i64, teardown,
     MUNIT_TEST_OPTION_NONE, NULL},
    {"/str-put-mismatch", test_str_put_mismatch, setup_str, teardown,
     MUNIT_TEST_OPTION_NONE, NULL},
    {"/str-index", test_str_index, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/str-cursor", test_str_cursor, setup_str, teardown,
     MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL}};
