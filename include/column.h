#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "type.h"

struct zcs_column;

struct zcs_column *zcs_column_new(enum zcs_column_type, enum zcs_encode_type);

void zcs_column_free(struct zcs_column *);

bool zcs_column_put_i32(struct zcs_column *, int32_t);
bool zcs_column_put_i64(struct zcs_column *, int64_t);

const void *zcs_column_export(const struct zcs_column *, size_t *);
