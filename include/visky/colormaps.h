#ifndef VKL_COLORMAP_HEADER
#define VKL_COLORMAP_HEADER

/*
References:

https://matplotlib.org/3.1.0/tutorials/colors/colormaps.html
https://colorcet.holoviz.org/user_guide/index.html
http://holoviews.org/user_guide/Colormaps.html
https://www.kennethmoreland.com/color-advice/

The color texture contains:

144  256-color  native colormaps
 32  256-color  user colormaps
 32  256-color  native color palettes
 32  256-color  user color palettes
 64  32-color   native color palettes (8 per row)
 64  32-color   user color palettes (8 per row)
----
256 x 256 RGBA colors


API
---

We need to generate different types of data structures for colors:

1. RGBA vec4 of floats between 0 and 1
2. RGBA ivec4 of uint8_t with cmap/cpal indices, decoded by the GPU

*/

#include <assert.h>
#include <math.h>
#include <stdint.h>

#include "common.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

// Colormaps: native, user-defined, total.
#define CMAP_OFS     0
#define CMAP_NAT     144
#define CMAP_USR_OFS CMAP_NAT
#define CMAP_USR     32
#define CMAP_TOT     (CMAP_NAT + CMAP_USR)

// Color palettes with 256 colors each: native, user-defined, total.
#define CPAL256_OFS     CMAP_TOT
#define CPAL256_NAT     32
#define CPAL256_USR_OFS (CPAL256_OFS + CPAL256_NAT)
#define CPAL256_USR     32
#define CPAL256_TOT     (CPAL256_NAT + CPAL256_USR)

// Color palettes with 32 colors each: native, user-defined, total.
// There are 8 palettes per row in the texture (8*32=256)
#define CPAL032_OFS     (CPAL256_OFS + CPAL256_TOT)
#define CPAL032_NAT     8
#define CPAL032_USR_OFS (CPAL032_OFS + CPAL032_NAT)
#define CPAL032_USR     8
#define CPAL032_PER_ROW 8
#define CPAL032_SIZ     32
#define CPAL032_TOT     (CPAL032_NAT + CPAL032_USR)

#define TO_BYTE(x) (uint8_t) round(CLIP((x), 0, 1) * 255)

#define CMAP_COUNT 256

#pragma GCC visibility push(default)
static const unsigned char* VKL_COLORMAP_ARRAY;
#pragma GCC visibility pop



/*************************************************************************************************/
/*  Color enums                                                                                  */
/*************************************************************************************************/

