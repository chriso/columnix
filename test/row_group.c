#include <stdio.h>

#include "row_group.h"

#include "helpers.h"

#define COLUMN_COUNT 4
#define ROW_COUNT 1111

struct cx_row_group_fixture {
    struct cx_row_group *row_group;
    struct cx_column *columns[COLUMN_COUNT];
    struct cx_column *nulls[COLUMN_COUNT];
};

static void *setup(const MunitParameter params[], void *data)
{
    struct cx_row_group_fixture *fixture = malloc(sizeof(*fixture));
    assert_not_null(fixture);

    fixture->row_group = cx_row_group_new();
    assert_not_null(fixture->row_group);

    enum cx_column_type types[] = {CX_COLUMN_I32, CX_COLUMN_I64, CX_COLUMN_BIT,
                                   CX_COLUMN_STR};

    for (size_t i = 0; i < COLUMN_COUNT; i++) {
        fixture->columns[i] = cx_column_new(types[i], CX_ENCODING_NONE);
        assert_not_null(fixture->columns[i]);
        fixture->nulls[i] = cx_column_new(CX_COLUMN_BIT, CX_ENCODING_NONE);
        assert_not_null(fixture->nulls[i]);
    }

    char buffer[64];
    for (size_t i = 0; i < ROW_COUNT; i++) {
        assert_true(cx_column_put_i32(fixture->columns[0], i));
        assert_true(cx_column_put_i64(fixture->columns[1], i * 10));
        assert_true(cx_column_put_bit(fixture->columns[2], i % 3 == 0));
        sprintf(buffer, "cx %zu", i);
        assert_true(cx_column_put_str(fixture->columns[3], buffer));

        assert_true(cx_column_put_bit(fixture->nulls[0], i % 2 == 0));
        assert_true(cx_column_put_bit(fixture->nulls[1], i % 3 == 0));
        assert_true(cx_column_put_bit(fixture->nulls[2], false));
        assert_true(cx_column_put_bit(fixture->nulls[3], true));
    }

    return fixture;
}

static void teardown(void *ptr)
{
    struct cx_row_group_fixture *fixture = ptr;
    for (size_t i = 0; i < COLUMN_COUNT; i++) {
        cx_column_free(fixture->columns[i]);
        cx_column_free(fixture->nulls[i]);
    }
    cx_row_group_free(fixture->row_group);
    free(fixture);
}

static MunitResult test_add_column(const MunitParameter params[], void *ptr)
{
    struct cx_row_group_fixture *fixture = ptr;
    struct cx_row_group *row_group = fixture->row_group;

    assert_size(cx_row_group_column_count(row_group), ==, 0);
    assert_size(cx_row_group_row_count(row_group), ==, 0);

    for (size_t i = 0; i < COLUMN_COUNT; i++)
        assert_true(cx_row_group_add_column(row_group, fixture->columns[i],
                                            fixture->nulls[i]));

    assert_size(cx_row_group_column_count(row_group), ==, COLUMN_COUNT);
    assert_size(cx_row_group_row_count(row_group), ==, ROW_COUNT);

    for (size_t i = 0; i < COLUMN_COUNT; i++) {
        struct cx_column *column = fixture->columns[i];
        struct cx_column *nulls = fixture->nulls[i];

        assert_ptr_equal(cx_row_group_column(row_group, i), column);
        assert_int(cx_row_group_column_type(row_group, i), ==,
                   cx_column_type(column));
        assert_int(cx_row_group_column_encoding(row_group, i), ==,
                   cx_column_encoding(column));
        assert_ptr_equal(cx_row_group_column_index(row_group, i),
                         cx_column_index(column));

        assert_ptr_equal(cx_row_group_nulls(row_group, i), nulls);
        assert_ptr_equal(cx_row_group_null_index(row_group, i),
                         cx_column_index(nulls));
    }

    return MUNIT_OK;
}

static MunitResult test_row_count_mismatch(const MunitParameter params[],
                                           void *ptr)
{
    struct cx_row_group_fixture *fixture = ptr;
    struct cx_row_group *row_group = fixture->row_group;
    assert_true(cx_row_group_add_column(row_group, fixture->columns[0],
                                        fixture->nulls[0]));
    assert_true(cx_column_put_i64(fixture->columns[1], 30));
    assert_false(cx_row_group_add_column(row_group, fixture->columns[1],
                                         fixture->nulls[1]));
    return MUNIT_OK;
}

