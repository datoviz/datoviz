/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing scene                                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_assertions.h"
#include "test_scene.h"
#include "testing.h"



/*************************************************************************************************/
/*  Entry-point                                                                                  */
/*************************************************************************************************/

int test_scene(TstSuite* suite)
{
    ANN(suite);

    test_scene_animation(suite);
    test_scene_panzoom_arcball(suite);
    test_scene_frame_demand(suite);
    test_scene_axis(suite);
    test_scene_fly(suite);
    test_scene_turntable(suite);
    test_scene_frame_plan(suite);
    test_scene_frame_plan_emit(suite);
    test_scene_dpi(suite);
    test_scene_sample_profile(suite);
    test_scene_fields(suite);
    test_scene_interaction(suite);
    test_scene_graph(suite);
    test_scene_query(suite);
    test_scene_text_atlas(suite);
    test_scene_app(suite);

    return 0;
}
