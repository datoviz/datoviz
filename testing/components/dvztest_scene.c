/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Datoviz scene test runner                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stddef.h>

#include "_log.h"
#include "../../src/scene/tests/test_scene.h"
#include "datoviz_testing.h"
#include "testing.h"



/*************************************************************************************************/
/*  Entry-point                                                                                  */
/*************************************************************************************************/

/**
 * Run scene tests.
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
#if defined(DVZ_HAS_TEST_GPU_ADAPTER) && DVZ_HAS_TEST_GPU_ADAPTER
    dvz_testing_install_gpu_adapter(&suite);
#endif

    test_scene(&suite);

    int res = tst_suite_run(&suite, argc, argv);
    tst_suite_destroy(&suite);
    return res;
}