static MunitResult test_cursor(const MunitParameter params[], void *ptr)
{
    struct cx_row_group_fixture *fixture = ptr;
    struct cx_row_group *row_group = fixture->row_group;

    for (size_t i = 0; i < COLUMN_COUNT; i++)
        assert_true(cx_row_group_add_column(row_group, fixture->columns[i],
                                            fixture->nulls[i]));

    struct cx_row_group_cursor *cursor = cx_row_group_cursor_new(row_group);
    assert_not_null(cursor);

    char buffer[64];
    const uint64_t *nulls;

    for (size_t cursor_repeat = 0; cursor_repeat < 2; cursor_repeat++) {
        size_t position = 0, count;
        for (size_t batch = 0; cx_row_group_cursor_next(cursor); batch++) {
            for (size_t batch_repeat = 0; batch_repeat < 2; batch_repeat++) {
                const int32_t *i32_batch =
                    cx_row_group_cursor_batch_i32(cursor, 0, &count);
                assert_not_null(i32_batch);
                for (size_t i = 0; i < count; i++)
                    assert_int32(i32_batch[i], ==, position + i);

                nulls = cx_row_group_cursor_batch_nulls(cursor, 0, &count);
                assert_not_null(nulls);
                for (size_t i = 0; i < count; i++) {
                    bool bit = *nulls & ((uint64_t)1 << i);
                    if ((i + position) % 2 == 0)
                        assert_true(bit);
                    else
                        assert_false(bit);
                }
            }

            if (batch % 3 == 1) {
                const int64_t *i64_batch =
                    cx_row_group_cursor_batch_i64(cursor, 1, &count);
                assert_not_null(i64_batch);
                for (size_t i = 0; i < count; i++)
                    assert_int64(i64_batch[i], ==, (position + i) * 10);

                nulls = cx_row_group_cursor_batch_nulls(cursor, 1, &count);
                assert_not_null(nulls);
                for (size_t i = 0; i < count; i++) {
                    bool bit = *nulls & ((uint64_t)1 << i);
                    if ((i + position) % 3 == 0)
                        assert_true(bit);
                    else
                        assert_false(bit);
                }
            }

            if (batch % 5 == 3) {
                const uint64_t *bitset =
                    cx_row_group_cursor_batch_bit(cursor, 2, &count);
                assert_not_null(bitset);
                for (size_t i = 0; i < count; i++) {
                    bool bit = *bitset & ((uint64_t)1 << i);
                    if ((i + position) % 3 == 0)
                        assert_true(bit);
                    else
                        assert_false(bit);
                }

                nulls = cx_row_group_cursor_batch_nulls(cursor, 2, &count);
                assert_not_null(nulls);
                for (size_t i = 0; i < count; i++) {
                    bool bit = *nulls & ((uint64_t)1 << i);
                    assert_false(bit);
                }
            }

            if (batch % 7 == 5) {
                const struct cx_string *str_batch =
                    cx_row_group_cursor_batch_str(cursor, 3, &count);
                assert_not_null(str_batch);
                for (size_t i = 0; i < count; i++) {
                    sprintf(buffer, "cx %zu", position + i);
                    assert_int(str_batch[i].len, ==, strlen(buffer));
                    assert_string_equal(buffer, str_batch[i].ptr);
                }

                nulls = cx_row_group_cursor_batch_nulls(cursor, 3, &count);
                assert_not_null(nulls);
                for (size_t i = 0; i < count; i++) {
                    bool bit = *nulls & ((uint64_t)1 << i);
                    assert_true(bit);
                }
            }

            position += count;
        }

        assert_size(position, ==, ROW_COUNT);

        cx_row_group_cursor_rewind(cursor);
    }

    cx_row_group_cursor_free(cursor);
    return MUNIT_OK;
}

static MunitResult test_cursor_empty(const MunitParameter params[], void *ptr)
{
    struct cx_row_group_fixture *fixture = ptr;
    struct cx_row_group *row_group = fixture->row_group;
    struct cx_row_group_cursor *cursor = cx_row_group_cursor_new(row_group);
    assert_not_null(cursor);
    assert_false(cx_row_group_cursor_next(cursor));
    cx_row_group_cursor_free(cursor);
    return MUNIT_OK;
}

MunitTest row_group_tests[] = {
    {"/add-column", test_add_column, setup, teardown, MUNIT_TEST_OPTION_NONE,
     NULL},
    {"/row-count-mismatch", test_row_count_mismatch, setup, teardown,
     MUNIT_TEST_OPTION_NONE, NULL},
    {"/cursor", test_cursor, setup, teardown, MUNIT_TEST_OPTION_NONE, NULL},
    {"/cursor-empty", test_cursor_empty, setup, teardown,
     MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL}};
