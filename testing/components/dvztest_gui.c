/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Datoviz GUI test runner                                                                      */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stddef.h>

#include "_log.h"
#include "../../src/gui/tests/test_gui.h"
#include "testing.h"



/*************************************************************************************************/
/*  Entry-point                                                                                  */
/*************************************************************************************************/

/**
 * Run GUI module tests.
 *
 * @param argc command-line argument count
 * @param argv command-line arguments
 * @return process exit code
 */
int main(int argc, char** argv)
{
    log_set_level_env();

    TstSuite suite = tst_suite();

    test_gui(&suite);

    tst_suite_run(&suite, argc >= 2 ? argv[1] : NULL);
    tst_suite_destroy(&suite);
    return 0;
}
