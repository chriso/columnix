#include <stdlib.h>

#include "column.h"

struct zcs_column {
    void *buffer;
    size_t size;
};

struct zcs_column *zcs_column_new()
{
    return calloc(1, sizeof(struct zcs_column));
}

void zcs_column_free(struct zcs_column *col)
{
    free(col);
}
