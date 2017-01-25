#include "munit.h"

extern MunitTest column_tests[];

MunitSuite suites[] = {
    {"/column", column_tests, NULL, 1, MUNIT_SUITE_OPTION_NONE},
    {NULL, NULL, NULL, 1, MUNIT_SUITE_OPTION_NONE}};

static const MunitSuite combined_suite = {"zcs", NULL, suites, 1,
                                          MUNIT_SUITE_OPTION_NONE};

int main(int argc, char *argv[])
{
    return munit_suite_main(&combined_suite, NULL, argc, argv);
}
