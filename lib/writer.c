#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64
#define _LARGEFILE_SOURCE

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "compress.h"
#include "file.h"
#include "writer.h"

#define CX_NULL_COMPRESSION_TYPE CX_COMPRESSION_LZ4
#define CX_NULL_COMPRESSION_LEVEL 0

struct cx_writer_physical_column {
    struct cx_column *values;
    struct cx_column *nulls;
};

struct cx_writer {
    struct cx_row_group_writer *writer;
    struct cx_writer_physical_column *columns;
    size_t row_group_size;
    size_t column_count;
    size_t position;
};

struct cx_row_group_writer {
    FILE *file;
    struct cx_column *strings;
    struct {
        struct cx_column_descriptor *descriptors;
        size_t count;
        size_t size;
    } columns;
    struct {
        struct cx_row_group_header *headers;
        size_t count;
        size_t size;
    } row_groups;
    size_t row_count;
    bool header_written;
    bool footer_written;
};

struct cx_writer *cx_writer_new(const char *path, size_t row_group_size)
{
    struct cx_writer *writer = calloc(1, sizeof(*writer));
    if (!writer)
        return NULL;
    writer->writer = cx_row_group_writer_new(path);
    if (!writer->writer)
        goto error;
    writer->row_group_size = row_group_size;
    return writer;
error:
    free(writer);
    return NULL;
}

void cx_writer_free(struct cx_writer *writer)
{
    if (writer->columns) {
        for (size_t i = 0; i < writer->column_count; i++) {
            cx_column_free(writer->columns[i].values);
            cx_column_free(writer->columns[i].nulls);
        }
        free(writer->columns);
    }
    cx_row_group_writer_free(writer->writer);
    free(writer);
}

bool cx_writer_add_column(struct cx_writer *writer, const char *name,
                          enum cx_column_type type,
                          enum cx_encoding_type encoding,
                          enum cx_compression_type compression, int level)
{
    if (!cx_row_group_writer_add_column(writer->writer, name, type, encoding,
                                        compression, level))
        return false;
    writer->column_count++;
    return true;
}

static bool cx_writer_flush_row_group(struct cx_writer *writer)
{
    if (!writer->columns)
        return true;
    struct cx_row_group *row_group = cx_row_group_new();
    if (!row_group)
        return false;
    for (size_t i = 0; i < writer->column_count; i++)
        if (!cx_row_group_add_column(row_group, writer->columns[i].values,
                                     writer->columns[i].nulls))
            goto error;
    if (!cx_row_group_writer_put(writer->writer, row_group))
        goto error;
    cx_row_group_free(row_group);
    for (size_t i = 0; i < writer->column_count; i++) {
        cx_column_free(writer->columns[i].values);
        cx_column_free(writer->columns[i].nulls);
    }
    free(writer->columns);
    writer->columns = NULL;
    writer->position = 0;
    return true;
error:
    cx_row_group_free(row_group);
    return false;
}

static bool cx_writer_ensure_columns(struct cx_writer *writer)
{
    assert(!writer->columns && writer->column_count);
    writer->columns = calloc(writer->column_count, sizeof(*writer->columns));
    if (!writer->columns)
        return false;
    for (size_t i = 0; i < writer->column_count; i++) {
        struct cx_column_descriptor *descriptor =
            &writer->writer->columns.descriptors[i];
        writer->columns[i].values =
            cx_column_new(descriptor->type, descriptor->encoding);
        if (!writer->columns[i].values)
            goto error;
        writer->columns[i].nulls =
            cx_column_new(CX_COLUMN_BIT, CX_ENCODING_NONE);
        if (!writer->columns[i].nulls)
            goto error;
    }
    return true;
error:
    for (size_t i = 0; i < writer->column_count; i++) {
        if (writer->columns[i].values)
            cx_column_free(writer->columns[i].values);
        if (writer->columns[i].nulls)
            cx_column_free(writer->columns[i].nulls);
    }
    free(writer->columns);
    writer->columns = NULL;
    return false;
}

