/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Datoviz test runner                                                                          */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stddef.h>

#include "_log.h"
#if defined(DVZ_HAS_APP) && DVZ_HAS_APP
#include "../src/app/tests/test_app.h"
#endif
#include "../src/canvas/tests/test_canvas.h"
#include "../src/common/tests/test_common.h"
#if defined(DVZ_HAS_CONTROLLER) && DVZ_HAS_CONTROLLER
#include "../src/controller/tests/test_controller.h"
#endif
#if defined(DVZ_HAS_DRP2) && DVZ_HAS_DRP2
#include "../src/drp2/tests/test_drp2.h"
#endif
#include "../src/fileio/tests/test_fileio.h"
#include "../src/geom/tests/test_geom.h"
#if defined(DVZ_HAS_GUI) && DVZ_HAS_GUI
#include "../src/gui/tests/test_gui.h"
#endif
#include "../src/input/tests/test_input.h"
#include "../src/math/tests/test_math.h"
#if defined(DVZ_HAS_SCENE) && DVZ_HAS_SCENE
#include "../src/scene/tests/test_scene.h"
#endif
#include "../src/stream/tests/test_stream.h"
#include "../src/thread/tests/test_thread.h"
#include "../src/window/tests/test_window.h"
#if (defined(DVZ_HAS_CUDA) && DVZ_HAS_CUDA) || (defined(DVZ_HAS_KVZ) && DVZ_HAS_KVZ)
#include "../src/video/tests/test_video.h"
#endif
#include "../src/vk/tests/test_vk.h"
#include "../src/vklite/tests/test_vklite.h"
#include "datoviz_testing.h"
#include "testing.h"



/*************************************************************************************************/
/*  Entry-point                                                                                  */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    log_set_level_env();

    TstSuite suite = tst_suite();
    dvz_testing_install_log_adapter(&suite);

#if defined(DVZ_HAS_APP) && DVZ_HAS_APP
    test_app(&suite);
#endif
    test_common(&suite);
#if defined(DVZ_HAS_CONTROLLER) && DVZ_HAS_CONTROLLER
    test_controller(&suite);
#endif
#if defined(DVZ_HAS_DRP2) && DVZ_HAS_DRP2
    test_drp2(&suite);
#endif
    test_fileio(&suite);
    test_geom(&suite);
    test_math(&suite);
#if defined(DVZ_HAS_SCENE) && DVZ_HAS_SCENE
    test_scene(&suite);
#endif
#if defined(DVZ_HAS_GUI) && DVZ_HAS_GUI
    test_gui(&suite);
#endif
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

    int res = tst_suite_run(&suite, argc, argv);
    tst_suite_destroy(&suite);
    return res;
}
