#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "compress.h"
#include "file.h"
#include "reader.h"
#include "row.h"

struct zcs_reader {
    struct zcs_row_group_reader *reader;
    struct zcs_predicate *predicate;
    struct zcs_row_group *row_group;
    struct zcs_row_cursor *row_cursor;
    size_t row_group_count;
    size_t position;
    bool match_all_rows;
    bool optimize_predicate;
    bool error;
};

struct zcs_row_group_reader {
    FILE *file;
    void *mmap_ptr;
    size_t file_size;
    size_t row_count;
    struct {
        const struct zcs_column_descriptor *descriptors;
        size_t count;
    } columns;
    struct {
        const struct zcs_row_group_header *headers;
        size_t count;
    } row_groups;
};

static struct zcs_reader *zcs_reader_new_impl(const char *path,
                                              struct zcs_predicate *predicate,
                                              bool match_all_rows)
{
    if (!predicate)
        return NULL;
    struct zcs_reader *reader = calloc(1, sizeof(*reader));
    if (!reader)
        return NULL;
    reader->reader = zcs_row_group_reader_new(path);
    if (!reader->reader)
        goto error;
    reader->predicate = predicate;
    reader->match_all_rows = match_all_rows;
    reader->optimize_predicate = !match_all_rows;
    reader->row_group_count =
        zcs_row_group_reader_row_group_count(reader->reader);
    return reader;
error:
    free(reader);
    return NULL;
}

struct zcs_reader *zcs_reader_new(const char *path)
{
    return zcs_reader_new_impl(path, zcs_predicate_new_true(), true);
}

struct zcs_reader *zcs_reader_new_matching(const char *path,
                                           struct zcs_predicate *predicate)
{
    return zcs_reader_new_impl(path, predicate, false);
}

void zcs_reader_free(struct zcs_reader *reader)
{
    if (reader->row_cursor)
        zcs_row_cursor_free(reader->row_cursor);
    if (reader->row_group)
        zcs_row_group_free(reader->row_group);
    zcs_predicate_free(reader->predicate);
    zcs_row_group_reader_free(reader->reader);
    free(reader);
}

void zcs_reader_rewind(struct zcs_reader *reader)
{
    if (reader->row_cursor)
        zcs_row_cursor_free(reader->row_cursor);
    if (reader->row_group)
        zcs_row_group_free(reader->row_group);
    reader->row_cursor = NULL;
    reader->row_group = NULL;
    reader->position = 0;
    reader->error = false;
}

static bool zcs_reader_load_cursor(struct zcs_reader *reader)
{
    reader->row_group =
        zcs_row_group_reader_get(reader->reader, reader->position);
    if (!reader->row_group)
        goto error;
    // validate and optimize the predicate on first use
    if (!reader->position && reader->optimize_predicate) {
        if (!zcs_predicate_valid(reader->predicate, reader->row_group))
            goto error;
        zcs_predicate_optimize(reader->predicate, reader->row_group);
        reader->optimize_predicate = false;
    }
    reader->row_cursor =
        zcs_row_cursor_new(reader->row_group, reader->predicate);
    if (!reader->row_cursor)
        goto error;
    return true;
error:
    if (reader->row_group)
        zcs_row_group_free(reader->row_group);
    reader->row_group = NULL;
    return false;
}

static bool zcs_reader_valid(const struct zcs_reader *reader)
{
    return reader->position < reader->row_group_count;
}

static void zcs_reader_advance(struct zcs_reader *reader)
{
    if (reader->row_cursor) {
        zcs_row_cursor_free(reader->row_cursor);
        reader->row_cursor = NULL;
    }
    if (reader->row_group) {
        zcs_row_group_free(reader->row_group);
        reader->row_group = NULL;
    }
    reader->position++;
}

bool zcs_reader_next(struct zcs_reader *reader)
{
    if (reader->error)
        return false;
    for (; zcs_reader_valid(reader); zcs_reader_advance(reader)) {
        if (!reader->row_cursor)
            if (!zcs_reader_load_cursor(reader))
                goto error;
        if (zcs_row_cursor_next(reader->row_cursor))
            return true;
        if (zcs_row_cursor_error(reader->row_cursor))
            goto error;
    }
    return false;
error:
    reader->error = true;
    return false;
}

bool zcs_reader_error(const struct zcs_reader *reader)
{
    return reader->error;
}

size_t zcs_reader_row_count(struct zcs_reader *reader)
{
    size_t total_row_count = zcs_row_group_reader_row_count(reader->reader);
    if (reader->match_all_rows || !total_row_count)
        return total_row_count;
    zcs_reader_rewind(reader);
    size_t count = 0;
    for (; zcs_reader_valid(reader); zcs_reader_advance(reader)) {
        if (!zcs_reader_load_cursor(reader))
            goto error;
        count += zcs_row_cursor_count(reader->row_cursor);
        if (zcs_row_cursor_error(reader->row_cursor))
            goto error;
    }
    return count;
error:
    reader->error = true;
    return 0;
}

size_t zcs_reader_column_count(const struct zcs_reader *reader)
{
    return zcs_row_group_reader_column_count(reader->reader);
}

enum zcs_column_type zcs_reader_column_type(const struct zcs_reader *reader,
                                            size_t column)
{
    return zcs_row_group_reader_column_type(reader->reader, column);
}

enum zcs_encoding_type zcs_reader_column_encoding(
    const struct zcs_reader *reader, size_t column)
{
    return zcs_row_group_reader_column_encoding(reader->reader, column);
}

