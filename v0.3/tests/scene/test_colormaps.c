/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing colormaps */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_colormaps.h"
#include "datoviz.h"
#include "testing.h"



/*************************************************************************************************/
/*  Colormaps tests                                                                              */
/*************************************************************************************************/

int test_colormaps_default(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);

    DvzColormap cmap = DVZ_CMAP_HSV;
    cvec4 color = {0};
    cvec4 expected = {0, 0, 0, 255};

    dvz_colormap_8bit(cmap, 0, color);
    expected[0] = 255;
    AEn(4, color, expected);

    dvz_colormap_8bit(cmap, 128, color);
    expected[0] = 0;
    expected[1] = 255;
    expected[2] = 245;
    AEn(4, color, expected);

    dvz_colormap_8bit(cmap, 255, color);
    expected[0] = 255;
    expected[1] = 0;
    expected[2] = 23;
    AEn(4, color, expected);

    return 0;
}



int test_colormaps_scale(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);

    DvzColormap cmap = DVZ_CMAP_HSV;
    DvzColor color = {0};
    DvzColor expected = {0, 0, 0, ALPHA_MAX};
    float vmin = -1;
    float vmax = +1;

    dvz_colormap_scale(cmap, -1, vmin, vmax, color);
    expected[0] = ALPHA_MAX;
    expected[1] = 0;
    expected[2] = 0;
    AEn(4, color, expected);

    dvz_colormap_scale(cmap, 0, vmin, vmax, color);
    expected[0] = 0;
    expected[1] = ALPHA_MAX;
    expected[2] = ALPHA_U2D(245);
    AEn(4, color, expected);

    dvz_colormap_scale(cmap, 1, vmin, vmax, color);
    expected[0] = ALPHA_MAX;
    expected[1] = 0;
    expected[2] = ALPHA_U2D(23);
    AEn(4, color, expected);

    return 0;
}



int test_colormaps_array(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);

    DvzColormap cmap = DVZ_CMAP_HSV;

    uint32_t n = 100;

    float* values = (float*)calloc(n, sizeof(float));
    for (uint32_t i = 0; i < n; i++)
    {
        values[i] = -1 + 2.0 / (n - 1) * i;
    }
    DvzColor* colors = (DvzColor*)calloc(n, sizeof(DvzColor));
    float vmin = -1, vmax = +1;

    dvz_colormap_array(cmap, n, values, vmin, vmax, colors);
    // for (uint32_t i = 0; i < n; i++)
    // {
    //     printf("%d %d %d %d \n", colors[i][0], colors[i][1], colors[i][2], colors[i][3]);
    // }
    AT(memcmp(colors[0], (DvzAlpha[]){ALPHA_MAX, 0, 0, ALPHA_MAX}, sizeof(DvzColor)) == 0);
    AT(memcmp(
           colors[n - 1], (DvzAlpha[]){ALPHA_MAX, 0, ALPHA_U2D(23), ALPHA_MAX},
           sizeof(DvzColor)) == 0);

    FREE(colors);
    FREE(values);
    return 0;
}



// int test_colormaps_uv(TstSuite* suite, TstItem* tstitem)
// {
//     ANN(suite);

//     DvzColormap cmap = DVZ_CMAP_BLUES;
//     DvzColormap cpal32 = DVZ_CPAL032_PAIRED;
//     DvzColormap cpal = DVZ_CPAL256_GLASBEY;
//     uint8_t value = 128;
//     vec2 uv = {0};
//     float eps = .01;

//     dvz_colormap_uv(cmap, value, uv);
//     AC(uv[0], .5, .05);
//     AC(uv[1], (int)cmap / 256.0, .05);

//     dvz_colormap_uv(cpal, value, uv);
//     AC(uv[0], .5, .05);
//     AC(uv[1], (int)cpal / 256.0, .05);

//     dvz_colormap_uv(cpal32, value, uv);
//     AC(uv[0], .7520, eps);
//     AC(uv[1], .9395, eps);

//     return 0;
// }



// int test_colormaps_extent(TstSuite* suite, TstItem* tstitem)
// {
//     ANN(suite);

//     DvzColormap cmap = DVZ_CMAP_BLUES;
//     DvzColormap cpal32 = DVZ_CPAL032_PAIRED;
//     DvzColormap cpal = DVZ_CPAL256_GLASBEY;
//     vec4 uvuv = {0};
//     float eps = .01;

//     dvz_colormap_extent(cmap, uvuv);
//     AC(uvuv[0], 0, eps);
//     AC(uvuv[2], 1, eps);
//     AC(uvuv[1], .029, eps);
//     AC(uvuv[3], .029, eps);

//     dvz_colormap_extent(cpal, uvuv);
//     AC(uvuv[0], 0, eps);
//     AC(uvuv[2], 1, eps);
//     AC(uvuv[1], .69, eps);
//     AC(uvuv[3], .69, eps);

//     dvz_colormap_extent(cpal32, uvuv);
//     AC(uvuv[0], .25, eps);
//     AC(uvuv[2], .37, eps);
//     AC(uvuv[1], .94, eps);
//     AC(uvuv[3], .94, eps);

//     return 0;
// }



// int test_colormaps_packuv(TstSuite* suite, TstItem* tstitem)
// {
//     ANN(suite);

//     vec2 uv = {0};

//     dvz_colormap_packuv((cvec3){10, 20, 30}, uv);
//     AT(uv[1] == -1);
//     AT(uv[0] == 10 + 256 * 20 + 256 * 256 * 30);

//     return 0;
// }



// int test_colormaps_array(TstSuite* suite, TstItem* tstitem)
// {
//     ANN(suite);

//     DvzColormap cmap = DVZ_CMAP_BLUES;
//     double vmin = -1;
//     double vmax = +1;
//     DvzColor color = {0};

//     uint32_t count = 100;
//     double* values = calloc(count, sizeof(double));
//     for (uint32_t i = 0; i < count; i++)
//         values[i] = -1.0 + 2.0 * i / (double)(count - 1);

//     DvzColor* colors = calloc(count, sizeof(DvzColor));
//     dvz_colormap_array(cmap, count, values, vmin, vmax, colors);
//     for (uint32_t i = 0; i < count; i++)
//     {
//         dvz_colormap_scale(cmap, values[i], vmin, vmax, color);
//         AEn(4, color, colors[i])
//     }

//     FREE(values);
//     FREE(colors);

//     return 0;
// }
