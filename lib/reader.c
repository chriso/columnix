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

struct zcs_reader {
    FILE *file;
    size_t file_size;
    struct {
        const struct zcs_column_descriptor *descriptors;
        size_t count;
    } columns;
    struct {
        const struct zcs_row_group_header *headers;
        size_t count;
    } row_groups;
    struct {
        void *ptr;
        size_t size;
    } mmap;
};

struct zcs_reader *zcs_reader_new(const char *path)
{
    struct zcs_reader *reader = calloc(1, sizeof(*reader));
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
    void *start = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (start == MAP_FAILED)
        goto error;
    reader->mmap.ptr = start;
    reader->mmap.size = file_size;
    void *end = (void *)((uintptr_t)start + file_size);

    // check the footer
    if (file_size < sizeof(struct zcs_footer))
        goto error;
    struct zcs_footer *footer =
        (void *)((uintptr_t)end - sizeof(struct zcs_footer));
    if (footer->magic != ZCS_FILE_MAGIC)
        goto error;

    // check the file contains the row group and column headers
    size_t row_group_headers_size =
        footer->row_group_count * sizeof(struct zcs_row_group_header);
    size_t column_descriptors_size =
        footer->column_count * sizeof(struct zcs_column_descriptor);
    size_t headers_size = row_group_headers_size + column_descriptors_size +
                          sizeof(struct zcs_footer);
    if (file_size < headers_size)
        goto error;

    reader->columns.count = footer->column_count;
    reader->row_groups.count = footer->row_group_count;
    reader->columns.descriptors =
        (const void *)((uintptr_t)end - sizeof(struct zcs_footer) -
                       column_descriptors_size);
    reader->row_groups.headers = (const void *)((uintptr_t)end - headers_size);

    return reader;
error:
    if (reader->mmap.ptr)
        munmap(reader->mmap.ptr, reader->mmap.size);
    if (reader->file)
        fclose(reader->file);
    free(reader);
    return NULL;
}

size_t zcs_reader_column_count(const struct zcs_reader *reader)
{
    return reader->columns.count;
}

size_t zcs_reader_row_group_count(const struct zcs_reader *reader)
{
    return reader->row_groups.count;
}

enum zcs_column_type zcs_reader_column_type(const struct zcs_reader *reader,
                                            size_t column)
{
    assert(column < reader->columns.count);
    return reader->columns.descriptors[column].type;
}

enum zcs_encoding_type zcs_reader_column_encoding(
    const struct zcs_reader *reader, size_t column)
{
    assert(column < reader->columns.count);
    return reader->columns.descriptors[column].encoding;
}

enum zcs_compression_type zcs_reader_column_compression(
    const struct zcs_reader *reader, size_t column)
{
    assert(column < reader->columns.count);
    return reader->columns.descriptors[column].compression;
}

static const struct zcs_column_header *zcs_reader_column_header(
    struct zcs_reader *reader, size_t row_group, size_t column)
{
    if (row_group >= reader->row_groups.count ||
        column >= reader->columns.count)
        return NULL;
    const struct zcs_row_group_header *row_group_header =
        &reader->row_groups.headers[row_group];
    size_t headers_size =
        reader->columns.count * sizeof(struct zcs_column_header);
    size_t headers_offset = row_group_header->offset + row_group_header->size;
    if (headers_offset + headers_size > reader->file_size)
        return NULL;
    const struct zcs_column_header *header =
        (void *)((uintptr_t)reader->mmap.ptr + headers_offset +
                 (column * sizeof(*header)));
    if (header->offset + header->size > reader->file_size)
        return NULL;
    return header;
}

struct zcs_row_group *zcs_reader_row_group(struct zcs_reader *reader,
                                           size_t index)
{
    struct zcs_row_group *row_group = zcs_row_group_new();
    if (!row_group)
        return NULL;
    for (size_t i = 0; i < reader->columns.count; i++) {
        const struct zcs_column_descriptor *descriptor =
            &reader->columns.descriptors[i];
        const struct zcs_column_header *header =
            zcs_reader_column_header(reader, index, i);
        if (!header)
            goto error;
        const void *ptr =
            (const void *)((uintptr_t)reader->mmap.ptr + header->offset);
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

void zcs_reader_free(struct zcs_reader *reader)
{
    if (reader->mmap.ptr)
        munmap(reader->mmap.ptr, reader->mmap.size);
    fclose(reader->file);
    free(reader);
}
