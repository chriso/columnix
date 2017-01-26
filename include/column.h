#ifndef ZCS_COLUMN_
#define ZCS_COLUMN_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "type.h"

struct zcs_column;

struct zcs_column_cursor;

typedef union {
    bool bit;
    int32_t i32;
    int64_t i64;
    uint64_t len;
} zcs_column_index_value_t;

struct zcs_column_index {
    uint64_t count;
    zcs_column_index_value_t min;
    zcs_column_index_value_t max;
};

struct zcs_string {
    const char *ptr;
    size_t len;
};

struct zcs_column *zcs_column_new(enum zcs_column_type, enum zcs_encode_type);

struct zcs_column *zcs_column_new_immutable(enum zcs_column_type,
                                            enum zcs_encode_type, const void *,
                                            size_t,
                                            const struct zcs_column_index *);

struct zcs_column *zcs_column_new_compressed(enum zcs_column_type,
                                             enum zcs_encode_type, void **,
                                             size_t,
                                             const struct zcs_column_index *);

void zcs_column_free(struct zcs_column *);

const void *zcs_column_export(const struct zcs_column *, size_t *);

enum zcs_column_type zcs_column_type(const struct zcs_column *);

enum zcs_encode_type zcs_column_encode(const struct zcs_column *);

const struct zcs_column_index *zcs_column_index(const struct zcs_column *);

bool zcs_column_put_bit(struct zcs_column *, bool);
bool zcs_column_put_i32(struct zcs_column *, int32_t);
bool zcs_column_put_i64(struct zcs_column *, int64_t);
bool zcs_column_put_str(struct zcs_column *, const char *);

struct zcs_column_cursor *zcs_column_cursor_new(const struct zcs_column *);

void zcs_column_cursor_free(struct zcs_column_cursor *);

void zcs_column_cursor_rewind(struct zcs_column_cursor *);

bool zcs_column_cursor_valid(const struct zcs_column_cursor *);

bool zcs_column_cursor_next_bit(struct zcs_column_cursor *);
int32_t zcs_column_cursor_next_i32(struct zcs_column_cursor *);
int64_t zcs_column_cursor_next_i64(struct zcs_column_cursor *);
struct zcs_string zcs_column_cursor_next_str(struct zcs_column_cursor *);

size_t zcs_column_cursor_skip_bit(struct zcs_column_cursor *, size_t);
size_t zcs_column_cursor_skip_i32(struct zcs_column_cursor *, size_t);
size_t zcs_column_cursor_skip_i64(struct zcs_column_cursor *, size_t);
size_t zcs_column_cursor_skip_str(struct zcs_column_cursor *, size_t);

const uint64_t *zcs_column_cursor_next_batch_bit(struct zcs_column_cursor *,
                                                 size_t, size_t *);
const int32_t *zcs_column_cursor_next_batch_i32(struct zcs_column_cursor *,
                                                size_t, size_t *);
const int64_t *zcs_column_cursor_next_batch_i64(struct zcs_column_cursor *,
                                                size_t, size_t *);
size_t zcs_column_cursor_next_batch_str(struct zcs_column_cursor *, size_t,
                                        struct zcs_string[]);

#endif
