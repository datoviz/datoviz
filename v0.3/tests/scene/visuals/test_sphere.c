/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing sphere                                                                               */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/test_sphere.h"
#include "datoviz_protocol.h"
#include "renderer.h"
#include "scene/scene_testing_utils.h"
#include "scene/viewport.h"
#include "scene/visual.h"
#include "scene/visuals/sphere.h"
#include "scene/visuals/visual_test.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Sphere tests                                                                                 */
/*************************************************************************************************/

int test_sphere_1(TstSuite* suite, TstItem* tstitem)
{
    VisualTest vt = visual_test_start("sphere", VISUAL_TEST_ARCBALL, 0);

    // Number of items.
    const uint32_t n = 1000;

    // Create the visual.
    DvzVisual* visual = dvz_sphere(vt.batch, DVZ_SPHERE_FLAGS_LIGHTING);

    // Visual allocation.
    dvz_sphere_alloc(visual, n);

    // Position.
    vec3* pos = dvz_mock_pos_3D(n, 0.25);
    dvz_sphere_position(visual, 0, n, pos, 0);

    // Color.
    DvzColor* color = dvz_mock_color(n, ALPHA_MAX);
    dvz_sphere_color(visual, 0, n, color, 0);

    // Size.
    float* size = dvz_mock_uniform(n, .01, .1);
    dvz_sphere_size(visual, 0, n, size, 0);

    // Light position.
    dvz_sphere_light_pos(visual, 0, (vec4){-1, +1, +10, 1});

    // Light color.
    dvz_sphere_light_color(visual, 0, (DvzColor){WHITE});

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual, 0);

    // Run the test.
    visual_test_end(vt);

    // Cleanup.
    FREE(pos);
    FREE(color);
    FREE(size);

    return 0;
}