typedef enum
{
    // Standard colormaps
    VKL_CMAP_BINARY, // grey level
    VKL_CMAP_HSV,    // all HSV hues

    // matplotlib perceptually uniform

    VKL_CMAP_CIVIDIS,
    VKL_CMAP_INFERNO,
    VKL_CMAP_MAGMA,
    VKL_CMAP_PLASMA,
    VKL_CMAP_VIRIDIS,

    // matplotlib sequential

    VKL_CMAP_BLUES,
    VKL_CMAP_BUGN,
    VKL_CMAP_BUPU,
    VKL_CMAP_GNBU,
    VKL_CMAP_GREENS,
    VKL_CMAP_GREYS,
    VKL_CMAP_ORANGES,
    VKL_CMAP_ORRD,
    VKL_CMAP_PUBU,
    VKL_CMAP_PUBUGN,
    VKL_CMAP_PURPLES,
    VKL_CMAP_RDPU,
    VKL_CMAP_REDS,
    VKL_CMAP_YLGN,
    VKL_CMAP_YLGNBU,
    VKL_CMAP_YLORBR,
    VKL_CMAP_YLORRD,

    // matplotlib sequential 2

    VKL_CMAP_AFMHOT,
    VKL_CMAP_AUTUMN,
    VKL_CMAP_BONE,
    VKL_CMAP_COOL,
    VKL_CMAP_COPPER,
    VKL_CMAP_GIST_HEAT,
    VKL_CMAP_GRAY,
    VKL_CMAP_HOT,
    VKL_CMAP_PINK,
    VKL_CMAP_SPRING,
    VKL_CMAP_SUMMER,
    VKL_CMAP_WINTER,
    VKL_CMAP_WISTIA,

    // matplotlib diverging

    VKL_CMAP_BRBG,
    VKL_CMAP_BWR,
    VKL_CMAP_COOLWARM,
    VKL_CMAP_PIYG,
    VKL_CMAP_PRGN,
    VKL_CMAP_PUOR,
    VKL_CMAP_RDBU,
    VKL_CMAP_RDGY,
    VKL_CMAP_RDYLBU,
    VKL_CMAP_RDYLGN,
    VKL_CMAP_SEISMIC,
    VKL_CMAP_SPECTRAL,

    // matplotlib cyclic

    VKL_CMAP_TWILIGHT_SHIFTED,
    VKL_CMAP_TWILIGHT,

    // matplotlib misc

    VKL_CMAP_BRG,
    VKL_CMAP_CMRMAP,
    VKL_CMAP_CUBEHELIX,
    VKL_CMAP_FLAG,
    VKL_CMAP_GIST_EARTH,
    VKL_CMAP_GIST_NCAR,
    VKL_CMAP_GIST_RAINBOW,
    VKL_CMAP_GIST_STERN,
    VKL_CMAP_GNUPLOT2,
    VKL_CMAP_GNUPLOT,
    VKL_CMAP_JET,
    VKL_CMAP_NIPY_SPECTRAL,
    VKL_CMAP_OCEAN,
    VKL_CMAP_PRISM,
    VKL_CMAP_RAINBOW,
    VKL_CMAP_TERRAIN,

    // colorcet diverging

    VKL_CMAP_BKR,
    VKL_CMAP_BKY,
    VKL_CMAP_CET_D10,
    VKL_CMAP_CET_D11,
    VKL_CMAP_CET_D8,
    VKL_CMAP_CET_D13,
    VKL_CMAP_CET_D3,
    VKL_CMAP_CET_D1A,
    VKL_CMAP_BJY,
    VKL_CMAP_GWV,
    VKL_CMAP_BWY,
    VKL_CMAP_CET_D12,
    VKL_CMAP_CET_R3,
    VKL_CMAP_CET_D9,
    VKL_CMAP_CWR,

    // colorcet colorblind

    VKL_CMAP_CET_CBC1,
    VKL_CMAP_CET_CBC2,
    VKL_CMAP_CET_CBL1,
    VKL_CMAP_CET_CBL2,
    VKL_CMAP_CET_CBTC1,
    VKL_CMAP_CET_CBTC2,
    VKL_CMAP_CET_CBTL1,

    // colorcet others

    VKL_CMAP_BGY,
    VKL_CMAP_BGYW,
    VKL_CMAP_BMW,
    VKL_CMAP_CET_C1,
    VKL_CMAP_CET_C1S,
    VKL_CMAP_CET_C2,
    VKL_CMAP_CET_C4,
    VKL_CMAP_CET_C4S,
    VKL_CMAP_CET_C5,
    VKL_CMAP_CET_I1,
    VKL_CMAP_CET_I3,
    VKL_CMAP_CET_L10,
    VKL_CMAP_CET_L11,
    VKL_CMAP_CET_L12,
    VKL_CMAP_CET_L16,
    VKL_CMAP_CET_L17,
    VKL_CMAP_CET_L18,
    VKL_CMAP_CET_L19,
    VKL_CMAP_CET_L4,
    VKL_CMAP_CET_L7,
    VKL_CMAP_CET_L8,
    VKL_CMAP_CET_L9,
    VKL_CMAP_CET_R1,
    VKL_CMAP_CET_R2,
    VKL_CMAP_COLORWHEEL,
    VKL_CMAP_FIRE,
    VKL_CMAP_ISOLUM,
    VKL_CMAP_KB,
    VKL_CMAP_KBC,
    VKL_CMAP_KG,
    VKL_CMAP_KGY,
    VKL_CMAP_KR,

    // Moreland colormaps

    VKL_CMAP_BLACK_BODY,
    VKL_CMAP_KINDLMANN,
    VKL_CMAP_EXTENDED_KINDLMANN,

    // colorcet palettes with 256 colors

    VKL_CPAL256_GLASBEY = CPAL256_OFS,
    VKL_CPAL256_GLASBEY_COOL,
    VKL_CPAL256_GLASBEY_DARK,
    VKL_CPAL256_GLASBEY_HV,
    VKL_CPAL256_GLASBEY_LIGHT,
    VKL_CPAL256_GLASBEY_WARM,

    // matplotlib palettes with <= 32 colors

    VKL_CPAL032_ACCENT = CPAL032_OFS,
    VKL_CPAL032_DARK2,
    VKL_CPAL032_PAIRED,
    VKL_CPAL032_PASTEL1,
    VKL_CPAL032_PASTEL2,
    VKL_CPAL032_SET1,
    VKL_CPAL032_SET2,
    VKL_CPAL032_SET3,

    // (new row in the texture after 8 palettes)

    VKL_CPAL032_TAB10,
    VKL_CPAL032_TAB20,
    VKL_CPAL032_TAB20B,
    VKL_CPAL032_TAB20C,

    // bokeh palettes with <= 32 colors

    VKL_CPAL032_CATEGORY10_10,
    VKL_CPAL032_CATEGORY20_20,
    VKL_CPAL032_CATEGORY20B_20,
    VKL_CPAL032_CATEGORY20C_20,

    // (new row in the texture after 8 palettes)
    VKL_CPAL032_COLORBLIND8,

    // OS palettes

    // VKL_CPAL032_WINDOWS_16,
    // VKL_CPAL032_WINDOWS_20,
    // VKL_CPAL032_APPLE_16,
    // VKL_CPAL032_RISC_16,
    // VKL_CPAL032_WEB_16,

} VklColormap;



