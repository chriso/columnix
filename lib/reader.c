#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "compress.h"
#include "file.h"
#include "reader.h"
#include "row.h"

struct cx_reader {
    struct cx_row_group_reader *reader;
    struct cx_predicate *predicate;
    struct cx_row_group *row_group;
    struct cx_row_cursor *row_cursor;
    size_t row_group_count;
    size_t position;
    bool match_all_rows;
    bool error;
};

struct cx_row_group_reader {
    FILE *file;
    void *mmap_ptr;
    size_t file_size;
    size_t row_count;
    struct cx_column *strings;
    struct {
        const struct cx_column_descriptor *descriptors;
        size_t count;
    } columns;
    struct {
        const struct cx_row_group_header *headers;
        size_t count;
    } row_groups;
    int32_t metadata;
};

struct cx_reader_query_context {
    struct cx_row_group_reader *reader;
    struct cx_predicate *predicate;
    size_t position;
    size_t row_group_count;
    void (*iter)(struct cx_row_cursor *, pthread_mutex_t *, void *);
    void *data;
    bool error;
    pthread_mutex_t mutex;
};

static struct cx_reader *cx_reader_new_impl(const char *path,
                                            struct cx_predicate *predicate,
                                            bool match_all_rows)
{
    if (!predicate)
        return NULL;
    struct cx_reader *reader = calloc(1, sizeof(*reader));
    if (!reader)
        return NULL;
    reader->reader = cx_row_group_reader_new(path);
    if (!reader->reader)
        goto error;
    reader->predicate = predicate;
    reader->match_all_rows = match_all_rows;
    reader->row_group_count =
        cx_row_group_reader_row_group_count(reader->reader);
    // validate and optimize the predicate
    if (reader->row_group_count && !match_all_rows) {
        struct cx_row_group *row_group =
            cx_row_group_reader_get(reader->reader, 0);
        if (!row_group)
            goto error;
        if (!cx_predicate_valid(predicate, row_group)) {
            cx_row_group_free(row_group);
            goto error;
        }
        cx_predicate_optimize(predicate, row_group);
        cx_row_group_free(row_group);
    }
    return reader;
error:
    free(reader);
    return NULL;
}

struct cx_reader *cx_reader_new(const char *path)
{
    return cx_reader_new_impl(path, cx_predicate_new_true(), true);
}

struct cx_reader *cx_reader_new_matching(const char *path,
                                         struct cx_predicate *predicate)
{
    return cx_reader_new_impl(path, predicate, false);
}

void cx_reader_free(struct cx_reader *reader)
{
    if (reader->row_cursor)
        cx_row_cursor_free(reader->row_cursor);
    if (reader->row_group)
        cx_row_group_free(reader->row_group);
    cx_predicate_free(reader->predicate);
    cx_row_group_reader_free(reader->reader);
    free(reader);
}

bool cx_reader_metadata(const struct cx_reader *reader, const char **metadata)
{
    return cx_row_group_reader_metadata(reader->reader, metadata);
}

void cx_reader_rewind(struct cx_reader *reader)
{
    if (reader->row_cursor)
        cx_row_cursor_free(reader->row_cursor);
    if (reader->row_group)
        cx_row_group_free(reader->row_group);
    reader->row_cursor = NULL;
    reader->row_group = NULL;
    reader->position = 0;
    reader->error = false;
}

static bool cx_reader_load_cursor(struct cx_reader *reader)
{
    reader->row_group =
        cx_row_group_reader_get(reader->reader, reader->position);
    if (!reader->row_group)
        goto error;
    reader->row_cursor =
        cx_row_cursor_new(reader->row_group, reader->predicate);
    if (!reader->row_cursor)
        goto error;
    return true;
error:
    if (reader->row_group)
        cx_row_group_free(reader->row_group);
    reader->row_group = NULL;
    return false;
}

static bool cx_reader_valid(const struct cx_reader *reader)
{
    return reader->position < reader->row_group_count;
}

static void cx_reader_advance(struct cx_reader *reader)
{
    if (reader->row_cursor) {
        cx_row_cursor_free(reader->row_cursor);
        reader->row_cursor = NULL;
    }
    if (reader->row_group) {
        cx_row_group_free(reader->row_group);
        reader->row_group = NULL;
    }
    reader->position++;
}

