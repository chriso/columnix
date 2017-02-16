#define _BSD_SOURCE
#include <stdio.h>

#include "reader.h"
#include "row.h"
#include "writer.h"

#include "helpers.h"
#include "temp_file.h"

#define ROW_GROUP_COUNT 4
#define COLUMN_COUNT 4
#define ROWS_PER_ROW_GROUP 25

struct zcs_file_fixture {
    struct {
        struct zcs_row_group *row_group;
        struct zcs_column *columns[COLUMN_COUNT];
    } row_groups[ROW_GROUP_COUNT];
    char *temp_file;
};

static enum zcs_compression_type zcs_compression_types[] = {
    ZCS_COMPRESSION_NONE, ZCS_COMPRESSION_LZ4, ZCS_COMPRESSION_LZ4HC,
    ZCS_COMPRESSION_ZSTD};

static void *setup(const MunitParameter params[], void *data)
{
    struct zcs_file_fixture *fixture = malloc(sizeof(*fixture));
    assert_not_null(fixture);

    size_t position = 0;

    char buffer[64];

    for (size_t i = 0; i < ROW_GROUP_COUNT; i++) {
        fixture->row_groups[i].row_group = zcs_row_group_new();
        assert_not_null(fixture->row_groups[i].row_group);

        fixture->row_groups[i].columns[0] = zcs_column_new(ZCS_COLUMN_I32, 0);
        fixture->row_groups[i].columns[1] = zcs_column_new(ZCS_COLUMN_I64, 0);
        fixture->row_groups[i].columns[2] = zcs_column_new(ZCS_COLUMN_BIT, 0);
        fixture->row_groups[i].columns[3] = zcs_column_new(ZCS_COLUMN_STR, 0);

        for (size_t j = 0; j < COLUMN_COUNT; j++)
            assert_not_null(fixture->row_groups[i].columns[j]);

        for (size_t j = 0; j < ROWS_PER_ROW_GROUP; j++, position++) {
            assert_true(zcs_column_put_i32(fixture->row_groups[i].columns[0],
                                           position));
            assert_true(zcs_column_put_i64(fixture->row_groups[i].columns[1],
                                           position * 10));
            assert_true(zcs_column_put_bit(fixture->row_groups[i].columns[2],
                                           position % 3 == 0));
            sprintf(buffer, "zcs %zu", position);
            assert_true(
                zcs_column_put_str(fixture->row_groups[i].columns[3], buffer));
        }

        for (size_t j = 0; j < COLUMN_COUNT; j++)
            assert_true(
                zcs_row_group_add_column(fixture->row_groups[i].row_group,
                                         fixture->row_groups[i].columns[j]));
    }

    fixture->temp_file = zcs_temp_file_new();
    assert_not_null(fixture->temp_file);

    return fixture;
}

static void teardown(void *ptr)
{
    struct zcs_file_fixture *fixture = ptr;
    for (size_t i = 0; i < ROW_GROUP_COUNT; i++) {
        for (size_t j = 0; j < COLUMN_COUNT; j++)
            zcs_column_free(fixture->row_groups[i].columns[j]);
        zcs_row_group_free(fixture->row_groups[i].row_group);
    }
    zcs_temp_file_free(fixture->temp_file);
    free(fixture);
}

