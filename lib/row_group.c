#include <assert.h>
#include <stdlib.h>

#include "row_group.h"

struct zcs_row_group_column {
    enum zcs_column_type type;
    enum zcs_encode_type encode;
    const struct zcs_column_index *index;
    const struct zcs_column *column;
};

struct zcs_row_group {
    struct zcs_row_group_column *columns;
    size_t count;
    size_t size;
};

struct zcs_row_group *zcs_row_group_new()
{
    struct zcs_row_group *row_group = malloc(sizeof(*row_group));
    if (!row_group)
        return NULL;
    row_group->columns = malloc(sizeof(*row_group->columns));
    if (!row_group->columns)
        goto error;
    row_group->count = 0;
    row_group->size = 1;
    return row_group;
error:
    free(row_group);
    return NULL;
}

void zcs_row_group_free(struct zcs_row_group *row_group)
{
    free(row_group->columns);
    free(row_group);
}

bool zcs_row_group_add_column(struct zcs_row_group *row_group,
                              const struct zcs_column *column)
{
    const struct zcs_column_index *index = zcs_column_index(column);
    if (row_group->count) {
        const struct zcs_column_index *last_index =
            zcs_row_group_column_index(row_group, row_group->count - 1);
        if (index->count != last_index->count)
            return false;
    }
    if (row_group->count == row_group->size) {
        struct zcs_row_group_column *columns =
            realloc(row_group->columns, row_group->size * 2 * sizeof(*columns));
        if (!columns)
            return false;
        row_group->columns = columns;
        row_group->size *= 2;
    }
    struct zcs_row_group_column *wrapper =
        &row_group->columns[row_group->count++];
    wrapper->column = column;
    wrapper->type = zcs_column_type(column);
    wrapper->encode = zcs_column_encode(column);
    wrapper->index = index;
    return true;
}

size_t zcs_row_group_column_count(const struct zcs_row_group *row_group)
{
    return row_group->count;
}

size_t zcs_row_group_row_count(const struct zcs_row_group *row_group)
{
    if (!row_group->count)
        return 0;
    const struct zcs_column_index *index =
        zcs_row_group_column_index(row_group, 0);
    return index->count;
}

enum zcs_column_type zcs_row_group_column_type(
    const struct zcs_row_group *row_group, size_t index)
{
    assert(index <= row_group->count);
    return row_group->columns[index].type;
}

enum zcs_encode_type zcs_row_group_column_encode(
    const struct zcs_row_group *row_group, size_t index)
{
    assert(index <= row_group->count);
    return row_group->columns[index].encode;
}

const struct zcs_column_index *zcs_row_group_column_index(
    const struct zcs_row_group *row_group, size_t index)
{
    assert(index <= row_group->count);
    return row_group->columns[index].index;
}

const struct zcs_column *zcs_row_group_column(
    const struct zcs_row_group *row_group, size_t index)
{
    assert(index <= row_group->count);
    return row_group->columns[index].column;
}
