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
/*  Utils                                                                                        */
/*************************************************************************************************/

static float* make_wiggle_texture(uint32_t channels, uint32_t samples)
{
    // Texture generation.
    DvzSize texsize = channels * samples * sizeof(float);
    float* texdata = (float*)calloc(texsize, sizeof(float));
    uint32_t i = 0;
    float t = 0, c0 = channels / 2.0, x = 0, y = 0, gamma = 0, //
        alpha = M_2PI * 12, beta = 2.0;

    for (uint32_t s = 0; s < samples; s++)
    {
        x = s / (float)(samples - 1);
        x -= .5;
        y = sin(alpha * x) / (alpha * x);

        for (uint32_t c = 0; c < channels; c++)
        {
            gamma = exp(-beta * pow(2 * (c - c0) / c0, 2));

            texdata[c * samples + s] = gamma * y;
        }
    }
    return texdata;
}



/*************************************************************************************************/
/*  Wiggle tests                                                                                 */
/*************************************************************************************************/

int test_wiggle_1(TstSuite* suite, TstItem* item)
{
    VisualTest vt = visual_test_start("wiggle", VISUAL_TEST_PANZOOM, 0);

    // Create the visual.
    DvzVisual* visual = dvz_wiggle(vt.batch, 0);

    // Texture generation.
    uint32_t channels = 16;
    uint32_t samples = 1024;
    float* texdata = make_wiggle_texture(channels, samples);

    // Attach the texture to the wiggle visual.
    DvzTexture* texture = dvz_texture_2D(
        visual->batch, DVZ_FORMAT_R32_SFLOAT, DVZ_FILTER_LINEAR,
        DVZ_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, samples, channels, texdata, 0);
    dvz_wiggle_texture(visual, texture);
    FREE(texdata);

    dvz_wiggle_scale(visual, 1.0);
    dvz_wiggle_color(visual, (DvzColor){128, 128, 128, 255}, (DvzColor){0, 0, 0, 255});
    dvz_wiggle_edgecolor(visual, (DvzColor){0, 0, 0, 255});

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual, 0);


    // Run the test.
    visual_test_end(vt);

    return 0;
}
