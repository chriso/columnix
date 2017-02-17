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

struct zcs_file_fixture {
    char *temp_file;
    struct zcs_predicate *true_predicate;
};

static enum zcs_compression_type zcs_compression_types[] = {
    ZCS_COMPRESSION_NONE, ZCS_COMPRESSION_LZ4, ZCS_COMPRESSION_LZ4HC,
    ZCS_COMPRESSION_ZSTD};

static void *setup(const MunitParameter params[], void *data)
{
    struct zcs_file_fixture *fixture = malloc(sizeof(*fixture));
    assert_not_null(fixture);

    fixture->temp_file = zcs_temp_file_new();
    assert_not_null(fixture->temp_file);

    fixture->true_predicate = zcs_predicate_new_true();
    assert_not_null(fixture->true_predicate);

    return fixture;
}

static void teardown(void *ptr)
{
    struct zcs_file_fixture *fixture = ptr;
    zcs_temp_file_free(fixture->temp_file);
    zcs_predicate_free(fixture->true_predicate);
    free(fixture);
}

static MunitResult test_read_write(const MunitParameter params[], void *ptr)
{
    struct zcs_file_fixture *fixture = ptr;

    enum zcs_compression_type compression;
    int level = 5;
    char buffer[64];

    enum zcs_column_type types[] = {ZCS_COLUMN_I32, ZCS_COLUMN_I64,
                                    ZCS_COLUMN_BIT, ZCS_COLUMN_STR};

    ZCS_FOREACH(zcs_compression_types, compression)
    {
        struct zcs_writer *writer =
            zcs_writer_new(fixture->temp_file, ROWS_PER_ROW_GROUP);
        assert_not_null(writer);

        for (size_t i = 0; i < COLUMN_COUNT; i++)
            assert_true(
                zcs_writer_add_column(writer, types[i], 0, compression, level));

        for (size_t i = 0; i < ROW_COUNT; i++) {
            assert_true(zcs_writer_put_i32(writer, 0, i));
            assert_true(zcs_writer_put_i64(writer, 1, i * 10));
            assert_true(zcs_writer_put_bit(writer, 2, i % 3 == 0));
            sprintf(buffer, "zcs %zu", i);
            assert_true(zcs_writer_put_str(writer, 3, buffer));
        }

        assert_true(zcs_writer_finish(writer, true));

        assert_true(zcs_writer_finish(writer, true));  // noop

        zcs_writer_free(writer);

        // high-level reader
        struct zcs_reader *reader = zcs_reader_new(fixture->temp_file);
        assert_not_null(reader);
        assert_size(zcs_reader_column_count(reader), ==, COLUMN_COUNT);
        assert_size(zcs_reader_row_count(reader), ==, ROW_COUNT);
        for (size_t i = 0; i < COLUMN_COUNT; i++) {
            assert_int(zcs_reader_column_type(reader, i), ==, types[i]);
            assert_int(zcs_reader_column_encoding(reader, i), ==, 0);
            assert_int(zcs_reader_column_compression(reader, i), ==,
                       compression);
        }
        size_t position = 0;
        for (; zcs_reader_next(reader); position++) {
            int32_t i32;
            assert_true(zcs_reader_get_i32(reader, 0, &i32));
            assert_int32(i32, ==, position);
            int64_t i64;
            assert_true(zcs_reader_get_i64(reader, 1, &i64));
            assert_int64(i64, ==, position * 10);
            bool bit;
            assert_true(zcs_reader_get_bit(reader, 2, &bit));
            if (position % 3 == 0)
                assert_true(bit);
            else
                assert_false(bit);
            const struct zcs_string *string;
            assert_true(zcs_reader_get_str(reader, 3, &string));
            sprintf(buffer, "zcs %zu", position);
            assert_int(string->len, ==, strlen(buffer));
            assert_string_equal(buffer, string->ptr);
        }
        assert_false(zcs_reader_error(reader));
        assert_size(position, ==, ROW_COUNT);
        zcs_reader_free(reader);

        // high-level reader matching rows
        struct zcs_predicate *predicate = zcs_predicate_new_and(
            4, zcs_predicate_new_i32_gt(0, 20),
            zcs_predicate_new_i64_lt(1, 900), zcs_predicate_new_bit_eq(2, true),
            zcs_predicate_new_str_contains(3, "0", false,
                                           ZCS_STR_LOCATION_END));
        assert_not_null(predicate);
        reader = zcs_reader_new_matching(fixture->temp_file, predicate);
        assert_not_null(reader);
        assert_size(zcs_reader_column_count(reader), ==, COLUMN_COUNT);
        assert_size(zcs_reader_row_count(reader), ==, 2);
        assert_false(zcs_reader_error(reader));
        zcs_reader_rewind(reader);
        int32_t value;
        assert_true(zcs_reader_next(reader));
        assert_true(zcs_reader_get_i32(reader, 0, &value));
        assert_int(value, ==, 30);
        assert_true(zcs_reader_next(reader));
        assert_true(zcs_reader_get_i32(reader, 0, &value));
        assert_int(value, ==, 60);
        assert_false(zcs_reader_next(reader));
        assert_false(zcs_reader_error(reader));
        zcs_reader_free(reader);

        // low-level reader
        struct zcs_row_group_reader *row_group_reader =
            zcs_row_group_reader_new(fixture->temp_file);
        assert_not_null(row_group_reader);
        assert_size(zcs_row_group_reader_row_group_count(row_group_reader), ==,
                    ROW_GROUP_COUNT);
        assert_size(zcs_row_group_reader_column_count(row_group_reader), ==,
                    COLUMN_COUNT);
        assert_size(zcs_row_group_reader_row_count(row_group_reader), ==,
                    ROW_COUNT);
        for (size_t i = 0; i < COLUMN_COUNT; i++) {
            assert_int(zcs_row_group_reader_column_type(row_group_reader, i),
                       ==, types[i]);
            assert_int(
                zcs_row_group_reader_column_encoding(row_group_reader, i), ==,
                0);
            assert_int(
                zcs_row_group_reader_column_compression(row_group_reader, i),
                ==, compression);
        }
        position = 0;
        for (size_t i = 0; i < ROW_GROUP_COUNT; i++) {
            struct zcs_row_group *row_group =
                zcs_row_group_reader_get(row_group_reader, i);
            assert_not_null(row_group);
            struct zcs_row_cursor *cursor =
                zcs_row_cursor_new(row_group, fixture->true_predicate);
            assert_not_null(cursor);
            for (; zcs_row_cursor_next(cursor); position++) {
                int32_t i32;
                assert_true(zcs_row_cursor_get_i32(cursor, 0, &i32));
                assert_int32(i32, ==, position);
                int64_t i64;
                assert_true(zcs_row_cursor_get_i64(cursor, 1, &i64));
                assert_int64(i64, ==, position * 10);
                bool bit;
                assert_true(zcs_row_cursor_get_bit(cursor, 2, &bit));
                if (position % 3 == 0)
                    assert_true(bit);
                else
                    assert_false(bit);
                const struct zcs_string *string;
                assert_true(zcs_row_cursor_get_str(cursor, 3, &string));
                char buffer[64];
                sprintf(buffer, "zcs %zu", position);
                assert_int(string->len, ==, strlen(buffer));
                assert_string_equal(buffer, string->ptr);
            }
            zcs_row_cursor_free(cursor);
            zcs_row_group_free(row_group);
        }
        assert_size(position, ==, ROW_GROUP_COUNT * ROWS_PER_ROW_GROUP);
        zcs_row_group_reader_free(row_group_reader);
    }

    return MUNIT_OK;
}

