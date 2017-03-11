#define _BSD_SOURCE
#include <stdio.h>

#include "reader.h"
#include "row.h"
#include "writer.h"

#include "helpers.h"
#include "temp_file.h"

#define COLUMN_COUNT 4
#define ROW_GROUP_COUNT 4
#define ROWS_PER_ROW_GROUP 25
#define ROW_COUNT (ROW_GROUP_COUNT * ROWS_PER_ROW_GROUP)

struct cx_file_fixture {
    char *temp_file;
    struct cx_predicate *true_predicate;
};

static enum cx_compression_type cx_compression_types[] = {
    CX_COMPRESSION_NONE, CX_COMPRESSION_LZ4, CX_COMPRESSION_LZ4HC,
    CX_COMPRESSION_ZSTD};

static void *setup(const MunitParameter params[], void *data)
{
    struct cx_file_fixture *fixture = malloc(sizeof(*fixture));
    assert_not_null(fixture);

    fixture->temp_file = cx_temp_file_new();
    assert_not_null(fixture->temp_file);

    fixture->true_predicate = cx_predicate_new_true();
    assert_not_null(fixture->true_predicate);

    return fixture;
}

static void teardown(void *ptr)
{
    struct cx_file_fixture *fixture = ptr;
    cx_temp_file_free(fixture->temp_file);
    cx_predicate_free(fixture->true_predicate);
    free(fixture);
}

static void count_rows(struct cx_row_cursor *cursor, pthread_mutex_t *mutex,
                       void *data)
{
    size_t *count = data;
    size_t row_group_count = cx_row_cursor_count(cursor);
    pthread_mutex_lock(mutex);
    *count += row_group_count;
    pthread_mutex_unlock(mutex);
}

