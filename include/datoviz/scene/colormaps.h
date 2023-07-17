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
#include "_math.h"
#include "fileio.h"



/*************************************************************************************************/
/*  Constants and macros                                                                         */
/*************************************************************************************************/

// Colormaps: native, user-defined, total.
#define CMAP_OFS     0                     //   0
#define CMAP_NAT     144                   // 144
#define CMAP_USR_OFS CMAP_NAT              // 144
#define CMAP_USR     32                    //  32
#define CMAP_TOT     (CMAP_NAT + CMAP_USR) // 176

// Color palettes with 256 colors each: native, user-defined, total.
#define CPAL256_OFS     CMAP_TOT                    // 176
#define CPAL256_NAT     32                          //  32
#define CPAL256_USR_OFS (CPAL256_OFS + CPAL256_NAT) // 208
#define CPAL256_USR     32                          //  32
#define CPAL256_TOT     (CPAL256_NAT + CPAL256_USR) //  64

// Color palettes with 32 colors each: native, user-defined, total.
// There are 8 palettes per row in the texture (8*32=256)
#define CPAL032_OFS     (CPAL256_OFS + CPAL256_TOT) // 240
#define CPAL032_NAT     8                           //   8
#define CPAL032_USR_OFS (CPAL032_OFS + CPAL032_NAT) // 248
#define CPAL032_USR     8                           //   8
#define CPAL032_PER_ROW 8                           //   8
#define CPAL032_SIZ     32                          //  32
#define CPAL032_TOT     (CPAL032_NAT + CPAL032_USR) //  16

#define CMAP_COUNT 256

// Custom colormaps.
#define CMAP_CUSTOM_COUNT 16
#define CMAP_CUSTOM       (CMAP_TOT - CMAP_CUSTOM_COUNT)    // 160
#define CPAL256_CUSTOM    (CPAL032_OFS - CMAP_CUSTOM_COUNT) // 224
// TODO: CPAL032 custom

#define TO_BYTE(x) (uint8_t) round(CLIP((x), 0, 1) * 255)


