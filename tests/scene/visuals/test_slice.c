/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing slice                                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/test_slice.h"
#include "datoviz_protocol.h"
#include "renderer.h"
#include "scene/scene_testing_utils.h"
#include "scene/viewport.h"
#include "scene/visual.h"
#include "scene/visuals/slice.h"
#include "scene/visuals/visual_test.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Slice tests                                                                                  */
/*************************************************************************************************/

int test_slice_1(TstSuite* suite, TstItem* tstitem)
{
    VisualTest vt = visual_test_start("slice", VISUAL_TEST_ARCBALL, 0);

    // Create the visual.
    DvzVisual* visual = dvz_slice(vt.batch, DVZ_VOLUME_FLAGS_RGBA);

    // Visual allocation.
    uint32_t n = 12;
    dvz_slice_alloc(visual, n);

    // Slice attributes.

    {
        float a = 1;

        vec3* p0 = (vec3*)calloc(n, sizeof(vec3));
        vec3* p1 = (vec3*)calloc(n, sizeof(vec3));
        vec3* p2 = (vec3*)calloc(n, sizeof(vec3));
        vec3* p3 = (vec3*)calloc(n, sizeof(vec3));
        vec3* uvw0 = (vec3*)calloc(n, sizeof(vec3));
        vec3* uvw1 = (vec3*)calloc(n, sizeof(vec3));
        vec3* uvw2 = (vec3*)calloc(n, sizeof(vec3));
        vec3* uvw3 = (vec3*)calloc(n, sizeof(vec3));

        float dw = 2.0 * a / (n - 1.0);
        float dt = 1.0 / (n - 1.0);
        float w = 0, t = 0;
        for (uint32_t i = 0; i < n; i++)
        {
            w = i * dw;
            p0[i][0] = -a;
            p0[i][1] = +a;
            p0[i][2] = -a + w;

            p1[i][0] = -a;
            p1[i][1] = -a;
            p1[i][2] = -a + w;

            p2[i][0] = +a;
            p2[i][1] = -a;
            p2[i][2] = -a + w;

            p3[i][0] = +a;
            p3[i][1] = +a;
            p3[i][2] = -a + w;

            t = i * dt;
            uvw0[i][0] = t;
            uvw0[i][1] = 0;
            uvw0[i][2] = 0;

            uvw1[i][0] = t;
            uvw1[i][1] = 1;
            uvw1[i][2] = 0;

            uvw2[i][0] = t;
            uvw2[i][1] = 1;
            uvw2[i][2] = 1;

            uvw3[i][0] = t;
            uvw3[i][1] = 0;
            uvw3[i][2] = 1;
        }

        dvz_slice_position(visual, 0, n, p0, p1, p2, p3, 0);
        //                         //
        // (vec3[]){{-a, +a, -a}, {-a, +a, +a}}, //
        // (vec3[]){{-a, -a, -a}, {-a, -a, +a}}, //
        // (vec3[]){{+a, -a, -a}, {+a, -a, +a}}, //
        // (vec3[]){{+a, +a, -a}, {+a, +a, +a}}, 0);

        // Slice texture coordinates.
        dvz_slice_texcoords(visual, 0, n, uvw0, uvw1, uvw2, uvw3, 0); //
        // (vec3[]){{0.25, 0, 0}, {0.75, 0, 0}}, //
        // (vec3[]){{0.25, 1, 0}, {0.75, 1, 0}}, //
        // (vec3[]){{0.25, 1, 1}, {0.75, 1, 1}}, //
        // (vec3[]){{0.25, 0, 1}, {0.75, 0, 1}}, 0);

        FREE(p0);
        FREE(p1);
        FREE(p2);
        FREE(p3);
        FREE(uvw0);
        FREE(uvw1);
        FREE(uvw2);
        FREE(uvw3);
    }

    // Visual transparency.
    dvz_slice_alpha(visual, .5);

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(vt.panel, visual, 0);

    // Create the texture and upload the volume data.
    uvec3 shape = {0};
    DvzTexture* texture = load_brain_volume(vt.batch, shape, true);

    if (texture != NULL)
        // Bind the volume texture to the visual.
        dvz_slice_texture(visual, texture);

    // Run the test.
    visual_test_end(vt);

    return 0;
}
