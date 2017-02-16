#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64
#define _LARGEFILE_SOURCE

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "row_group.h"

#define ZCS_FILE_MAGIC 0x65726f7473637a1dLLU

#define ZCS_WRITE_ALIGN 8

struct zcs_header {
    uint64_t magic;
};

struct zcs_footer {
    uint32_t row_group_count;
    uint32_t column_count;
    uint64_t magic;
};

struct zcs_column_descriptor {
    uint32_t type;
    uint32_t encoding;
};

struct zcs_row_group_header {
    uint64_t size;
    uint64_t offset;
};

struct zcs_column_header {
    uint64_t size;
    uint64_t offset;
    struct zcs_column_index index;
};

struct zcs_writer {
    FILE *file;
    struct {
        struct zcs_column_descriptor *descriptors;
        size_t count;
        size_t size;
    } columns;
    struct {
        struct zcs_row_group_header *headers;
        size_t count;
        size_t size;
    } row_groups;
    bool header_written;
    bool footer_written;
};

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

struct zcs_writer *zcs_writer_new(const char *path)
{
    struct zcs_writer *writer = calloc(1, sizeof(*writer));
    if (!writer)
        return NULL;
    writer->file = fopen(path, "wb");
    if (!writer->file)
        goto error;
    return writer;
error:
    free(writer);
    return NULL;
}

bool zcs_writer_add_column(struct zcs_writer *writer, enum zcs_column_type type,
                           enum zcs_encoding_type encoding)
{
    if (writer->header_written)
        return false;

    // make room for the extra column descriptor
    if (!writer->columns.count) {
        writer->columns.descriptors =
            malloc(sizeof(*writer->columns.descriptors));
        if (!writer->columns.descriptors)
            return false;
        writer->columns.size = 1;
    } else if (writer->columns.count == writer->columns.size) {
        size_t new_size = writer->columns.size * 2;
        assert(new_size && new_size > writer->columns.size);
        struct zcs_column_descriptor *descriptors = realloc(
            writer->columns.descriptors, new_size * sizeof(*descriptors));
        if (!descriptors)
            return false;
        writer->columns.descriptors = descriptors;
        writer->columns.size = new_size;
    }

    // save column type and encoding
    struct zcs_column_descriptor *descriptor =
        &writer->columns.descriptors[writer->columns.count++];
    descriptor->type = type;
    descriptor->encoding = encoding;

    return true;
}

static size_t zcs_write_align(size_t offset)
{
    size_t mod = offset % ZCS_WRITE_ALIGN;
    return mod ? offset - mod + ZCS_WRITE_ALIGN : offset;
}

static size_t zcs_writer_offset(const struct zcs_writer *writer)
{
    return ftello(writer->file);
}

static bool zcs_writer_seek(const struct zcs_writer *writer, size_t offset)
{
    return fseeko(writer->file, offset, SEEK_SET) == 0;
}

static bool zcs_writer_write(const struct zcs_writer *writer, const void *buf,
                             size_t size)
{
    if (!size)
        return true;
    size_t offset = zcs_writer_offset(writer);
    size_t aligned_offset = zcs_write_align(offset);
    if (offset != aligned_offset && !zcs_writer_seek(writer, aligned_offset))
        return false;
    return fwrite(buf, size, 1, writer->file) == 1;
}

static bool zcs_writer_ensure_header(struct zcs_writer *writer)
{
    if (writer->header_written)
        return true;
    struct zcs_header header = {ZCS_FILE_MAGIC};
    if (!zcs_writer_write(writer, &header, sizeof(header)))
        goto error;
    writer->header_written = true;
    return true;
error:
    rewind(writer->file);
    return false;
}

bool zcs_writer_add_row_group(struct zcs_writer *writer,
                              struct zcs_row_group *row_group)
{
    if (writer->footer_written)
        return false;

    // check column types match
    size_t column_count = zcs_row_group_column_count(row_group);
    if (column_count != writer->columns.count)
        return false;
    if (!column_count)
        return true;  // noop
    for (size_t i = 0; i < column_count; i++) {
        struct zcs_column_descriptor *descriptor =
            &writer->columns.descriptors[i];
        if (descriptor->type != zcs_row_group_column_type(row_group, i) ||
            descriptor->encoding != zcs_row_group_column_encoding(row_group, i))
            return false;
    }

