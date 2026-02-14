/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Datoviz canvas test runner                                                                   */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stddef.h>

#include "_log.h"
#include "../../src/canvas/tests/test_canvas.h"
#include "../../src/input/tests/test_input.h"
#include "../../src/stream/tests/test_stream.h"
#include "../../src/window/tests/test_window.h"
#if (defined(DVZ_HAS_CUDA) && DVZ_HAS_CUDA) || (defined(DVZ_HAS_KVZ) && DVZ_HAS_KVZ)
#include "../../src/video/tests/test_video.h"
#endif
#include "testing.h"



/*************************************************************************************************/
/*  Entry-point                                                                                  */
/*************************************************************************************************/

/**
 * Run canvas-stack module tests.
 *
 * @param argc command-line argument count
 * @param argv command-line arguments
 * @return process exit code
 */
int main(int argc, char** argv)
{
    log_set_level_env();

    TstSuite suite = tst_suite();

    test_stream(&suite);
    test_input(&suite);
    test_window(&suite);
    test_canvas(&suite);
#if (defined(DVZ_HAS_CUDA) && DVZ_HAS_CUDA) || (defined(DVZ_HAS_KVZ) && DVZ_HAS_KVZ)
    test_video(&suite);
#endif

    tst_suite_run(&suite, argc >= 2 ? argv[1] : NULL);
    tst_suite_destroy(&suite);
    return 0;
}