static bool cx_writer_put_check(struct cx_writer *writer, size_t column_index)
{
    if (column_index >= writer->column_count)
        return false;
    if (column_index == 0 && writer->position == writer->row_group_size)
        if (!cx_writer_flush_row_group(writer))
            return false;
    if (!writer->columns)
        if (!cx_writer_ensure_columns(writer))
            return false;
    return true;
}

bool cx_writer_put_bit(struct cx_writer *writer, size_t column_index,
                       bool value)
{
    if (!cx_writer_put_check(writer, column_index))
        return false;
    if (!cx_column_put_bit(writer->columns[column_index].values, value))
        return false;
    if (!cx_column_put_bit(writer->columns[column_index].nulls, false))
        return false;
    writer->position += (column_index == 0);
    return true;
}

bool cx_writer_put_i32(struct cx_writer *writer, size_t column_index,
                       int32_t value)
{
    if (!cx_writer_put_check(writer, column_index))
        return false;
    if (!cx_column_put_i32(writer->columns[column_index].values, value))
        return false;
    if (!cx_column_put_bit(writer->columns[column_index].nulls, false))
        return false;
    writer->position += (column_index == 0);
    return true;
}

bool cx_writer_put_i64(struct cx_writer *writer, size_t column_index,
                       int64_t value)
{
    if (!cx_writer_put_check(writer, column_index))
        return false;
    if (!cx_column_put_i64(writer->columns[column_index].values, value))
        return false;
    if (!cx_column_put_bit(writer->columns[column_index].nulls, false))
        return false;
    writer->position += (column_index == 0);
    return true;
}

bool cx_writer_put_str(struct cx_writer *writer, size_t column_index,
                       const char *value)
{
    if (!cx_writer_put_check(writer, column_index))
        return false;
    if (!cx_column_put_str(writer->columns[column_index].values, value))
        return false;
    if (!cx_column_put_bit(writer->columns[column_index].nulls, false))
        return false;
    writer->position += (column_index == 0);
    return true;
}

bool cx_writer_put_null(struct cx_writer *writer, size_t column_index)
{
    if (!cx_writer_put_check(writer, column_index))
        return false;
    if (!cx_column_put_unit(writer->columns[column_index].values))
        return false;
    if (!cx_column_put_bit(writer->columns[column_index].nulls, true))
        return false;
    writer->position += (column_index == 0);
    return true;
}

bool cx_writer_finish(struct cx_writer *writer, bool sync)
{
    if (!cx_writer_flush_row_group(writer))
        return false;
    return cx_row_group_writer_finish(writer->writer, sync);
}

struct cx_row_group_writer *cx_row_group_writer_new(const char *path)
{
    struct cx_row_group_writer *writer = calloc(1, sizeof(*writer));
    if (!writer)
        return NULL;
    writer->strings = cx_column_new(CX_COLUMN_STR, CX_ENCODING_NONE);
    if (!writer->strings)
        goto error;
    writer->file = fopen(path, "wb");
    if (!writer->file)
        goto error;
    return writer;
error:
    if (writer->strings)
        cx_column_free(writer->strings);
    free(writer);
    return NULL;
}

bool cx_row_group_writer_add_column(struct cx_row_group_writer *writer,
                                    const char *name, enum cx_column_type type,
                                    enum cx_encoding_type encoding,
                                    enum cx_compression_type compression,
                                    int level)
{
    if (writer->header_written)
        return false;

    if (!writer->columns.count) {
        writer->columns.descriptors =
            malloc(sizeof(*writer->columns.descriptors) * 8);
        if (!writer->columns.descriptors)
            return false;
        writer->columns.size = 8;
    } else if (writer->columns.count == writer->columns.size) {
        size_t new_size = writer->columns.size * 2;
        assert(new_size && new_size > writer->columns.size);
        struct cx_column_descriptor *descriptors = realloc(
            writer->columns.descriptors, new_size * sizeof(*descriptors));
        if (!descriptors)
            return false;
        writer->columns.descriptors = descriptors;
        writer->columns.size = new_size;
    }

    if (!cx_column_put_str(writer->strings, name))
        return false;

    size_t string_count = cx_column_count(writer->strings);
    assert(string_count);

    struct cx_column_descriptor *descriptor =
        &writer->columns.descriptors[writer->columns.count++];
    memset(descriptor, 0, sizeof(*descriptor));
    descriptor->name = string_count - 1;
    descriptor->type = type;
    descriptor->encoding = encoding;
    descriptor->compression = compression;
    descriptor->level = level;

    return true;
}