    // make room for the extra row group header
    if (!writer->row_groups.count) {
        writer->row_groups.headers =
            malloc(sizeof(*writer->row_groups.headers));
        if (!writer->row_groups.headers)
            return false;
        writer->row_groups.size = 1;
    } else if (writer->row_groups.count == writer->row_groups.size) {
        size_t new_size = writer->row_groups.size * 2;
        assert(new_size && new_size > writer->row_groups.size);
        struct zcs_row_group_header *headers =
            realloc(writer->row_groups.headers, new_size * sizeof(*headers));
        if (!headers)
            return false;
        writer->row_groups.headers = headers;
        writer->row_groups.size = new_size;
    }

    // write the header if it hasn't already been written
    if (!zcs_writer_ensure_header(writer))
        return false;

    size_t row_group_offset = zcs_writer_offset(writer);

    size_t column_headers_size =
        column_count * sizeof(struct zcs_column_header);
    struct zcs_column_header *column_headers = malloc(column_headers_size);
    if (!column_headers)
        goto error;

    // write columns
    for (size_t i = 0; i < column_count; i++) {
        const struct zcs_column *column = zcs_row_group_column(row_group, i);
        if (!column)
            goto error;
        size_t column_size;
        const void *buffer = zcs_column_export(column, &column_size);
        column_headers[i].size = column_size;
        column_headers[i].offset = zcs_write_align(zcs_writer_offset(writer));
        const struct zcs_column_index *index = zcs_column_index(column);
        memcpy(&column_headers[i].index, index, sizeof(*index));
        if (!zcs_writer_write(writer, buffer, column_size))
            goto error;
    }

    // update the row group header
    struct zcs_row_group_header *row_group_header =
        &writer->row_groups.headers[writer->row_groups.count++];
    row_group_header->size =
        zcs_write_align(zcs_writer_offset(writer)) - row_group_offset;
    row_group_header->offset = row_group_offset;

    // write column headers
    if (!zcs_writer_write(writer, column_headers, column_headers_size))
        goto error;

    free(column_headers);
    return true;
error:
    zcs_writer_seek(writer, row_group_offset);
    free(column_headers);
    return false;
}

bool zcs_writer_finish(struct zcs_writer *writer, bool sync)
{
    if (writer->footer_written)
        return true;
    if (!zcs_writer_ensure_header(writer))
        return false;

    size_t offset = zcs_writer_offset(writer);

    // write row group headers
    size_t row_group_headers_size =
        writer->row_groups.count * sizeof(struct zcs_row_group_header);
    if (!zcs_writer_write(writer, writer->row_groups.headers,
                          row_group_headers_size))
        goto error;

    // write column headers
    size_t column_descriptors_size =
        writer->columns.count * sizeof(struct zcs_column_descriptor);
    if (!zcs_writer_write(writer, writer->columns.descriptors,
                          column_descriptors_size))
        goto error;

    // write the footer
    struct zcs_footer footer = {writer->row_groups.count, writer->columns.count,
                                ZCS_FILE_MAGIC};
    if (!zcs_writer_write(writer, &footer, sizeof(footer)))
        goto error;

    // set the file size
    int fd = fileno(writer->file);
    if (ftruncate(fd, zcs_writer_offset(writer)))
        goto error;

    // sync the file
    if (sync) {
#ifdef __APPLE__
        if (fcntl(fd, F_FULLFSYNC))
            goto error;
#else
        if (fsync(fd))
            goto error;
#endif
    }

    writer->footer_written = true;
    return true;
error:
    zcs_writer_seek(writer, offset);
    return false;
}

void zcs_writer_free(struct zcs_writer *writer)
{
    if (writer->columns.descriptors)
        free(writer->columns.descriptors);
    if (writer->row_groups.headers)
        free(writer->row_groups.headers);
    fclose(writer->file);
    free(writer);
}

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
        if (!zcs_row_group_add_column_lazy(row_group, descriptor->type,
                                           descriptor->encoding, &header->index,
                                           ptr, header->size))
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
