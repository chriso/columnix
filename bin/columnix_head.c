#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include <columnix/reader.h>

const char *null_string = "(null)";

static bool cx_print_value(struct cx_reader *reader, size_t column);

int main(int argc, char *argv[])
{
    struct cx_reader *reader = NULL;

    size_t count = 20;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <file>\n", argv[0]);
        goto error;
    }

    reader = cx_reader_new(argv[1]);
    if (!reader) {
        fprintf(stderr, "error: unable to open '%s'\n", argv[1]);
        goto error;
    }

    size_t column_count = cx_reader_column_count(reader);
    for (size_t i = 0; i < column_count; i++) {
        if (i)
            printf("\t");
        const char *name = cx_reader_column_name(reader, i);
        if (!name) {
            fprintf(stderr, "error: failed to read column name\n");
            goto error;
        }
        printf("%s", name);
    }
    printf("\n");

    while (cx_reader_next(reader) && count--) {
        for (size_t i = 0; i < column_count; i++) {
            if (i)
                printf("\t");
            if (!cx_print_value(reader, i)) {
                fprintf(stderr, "error: failed to read column value\n");
                goto error;
            }
        }
        printf("\n");
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

static bool cx_print_value(struct cx_reader *reader, size_t column)
{
    cx_value_t value;
    if (!cx_reader_get_null(reader, column, &value.bit))
        goto error;
    if (value.bit) {
        printf("%s", null_string);
    } else {
        switch (cx_reader_column_type(reader, column)) {
            case CX_COLUMN_BIT:
                if (!cx_reader_get_bit(reader, column, &value.bit))
                    goto error;
                printf("%d", value.bit ? 1 : 0);
                break;
            case CX_COLUMN_I32:
                if (!cx_reader_get_i32(reader, column, &value.i32))
                    goto error;
                printf("%d", value.i32);
                break;
            case CX_COLUMN_I64:
                if (!cx_reader_get_i64(reader, column, &value.i64))
                    goto error;
                printf("%" PRIi64, value.i64);
                break;
            case CX_COLUMN_FLT:
                if (!cx_reader_get_flt(reader, column, &value.flt))
                    goto error;
                printf("%f", value.flt);
                break;
            case CX_COLUMN_DBL:
                if (!cx_reader_get_dbl(reader, column, &value.dbl))
                    goto error;
                printf("%f", value.dbl);
                break;
            case CX_COLUMN_STR:
                if (!cx_reader_get_str(reader, column, &value.str))
                    goto error;
                printf("%s", value.str.ptr);
                break;
        }
    }
    return true;
error:
    return false;
}