static size_t cx_write_align(size_t offset)
{
    size_t mod = offset % CX_WRITE_ALIGN;
    return mod ? offset - mod + CX_WRITE_ALIGN : offset;
}

static size_t cx_row_group_writer_offset(
    const struct cx_row_group_writer *writer)
{
    return ftello(writer->file);
}

static bool cx_row_group_writer_seek(const struct cx_row_group_writer *writer,
                                     size_t offset)
{
    return fseeko(writer->file, offset, SEEK_SET) == 0;
}

static bool cx_row_group_writer_write(const struct cx_row_group_writer *writer,
                                      const void *buf, size_t size)
{
    if (!size)
        return true;
    size_t offset = cx_row_group_writer_offset(writer);
    size_t aligned_offset = cx_write_align(offset);
    if (offset != aligned_offset &&
        !cx_row_group_writer_seek(writer, aligned_offset))
        return false;
    return fwrite(buf, size, 1, writer->file) == 1;
}

static bool cx_row_group_writer_ensure_header(
    struct cx_row_group_writer *writer)
{
    if (writer->header_written)
        return true;
    struct cx_header header = {CX_FILE_MAGIC, CX_FILE_VERSION};
    if (!cx_row_group_writer_write(writer, &header, sizeof(header)))
        goto error;
    writer->header_written = true;
    return true;
error:
    rewind(writer->file);
    return false;
}

static bool cx_row_group_writer_put_column(struct cx_row_group_writer *writer,
                                           const struct cx_column *column,
                                           struct cx_column_header *header,
                                           enum cx_compression_type compression,
                                           int compression_level)
{
    size_t column_size;
    const void *buffer = cx_column_export(column, &column_size);
    header->decompressed_size = column_size;
    header->offset = cx_write_align(cx_row_group_writer_offset(writer));
    const struct cx_column_index *index = cx_column_index(column);
    memcpy(&header->index, index, sizeof(*index));
    size_t compressed_size = 0;
    void *compressed = NULL;
    if (compression && column_size) {
        compressed = cx_compress(compression, compression_level, buffer,
                                 column_size, &compressed_size);
        if (!compressed)
            goto error;
        // fallback if the compression leads to an increase in size
        if (compressed_size >= column_size) {
            header->compression = CX_COMPRESSION_NONE;
        } else {
            header->compression = compression;
            buffer = compressed;
            column_size = compressed_size;
        }
    }
    header->size = column_size;
    if (!cx_row_group_writer_write(writer, buffer, column_size))
        goto error;
    if (compressed)
        free(compressed);
    return true;
error:
    if (compressed)
        free(compressed);
    return false;
}

bool cx_row_group_writer_put(struct cx_row_group_writer *writer,
                             struct cx_row_group *row_group)
{
    if (writer->footer_written)
        return false;

    // check column types match
    size_t column_count = cx_row_group_column_count(row_group);
    if (column_count != writer->columns.count)
        return false;
    if (!column_count)
        return true;  // noop
    for (size_t i = 0; i < column_count; i++) {
        struct cx_column_descriptor *descriptor =
            &writer->columns.descriptors[i];
        if (descriptor->type != cx_row_group_column_type(row_group, i) ||
            descriptor->encoding != cx_row_group_column_encoding(row_group, i))
            return false;
    }

