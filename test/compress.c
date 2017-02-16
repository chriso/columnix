#include "compress.h"

#include "helpers.h"

#define BUFFER_SIZE 11111

static void *setup(const MunitParameter params[], void *data)
{
    unsigned char *buffer = malloc(BUFFER_SIZE);
    assert_not_null(buffer);
    for (size_t i = 0; i < BUFFER_SIZE; i++)
        buffer[i] = i / 10;
    return buffer;
}

void test_compression(unsigned char *buffer, enum zcs_compression_type type,
                      int level)
{
    size_t compressed_size;
    void *compressed =
        zcs_compress(type, level, buffer, BUFFER_SIZE, &compressed_size);
    assert_not_null(compressed);
    assert_size(BUFFER_SIZE, >, compressed_size);
    for (size_t i = 0; i < BUFFER_SIZE; i++)
        buffer[i] = 0;
    assert_true(
        zcs_decompress(type, compressed, compressed_size, buffer, BUFFER_SIZE));
    for (size_t i = 0; i < BUFFER_SIZE; i++)
        assert_uchar(buffer[i], ==, i / 10);
    free(compressed);
}

static MunitResult test_lz4(const MunitParameter params[], void *buffer)
{
    test_compression(buffer, ZCS_COMPRESSION_LZ4, 1);
    test_compression(buffer, ZCS_COMPRESSION_LZ4, 15);
    return MUNIT_OK;
}

static MunitResult test_zstd(const MunitParameter params[], void *buffer)
{
    test_compression(buffer, ZCS_COMPRESSION_ZSTD, 1);
    test_compression(buffer, ZCS_COMPRESSION_ZSTD, 15);
    return MUNIT_OK;
}

MunitTest compress_tests[] = {
    {"/lz4", test_lz4, setup, free, MUNIT_TEST_OPTION_NONE, NULL},
    {"/zstd", test_zstd, setup, free, MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL}};