bool cx_reader_next(struct cx_reader *reader)
{
    if (reader->error)
        return false;
    for (; cx_reader_valid(reader); cx_reader_advance(reader)) {
        if (!reader->row_cursor)
            if (!cx_reader_load_cursor(reader))
                goto error;
        if (cx_row_cursor_next(reader->row_cursor))
            return true;
        if (cx_row_cursor_error(reader->row_cursor))
            goto error;
    }
    return false;
error:
    reader->error = true;
    return false;
}

bool cx_reader_error(const struct cx_reader *reader)
{
    return reader->error;
}

size_t cx_reader_row_count(struct cx_reader *reader)
{
    size_t total_row_count = cx_row_group_reader_row_count(reader->reader);
    if (reader->match_all_rows || !total_row_count)
        return total_row_count;
    cx_reader_rewind(reader);
    size_t count = 0;
    for (; cx_reader_valid(reader); cx_reader_advance(reader)) {
        if (!cx_reader_load_cursor(reader))
            goto error;
        count += cx_row_cursor_count(reader->row_cursor);
        if (cx_row_cursor_error(reader->row_cursor))
            goto error;
    }
    return count;
error:
    reader->error = true;
    return 0;
}

static void *cx_reader_query_thread(void *ptr)
{
    struct cx_reader_query_context *context = ptr;
    struct cx_row_group *row_group = NULL;
    struct cx_row_cursor *cursor = NULL;
    for (;;) {
        pthread_mutex_lock(&context->mutex);
        size_t position = context->position++;
        pthread_mutex_unlock(&context->mutex);
        if (position >= context->row_group_count)
            break;
        row_group = cx_row_group_reader_get(context->reader, position);
        if (!row_group)
            goto error;
        cursor = cx_row_cursor_new(row_group, context->predicate);
        if (!cursor)
            goto error;
        context->iter(cursor, &context->mutex, context->data);
        if (cx_row_cursor_error(cursor))
            goto error;
        cx_row_group_free(row_group);
        cx_row_cursor_free(cursor);
        row_group = NULL;
        cursor = NULL;
    }
    return NULL;
error:
    if (row_group)
        cx_row_group_free(row_group);
    if (cursor)
        cx_row_cursor_free(cursor);
    pthread_mutex_lock(&context->mutex);
    context->error = true;
    pthread_mutex_unlock(&context->mutex);
    return NULL;
}

bool cx_reader_query(struct cx_reader *reader, int thread_count, void *data,
                     void (*iter)(struct cx_row_cursor *, pthread_mutex_t *,
                                  void *))
{
    if (thread_count <= 0)
        return false;
    if (!reader->row_group_count)
        return true;
    pthread_t *threads = NULL;
    struct cx_reader_query_context query_context = {
        .reader = reader->reader,
        .predicate = reader->predicate,
        .position = 0,
        .row_group_count = reader->row_group_count,
        .iter = iter,
        .data = data,
        .error = false};
    if (pthread_mutex_init(&query_context.mutex, NULL))
        return false;
    threads = malloc(thread_count * sizeof(pthread_t));
    if (!threads)
        goto error;
    for (int i = 0; i < thread_count; i++)
        if (pthread_create(&threads[i], NULL, cx_reader_query_thread,
                           &query_context))
            goto error;
    for (int i = 0; i < thread_count; i++)
        pthread_join(threads[i], NULL);
    free(threads);
    pthread_mutex_destroy(&query_context.mutex);
    return !query_context.error;
error:
    free(threads);
    pthread_mutex_destroy(&query_context.mutex);
    return false;
}

size_t cx_reader_column_count(const struct cx_reader *reader)
{
    return cx_row_group_reader_column_count(reader->reader);
}

const char *cx_reader_column_name(const struct cx_reader *reader, size_t column)
{
    return cx_row_group_reader_column_name(reader->reader, column);
}

enum cx_column_type cx_reader_column_type(const struct cx_reader *reader,
                                          size_t column)
{
    return cx_row_group_reader_column_type(reader->reader, column);
}

enum cx_encoding_type cx_reader_column_encoding(const struct cx_reader *reader,
                                                size_t column)
{
    return cx_row_group_reader_column_encoding(reader->reader, column);
}

enum cx_compression_type cx_reader_column_compression(
    const struct cx_reader *reader, size_t column)
{
    return cx_row_group_reader_column_compression(reader->reader, column);
}

