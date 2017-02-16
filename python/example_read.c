#define __STDC_FORMAT_MACROS
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>

#include <zcs/file.h>
#include <zcs/row.h>

int main()
{
    struct zcs_reader *reader = zcs_reader_new("example.zcs");
    assert(reader);

    size_t row_groups = zcs_reader_row_group_count(reader);
    for (size_t i = 0; i < row_groups; i++) {

        struct zcs_row_group *row_group = zcs_reader_row_group(reader, i);
        assert(row_group);

        struct zcs_row_cursor *cursor = zcs_row_cursor_new(row_group);
        assert(cursor);

        while (zcs_row_cursor_next(cursor)) {

            int64_t timestamp;
            const struct zcs_string *email;
            int32_t event;

            assert(zcs_row_cursor_get_i64(cursor, 0, &timestamp) &&
                   zcs_row_cursor_get_str(cursor, 1, &email) &&
                   zcs_row_cursor_get_i32(cursor, 2, &event));

            printf("{%" PRIi64 ", %s, %d}\n", timestamp, email->ptr, event);
        }

        zcs_row_cursor_free(cursor);
        zcs_row_group_free(row_group);
    }

    zcs_reader_free(reader);
}
