/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Datoviz integration test runner                                                              */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stddef.h>

#include "_log.h"
#include "../../src/canvas/tests/test_canvas.h"
#include "../../src/common/tests/test_common.h"
#include "../../src/ds/tests/test_ds.h"
#if defined(DVZ_HAS_DRP2) && DVZ_HAS_DRP2
#include "../../src/drp2/tests/test_drp2.h"
#endif
#include "../../src/fileio/tests/test_fileio.h"
#include "../../src/input/tests/test_input.h"
#include "../../src/math/tests/test_math.h"
#include "../../src/stream/tests/test_stream.h"
#include "../../src/thread/tests/test_thread.h"
#include "../../src/window/tests/test_window.h"
#if (defined(DVZ_HAS_CUDA) && DVZ_HAS_CUDA) || (defined(DVZ_HAS_KVZ) && DVZ_HAS_KVZ)
#include "../../src/video/tests/test_video.h"
#endif
#include "../../src/vk/tests/test_vk.h"
#include "../../src/vklite/tests/test_vklite.h"
#include "testing.h"



/*************************************************************************************************/
/*  Entry-point                                                                                  */
/*************************************************************************************************/

/**
 * Run all active-module tests in a single integration runner.
 *
 * @param argc command-line argument count
 * @param argv command-line arguments
 * @return process exit code
 */
int main(int argc, char** argv)
{
    log_set_level_env();

    TstSuite suite = tst_suite();

    test_common(&suite);
    test_ds(&suite);
#if defined(DVZ_HAS_DRP2) && DVZ_HAS_DRP2
    test_drp2(&suite);
#endif
    test_fileio(&suite);
    test_math(&suite);
    test_stream(&suite);
    test_thread(&suite);
    test_input(&suite);
    test_window(&suite);
    test_canvas(&suite);
#if (defined(DVZ_HAS_CUDA) && DVZ_HAS_CUDA) || (defined(DVZ_HAS_KVZ) && DVZ_HAS_KVZ)
    test_video(&suite);
#endif
    test_vk(&suite);
    test_vklite(&suite);

    tst_suite_run(&suite, argc >= 2 ? argv[1] : NULL);
    tst_suite_destroy(&suite);
    return 0;
}
