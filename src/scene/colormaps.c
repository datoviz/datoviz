/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Colormaps                                                                                    */
/*************************************************************************************************/

#ifndef DVZ_HEADER_COLORMAPS
#define DVZ_HEADER_COLORMAPS



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <assert.h>
#include <math.h>
#include <stdint.h>

#include "_log.h"
#include "_macros.h"
#include "datoviz.h"
#include "datoviz_math.h"
#include "fileio.h"



/*************************************************************************************************/
/*  Constants and macros                                                                         */
/*************************************************************************************************/

#pragma GCC visibility push(default)
static unsigned char* DVZ_COLORMAP_ARRAY;
#pragma GCC visibility pop



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

// Rescale a float value to a byte.
static uint8_t _scale_uint8(float value, float vmin, float vmax)
{
    if (vmin == vmax)
    {
        log_warn("error in colormap_value(): vmin=vmax");
        return 0;
    }
    float d = vmax - vmin;
    float x = (CLIP(value, vmin, vmax - d * 1e-7) - vmin) / d;
    // printf("%f %f %f %f\n", value, vmin, vmax, x);
    if (x >= 1 - EPSILON)
        x = 1 - EPSILON;
    ASSERT(0 <= x && x < 1);
    return (uint8_t)floor(x * 256);
}



// Load the colormap array.
static unsigned char* _load_colormaps(void)
{
    if (DVZ_COLORMAP_ARRAY != NULL)
        return DVZ_COLORMAP_ARRAY;
    unsigned long size = 0;
    DVZ_COLORMAP_ARRAY = dvz_resource_texture("cmap_atlas", &size);
    ANN(DVZ_COLORMAP_ARRAY);
    ASSERT(size > 0);
    return DVZ_COLORMAP_ARRAY;
}



/**
 * Get the texture integer coordinates corresponding to a colormap and value.
 *
 * @param cmap the colormap
 * @param value the value
 * @param[out] out the colormap coordinates within the texture
 */
static inline void _colormap_idx(DvzColormap cmap, uint8_t value, cvec2 out)
{
    uint8_t row = 0, col = 0;
    if (cmap >= CPAL032_OFS)
    {
        // For 32-color palettes, we need to alter the cmap and value.
        row = (CPAL032_OFS + (cmap - CPAL032_OFS) / CPAL032_PER_ROW);
        col = CPAL032_SIZ * ((cmap - CPAL032_OFS) % CPAL032_PER_ROW) + value;
    }
    else
    {
        row = (uint8_t)cmap;
        col = value;
    }
    out[0] = row;
    out[1] = col;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

void dvz_colormap(DvzColormap cmap, uint8_t value, DvzColor color)
{
#if DVZ_COLOR_CVEC4
    dvz_colormap_8bit(cmap, value, color);
#else
    cvec4 col = {0};
    dvz_colormap_8bit(cmap, value, col);
    color[0] = ALPHA_F2U(col[0]);
    color[1] = ALPHA_F2U(col[1]);
    color[2] = ALPHA_F2U(col[2]);
    color[3] = ALPHA_F2U(col[3]);
#endif
}



void dvz_colormap_8bit(DvzColormap cmap, uint8_t value, cvec4 color)
{
    cvec2 out = {0};
    _colormap_idx(cmap, value, out);
    uint8_t row = out[0];
    uint8_t col = out[1];

    // Make sure the colormap array is loaded in memory.
    _load_colormaps();
    ANN(DVZ_COLORMAP_ARRAY);

    uint32_t offset = (uint32_t)row * 256 * 4 + (uint32_t)col * 4;
    ASSERT(offset < 256 * 256 * 4 - 4);
    color[0] = DVZ_COLORMAP_ARRAY[offset + 0];
    color[1] = DVZ_COLORMAP_ARRAY[offset + 1];
    color[2] = DVZ_COLORMAP_ARRAY[offset + 2];
    color[3] = 255;
}



void dvz_colormap_scale(DvzColormap cmap, float value, float vmin, float vmax, DvzColor color)
{
    uint8_t u_value = _scale_uint8(value, vmin, vmax);
    dvz_colormap(cmap, u_value, color);
}



void dvz_colormap_array(
    DvzColormap cmap, uint32_t count, float* values, float vmin, float vmax, DvzColor* out)
{
    ANN(values);
    ANN(out);
    for (uint32_t i = 0; i < count; i++)
    {
        dvz_colormap_scale(cmap, values[i], vmin, vmax, out[i]);
    }
}



#endif
