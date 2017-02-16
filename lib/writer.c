#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64
#define _LARGEFILE_SOURCE

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "compression.h"
#include "file.h"
#include "writer.h"

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
                           enum zcs_encoding_type encoding,
                           enum zcs_compression_type compression, int level)
{
    if (writer->header_written)
        return false;

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

    struct zcs_column_descriptor *descriptor =
        &writer->columns.descriptors[writer->columns.count++];
    descriptor->type = type;
    descriptor->encoding = encoding;
    descriptor->compression = compression;
    descriptor->level = level;

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

        column_headers[i].decompressed_size = column_size;
        column_headers[i].offset = zcs_write_align(zcs_writer_offset(writer));
        const struct zcs_column_index *index = zcs_column_index(column);
        memcpy(&column_headers[i].index, index, sizeof(*index));

        const struct zcs_column_descriptor *descriptor =
            &writer->columns.descriptors[i];
        if (descriptor->compression && column_size) {
            size_t compressed_size;
            void *compressed =
                zcs_compress(descriptor->compression, descriptor->level, buffer,
                             column_size, &compressed_size);
            if (!compressed)
                goto error;
            column_headers[i].size = compressed_size;
            bool ok = zcs_writer_write(writer, compressed, compressed_size);
            free(compressed);
            if (!ok)
                goto error;
        } else {
            column_headers[i].size = column_size;
            if (!zcs_writer_write(writer, buffer, column_size))
                goto error;
        }
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
