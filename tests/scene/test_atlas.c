/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing atlas                                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_atlas.h"
#include "_cglm.h"
#include "scene/atlas.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Atlas test utils                                                                             */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Atlas tests                                                                                  */
/*************************************************************************************************/

int test_atlas_1(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    unsigned long ttf_size = 0;
    unsigned char* ttf_bytes = dvz_resource_font("Roboto_Medium", &ttf_size);
    ASSERT(ttf_size > 0);
    ANN(ttf_bytes);

    DvzAtlas* atlas = dvz_atlas(ttf_size, ttf_bytes);

    // dvz_atlas_string(atlas, "ABCabc"); // By default, ASCII
    // dvz_atlas_codepoints(atlas, 2, (uint32_t[]){9785, 9786});

    // Generate the atlas.
    AT(!dvz_atlas_valid(atlas));
    dvz_atlas_generate(atlas);
    AT(dvz_atlas_valid(atlas));

    // Atlas size.
    uvec3 shape = {0};
    dvz_atlas_shape(atlas, shape);
    uint32_t w = shape[0];
    uint32_t h = shape[1];
    AT(w > 0);
    AT(h > 0);

    // Save the atlas PNG.
    char imgpath[1024] = {0};
    snprintf(imgpath, sizeof(imgpath), "%s/atlas.png", ARTIFACTS_DIR);
    dvz_atlas_png(atlas, imgpath);

    // Show a glyph's coordinates.
    vec4 coords = {0};
    if (dvz_atlas_glyph(atlas, 101, coords) == 0)
        glm_vec4_print(coords, stdout);
    AT(coords[0] > 0);
    AT(coords[1] > 0);
    AT(coords[2] > 0);
    AT(coords[3] > 0);

    {
        // Make a border around a glyph in the atlas for testing purposes.
        // uint32_t x = (uint32_t)coords[0];
        // uint32_t y = (uint32_t)coords[1];
        // uint32_t gw = (uint32_t)coords[2];
        // uint32_t gh = (uint32_t)coords[3];
        // uint8_t* rgb = dvz_atlas_rgb(atlas);
        // for (uint32_t i = 0; i < h; i++)
        // {
        //     for (uint32_t j = 0; j < w; j++)
        //     {
        //         if (j >= x && j < x + gw && (i == y || i == y + gh - 1))
        //         {
        //             uint32_t index = (i * w + j) * 3;
        //             rgb[index] = 255;
        //             rgb[index + 1] = 0;
        //             rgb[index + 2] = 0;
        //         }
        //         else if (i >= y && i < y + gh && (j == x || j == x + gw - 1))
        //         {
        //             uint32_t index = (i * w + j) * 3;
        //             rgb[index] = 255;
        //             rgb[index + 1] = 0;
        //             rgb[index + 2] = 0;
        //         }
        //     }
        // }
        // snprintf(imgpath, sizeof(imgpath), "%s/atlas2.png", ARTIFACTS_DIR);
        // dvz_write_png(imgpath, w, h, rgb);
    }

    dvz_atlas_destroy(atlas);
    return 0;
}
