#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "type.h"

struct zcs_column;

struct zcs_column_cursor;

struct zcs_column *zcs_column_new(enum zcs_column_type, enum zcs_encode_type);
void zcs_column_free(struct zcs_column *);
const void *zcs_column_export(const struct zcs_column *, size_t *);

bool zcs_column_put_i32(struct zcs_column *, int32_t);
bool zcs_column_put_i64(struct zcs_column *, int64_t);

struct zcs_column_cursor *zcs_column_cursor_new(const struct zcs_column *);
void zcs_column_cursor_free(struct zcs_column_cursor *);
void zcs_column_cursor_rewind(struct zcs_column_cursor *);
bool zcs_column_cursor_valid(const struct zcs_column_cursor *);

int32_t zcs_column_cursor_next_i32(struct zcs_column_cursor *);
int64_t zcs_column_cursor_next_i64(struct zcs_column_cursor *);

size_t zcs_column_cursor_skip_i32(struct zcs_column_cursor *, size_t);
size_t zcs_column_cursor_skip_i64(struct zcs_column_cursor *, size_t);

const int32_t *zcs_column_cursor_next_batch_i32(struct zcs_column_cursor *,
                                                size_t, size_t *);
const int64_t *zcs_column_cursor_next_batch_i64(struct zcs_column_cursor *,
                                                size_t, size_t *);
