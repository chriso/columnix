#define MUNIT_ENABLE_ASSERT_ALIASES
#include "munit.h"

#include "column.h"

static MunitResult test_alloc(const MunitParameter params[], void* data)
{
    return MUNIT_OK;
}

MunitTest column_tests[] = {
    {"/alloc", test_alloc, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL}};
