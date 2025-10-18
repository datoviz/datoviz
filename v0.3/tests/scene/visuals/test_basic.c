/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing basic                                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/test_basic.h"
#include "datoviz_protocol.h"
#include "renderer.h"
#include "scene/scene_testing_utils.h"
#include "scene/viewport.h"
#include "scene/visual.h"
#include "scene/visuals/basic.h"
#include "scene/visuals/visual_test.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Basic tests                                                                                  */
/*************************************************************************************************/

int test_basic_1(TstSuite* suite, TstItem* tstitem)
{
    VisualTest vt = visual_test_start("basic", VISUAL_TEST_PANZOOM, 0);

    // Number of items.
    const uint32_t n = 1000;

    // Create the visual.
    DvzVisual* visual = dvz_basic(vt.batch, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);

    // Visual allocation.
    dvz_basic_alloc(visual, n);

    // Position.
    vec3* pos = dvz_mock_pos_2D(n, 0.25);
    dvz_basic_position(visual, 0, n, pos, 0);

    // Color.
    DvzColor* color = dvz_mock_color(n, ALPHA_U2D(128));
    dvz_basic_color(visual, 0, n, color, 0);

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual, 0);

    // Run the test.
    visual_test_end(vt);

    // Cleanup.
    FREE(pos);
    FREE(color);

    return 0;
}



int test_basic_2(TstSuite* suite, TstItem* tstitem)
{
    VisualTest vt = visual_test_start("basic_group", VISUAL_TEST_PANZOOM, 0);

    // Number of items.
    const uint32_t n = 50;

    // Create the visual.
    DvzVisual* visual = dvz_basic(vt.batch, DVZ_PRIMITIVE_TOPOLOGY_LINE_STRIP, 0);

    // Visual allocation.
    dvz_basic_alloc(visual, n);

    // Position.
    vec3* pos = dvz_mock_band(n, (vec2){-1.5, +0.25});
    dvz_basic_position(visual, 0, n, pos, 0);

    // Color.
    DvzColor* color = dvz_mock_cmap(n, DVZ_CMAP_HSV, ALPHA_MAX);
    dvz_basic_color(visual, 0, n, color, 0);

    // Group.
    float* group = dvz_mock_full(n, 0);
    uint32_t group_size = 5;
    for (uint32_t i = 0; i < n; i++)
    {
        group[i] = i / group_size;
    }
    dvz_basic_group(visual, 0, n, group, 0);

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual, 0);

    // Run the test.
    visual_test_end(vt);

    // Cleanup.
    FREE(pos);
    FREE(color);
    FREE(group);

    return 0;
}
