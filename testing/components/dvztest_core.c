/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Datoviz core test runner                                                                     */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stddef.h>

#include "_log.h"
#include "../../src/common/tests/test_common.h"
#include "../../src/fileio/tests/test_fileio.h"
#include "../../src/geom/tests/test_geom.h"
#include "../../src/math/tests/test_math.h"
#include "../../src/thread/tests/test_thread.h"
#include "datoviz_testing.h"
#include "testing.h"



/*************************************************************************************************/
/*  Entry-point                                                                                  */
/*************************************************************************************************/

/**
 * Run core-module tests.
 *
 * @param argc command-line argument count
 * @param argv command-line arguments
 * @return process exit code
 */
int main(int argc, char** argv)
{
    log_set_level_env();

    TstSuite suite = tst_suite();
    dvz_testing_install_log_adapter(&suite);

    test_common(&suite);
    test_fileio(&suite);
    test_geom(&suite);
    test_math(&suite);
    test_thread(&suite);

    int res = tst_suite_run(&suite, argc, argv);
    tst_suite_destroy(&suite);
    return res;
}
