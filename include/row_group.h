#ifndef ZCS_ROW_GROUP_
#define ZCS_ROW_GROUP_

#include "column.h"

struct zcs_row_group;

struct zcs_row_group *zcs_row_group_new(void);

void zcs_row_group_free(struct zcs_row_group *);

bool zcs_row_group_add_column(struct zcs_row_group *,
                              const struct zcs_column *);

size_t zcs_row_group_column_count(const struct zcs_row_group *);

enum zcs_column_type zcs_row_group_column_type(const struct zcs_row_group *,
                                               size_t);

enum zcs_encode_type zcs_row_group_column_encode(const struct zcs_row_group *,
                                                 size_t);

const struct zcs_column_index *zcs_row_group_column_index(
    const struct zcs_row_group *, size_t);

const struct zcs_column *zcs_row_group_column(const struct zcs_row_group *,
                                              size_t);

#endif