/*************************************************************************************************/
/*  Color utils                                                                                  */
/*************************************************************************************************/

static uint8_t _scale_uint8(double value, double vmin, double vmax)
{
    if (vmin == vmax)
    {
        log_warn("error in colormap_value(): vmin=vmax");
        return 0;
    }
    double x = (CLIP(value, vmin, vmax) - vmin) / (vmax - vmin);
    if (x == 1)
        x = .99999999;
    ASSERT(0 <= x && x < 1);
    return (uint8_t)(x * 256);
}

static const unsigned char* _load_colormaps()
{
    if (VKL_COLORMAP_ARRAY != NULL)
        return VKL_COLORMAP_ARRAY;
    unsigned long size = 0;
    VKL_COLORMAP_ARRAY = vkl_resource_texture("color_texture", &size);
    ASSERT(VKL_COLORMAP_ARRAY != NULL);
    ASSERT(size > 0);
    return VKL_COLORMAP_ARRAY;
}

VKY_INLINE void vkl_colormap_idx(VklColormap cmap, uint8_t value, cvec2 out)
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

VKY_INLINE void vkl_colormap(VklColormap cmap, uint8_t value, cvec4 color)
{
    cvec2 out = {0};
    vkl_colormap_idx(cmap, value, out);
    uint8_t row = out[0];
    uint8_t col = out[1];

    // Make sure the colormap array is loaded in memory.
    _load_colormaps();
    ASSERT(VKL_COLORMAP_ARRAY != NULL);

    uint32_t offset = (uint32_t)row * 256 * 4 + (uint32_t)col * 4;
    ASSERT(offset < 256 * 256 * 4 - 4);
    color[0] = VKL_COLORMAP_ARRAY[offset + 0];
    color[1] = VKL_COLORMAP_ARRAY[offset + 1];
    color[2] = VKL_COLORMAP_ARRAY[offset + 2];
    color[3] = 255;
}

VKY_INLINE void
vkl_colormap_scale(VklColormap cmap, double value, double vmin, double vmax, cvec4 color)
{
    uint8_t u_value = _scale_uint8(value, vmin, vmax);
    vkl_colormap(cmap, u_value, color);
}



#endif
