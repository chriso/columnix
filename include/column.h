#ifndef CX_COLUMN_
#define CX_COLUMN_

#include "index.h"

#define CX_BATCH_SIZE 64

struct cx_column;

struct cx_column_cursor;

struct cx_column *cx_column_new(enum cx_column_type, enum cx_encoding_type);

struct cx_column *cx_column_new_mmapped(enum cx_column_type,
                                        enum cx_encoding_type, const void *,
                                        size_t, const struct cx_column_index *);

struct cx_column *cx_column_new_compressed(enum cx_column_type,
                                           enum cx_encoding_type, void **,
                                           size_t,
                                           const struct cx_column_index *);

void cx_column_free(struct cx_column *);

const void *cx_column_export(const struct cx_column *, size_t *);

enum cx_column_type cx_column_type(const struct cx_column *);

enum cx_encoding_type cx_column_encoding(const struct cx_column *);

const struct cx_column_index *cx_column_index(const struct cx_column *);

size_t cx_column_count(const struct cx_column *);

bool cx_column_put_bit(struct cx_column *, bool);
bool cx_column_put_i32(struct cx_column *, int32_t);
bool cx_column_put_i64(struct cx_column *, int64_t);
bool cx_column_put_str(struct cx_column *, const char *);

bool cx_column_put_unit(struct cx_column *);

struct cx_column_cursor *cx_column_cursor_new(const struct cx_column *);

void cx_column_cursor_free(struct cx_column_cursor *);

void cx_column_cursor_rewind(struct cx_column_cursor *);

bool cx_column_cursor_valid(const struct cx_column_cursor *);

const uint64_t *cx_column_cursor_next_batch_bit(struct cx_column_cursor *,
                                                size_t *);
const int32_t *cx_column_cursor_next_batch_i32(struct cx_column_cursor *,
                                               size_t *);
const int64_t *cx_column_cursor_next_batch_i64(struct cx_column_cursor *,
                                               size_t *);
const struct cx_string *cx_column_cursor_next_batch_str(
    struct cx_column_cursor *, size_t *);

size_t cx_column_cursor_skip_bit(struct cx_column_cursor *, size_t);
size_t cx_column_cursor_skip_i32(struct cx_column_cursor *, size_t);
size_t cx_column_cursor_skip_i64(struct cx_column_cursor *, size_t);
size_t cx_column_cursor_skip_str(struct cx_column_cursor *, size_t);

const struct cx_string *cx_column_cursor_get_str(
    const struct cx_column_cursor *);

#endif