enum zcs_compression_type zcs_reader_column_compression(
    const struct zcs_reader *reader, size_t column)
{
    return zcs_row_group_reader_column_compression(reader->reader, column);
}

bool zcs_reader_get_bit(const struct zcs_reader *reader, size_t column_index,
                        bool *value)
{
    if (!reader->row_cursor)
        return false;
    return zcs_row_cursor_get_bit(reader->row_cursor, column_index, value);
}

bool zcs_reader_get_i32(const struct zcs_reader *reader, size_t column_index,
                        int32_t *value)
{
    if (!reader->row_cursor)
        return false;
    return zcs_row_cursor_get_i32(reader->row_cursor, column_index, value);
}

bool zcs_reader_get_i64(const struct zcs_reader *reader, size_t column_index,
                        int64_t *value)
{
    if (!reader->row_cursor)
        return false;
    return zcs_row_cursor_get_i64(reader->row_cursor, column_index, value);
}

bool zcs_reader_get_str(const struct zcs_reader *reader, size_t column_index,
                        const struct zcs_string **value)
{
    if (!reader->row_cursor)
        return false;
    return zcs_row_cursor_get_str(reader->row_cursor, column_index, value);
}

static const void *zcs_row_group_reader_at(
    const struct zcs_row_group_reader *reader, size_t offset)
{
    return (const void *)((uintptr_t)reader->mmap_ptr + offset);
}

struct zcs_row_group_reader *zcs_row_group_reader_new(const char *path)
{
    struct zcs_row_group_reader *reader = calloc(1, sizeof(*reader));
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
    if (file_size < sizeof(struct zcs_footer))
        goto error;
    const struct zcs_footer *footer =
        zcs_row_group_reader_at(reader, file_size - sizeof(struct zcs_footer));
    if (footer->magic != ZCS_FILE_MAGIC)
        goto error;

    // check the file contains the row group headers and column descriptors
    size_t row_group_headers_size =
        footer->row_group_count * sizeof(struct zcs_row_group_header);
    size_t descriptors_size =
        footer->column_count * sizeof(struct zcs_column_descriptor);
    size_t headers_size =
        row_group_headers_size + descriptors_size + sizeof(struct zcs_footer);
    if (file_size < headers_size)
        goto error;

    // cache counts and header locations
    reader->row_count = footer->row_count;
    reader->columns.count = footer->column_count;
    reader->row_groups.count = footer->row_group_count;
    reader->columns.descriptors = zcs_row_group_reader_at(
        reader, file_size - sizeof(struct zcs_footer) - descriptors_size);
    reader->row_groups.headers =
        zcs_row_group_reader_at(reader, file_size - headers_size);

    return reader;
error:
    if (reader->mmap_ptr)
        munmap(reader->mmap_ptr, reader->file_size);
    if (reader->file)
        fclose(reader->file);
    free(reader);
    return NULL;
}

size_t zcs_row_group_reader_column_count(
    const struct zcs_row_group_reader *reader)
{
    return reader->columns.count;
}

size_t zcs_row_group_reader_row_count(const struct zcs_row_group_reader *reader)
{
    return reader->row_count;
}

size_t zcs_row_group_reader_row_group_count(
    const struct zcs_row_group_reader *reader)
{
    return reader->row_groups.count;
}

enum zcs_column_type zcs_row_group_reader_column_type(
    const struct zcs_row_group_reader *reader, size_t column)
{
    assert(column < reader->columns.count);
    return reader->columns.descriptors[column].type;
}

enum zcs_encoding_type zcs_row_group_reader_column_encoding(
    const struct zcs_row_group_reader *reader, size_t column)
{
    assert(column < reader->columns.count);
    return reader->columns.descriptors[column].encoding;
}

enum zcs_compression_type zcs_row_group_reader_column_compression(
    const struct zcs_row_group_reader *reader, size_t column)
{
    assert(column < reader->columns.count);
    return reader->columns.descriptors[column].compression;
}

struct zcs_row_group *zcs_row_group_reader_get(
    const struct zcs_row_group_reader *reader, size_t index)
{
    const struct zcs_row_group_header *row_group_header =
        &reader->row_groups.headers[index];
    size_t headers_size =
        reader->columns.count * sizeof(struct zcs_column_header);
    size_t headers_offset = row_group_header->offset + row_group_header->size;
    if (headers_offset + headers_size > reader->file_size)
        return NULL;
    const struct zcs_column_header *columns_headers =
        zcs_row_group_reader_at(reader, headers_offset);
    struct zcs_row_group *row_group = zcs_row_group_new();
    if (!row_group)
        return NULL;
    for (size_t i = 0; i < reader->columns.count; i++) {
        const struct zcs_column_descriptor *descriptor =
            &reader->columns.descriptors[i];
        const struct zcs_column_header *header = &columns_headers[i];
        if (header->offset + header->size > reader->file_size)
            goto error;
        const void *ptr = zcs_row_group_reader_at(reader, header->offset);
        if (!zcs_row_group_add_column_lazy(
                row_group, descriptor->type, descriptor->encoding,
                descriptor->compression, &header->index, ptr, header->size,
                header->decompressed_size))
            goto error;
    }
    return row_group;
error:
    zcs_row_group_free(row_group);
    return NULL;
}

void zcs_row_group_reader_free(struct zcs_row_group_reader *reader)
{
    if (reader->mmap_ptr)
        munmap(reader->mmap_ptr, reader->file_size);
    fclose(reader->file);
    free(reader);
}