#pragma GCC visibility push(default)
static unsigned char* DVZ_COLORMAP_ARRAY;
#pragma GCC visibility pop



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    // Standard colormaps
    DVZ_CMAP_BINARY, // grey level
    DVZ_CMAP_HSV,    // all HSV hues

    // matplotlib perceptually uniform

    DVZ_CMAP_CIVIDIS,
    DVZ_CMAP_INFERNO,
    DVZ_CMAP_MAGMA,
    DVZ_CMAP_PLASMA,
    DVZ_CMAP_VIRIDIS,

    // matplotlib sequential

    DVZ_CMAP_BLUES,
    DVZ_CMAP_BUGN,
    DVZ_CMAP_BUPU,
    DVZ_CMAP_GNBU,
    DVZ_CMAP_GREENS,
    DVZ_CMAP_GREYS,
    DVZ_CMAP_ORANGES,
    DVZ_CMAP_ORRD,
    DVZ_CMAP_PUBU,
    DVZ_CMAP_PUBUGN,
    DVZ_CMAP_PURPLES,
    DVZ_CMAP_RDPU,
    DVZ_CMAP_REDS,
    DVZ_CMAP_YLGN,
    DVZ_CMAP_YLGNBU,
    DVZ_CMAP_YLORBR,
    DVZ_CMAP_YLORRD,

    // matplotlib sequential 2

    DVZ_CMAP_AFMHOT,
    DVZ_CMAP_AUTUMN,
    DVZ_CMAP_BONE,
    DVZ_CMAP_COOL,
    DVZ_CMAP_COPPER,
    DVZ_CMAP_GIST_HEAT,
    DVZ_CMAP_GRAY,
    DVZ_CMAP_HOT,
    DVZ_CMAP_PINK,
    DVZ_CMAP_SPRING,
    DVZ_CMAP_SUMMER,
    DVZ_CMAP_WINTER,
    DVZ_CMAP_WISTIA,

    // matplotlib diverging

    DVZ_CMAP_BRBG,
    DVZ_CMAP_BWR,
    DVZ_CMAP_COOLWARM,
    DVZ_CMAP_PIYG,
    DVZ_CMAP_PRGN,
    DVZ_CMAP_PUOR,
    DVZ_CMAP_RDBU,
    DVZ_CMAP_RDGY,
    DVZ_CMAP_RDYLBU,
    DVZ_CMAP_RDYLGN,
    DVZ_CMAP_SEISMIC,
    DVZ_CMAP_SPECTRAL,

    // matplotlib cyclic

    DVZ_CMAP_TWILIGHT_SHIFTED,
    DVZ_CMAP_TWILIGHT,

    // matplotlib misc

    DVZ_CMAP_BRG,
    DVZ_CMAP_CMRMAP,
    DVZ_CMAP_CUBEHELIX,
    DVZ_CMAP_FLAG,
    DVZ_CMAP_GIST_EARTH,
    DVZ_CMAP_GIST_NCAR,
    DVZ_CMAP_GIST_RAINBOW,
    DVZ_CMAP_GIST_STERN,
    DVZ_CMAP_GNUPLOT2,
    DVZ_CMAP_GNUPLOT,
    DVZ_CMAP_JET,
    DVZ_CMAP_NIPY_SPECTRAL,
    DVZ_CMAP_OCEAN,
    DVZ_CMAP_PRISM,
    DVZ_CMAP_RAINBOW,
    DVZ_CMAP_TERRAIN,

    // colorcet diverging

    DVZ_CMAP_BKR,
    DVZ_CMAP_BKY,
    DVZ_CMAP_CET_D10,
    DVZ_CMAP_CET_D11,
    DVZ_CMAP_CET_D8,
    DVZ_CMAP_CET_D13,
    DVZ_CMAP_CET_D3,
    DVZ_CMAP_CET_D1A,
    DVZ_CMAP_BJY,
    DVZ_CMAP_GWV,
    DVZ_CMAP_BWY,
    DVZ_CMAP_CET_D12,
    DVZ_CMAP_CET_R3,
    DVZ_CMAP_CET_D9,
    DVZ_CMAP_CWR,

    // colorcet colorblind

    DVZ_CMAP_CET_CBC1,
    DVZ_CMAP_CET_CBC2,
    DVZ_CMAP_CET_CBL1,
    DVZ_CMAP_CET_CBL2,
    DVZ_CMAP_CET_CBTC1,
    DVZ_CMAP_CET_CBTC2,
    DVZ_CMAP_CET_CBTL1,

    // colorcet others

    DVZ_CMAP_BGY,
    DVZ_CMAP_BGYW,
    DVZ_CMAP_BMW,
    DVZ_CMAP_CET_C1,
    DVZ_CMAP_CET_C1S,
    DVZ_CMAP_CET_C2,
    DVZ_CMAP_CET_C4,
    DVZ_CMAP_CET_C4S,
    DVZ_CMAP_CET_C5,
    DVZ_CMAP_CET_I1,
    DVZ_CMAP_CET_I3,
    DVZ_CMAP_CET_L10,
    DVZ_CMAP_CET_L11,
    DVZ_CMAP_CET_L12,
    DVZ_CMAP_CET_L16,
    DVZ_CMAP_CET_L17,
    DVZ_CMAP_CET_L18,
    DVZ_CMAP_CET_L19,
    DVZ_CMAP_CET_L4,
    DVZ_CMAP_CET_L7,
    DVZ_CMAP_CET_L8,
    DVZ_CMAP_CET_L9,
    DVZ_CMAP_CET_R1,
    DVZ_CMAP_CET_R2,
    DVZ_CMAP_COLORWHEEL,
    DVZ_CMAP_FIRE,
    DVZ_CMAP_ISOLUM,
    DVZ_CMAP_KB,
    DVZ_CMAP_KBC,
    DVZ_CMAP_KG,
    DVZ_CMAP_KGY,
    DVZ_CMAP_KR,

    // Moreland colormaps

    DVZ_CMAP_BLACK_BODY,
    DVZ_CMAP_KINDLMANN,
    DVZ_CMAP_EXTENDED_KINDLMANN,

    // colorcet palettes with 256 colors

    DVZ_CPAL256_GLASBEY = CPAL256_OFS,
    DVZ_CPAL256_GLASBEY_COOL,
    DVZ_CPAL256_GLASBEY_DARK,
    DVZ_CPAL256_GLASBEY_HV,
    DVZ_CPAL256_GLASBEY_LIGHT,
    DVZ_CPAL256_GLASBEY_WARM,

    // matplotlib palettes with <= 32 colors

    DVZ_CPAL032_ACCENT = CPAL032_OFS,
    DVZ_CPAL032_DARK2,
    DVZ_CPAL032_PAIRED,
    DVZ_CPAL032_PASTEL1,
    DVZ_CPAL032_PASTEL2,
    DVZ_CPAL032_SET1,
    DVZ_CPAL032_SET2,
    DVZ_CPAL032_SET3,

    // (new row in the texture after 8 palettes)

    DVZ_CPAL032_TAB10,
    DVZ_CPAL032_TAB20,
    DVZ_CPAL032_TAB20B,
    DVZ_CPAL032_TAB20C,

    // bokeh palettes with <= 32 colors

    DVZ_CPAL032_CATEGORY10_10,
    DVZ_CPAL032_CATEGORY20_20,
    DVZ_CPAL032_CATEGORY20B_20,
    DVZ_CPAL032_CATEGORY20C_20,

    // (new row in the texture after 8 palettes)

    // BUG: this is 256, =0 with uint8, so this colormap is not working atm
    DVZ_CPAL032_COLORBLIND8,

    // OS palettes

    // DVZ_CPAL032_WINDOWS_16,
    // DVZ_CPAL032_WINDOWS_20,
    // DVZ_CPAL032_APPLE_16,
    // DVZ_CPAL032_RISC_16,
    // DVZ_CPAL032_WEB_16,

} DvzColormap;