static MunitResult test_read_write(const MunitParameter params[], void *ptr)
{
    struct cx_file_fixture *fixture = ptr;

    enum cx_compression_type compression;
    int level = 5;
    char buffer[64];

    enum cx_column_type types[] = {CX_COLUMN_I32, CX_COLUMN_I64, CX_COLUMN_BIT,
                                   CX_COLUMN_STR};

    CX_FOREACH(cx_compression_types, compression)
    {
        struct cx_writer *writer =
            cx_writer_new(fixture->temp_file, ROWS_PER_ROW_GROUP);
        assert_not_null(writer);

        for (size_t i = 0; i < COLUMN_COUNT; i++) {
            sprintf(buffer, "column %zu", i);
            assert_true(cx_writer_add_column(writer, buffer, types[i], 0,
                                             compression, level));
        }

        for (size_t i = 0; i < ROW_COUNT; i++) {
            assert_true(cx_writer_put_i32(writer, 0, i));
            assert_true(cx_writer_put_i64(writer, 1, i * 10));
            assert_true(cx_writer_put_bit(writer, 2, i % 3 == 0));
            sprintf(buffer, "cx %zu", i);

            if (i % 12 == 0)
                assert_true(cx_writer_put_null(writer, 3));
            else
                assert_true(cx_writer_put_str(writer, 3, buffer));
        }

        assert_true(cx_writer_finish(writer, true));

        assert_true(cx_writer_finish(writer, true));  // noop

        cx_writer_free(writer);

        // high-level reader
        struct cx_reader *reader = cx_reader_new(fixture->temp_file);
        assert_not_null(reader);
        assert_size(cx_reader_column_count(reader), ==, COLUMN_COUNT);
        assert_size(cx_reader_row_count(reader), ==, ROW_COUNT);
        for (size_t i = 0; i < COLUMN_COUNT; i++) {
            sprintf(buffer, "column %zu", i);
            const char *name = cx_reader_column_name(reader, i);
            assert_not_null(name);
            assert_string_equal(name, buffer);

            assert_int(cx_reader_column_type(reader, i), ==, types[i]);
            assert_int(cx_reader_column_encoding(reader, i), ==, 0);
            assert_int(cx_reader_column_compression(reader, i), ==,
                       compression);
        }
        size_t position = 0;
        for (; cx_reader_next(reader); position++) {
            int32_t i32;
            assert_true(cx_reader_get_i32(reader, 0, &i32));
            assert_int32(i32, ==, position);
            int64_t i64;
            assert_true(cx_reader_get_i64(reader, 1, &i64));
            assert_int64(i64, ==, position * 10);
            bool bit;
            assert_true(cx_reader_get_bit(reader, 2, &bit));
            if (position % 3 == 0)
                assert_true(bit);
            else
                assert_false(bit);

            const struct cx_string *string;
            bool null;
            assert_true(cx_reader_get_null(reader, 3, &null));
            if (position % 12 == 0) {
                assert_true(null);
            } else {
                assert_false(null);
                assert_true(cx_reader_get_str(reader, 3, &string));
                sprintf(buffer, "cx %zu", position);
                assert_int(string->len, ==, strlen(buffer));
                assert_string_equal(buffer, string->ptr);
            }
        }
        assert_false(cx_reader_error(reader));
        assert_size(position, ==, ROW_COUNT);
        size_t count = 0;
        assert_true(cx_reader_query(reader, 4, (void *)&count, count_rows));
        assert_size(count, ==, ROW_COUNT);
        cx_reader_free(reader);

        // high-level reader matching rows
        struct cx_predicate *predicate = cx_predicate_new_and(
            5, cx_predicate_new_i32_gt(0, 20), cx_predicate_new_i64_lt(1, 900),
            cx_predicate_new_bit_eq(2, true),
            cx_predicate_negate(cx_predicate_new_null(3)),
            cx_predicate_new_str_contains(3, "0", false, CX_STR_LOCATION_END));
        assert_not_null(predicate);
        reader = cx_reader_new_matching(fixture->temp_file, predicate);
        assert_not_null(reader);
        assert_size(cx_reader_column_count(reader), ==, COLUMN_COUNT);
        assert_size(cx_reader_row_count(reader), ==, 1);
        assert_false(cx_reader_error(reader));
        cx_reader_rewind(reader);
        int32_t value;
        assert_true(cx_reader_next(reader));
        assert_true(cx_reader_get_i32(reader, 0, &value));
        assert_int(value, ==, 30);
        assert_false(cx_reader_next(reader));
        assert_false(cx_reader_error(reader));
        cx_reader_free(reader);

        // low-level reader
        struct cx_row_group_reader *row_group_reader =
            cx_row_group_reader_new(fixture->temp_file);
        assert_not_null(row_group_reader);
        assert_size(cx_row_group_reader_row_group_count(row_group_reader), ==,
                    ROW_GROUP_COUNT);
        assert_size(cx_row_group_reader_column_count(row_group_reader), ==,
                    COLUMN_COUNT);
        assert_size(cx_row_group_reader_row_count(row_group_reader), ==,
                    ROW_COUNT);
        for (size_t i = 0; i < COLUMN_COUNT; i++) {
            assert_int(cx_row_group_reader_column_type(row_group_reader, i), ==,
                       types[i]);
            assert_int(cx_row_group_reader_column_encoding(row_group_reader, i),
                       ==, 0);
            assert_int(
                cx_row_group_reader_column_compression(row_group_reader, i), ==,
                compression);
        }
        position = 0;
        for (size_t i = 0; i < ROW_GROUP_COUNT; i++) {
            struct cx_row_group *row_group =
                cx_row_group_reader_get(row_group_reader, i);
            assert_not_null(row_group);
            struct cx_row_cursor *cursor =
                cx_row_cursor_new(row_group, fixture->true_predicate);
            assert_not_null(cursor);
            for (; cx_row_cursor_next(cursor); position++) {
                int32_t i32;
                assert_true(cx_row_cursor_get_i32(cursor, 0, &i32));
                assert_int32(i32, ==, position);
                int64_t i64;
                assert_true(cx_row_cursor_get_i64(cursor, 1, &i64));
                assert_int64(i64, ==, position * 10);
                bool bit;
                assert_true(cx_row_cursor_get_bit(cursor, 2, &bit));
                if (position % 3 == 0)
                    assert_true(bit);
                else
                    assert_false(bit);
                bool null;
                assert_true(cx_row_cursor_get_null(cursor, 3, &null));
                if (position % 12 == 0) {
                    assert_true(null);
                } else {
                    assert_false(null);
                    const struct cx_string *string;
                    assert_true(cx_row_cursor_get_str(cursor, 3, &string));
                    char buffer[64];
                    sprintf(buffer, "cx %zu", position);
                    assert_int(string->len, ==, strlen(buffer));
                    assert_string_equal(buffer, string->ptr);
                }
            }
            cx_row_cursor_free(cursor);
            cx_row_group_free(row_group);
        }
        assert_size(position, ==, ROW_GROUP_COUNT * ROWS_PER_ROW_GROUP);
        cx_row_group_reader_free(row_group_reader);
    }

    return MUNIT_OK;
}

