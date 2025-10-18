/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing point                                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/test_point.h"
#include "datoviz_protocol.h"
#include "renderer.h"
#include "scene/scene_testing_utils.h"
#include "scene/viewport.h"
#include "scene/visual.h"
#include "scene/visuals/point.h"
#include "scene/visuals/visual_test.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Point tests                                                                                  */
/*************************************************************************************************/

int test_point_1(TstSuite* suite, TstItem* tstitem)
{
    VisualTest vt = visual_test_start("point", VISUAL_TEST_PANZOOM, 0);

    // Number of items.
    const uint32_t n = 10000;

    // Create the visual.
    DvzVisual* visual = dvz_point(vt.batch, 0);

    // Visual allocation.
    dvz_point_alloc(visual, n);

    // Position.
    vec3* pos = dvz_mock_pos_2D(n, 0.25);
    dvz_point_position(visual, 0, n, pos, 0);

    // Color.
    DvzColor* color = dvz_mock_color(n, ALPHA_U2D(128));
    dvz_point_color(visual, 0, n, color, 0);

    // Size.
    float* size = dvz_mock_uniform(n, 1, 50);
    dvz_point_size(visual, 0, n, size, 0);

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