EXTERN_C_ON

/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

// Rescale a double value to a byte.
static uint8_t _scale_uint8(double value, double vmin, double vmax)
{
    if (vmin == vmax)
    {
        log_warn("error in colormap_value(): vmin=vmax");
        return 0;
    }
    double x = (CLIP(value, vmin, vmax) - vmin) / (vmax - vmin);
    if (x == 1)
        x = 0.99999999;
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



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Get the texture integer coordinates corresponding to a colormap and value.
 *
 * @param cmap the colormap
 * @param value the value
 * @param[out] out the colormap coordinates within the texture
 */
DVZ_INLINE void dvz_colormap_idx(DvzColormap cmap, uint8_t value, cvec2 out)
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



/**
 * Get the texture normalized coordinates corresponding to a colormap and value.
 *
 * @param cmap the colormap
 * @param value the value
 * @param[out] uv the colormap coordinates within the texture
 */
DVZ_INLINE void dvz_colormap_uv(DvzColormap cmap, uint8_t value, vec2 uv)
{
    cvec2 ij = {0};
    dvz_colormap_idx(cmap, value, ij);
    uv[0] = (ij[1] + .5) / 256.;
    uv[1] = (ij[0] + .5) / 256.;
}



/**
 * Get the tex coords extent of a colormap.
 *
 * @param cmap the colormap
 * @param[out] uvuv the texture coordinates of the upper-left and lower-right corners
 */
DVZ_INLINE void dvz_colormap_extent(DvzColormap cmap, vec4 uvuv)
{
    cvec2 row_col;
    dvz_colormap_idx(cmap, 0, row_col);
    uint8_t row, col0, col1;
    row = row_col[0];
    col0 = row_col[1];
    uint8_t max = cmap >= CPAL032_OFS ? 31 : 255;
    dvz_colormap_idx(cmap, max, row_col);
    col1 = row_col[1];
    uvuv[0] = (col0 + .5) / 256.;
    uvuv[1] = (row + .5) / 256.;
    uvuv[2] = (col1 + .5) / 256.;
    uvuv[3] = (row + .5) / 256.;
}



/**
 * Fetch a color from a colormap and a value.
 *
 * @param cmap the colormap
 * @param value the value
 * @param[out] color the fetched color
 */
DVZ_INLINE void dvz_colormap(DvzColormap cmap, uint8_t value, cvec4 color)
{
    cvec2 out = {0};
    dvz_colormap_idx(cmap, value, out);
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



/**
 * Fetch a color from a colormap and an interpolated value.
 *
 * @param cmap the colormap
 * @param value the value
 * @param vmin the minimum value
 * @param vmax the maximum value
 * @param[out] color the fetched color
 */
DVZ_INLINE void
dvz_colormap_scale(DvzColormap cmap, double value, double vmin, double vmax, cvec4 color)
{
    uint8_t u_value = _scale_uint8(value, vmin, vmax);
    dvz_colormap(cmap, u_value, color);
}



/**
 * Fetch colors from a colormap and an array of values.
 *
 * @param cmap the colormap
 * @param count the number of values
 * @param values pointer to the array of double numbers
 * @param vmin the minimum value
 * @param vmax the maximum value
 * @param[out] out the fetched colors
 */
DVZ_INLINE void dvz_colormap_array(
    DvzColormap cmap, uint32_t count, double* values, double vmin, double vmax, cvec4* out)
{
    ANN(values);
    ANN(out);
    for (uint32_t i = 0; i < count; i++)
        dvz_colormap_scale(cmap, values[i], vmin, vmax, out[i]);
}



/**
 * Pack an arbitrary RGB color into a special uv texture coordinates
 *
 * This is used by the mesh visual, that only accepts texture coordinates in its vertices. When
 * setting the first texture coordinate to -1, the second coordinate, a float, is used to unpack 3
 * uint8_t RGB values. It only works because integers up to 2^24 can be represented exactly with
 * float32.
 *
 * @param color the RGB color
 * @param[out] uv the texture coordinates
 */
DVZ_INLINE void dvz_colormap_packuv(cvec3 color, vec2 uv)
{
    uv[1] = -1;
    uv[0] = color[0] + 256.0 * color[1] + 65536.0 * color[2];
}


/**
 * Modify a color in the colormap array (on the CPU only).
 *
 * @param row the row index in the colormap array
 * @param col the column index in the colormap array
 * @param color the color
 */

DVZ_INLINE void dvz_colormap_set(uint8_t row, uint8_t col, cvec4 color)
{
    // Make sure the colormap array is loaded in memory.
    _load_colormaps();
    ANN(DVZ_COLORMAP_ARRAY);

    uint32_t offset = (uint32_t)row * 256 * 4 + (uint32_t)col * 4;
    ASSERT(offset < 256 * 256 * 4 - 4);
    DVZ_COLORMAP_ARRAY[offset + 0] = color[0];
    DVZ_COLORMAP_ARRAY[offset + 1] = color[1];
    DVZ_COLORMAP_ARRAY[offset + 2] = color[2];
    DVZ_COLORMAP_ARRAY[offset + 3] = color[3];
}



/**
 * Add a custom colormap.
 *
 * The cmap index must be between 160 and 175 for continuous colormaps, or between 224 and 239 for
 * categorical colormaps. The maximum number of colors in the colormap is 256.
 *
 * @param cmap the custom colormap index
 * @param color_count the number of colors in the custom colormap
 * @param colors the colors
 */
DVZ_INLINE void dvz_colormap_custom(uint8_t cmap, uint32_t color_count, cvec4* colors)
{
    ASSERT(
        (cmap >= CMAP_CUSTOM && cmap < CMAP_TOT) ||
        (cmap >= CPAL256_CUSTOM && cmap < CPAL032_OFS));
    ASSERT(color_count > 0);
    ANN(colors);
    log_debug("setting custom colormap #%d with %d colors", cmap, color_count);

    cvec2 ij = {0};
    for (uint32_t i = 0; i < color_count; i++)
    {
        dvz_colormap_idx((DvzColormap)cmap, i, ij);
        dvz_colormap_set(ij[0], ij[1], colors[i]);
    }
}



EXTERN_C_OFF

#endif
