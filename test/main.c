#include "munit.h"

extern MunitTest column_tests[];
extern MunitTest row_group_tests[];
extern MunitTest match_tests[];

MunitSuite suites[] = {
    {"/column", column_tests, NULL, 1, MUNIT_SUITE_OPTION_NONE},
    {"/row_group", row_group_tests, NULL, 1, MUNIT_SUITE_OPTION_NONE},
    {"/match", match_tests, NULL, 1, MUNIT_SUITE_OPTION_NONE},
    {NULL, NULL, NULL, 1, MUNIT_SUITE_OPTION_NONE}};

static const MunitSuite combined_suite = {"zcs", NULL, suites, 1,
                                          MUNIT_SUITE_OPTION_NONE};

int main(int argc, char *argv[])
{
    return munit_suite_main(&combined_suite, NULL, argc, argv);
}