static MunitResult test_read_write(const MunitParameter params[], void *ptr)
{
    struct zcs_file_fixture *fixture = ptr;

    enum zcs_compression_type compression;
    int level = 5;

    ZCS_FOREACH(zcs_compression_types, compression)
    {
        struct zcs_row_group_writer *writer =
            zcs_row_group_writer_new(fixture->temp_file);
        assert_not_null(writer);

        for (size_t i = 0; i < COLUMN_COUNT; i++)
            assert_true(zcs_row_group_writer_add_column(
                writer, zcs_column_type(fixture->row_groups[0].columns[i]),
                zcs_column_encoding(fixture->row_groups[0].columns[i]),
                compression, level));

        for (size_t i = 0; i < ROW_GROUP_COUNT; i++)
            assert_true(zcs_row_group_writer_put(
                writer, fixture->row_groups[i].row_group));

        // header has already been written:
        assert_false(zcs_row_group_writer_add_column(writer, 0, 0, 0, 0));

        assert_true(zcs_row_group_writer_finish(writer, true));

        // out of order calls either fail or are noops:
        assert_true(zcs_row_group_writer_finish(writer, true));
        assert_false(
            zcs_row_group_writer_put(writer, fixture->row_groups[0].row_group));
        assert_false(zcs_row_group_writer_add_column(writer, 0, 0, 0, 0));

        zcs_row_group_writer_free(writer);

        struct zcs_row_group_reader *reader =
            zcs_row_group_reader_new(fixture->temp_file);
        assert_not_null(reader);

        assert_size(zcs_row_group_reader_row_group_count(reader), ==,
                    ROW_GROUP_COUNT);
        assert_size(zcs_row_group_reader_column_count(reader), ==,
                    COLUMN_COUNT);

        for (size_t i = 0; i < COLUMN_COUNT; i++) {
            assert_int(zcs_row_group_reader_column_type(reader, i), ==,
                       zcs_column_type(fixture->row_groups[0].columns[i]));
            assert_int(zcs_row_group_reader_column_encoding(reader, i), ==,
                       zcs_column_encoding(fixture->row_groups[0].columns[i]));
            assert_int(zcs_row_group_reader_column_compression(reader, i), ==,
                       compression);
        }

        size_t position = 0;
        for (size_t i = 0; i < ROW_GROUP_COUNT; i++) {
            struct zcs_row_group *row_group =
                zcs_row_group_reader_row_group(reader, i);
            assert_not_null(row_group);
            struct zcs_row_cursor *cursor = zcs_row_cursor_new(row_group);
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

        zcs_row_group_reader_free(reader);
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
        assert_true(zcs_row_group_writer_add_column(
            writer, zcs_column_type(fixture->row_groups[0].columns[i]),
            zcs_column_encoding(fixture->row_groups[0].columns[i]),
            ZCS_COMPRESSION_NONE, 0));

    assert_true(zcs_row_group_writer_finish(writer, true));

    zcs_row_group_writer_free(writer);

    struct zcs_row_group_reader *reader =
        zcs_row_group_reader_new(fixture->temp_file);
    assert_not_null(reader);

    assert_size(zcs_row_group_reader_row_group_count(reader), ==, 0);
    assert_size(zcs_row_group_reader_column_count(reader), ==, COLUMN_COUNT);

    for (size_t i = 0; i < COLUMN_COUNT; i++) {
        assert_int(zcs_row_group_reader_column_type(reader, i), ==,
                   zcs_column_type(fixture->row_groups[0].columns[i]));
        assert_int(zcs_row_group_reader_column_encoding(reader, i), ==,
                   zcs_column_encoding(fixture->row_groups[0].columns[i]));
    }

    zcs_row_group_reader_free(reader);

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
    struct zcs_row_group_reader *reader =
        zcs_row_group_reader_new(fixture->temp_file);
    assert_not_null(reader);
    assert_size(zcs_row_group_reader_row_group_count(reader), ==, 0);
    assert_size(zcs_row_group_reader_column_count(reader), ==, 0);
    zcs_row_group_reader_free(reader);
    return MUNIT_OK;
}

static MunitResult test_empty_columns(const MunitParameter params[], void *ptr)
{
    struct zcs_file_fixture *fixture = ptr;

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

        struct zcs_row_group_reader *reader =
            zcs_row_group_reader_new(fixture->temp_file);
        assert_not_null(reader);
        assert_size(zcs_row_group_reader_row_group_count(reader), ==, 1);
        assert_size(zcs_row_group_reader_column_count(reader), ==, 1);
        row_group = zcs_row_group_reader_row_group(reader, 0);
        assert_not_null(row_group);
        struct zcs_row_cursor *cursor = zcs_row_cursor_new(row_group);
        assert_not_null(cursor);
        assert_false(zcs_row_cursor_next(cursor));
        zcs_row_cursor_free(cursor);
        zcs_row_group_free(row_group);
        zcs_row_group_reader_free(reader);
    }

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
