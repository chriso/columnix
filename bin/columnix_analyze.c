#include <stdio.h>
#include <sys/stat.h>

#include <columnix/reader.h>

const char *unknown_str = "<!> UNKNOWN";

static const char *type_str(enum cx_column_type);
static const char *encoding_str(enum cx_encoding_type);
static const char *compression_str(enum cx_compression_type);

int main(int argc, char *argv[])
{
    struct cx_reader *reader = NULL;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <file>\n", argv[0]);
        return 1;
    }

    const char *path = argv[1];
    reader = cx_reader_new(path);
    if (!reader) {
        fprintf(stderr, "error: unable to open %s\n", path);
        return 1;
    }

    size_t column_count = cx_reader_column_count(reader);

    printf("Summary:\n");
    printf(" - path: %s\n", path);
    printf(" - rows: %zu\n", cx_reader_row_count(reader));
    printf(" - columns: %zu\n", column_count);

    for (size_t i = 0; i < column_count; i++) {
        enum cx_column_type type =
            cx_reader_column_type(reader, i);
        enum cx_encoding_type encoding =
            cx_reader_column_encoding(reader, i);
        int level = 0;
        enum cx_compression_type compression =
            cx_reader_column_compression(reader, i, &level);

        printf("\nColumn %zu:\n", i);
        printf(" - name: %s\n", cx_reader_column_name(reader, i));
        printf(" - type: %s\n", type_str(type));
        if (encoding)
            printf(" - type: %s\n", encoding_str(encoding));
        if (compression)
            printf(" - compression: %s (level %d)\n",
                   compression_str(compression), level);
    }

    cx_reader_free(reader);

    return 0;
}

static const char *type_str(enum cx_column_type type)
{
    switch (type) {
        case CX_COLUMN_BIT:
            return "BIT";
        case CX_COLUMN_I32:
            return "I32";
        case CX_COLUMN_I64:
            return "I64";
        case CX_COLUMN_FLT:
            return "FLT";
        case CX_COLUMN_DBL:
            return "DBL";
        case CX_COLUMN_STR:
            return "STR";
        default:
            break;
    }
    return unknown_str;
}

static const char *encoding_str(enum cx_encoding_type type)
{
    return unknown_str;
}

static const char *compression_str(enum cx_compression_type type)
{
    switch (type) {
        case CX_COMPRESSION_LZ4:
            return "LZ4";
        case CX_COMPRESSION_LZ4HC:
            return "LZ4HC";
        case CX_COMPRESSION_ZSTD:
            return "ZSTD";
        default:
            break;
    }
    return unknown_str;
}