static MunitResult test_no_row_groups(const MunitParameter params[], void *ptr)
{
    struct zcs_file_fixture *fixture = ptr;

    struct zcs_row_group_writer *writer =
        zcs_row_group_writer_new(fixture->temp_file);
    assert_not_null(writer);

    for (size_t i = 0; i < COLUMN_COUNT; i++)
        assert_true(zcs_row_group_writer_add_column(writer, ZCS_COLUMN_I32, 0,
                                                    ZCS_COMPRESSION_NONE, 0));

    assert_true(zcs_row_group_writer_finish(writer, true));

    zcs_row_group_writer_free(writer);

    // high-level reader
    struct zcs_reader *reader = zcs_reader_new(fixture->temp_file);
    assert_not_null(reader);
    assert_size(zcs_reader_column_count(reader), ==, COLUMN_COUNT);
    assert_size(zcs_reader_row_count(reader), ==, 0);
    for (size_t i = 0; i < COLUMN_COUNT; i++) {
        assert_int(zcs_reader_column_type(reader, i), ==, ZCS_COLUMN_I32);
        assert_int(zcs_reader_column_encoding(reader, i), ==, 0);
    }
    zcs_reader_free(reader);

    // low-level reader
    struct zcs_row_group_reader *row_group_reader =
        zcs_row_group_reader_new(fixture->temp_file);
    assert_not_null(row_group_reader);
    assert_size(zcs_row_group_reader_row_group_count(row_group_reader), ==, 0);
    assert_size(zcs_row_group_reader_column_count(row_group_reader), ==,
                COLUMN_COUNT);
    assert_size(zcs_row_group_reader_row_count(row_group_reader), ==, 0);
    for (size_t i = 0; i < COLUMN_COUNT; i++) {
        assert_int(zcs_row_group_reader_column_type(row_group_reader, i), ==,
                   ZCS_COLUMN_I32);
        assert_int(zcs_row_group_reader_column_encoding(row_group_reader, i),
                   ==, 0);
    }
    zcs_row_group_reader_free(row_group_reader);

    return MUNIT_OK;
}

