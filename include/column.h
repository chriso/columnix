#ifndef ZCS_COLUMN_
#define ZCS_COLUMN_

#include "index.h"

#define ZCS_BATCH_SIZE 64

struct zcs_column;

struct zcs_column_cursor;

struct zcs_column *zcs_column_new(enum zcs_column_type, enum zcs_encoding_type);

struct zcs_column *zcs_column_new_immutable(enum zcs_column_type,
                                            enum zcs_encoding_type,
                                            const void *, size_t,
                                            const struct zcs_column_index *);

struct zcs_column *zcs_column_new_compressed(enum zcs_column_type,
                                             enum zcs_encoding_type, void **,
                                             size_t,
                                             const struct zcs_column_index *);

void zcs_column_free(struct zcs_column *);

const void *zcs_column_export(const struct zcs_column *, size_t *);

enum zcs_column_type zcs_column_type(const struct zcs_column *);

enum zcs_encoding_type zcs_column_encoding(const struct zcs_column *);

const struct zcs_column_index *zcs_column_index(const struct zcs_column *);

bool zcs_column_put_bit(struct zcs_column *, bool);
bool zcs_column_put_i32(struct zcs_column *, int32_t);
bool zcs_column_put_i64(struct zcs_column *, int64_t);
bool zcs_column_put_str(struct zcs_column *, const char *);

struct zcs_column_cursor *zcs_column_cursor_new(const struct zcs_column *);

void zcs_column_cursor_free(struct zcs_column_cursor *);

void zcs_column_cursor_rewind(struct zcs_column_cursor *);

bool zcs_column_cursor_valid(const struct zcs_column_cursor *);

const uint64_t *zcs_column_cursor_next_batch_bit(struct zcs_column_cursor *,
                                                 size_t *);
const int32_t *zcs_column_cursor_next_batch_i32(struct zcs_column_cursor *,
                                                size_t *);
const int64_t *zcs_column_cursor_next_batch_i64(struct zcs_column_cursor *,
                                                size_t *);
const struct zcs_string *zcs_column_cursor_next_batch_str(
    struct zcs_column_cursor *, size_t *);

size_t zcs_column_cursor_skip_bit(struct zcs_column_cursor *, size_t);
size_t zcs_column_cursor_skip_i32(struct zcs_column_cursor *, size_t);
size_t zcs_column_cursor_skip_i64(struct zcs_column_cursor *, size_t);
size_t zcs_column_cursor_skip_str(struct zcs_column_cursor *, size_t);

#endif
