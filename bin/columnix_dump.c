#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include "reader.h"

const char *null_string = "";

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

    cx_value_t value = {0};
    const struct cx_string *string = NULL;
    bool is_null = false;

    enum cx_column_type type = cx_reader_column_type(reader, column);

    while (cx_reader_next(reader)) {
        if (!cx_reader_get_null(reader, column, &is_null))
            goto read_error;

        if (is_null) {
            puts(null_string);
            continue;
        }

        switch (type) {
            case CX_COLUMN_BIT:
                if (!cx_reader_get_bit(reader, column, &value.bit))
                    goto read_error;
                puts(value.bit ? "1" : "0");
                break;
            case CX_COLUMN_I32:
                if (!cx_reader_get_i32(reader, column, &value.i32))
                    goto read_error;
                printf("%d\n", value.i32);
                break;
            case CX_COLUMN_I64:
                if (!cx_reader_get_i64(reader, column, &value.i64))
                    goto read_error;
                printf("%" PRIi64 "\n", value.i64);
                break;
            case CX_COLUMN_FLT:
                if (!cx_reader_get_flt(reader, column, &value.flt))
                    goto read_error;
                printf("%f\n", value.flt);
                break;
            case CX_COLUMN_DBL:
                if (!cx_reader_get_dbl(reader, column, &value.dbl))
                    goto read_error;
                printf("%f\n", value.dbl);
                break;
            case CX_COLUMN_STR:
                if (!cx_reader_get_str(reader, column, &string))
                    goto read_error;
                printf("%s\n", string->ptr);
                break;
        }
    }

    if (cx_reader_error(reader)) {
        fprintf(stderr, "error: iter\n");
        goto error;
    }

    cx_reader_free(reader);

    return 0;

read_error:
    fprintf(stderr, "error: failed to read column value\n");
error:
    if (reader)
        cx_reader_free(reader);
    return 1;
}
