/*************************************************************************************************/
/*  Command-line utility for running tests and demos                                             */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "main.h"
#include "_macros.h"
#include "common.h"
#include "tests.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define SWITCH_CLI_ARG(arg)                                                                       \
    if (argc >= 1 && strcmp(argv[1], #arg) == 0)                                                  \
        res = arg(argc - 1, &argv[1]);



/*************************************************************************************************/
/*  Main functions                                                                               */
/*************************************************************************************************/

static int test(int argc, char** argv)
{
    dvz_run_tests();
    return 0;
}



static int info(int argc, char** argv)
{
    // TODO
    return 0;
}



int main(int argc, char** argv)
{
    log_set_level_env();
    if (argc <= 1)
    {
        log_error("specify a command: info, demo, test");
        return 1;
    }
    ASSERT(argc >= 2);
    int res = 0;
    // SWITCH_CLI_ARG(info)
    SWITCH_CLI_ARG(test)
    // SWITCH_CLI_ARG(demo)
    return res;
}
