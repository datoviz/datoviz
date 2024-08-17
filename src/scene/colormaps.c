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

void dvz_colormap(DvzColormap cmap, uint8_t value, cvec4 color)
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



void dvz_colormap_scale(DvzColormap cmap, float value, float vmin, float vmax, cvec4 color)
{
    uint8_t u_value = _scale_uint8(value, vmin, vmax);
    dvz_colormap(cmap, u_value, color);
}



void dvz_colormap_array(
    DvzColormap cmap, uint32_t count, float* values, float vmin, float vmax, cvec4* out)
{
    ANN(values);
    ANN(out);
    for (uint32_t i = 0; i < count; i++)
    {
        dvz_colormap_scale(cmap, values[i], vmin, vmax, out[i]);
    }
}



// /**
//  * Get the texture normalized coordinates corresponding to a colormap and value.
//  *
//  * @param cmap the colormap
//  * @param value the value
//  * @param[out] uv the colormap coordinates within the texture
//  */
// DVZ_INLINE void dvz_colormap_uv(DvzColormap cmap, uint8_t value, vec2 uv)
// {
//     cvec2 ij = {0};
//     dvz_colormap_idx(cmap, value, ij);
//     uv[0] = (ij[1] + .5) / 256.;
//     uv[1] = (ij[0] + .5) / 256.;
// }



// /**
//  * Get the tex coords extent of a colormap.
//  *
//  * @param cmap the colormap
//  * @param[out] uvuv the texture coordinates of the top left and bottom right corners
//  */
// DVZ_INLINE void dvz_colormap_extent(DvzColormap cmap, vec4 uvuv)
// {
//     cvec2 row_col;
//     dvz_colormap_idx(cmap, 0, row_col);
//     uint8_t row, col0, col1;
//     row = row_col[0];
//     col0 = row_col[1];
//     uint8_t max = cmap >= CPAL032_OFS ? 31 : 255;
//     dvz_colormap_idx(cmap, max, row_col);
//     col1 = row_col[1];
//     uvuv[0] = (col0 + .5) / 256.;
//     uvuv[1] = (row + .5) / 256.;
//     uvuv[2] = (col1 + .5) / 256.;
//     uvuv[3] = (row + .5) / 256.;
// }



// /**
//  * Pack an arbitrary RGB color into a special uv texture coordinates
//  *
//  * This is used by the mesh visual, that only accepts texture coordinates in its vertices. When
//  * setting the first texture coordinate to -1, the second coordinate, a float, is used to unpack
//  3
//  * uint8_t RGB values. It only works because integers up to 2^24 can be represented exactly with
//  * float32.
//  *
//  * @param color the RGB color
//  * @param[out] uv the texture coordinates
//  */
// DVZ_INLINE void dvz_colormap_packuv(cvec3 color, vec2 uv)
// {
//     uv[1] = -1;
//     uv[0] = color[0] + 256.0 * color[1] + 65536.0 * color[2];
// }


// /**
//  * Modify a color in the colormap array (on the CPU only).
//  *
//  * @param row the row index in the colormap array
//  * @param col the column index in the colormap array
//  * @param color the color
//  */

// DVZ_INLINE void dvz_colormap_set(uint8_t row, uint8_t col, cvec4 color)
// {
//     // Make sure the colormap array is loaded in memory.
//     _load_colormaps();
//     ANN(DVZ_COLORMAP_ARRAY);

//     uint32_t offset = (uint32_t)row * 256 * 4 + (uint32_t)col * 4;
//     ASSERT(offset < 256 * 256 * 4 - 4);
//     DVZ_COLORMAP_ARRAY[offset + 0] = color[0];
//     DVZ_COLORMAP_ARRAY[offset + 1] = color[1];
//     DVZ_COLORMAP_ARRAY[offset + 2] = color[2];
//     DVZ_COLORMAP_ARRAY[offset + 3] = color[3];
// }



// /**
//  * Add a custom colormap.
//  *
//  * The cmap index must be between 160 and 175 for continuous colormaps, or between 224 and 239
//  for
//  * categorical colormaps. The maximum number of colors in the colormap is 256.
//  *
//  * @param cmap the custom colormap index
//  * @param color_count the number of colors in the custom colormap
//  * @param colors the colors
//  */
// DVZ_INLINE void dvz_colormap_custom(uint8_t cmap, uint32_t color_count, cvec4* colors)
// {
//     ASSERT(
//         (cmap >= CMAP_CUSTOM && cmap < CMAP_TOT) ||
//         (cmap >= CPAL256_CUSTOM && cmap < CPAL032_OFS));
//     ASSERT(color_count > 0);
//     ANN(colors);
//     log_debug("setting custom colormap #%d with %d colors", cmap, color_count);

//     cvec2 ij = {0};
//     for (uint32_t i = 0; i < color_count; i++)
//     {
//         dvz_colormap_idx((DvzColormap)cmap, i, ij);
//         dvz_colormap_set(ij[0], ij[1], colors[i]);
//     }
// }



#endif
