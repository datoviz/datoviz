/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing pixel                                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/test_pixel.h"
#include "datoviz_protocol.h"
#include "renderer.h"
#include "scene/scene_testing_utils.h"
#include "scene/viewport.h"
#include "scene/visual.h"
#include "scene/visuals/pixel.h"
#include "scene/visuals/visual_test.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Pixel tests                                                                                  */
/*************************************************************************************************/

int test_pixel_1(TstSuite* suite, TstItem* tstitem)
{
    VisualTest vt = visual_test_start("pixel", VISUAL_TEST_PANZOOM, 0);

    // Number of items.
    const uint32_t n = 10000;

    // Create the visual.
    DvzVisual* visual = dvz_pixel(vt.batch, 0);

    // Visual allocation.
    dvz_pixel_alloc(visual, n);

    // Position.
    vec3* pos = dvz_mock_pos_2D(n, 0.25);
    dvz_pixel_position(visual, 0, n, pos, 0);

    // Color.
    DvzColor* color = dvz_mock_color(n, ALPHA_U2D(128));
    dvz_pixel_color(visual, 0, n, color, 0);

    dvz_pixel_size(visual, 2);

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual, 0);

    // Run the test.
    visual_test_end(vt);

    // Cleanup.
    FREE(pos);
    FREE(color);

    return 0;
}
