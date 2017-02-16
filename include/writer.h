#ifndef ZCS_WRITER_
#define ZCS_WRITER_

#include "row_group.h"

struct zcs_row_group_writer;

struct zcs_row_group_writer *zcs_row_group_writer_new(const char *);

void zcs_row_group_writer_free(struct zcs_row_group_writer *);

bool zcs_row_group_writer_add_column(struct zcs_row_group_writer *,
                                     enum zcs_column_type,
                                     enum zcs_encoding_type,
                                     enum zcs_compression_type, int level);

bool zcs_row_group_writer_put(struct zcs_row_group_writer *,
                              struct zcs_row_group *);

bool zcs_row_group_writer_finish(struct zcs_row_group_writer *, bool sync);

#endif