static MunitResult test_no_row_groups(const MunitParameter params[], void *ptr)
{
    struct cx_file_fixture *fixture = ptr;

    struct cx_row_group_writer *writer =
        cx_row_group_writer_new(fixture->temp_file);
    assert_not_null(writer);

    for (size_t i = 0; i < COLUMN_COUNT; i++)
        assert_true(cx_row_group_writer_add_column(writer, "foo", CX_COLUMN_I32,
                                                   0, CX_COMPRESSION_NONE, 0));

    assert_true(cx_row_group_writer_finish(writer, true));

    cx_row_group_writer_free(writer);

    // high-level reader
    struct cx_reader *reader = cx_reader_new(fixture->temp_file);
    assert_not_null(reader);
    assert_size(cx_reader_column_count(reader), ==, COLUMN_COUNT);
    assert_size(cx_reader_row_count(reader), ==, 0);
    for (size_t i = 0; i < COLUMN_COUNT; i++) {
        assert_int(cx_reader_column_type(reader, i), ==, CX_COLUMN_I32);
        assert_int(cx_reader_column_encoding(reader, i), ==, 0);
    }
    cx_reader_free(reader);

    // low-level reader
    struct cx_row_group_reader *row_group_reader =
        cx_row_group_reader_new(fixture->temp_file);
    assert_not_null(row_group_reader);
    assert_size(cx_row_group_reader_row_group_count(row_group_reader), ==, 0);
    assert_size(cx_row_group_reader_column_count(row_group_reader), ==,
                COLUMN_COUNT);
    assert_size(cx_row_group_reader_row_count(row_group_reader), ==, 0);
    for (size_t i = 0; i < COLUMN_COUNT; i++) {
        assert_int(cx_row_group_reader_column_type(row_group_reader, i), ==,
                   CX_COLUMN_I32);
        assert_int(cx_row_group_reader_column_encoding(row_group_reader, i), ==,
                   0);
    }
    cx_row_group_reader_free(row_group_reader);

    return MUNIT_OK;
}

static MunitResult test_no_columns(const MunitParameter params[], void *ptr)
{
    struct cx_file_fixture *fixture = ptr;
    struct cx_row_group_writer *writer =
        cx_row_group_writer_new(fixture->temp_file);
    assert_not_null(writer);
    assert_true(cx_row_group_writer_finish(writer, true));
    cx_row_group_writer_free(writer);

    // high-level reader
    struct cx_reader *reader = cx_reader_new(fixture->temp_file);
    assert_not_null(reader);
    assert_size(cx_reader_column_count(reader), ==, 0);
    assert_size(cx_reader_row_count(reader), ==, 0);
    assert_false(cx_reader_next(reader));
    assert_false(cx_reader_error(reader));
    cx_reader_free(reader);

    // low-level reader
    struct cx_row_group_reader *row_group_reader =
        cx_row_group_reader_new(fixture->temp_file);
    assert_not_null(row_group_reader);
    assert_size(cx_row_group_reader_row_group_count(row_group_reader), ==, 0);
    assert_size(cx_row_group_reader_column_count(row_group_reader), ==, 0);
    assert_size(cx_row_group_reader_row_count(row_group_reader), ==, 0);
    cx_row_group_reader_free(row_group_reader);

    return MUNIT_OK;
}

