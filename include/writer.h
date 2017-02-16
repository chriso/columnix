#ifndef ZCS_WRITER_
#define ZCS_WRITER_

#include "row_group.h"

struct zcs_writer *zcs_writer_new(const char *);

void zcs_writer_free(struct zcs_writer *);

bool zcs_writer_add_column(struct zcs_writer *, enum zcs_column_type,
                           enum zcs_encoding_type, enum zcs_compression_type,
                           int level);

bool zcs_writer_add_row_group(struct zcs_writer *, struct zcs_row_group *);

bool zcs_writer_finish(struct zcs_writer *, bool sync);

#endif
