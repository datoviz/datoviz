/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing wiggle */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/test_wiggle.h"
#include "datoviz_protocol.h"
#include "renderer.h"
#include "scene/scene_testing_utils.h"
#include "scene/viewport.h"
#include "scene/visual.h"
#include "scene/visuals/visual_test.h"
#include "scene/visuals/wiggle.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Wiggle tests                                                                                 */
/*************************************************************************************************/

int test_wiggle_1(TstSuite* suite)
{
    VisualTest vt = visual_test_start("wiggle", VISUAL_TEST_PANZOOM, 0);

    // Create the visual.
    DvzVisual* visual = dvz_wiggle(vt.batch, 0);

    // Texture generation.
    uint32_t channels = 5;
    uint32_t samples = 64;
    DvzSize texsize = channels * samples * sizeof(float);
    float* texdata = (float*)calloc(texsize, sizeof(float));
    for (uint32_t i = 0; i < texsize; i++)
    {
        texdata[i] = sin(M_2PI * 4 * i / (float)texsize);
    }

    // Attach the texture to the wiggle visual.
    DvzTexture* texture = dvz_texture_2D(
        visual->batch, DVZ_FORMAT_R32_SFLOAT, DVZ_FILTER_NEAREST,
        DVZ_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, samples, channels, texdata, 0);
    dvz_wiggle_texture(visual, texture);
    FREE(texdata);

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual, 0);


    // Run the test.
    visual_test_end(vt);

    return 0;
}