    // make room for the extra row group header
    if (!writer->row_groups.count) {
        writer->row_groups.headers =
            malloc(sizeof(*writer->row_groups.headers) * 16);
        if (!writer->row_groups.headers)
            return false;
        writer->row_groups.size = 16;
    } else if (writer->row_groups.count == writer->row_groups.size) {
        size_t new_size = writer->row_groups.size * 2;
        assert(new_size && new_size > writer->row_groups.size);
        struct cx_row_group_header *headers =
            realloc(writer->row_groups.headers, new_size * sizeof(*headers));
        if (!headers)
            return false;
        writer->row_groups.headers = headers;
        writer->row_groups.size = new_size;
    }

    // write the header if it hasn't already been written
    if (!cx_row_group_writer_ensure_header(writer))
        return false;

    size_t row_group_offset = cx_row_group_writer_offset(writer);

    size_t headers_size = 2 * column_count * sizeof(struct cx_column_header);
    struct cx_column_header *headers = calloc(column_count, headers_size);
    if (!headers)
        goto error;

    // write columns
    for (size_t i = 0; i < column_count; i++) {
        const struct cx_column_descriptor *descriptor =
            &writer->columns.descriptors[i];
        const struct cx_column *column = cx_row_group_column(row_group, i);
        const struct cx_column *nulls = cx_row_group_nulls(row_group, i);
        if (!column || !nulls)
            goto error;
        if (!cx_row_group_writer_put_column(writer, column, &headers[i * 2],
                                            descriptor->compression,
                                            descriptor->level))
            goto error;
        if (!cx_row_group_writer_put_column(writer, nulls, &headers[i * 2 + 1],
                                            CX_NULL_COMPRESSION_TYPE,
                                            CX_NULL_COMPRESSION_LEVEL))
            goto error;
    }

    // update the row group header
    struct cx_row_group_header *row_group_header =
        &writer->row_groups.headers[writer->row_groups.count++];
    row_group_header->size =
        cx_write_align(cx_row_group_writer_offset(writer)) - row_group_offset;
    row_group_header->offset = row_group_offset;

    // write column headers
    if (!cx_row_group_writer_write(writer, headers, headers_size))
        goto error;

    writer->row_count += cx_row_group_row_count(row_group);

    free(headers);
    return true;
error:
    cx_row_group_writer_seek(writer, row_group_offset);
    free(headers);
    return false;
}

bool cx_row_group_writer_finish(struct cx_row_group_writer *writer, bool sync)
{
    if (writer->footer_written)
        return true;
    if (!cx_row_group_writer_ensure_header(writer))
        return false;

    size_t offset = cx_row_group_writer_offset(writer);

    // write strings
    size_t strings_size;
    const void *strings = cx_column_export(writer->strings, &strings_size);
    if (!cx_row_group_writer_write(writer, strings, strings_size))
        goto error;

    // write row group headers
    size_t row_group_headers_size =
        writer->row_groups.count * sizeof(struct cx_row_group_header);
    if (!cx_row_group_writer_write(writer, writer->row_groups.headers,
                                   row_group_headers_size))
        goto error;

    // write column headers
    size_t column_descriptors_size =
        writer->columns.count * sizeof(struct cx_column_descriptor);
    if (!cx_row_group_writer_write(writer, writer->columns.descriptors,
                                   column_descriptors_size))
        goto error;

    // write the footer
    struct cx_footer footer = {offset,
                               strings_size,
                               writer->row_groups.count,
                               writer->columns.count,
                               writer->row_count,
                               sizeof(footer),
                               CX_FILE_VERSION,
                               CX_FILE_MAGIC};
    if (!cx_row_group_writer_write(writer, &footer, sizeof(footer)))
        goto error;

    // set the file size
    int fd = fileno(writer->file);
    if (ftruncate(fd, cx_row_group_writer_offset(writer)))
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
    cx_row_group_writer_seek(writer, offset);
    return false;
}

void cx_row_group_writer_free(struct cx_row_group_writer *writer)
{
    cx_column_free(writer->strings);
    if (writer->columns.descriptors)
        free(writer->columns.descriptors);
    if (writer->row_groups.headers)
        free(writer->row_groups.headers);
    fclose(writer->file);
    free(writer);
}