bool cx_reader_get_null(const struct cx_reader *reader, size_t column_index,
                        bool *value)
{
    if (!reader->row_cursor)
        return false;
    return cx_row_cursor_get_null(reader->row_cursor, column_index, value);
}

bool cx_reader_get_bit(const struct cx_reader *reader, size_t column_index,
                       bool *value)
{
    if (!reader->row_cursor)
        return false;
    return cx_row_cursor_get_bit(reader->row_cursor, column_index, value);
}

bool cx_reader_get_i32(const struct cx_reader *reader, size_t column_index,
                       int32_t *value)
{
    if (!reader->row_cursor)
        return false;
    return cx_row_cursor_get_i32(reader->row_cursor, column_index, value);
}

bool cx_reader_get_i64(const struct cx_reader *reader, size_t column_index,
                       int64_t *value)
{
    if (!reader->row_cursor)
        return false;
    return cx_row_cursor_get_i64(reader->row_cursor, column_index, value);
}

bool cx_reader_get_flt(const struct cx_reader *reader, size_t column_index,
                       float *value)
{
    if (!reader->row_cursor)
        return false;
    return cx_row_cursor_get_flt(reader->row_cursor, column_index, value);
}

bool cx_reader_get_dbl(const struct cx_reader *reader, size_t column_index,
                       double *value)
{
    if (!reader->row_cursor)
        return false;
    return cx_row_cursor_get_dbl(reader->row_cursor, column_index, value);
}

bool cx_reader_get_str(const struct cx_reader *reader, size_t column_index,
                       struct cx_string *value)
{
    if (!reader->row_cursor)
        return false;
    return cx_row_cursor_get_str(reader->row_cursor, column_index, value);
}

static const void *cx_row_group_reader_at(
    const struct cx_row_group_reader *reader, size_t offset)
{
    return (const void *)((uintptr_t)reader->mmap_ptr + offset);
}

struct cx_row_group_reader *cx_row_group_reader_new(const char *path)
{
    struct cx_row_group_reader *reader = calloc(1, sizeof(*reader));
    if (!reader)
        return NULL;
    reader->file = fopen(path, "rb");
    if (!reader->file)
        goto error;

    // get file size
    int fd = fileno(reader->file);
    struct stat stat;
    if (fstat(fd, &stat))
        goto error;
    size_t file_size = stat.st_size;
    if (!file_size)
        goto error;
    reader->file_size = file_size;

    // mmap the file
    void *mmap_ptr = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mmap_ptr == MAP_FAILED)
        goto error;
    reader->mmap_ptr = mmap_ptr;

    // check the footer
    if (file_size < sizeof(struct cx_footer))
        goto error;
    const struct cx_footer *footer =
        cx_row_group_reader_at(reader, file_size - sizeof(*footer));
    if (footer->magic != CX_FILE_MAGIC || footer->size < sizeof(*footer))
        goto error;

    // future extensions
    if (footer->version != CX_FILE_VERSION)
        goto error;

    // check the file contains the row group headers, column descriptors and
    // string repository
    size_t row_group_headers_size =
        footer->row_group_count * sizeof(struct cx_row_group_header);
    size_t descriptors_size =
        footer->column_count * sizeof(struct cx_column_descriptor);
    size_t headers_size =
        row_group_headers_size + descriptors_size + footer->size;
    if (file_size < headers_size)
        goto error;

    // load strings
    if (footer->strings_offset + footer->strings_size > file_size)
        goto error;
    const void *strings =
        cx_row_group_reader_at(reader, footer->strings_offset);
    reader->strings = cx_column_new_mmapped(CX_COLUMN_STR, CX_ENCODING_NONE,
                                            strings, footer->strings_size, 0);
    if (!reader->strings)
        goto error;

    // cache counts and header locations
    reader->row_count = footer->row_count;
    reader->columns.count = footer->column_count;
    reader->row_groups.count = footer->row_group_count;
    reader->columns.descriptors = cx_row_group_reader_at(
        reader, file_size - footer->size - descriptors_size);
    reader->row_groups.headers =
        cx_row_group_reader_at(reader, file_size - headers_size);
    reader->metadata = footer->metadata;

    return reader;
error:
    if (reader->mmap_ptr)
        munmap(reader->mmap_ptr, reader->file_size);
    if (reader->file)
        fclose(reader->file);
    if (reader->strings)
        cx_column_free(reader->strings);
    free(reader);
    return NULL;
}

