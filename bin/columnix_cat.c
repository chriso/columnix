#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "reader.h"
#include "writer.h"

static void show_usage(const char *name)
{
    fprintf(stderr, "usage: %s [--row-group-size=N] [--help] OUTPUT INPUT...\n",
            name);
    exit(0);
}

static bool cx_reader_cat(struct cx_writer *writer, struct cx_reader *reader,
                          enum cx_column_type types[]);

int main(int argc, char *argv[])
{
    static struct option options[] = {
        {"row-group-size", required_argument, 0, 'r'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}};

    const char *name = argv[0];

    bool sync = true;
    size_t row_group_size = 100000;

    const char *row_group_size_arg = NULL;

    for (;;) {
        int index = 0;
        int c = getopt_long(argc, argv, "hr:", options, &index);
        if (c == -1)
            break;
        switch (c) {
            case 0:
                if (options[index].flag)
                    break;
                switch (index) {
                    case 0:
                        row_group_size_arg = optarg;
                        break;
                    case 1:
                        show_usage(name);
                        break;
                    default:
                        abort();
                }
                break;
            case 'r':
                row_group_size_arg = optarg;
                break;
            case 'h':
            case '?':
                show_usage(name);
                break;
            default:
                abort();
        }
    }

    argc -= optind;
    argv += optind;

    if (argc < 2)
        show_usage(name);

    if (row_group_size_arg) {
        char *row_group_size_end = NULL;
        row_group_size = strtoull(row_group_size_arg, &row_group_size_end, 10);
        if (row_group_size_end[0] != '\0') {
            fprintf(stderr, "error: invalid row group size: %s\n",
                    row_group_size_arg);
            return 1;
        }
    }

    const char *output_path = argv[0];

    fprintf(stderr, "Writing to %s with row group size %zu\n", output_path,
            row_group_size);

    enum cx_column_type *types = NULL;

    struct cx_reader *reader = NULL;
    struct cx_writer *writer = cx_writer_new(output_path, row_group_size);
    if (!writer) {
        fprintf(stderr, "Failed to create writer for path %s\n", output_path);
        goto error;
    }

    size_t writer_column_count = 0;

    for (int i = 1; i < argc; i++) {
        const char *input_path = argv[i];
        fprintf(stderr, "Adding %s to %s\n", input_path, output_path);

        bool first_input = i == 1;

        reader = cx_reader_new(input_path);
        if (!reader) {
            fprintf(stderr, "Failed to create reader for path %s\n",
                    input_path);
            goto error;
        }

        size_t column_count = cx_reader_column_count(reader);
        if (!column_count) {
            fprintf(stderr, "error: no columns in %s\n", input_path);
            goto error;
        }

        if (first_input) {
            writer_column_count = column_count;
            types = malloc(column_count * sizeof(*types));
            if (!types) {
                fprintf(stderr, "error: no memory\n");
                goto error;
            }
        } else if (writer_column_count != column_count) {
            fprintf(stderr, "error: column count mismatch\n");
            goto error;
        }

        for (size_t j = 0; j < column_count; j++) {
            enum cx_column_type type = cx_reader_column_type(reader, j);
            if (first_input) {
                types[j] = type;
                // FIXME: use compression level from the reader
                int compression_level = 0;
                if (!cx_writer_add_column(
                        writer, cx_reader_column_name(reader, j), type,
                        cx_reader_column_encoding(reader, j),
                        cx_reader_column_compression(reader, j),
                        compression_level)) {
                    fprintf(stderr, "Failed to add column %zu from %s\n", j,
                            input_path);
                    goto error;
                }
            } else if (type != types[j]) {
                fprintf(stderr, "error: type mismatch for column %zu of %s\n",
                        j, input_path);
                goto error;
            }
        }

        if (!cx_reader_cat(writer, reader, types)) {
            fprintf(stderr, "error: failed to cat %s\n", input_path);
            goto error;
        }

        cx_reader_free(reader);
    }

    if (!cx_writer_finish(writer, sync)) {
        fprintf(stderr, "error: failed to finish writes\n");
        goto error;
    }

    if (types)
        free(types);

    cx_writer_free(writer);

    return 0;

error:
    if (types)
        free(types);
    if (writer)
        cx_writer_free(writer);
    if (reader)
        cx_reader_free(reader);
    return 1;
}

static bool cx_reader_cat(struct cx_writer *writer, struct cx_reader *reader,
                          enum cx_column_type types[])
{
    size_t column_count = cx_reader_column_count(reader);
    while (cx_reader_next(reader)) {
        for (size_t j = 0; j < column_count; j++) {
            cx_value_t value;
            if (!cx_reader_get_null(reader, j, &value.bit))
                goto error;
            if (value.bit) {
                if (!cx_writer_put_null(writer, j))
                    goto error;
            } else {
                switch (types[j]) {
                    case CX_COLUMN_BIT:
                        if (!cx_reader_get_bit(reader, j, &value.bit))
                            goto error;
                        if (!cx_writer_put_bit(writer, j, value.bit))
                            goto error;
                        break;
                    case CX_COLUMN_I32:
                        if (!cx_reader_get_i32(reader, j, &value.i32))
                            goto error;
                        if (!cx_writer_put_i32(writer, j, value.i32))
                            goto error;
                        break;
                    case CX_COLUMN_I64:
                        if (!cx_reader_get_i64(reader, j, &value.i64))
                            goto error;
                        if (!cx_writer_put_i64(writer, j, value.i64))
                            goto error;
                        break;
                    case CX_COLUMN_FLT:
                        if (!cx_reader_get_flt(reader, j, &value.flt))
                            goto error;
                        if (!cx_writer_put_flt(writer, j, value.flt))
                            goto error;
                        break;
                    case CX_COLUMN_DBL:
                        if (!cx_reader_get_dbl(reader, j, &value.dbl))
                            goto error;
                        if (!cx_writer_put_dbl(writer, j, value.dbl))
                            goto error;
                        break;
                    case CX_COLUMN_STR:
                        if (!cx_reader_get_str(reader, j, &value.str))
                            goto error;
                        if (!cx_writer_put_str(writer, j, value.str.ptr))
                            goto error;
                        break;
                }
            }
        }
    }
    return !cx_reader_error(reader);
error:
    return false;
}
