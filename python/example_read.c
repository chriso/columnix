#define __STDC_FORMAT_MACROS
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>

#include <zcs/reader.h>

int main()
{
    struct zcs_reader *reader = zcs_reader_new("example.zcs");
    assert(reader);

    while (zcs_reader_next(reader)) {
        int64_t timestamp;
        const struct zcs_string *email;
        int32_t event;

        assert(zcs_reader_get_i64(reader, 0, &timestamp) &&
               zcs_reader_get_str(reader, 1, &email) &&
               zcs_reader_get_i32(reader, 2, &event));

        printf("{%" PRIi64 ", %s, %d}\n", timestamp, email->ptr, event);
    }

    assert(!zcs_reader_error(reader));

    zcs_reader_free(reader);
}