size_t cx_row_group_reader_column_count(
    const struct cx_row_group_reader *reader)
{
    return reader->columns.count;
}

size_t cx_row_group_reader_row_count(const struct cx_row_group_reader *reader)
{
    return reader->row_count;
}

size_t cx_row_group_reader_row_group_count(
    const struct cx_row_group_reader *reader)
{
    return reader->row_groups.count;
}

const char *cx_row_group_reader_string(const struct cx_row_group_reader *reader,
                                       size_t index)
{
    size_t size;
    assert(cx_column_type(reader->strings) == CX_COLUMN_STR);
    assert(cx_column_encoding(reader->strings) == CX_ENCODING_NONE);
    const char *string = cx_column_export(reader->strings, &size);
    const char *end = string + size;
    for (; index && string < end; index--)
        string += strlen(string) + 1;
    assert(string <= end);
    return index == 0 && string != end ? string : NULL;
}

bool cx_row_group_reader_metadata(const struct cx_row_group_reader *reader,
                                  const char **metadata)
{
    if (reader->metadata < 0) {
        *metadata = NULL;
        return true;
    }
    const char *string = cx_row_group_reader_string(reader, reader->metadata);
    if (!string)
        return false;
    *metadata = string;
    return true;
}

const char *cx_row_group_reader_column_name(
    const struct cx_row_group_reader *reader, size_t column)
{
    if (column >= reader->columns.count)
        return NULL;
    return cx_row_group_reader_string(reader,
                                      reader->columns.descriptors[column].name);
}

enum cx_column_type cx_row_group_reader_column_type(
    const struct cx_row_group_reader *reader, size_t column)
{
    assert(column < reader->columns.count);
    return reader->columns.descriptors[column].type;
}

enum cx_encoding_type cx_row_group_reader_column_encoding(
    const struct cx_row_group_reader *reader, size_t column)
{
    assert(column < reader->columns.count);
    return reader->columns.descriptors[column].encoding;
}

enum cx_compression_type cx_row_group_reader_column_compression(
    const struct cx_row_group_reader *reader, size_t column)
{
    assert(column < reader->columns.count);
    return reader->columns.descriptors[column].compression;
}

struct cx_row_group *cx_row_group_reader_get(
    const struct cx_row_group_reader *reader, size_t index)
{
    const struct cx_row_group_header *row_group_header =
        &reader->row_groups.headers[index];
    size_t headers_size =
        reader->columns.count * sizeof(struct cx_column_header);
    size_t headers_offset = row_group_header->offset + row_group_header->size;
    if (headers_offset + headers_size > reader->file_size)
        return NULL;
    const struct cx_column_header *columns_headers =
        cx_row_group_reader_at(reader, headers_offset);
    struct cx_row_group *row_group = cx_row_group_new();
    if (!row_group)
        return NULL;
    for (size_t i = 0; i < reader->columns.count; i++) {
        const struct cx_column_descriptor *descriptor =
            &reader->columns.descriptors[i];
        const struct cx_column_header *header = &columns_headers[i * 2];
        if (header->offset + header->size > reader->file_size)
            goto error;
        const struct cx_column_header *null_header =
            &columns_headers[i * 2 + 1];
        if (null_header->offset + null_header->size > reader->file_size)
            goto error;

        struct cx_lazy_column column = {
            .type = descriptor->type,
            .encoding = header->encoding,
            .compression = header->compression,
            .index = &header->index,
            .ptr = cx_row_group_reader_at(reader, header->offset),
            .size = header->size,
            .decompressed_size = header->decompressed_size};

        struct cx_lazy_column nulls = {
            .type = CX_COLUMN_BIT,
            .encoding = null_header->encoding,
            .compression = null_header->compression,
            .index = &null_header->index,
            .ptr = cx_row_group_reader_at(reader, null_header->offset),
            .size = null_header->size,
            .decompressed_size = null_header->decompressed_size};

        if (!cx_row_group_add_lazy_column(row_group, &column, &nulls))
            goto error;
    }
    return row_group;
error:
    cx_row_group_free(row_group);
    return NULL;
}

void cx_row_group_reader_free(struct cx_row_group_reader *reader)
{
    if (reader->mmap_ptr)
        munmap(reader->mmap_ptr, reader->file_size);
    fclose(reader->file);
    cx_column_free(reader->strings);
    free(reader);
}