static MunitResult test_no_columns(const MunitParameter params[], void *ptr)
{
    struct zcs_file_fixture *fixture = ptr;
    struct zcs_row_group_writer *writer =
        zcs_row_group_writer_new(fixture->temp_file);
    assert_not_null(writer);
    assert_true(zcs_row_group_writer_finish(writer, true));
    zcs_row_group_writer_free(writer);

    // high-level reader
    struct zcs_reader *reader = zcs_reader_new(fixture->temp_file);
    assert_not_null(reader);
    assert_size(zcs_reader_column_count(reader), ==, 0);
    assert_size(zcs_reader_row_count(reader), ==, 0);
    assert_false(zcs_reader_next(reader));
    assert_false(zcs_reader_error(reader));
    zcs_reader_free(reader);

    // low-level reader
    struct zcs_row_group_reader *row_group_reader =
        zcs_row_group_reader_new(fixture->temp_file);
    assert_not_null(row_group_reader);
    assert_size(zcs_row_group_reader_row_group_count(row_group_reader), ==, 0);
    assert_size(zcs_row_group_reader_column_count(row_group_reader), ==, 0);
    assert_size(zcs_row_group_reader_row_count(row_group_reader), ==, 0);
    zcs_row_group_reader_free(row_group_reader);

    return MUNIT_OK;
}

static MunitResult test_empty_columns(const MunitParameter params[], void *ptr)
{
    struct zcs_file_fixture *fixture = ptr;

    struct zcs_predicate *predicate = zcs_predicate_new_true();
    assert_not_null(predicate);

    enum zcs_compression_type compression;
    int level = 5;

    ZCS_FOREACH(zcs_compression_types, compression)
    {
        struct zcs_row_group_writer *writer =
            zcs_row_group_writer_new(fixture->temp_file);
        assert_not_null(writer);
        struct zcs_row_group *row_group = zcs_row_group_new();
        assert_not_null(row_group);
        struct zcs_column *column = zcs_column_new(ZCS_COLUMN_I32, 0);
        assert_not_null(column);
        assert_true(zcs_row_group_writer_add_column(writer, ZCS_COLUMN_I32, 0,
                                                    compression, level));
        assert_true(zcs_row_group_add_column(row_group, column));
        assert_true(zcs_row_group_writer_put(writer, row_group));
        assert_true(zcs_row_group_writer_finish(writer, true));
        zcs_row_group_writer_free(writer);
        zcs_row_group_free(row_group);
        zcs_column_free(column);

        // high-level reader
        struct zcs_reader *reader = zcs_reader_new(fixture->temp_file);
        assert_not_null(reader);
        assert_size(zcs_reader_column_count(reader), ==, 1);
        assert_size(zcs_reader_row_count(reader), ==, 0);
        assert_false(zcs_reader_next(reader));
        assert_false(zcs_reader_error(reader));
        zcs_reader_free(reader);

        // low-level reader
        struct zcs_row_group_reader *row_group_reader =
            zcs_row_group_reader_new(fixture->temp_file);
        assert_not_null(reader);
        assert_size(zcs_row_group_reader_row_group_count(row_group_reader), ==,
                    1);
        assert_size(zcs_row_group_reader_column_count(row_group_reader), ==, 1);
        assert_size(zcs_row_group_reader_row_count(row_group_reader), ==, 0);
        row_group = zcs_row_group_reader_get(row_group_reader, 0);
        assert_not_null(row_group);
        struct zcs_row_cursor *cursor =
            zcs_row_cursor_new(row_group, predicate);
        assert_not_null(cursor);
        assert_false(zcs_row_cursor_next(cursor));
        assert_false(zcs_row_cursor_error(cursor));
        zcs_row_cursor_free(cursor);
        zcs_row_group_free(row_group);
        zcs_row_group_reader_free(row_group_reader);
    }

    zcs_predicate_free(predicate);

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