static MunitResult test_empty_columns(const MunitParameter params[], void *ptr)
{
    struct cx_file_fixture *fixture = ptr;

    struct cx_predicate *predicate = cx_predicate_new_true();
    assert_not_null(predicate);

    enum cx_compression_type compression;
    int level = 5;

    CX_FOREACH(cx_compression_types, compression)
    {
        struct cx_row_group_writer *writer =
            cx_row_group_writer_new(fixture->temp_file);
        assert_not_null(writer);
        struct cx_row_group *row_group = cx_row_group_new();
        assert_not_null(row_group);
        struct cx_column *column = cx_column_new(CX_COLUMN_I32, 0);
        assert_not_null(column);
        struct cx_column *nulls = cx_column_new(CX_COLUMN_BIT, 0);
        assert_not_null(nulls);
        assert_true(cx_row_group_writer_add_column(writer, "foo", CX_COLUMN_I32,
                                                   0, compression, level));
        assert_true(cx_row_group_add_column(row_group, column, nulls));
        assert_true(cx_row_group_writer_put(writer, row_group));
        assert_true(cx_row_group_writer_finish(writer, true));
        cx_row_group_writer_free(writer);
        cx_row_group_free(row_group);
        cx_column_free(column);
        cx_column_free(nulls);

        // high-level reader
        struct cx_reader *reader = cx_reader_new(fixture->temp_file);
        assert_not_null(reader);
        const char *name = cx_reader_column_name(reader, 0);
        assert_not_null(name);
        assert_string_equal(name, "foo");
        assert_size(cx_reader_column_count(reader), ==, 1);
        assert_size(cx_reader_row_count(reader), ==, 0);
        assert_false(cx_reader_next(reader));
        assert_false(cx_reader_error(reader));
        cx_reader_free(reader);

        // low-level reader
        struct cx_row_group_reader *row_group_reader =
            cx_row_group_reader_new(fixture->temp_file);
        assert_not_null(reader);
        assert_size(cx_row_group_reader_row_group_count(row_group_reader), ==,
                    1);
        assert_size(cx_row_group_reader_column_count(row_group_reader), ==, 1);
        assert_size(cx_row_group_reader_row_count(row_group_reader), ==, 0);
        row_group = cx_row_group_reader_get(row_group_reader, 0);
        assert_not_null(row_group);
        struct cx_row_cursor *cursor = cx_row_cursor_new(row_group, predicate);
        assert_not_null(cursor);
        assert_false(cx_row_cursor_next(cursor));
        assert_false(cx_row_cursor_error(cursor));
        cx_row_cursor_free(cursor);
        cx_row_group_free(row_group);
        cx_row_group_reader_free(row_group_reader);
    }

    cx_predicate_free(predicate);

    return MUNIT_OK;
}

MunitTest file_tests[] = {
    {"/read-write", test_read_write, setup, teardown, MUNIT_TEST_OPTION_NONE,
     NULL},
    {"/no-row-groups", test_no_row_groups, setup, teardown,
     MUNIT_TEST_OPTION_NONE, NULL},
    {"/no-columns", test_no_columns, setup, teardown, MUNIT_TEST_OPTION_NONE,
     NULL},
    {"/empty-columns", test_empty_columns, setup, teardown,
     MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL}};
