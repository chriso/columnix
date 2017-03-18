#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include <columnix/reader.h>

const char *null_string = "(null)";

static bool cx_print_value(struct cx_reader *reader, size_t column,
                           enum cx_column_type type);

int main(int argc, char *argv[])
{
    struct cx_reader *reader = NULL;

    if (argc != 3) {
        fprintf(stderr, "usage: %s <file> <column_index>\n", argv[0]);
        goto error;
    }

    reader = cx_reader_new(argv[1]);
    if (!reader) {
        fprintf(stderr, "error: unable to open '%s'\n", argv[1]);
        goto error;
    }

    int column = atoi(argv[2]);
    if (column < 0 || column >= cx_reader_column_count(reader)) {
        fprintf(stderr, "error: invalid column index '%s'\n", argv[2]);
        goto error;
    }

    enum cx_column_type type = cx_reader_column_type(reader, column);

    while (cx_reader_next(reader)) {
        if (!cx_print_value(reader, column, type)) {
            fprintf(stderr, "error: failed to read column value\n");
            goto error;
        }
    }

    if (cx_reader_error(reader)) {
        fprintf(stderr, "error: iter\n");
        goto error;
    }

    cx_reader_free(reader);

    return 0;

error:
    if (reader)
        cx_reader_free(reader);
    return 1;
}

static bool cx_print_value(struct cx_reader *reader, size_t column,
                           enum cx_column_type type)
{
    cx_value_t value;
    if (!cx_reader_get_null(reader, column, &value.bit))
        goto error;
    if (value.bit) {
        puts(null_string);
    } else {
        switch (type) {
            case CX_COLUMN_BIT:
                if (!cx_reader_get_bit(reader, column, &value.bit))
                    goto error;
                puts(value.bit ? "1" : "0");
                break;
            case CX_COLUMN_I32:
                if (!cx_reader_get_i32(reader, column, &value.i32))
                    goto error;
                printf("%d\n", value.i32);
                break;
            case CX_COLUMN_I64:
                if (!cx_reader_get_i64(reader, column, &value.i64))
                    goto error;
                printf("%" PRIi64 "\n", value.i64);
                break;
            case CX_COLUMN_FLT:
                if (!cx_reader_get_flt(reader, column, &value.flt))
                    goto error;
                printf("%f\n", value.flt);
                break;
            case CX_COLUMN_DBL:
                if (!cx_reader_get_dbl(reader, column, &value.dbl))
                    goto error;
                printf("%f\n", value.dbl);
                break;
            case CX_COLUMN_STR:
                if (!cx_reader_get_str(reader, column, &value.str))
                    goto error;
                printf("%s\n", value.str.ptr);
                break;
        }
    }
    return true;
error:
    return false;
}
