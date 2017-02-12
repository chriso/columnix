#include <stdio.h>

#include "row_group.h"

#define MUNIT_ENABLE_ASSERT_ALIASES
#include "munit.h"

#define COLUMN_COUNT 4
#define ROW_COUNT 1111

struct zcs_row_group_fixture {
    struct zcs_row_group *row_group;
    struct zcs_column *columns[COLUMN_COUNT];
};

static void *setup(const MunitParameter params[], void *data)
{
    struct zcs_row_group_fixture *fixture = malloc(sizeof(*fixture));
    assert_not_null(fixture);

    fixture->row_group = zcs_row_group_new();
    assert_not_null(fixture->row_group);

    fixture->columns[0] = zcs_column_new(ZCS_COLUMN_I32, ZCS_ENCODE_NONE);
    fixture->columns[1] = zcs_column_new(ZCS_COLUMN_I64, ZCS_ENCODE_NONE);
    fixture->columns[2] = zcs_column_new(ZCS_COLUMN_BIT, ZCS_ENCODE_NONE);
    fixture->columns[3] = zcs_column_new(ZCS_COLUMN_STR, ZCS_ENCODE_NONE);

    for (size_t i = 0; i < COLUMN_COUNT; i++)
        assert_not_null(fixture->columns[i]);

    char buffer[64];
    for (size_t i = 0; i < ROW_COUNT; i++) {
        assert_true(zcs_column_put_i32(fixture->columns[0], i));
        assert_true(zcs_column_put_i64(fixture->columns[1], i * 10));
        assert_true(zcs_column_put_bit(fixture->columns[2], i % 3 == 0));
        sprintf(buffer, "zcs %zu", i);
        assert_true(zcs_column_put_str(fixture->columns[3], buffer));
    }

    return fixture;
}

static void teardown(void *ptr)
{
    struct zcs_row_group_fixture *fixture = ptr;
    for (size_t i = 0; i < COLUMN_COUNT; i++)
        zcs_column_free(fixture->columns[i]);
    zcs_row_group_free(fixture->row_group);
    free(fixture);
}

static MunitResult test_add_column(const MunitParameter params[], void *ptr)
{
    struct zcs_row_group_fixture *fixture = ptr;
    struct zcs_row_group *row_group = fixture->row_group;

    assert_size(zcs_row_group_column_count(row_group), ==, 0);
    assert_size(zcs_row_group_row_count(row_group), ==, 0);

    for (size_t i = 0; i < COLUMN_COUNT; i++)
        assert_true(zcs_row_group_add_column(row_group, fixture->columns[i]));

    assert_size(zcs_row_group_column_count(row_group), ==, COLUMN_COUNT);
    assert_size(zcs_row_group_row_count(row_group), ==, ROW_COUNT);

    for (size_t i = 0; i < COLUMN_COUNT; i++) {
        struct zcs_column *column = fixture->columns[i];
        assert_ptr_equal(zcs_row_group_column(row_group, i), column);
        assert_int(zcs_row_group_column_type(row_group, i), ==,
                   zcs_column_type(column));
        assert_int(zcs_row_group_column_encode(row_group, i), ==,
                   zcs_column_encode(column));
        assert_ptr_equal(zcs_row_group_column_index(row_group, i),
                         zcs_column_index(column));
    }

    return MUNIT_OK;
}

static MunitResult test_row_count_mismatch(const MunitParameter params[],
                                           void *ptr)
{
    struct zcs_row_group_fixture *fixture = ptr;
    struct zcs_row_group *row_group = fixture->row_group;
    assert_true(zcs_row_group_add_column(row_group, fixture->columns[0]));
    assert_true(zcs_column_put_i64(fixture->columns[1], 30));
    assert_false(zcs_row_group_add_column(row_group, fixture->columns[1]));
    return MUNIT_OK;
}

static MunitResult test_cursor(const MunitParameter params[], void *ptr)
{
    struct zcs_row_group_fixture *fixture = ptr;
    struct zcs_row_group *row_group = fixture->row_group;

    for (size_t i = 0; i < COLUMN_COUNT; i++)
        assert_true(zcs_row_group_add_column(row_group, fixture->columns[i]));

    struct zcs_row_group_cursor *cursor = zcs_row_group_cursor_new(row_group);
    assert_not_null(cursor);

    char buffer[64];
    size_t position = 0, batch = 0, count;
    for (; zcs_row_group_cursor_valid(cursor);
         zcs_row_group_cursor_advance(cursor), batch++) {
        for (size_t repeat = 0; repeat < 2; repeat++) {
            const int32_t *i32_batch =
                zcs_row_group_cursor_next_i32(cursor, 0, &count);
            assert_not_null(i32_batch);
            for (size_t i = 0; i < count; i++)
                assert_int32(i32_batch[i], ==, position + i);
        }

        if (batch % 3 == 1) {
            const int64_t *i64_batch =
                zcs_row_group_cursor_next_i64(cursor, 1, &count);
            assert_not_null(i64_batch);
            for (size_t i = 0; i < count; i++)
                assert_int64(i64_batch[i], ==, (position + i) * 10);
        }

        if (batch % 5 == 3) {
            const uint64_t *bitset =
                zcs_row_group_cursor_next_bit(cursor, 2, &count);
            assert_not_null(bitset);
            for (size_t i = 0; i < count; i++) {
                bool bit = *bitset & ((uint64_t)1 << i);
                if ((i + position) % 3 == 0)
                    assert_true(bit);
                else
                    assert_false(bit);
            }
        }

        if (batch % 7 == 5) {
            const struct zcs_string *str_batch =
                zcs_row_group_cursor_next_str(cursor, 3, &count);
            assert_not_null(str_batch);
            for (size_t i = 0; i < count; i++) {
                sprintf(buffer, "zcs %zu", position + i);
                assert_int(str_batch[i].len, ==, strlen(buffer));
                assert_string_equal(buffer, str_batch[i].ptr);
            }
        }

        position += count;
    }

    assert_size(position, ==, ROW_COUNT);

    zcs_row_group_cursor_free(cursor);
    return MUNIT_OK;
}

MunitTest row_group_tests[] = {
    {"/add-column", test_add_column, setup, teardown, MUNIT_TEST_OPTION_NONE,
     NULL},
    {"/row-count-mismatch", test_row_count_mismatch, setup, teardown,
     MUNIT_TEST_OPTION_NONE, NULL},
    {"/cursor", test_cursor, setup, teardown, MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL}};
