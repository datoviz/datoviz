/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Command-line utility for running tests and demos                                             */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#if OS_MACOS
#define INCLUDE_VK_DRIVER_FILES
#endif

#include "main.h"
#include "_macros.h"
#include "common.h"
#include "datoviz.h"
#include "test.h"



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
    const char* match = argc >= 2 ? argv[1] : NULL;
    dvz_run_tests(match);
    return 0;
}



static int info(int argc, char** argv)
{
    printf("%s version %s\n", DVZ_NAME, dvz_version());
    return 0;
}



static int demo(int argc, char** argv)
{
    // dvz_demo();
    return 0;
}



int main(int argc, char** argv)
{
    log_set_level_env();
    if (argc <= 1)
    {
        demo(argc, argv);
        return 0;
    }
    ASSERT(argc >= 2);
    int res = 0;

    SWITCH_CLI_ARG(info)
    SWITCH_CLI_ARG(test)
    SWITCH_CLI_ARG(demo)

    return res;
}
