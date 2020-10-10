#ifndef VKY_COLORMAP_HEADER
#define VKY_COLORMAP_HEADER

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
extern uint8_t VKY_COLOR_TEXTURE[CMAP_COUNT * CMAP_COUNT * 4];
#pragma GCC visibility pop


/*************************************************************************************************/
/*  Color enums                                                                                  */
/*************************************************************************************************/

typedef enum
{
    // Standard colormaps
    VKY_CMAP_BINARY, // grey level
    VKY_CMAP_HSV,    // all HSV hues

    // matplotlib perceptually uniform

    VKY_CMAP_CIVIDIS,
    VKY_CMAP_INFERNO,
    VKY_CMAP_MAGMA,
    VKY_CMAP_PLASMA,
    VKY_CMAP_VIRIDIS,

    // matplotlib sequential

    VKY_CMAP_BLUES,
    VKY_CMAP_BUGN,
    VKY_CMAP_BUPU,
    VKY_CMAP_GNBU,
    VKY_CMAP_GREENS,
    VKY_CMAP_GREYS,
    VKY_CMAP_ORANGES,
    VKY_CMAP_ORRD,
    VKY_CMAP_PUBU,
    VKY_CMAP_PUBUGN,
    VKY_CMAP_PURPLES,
    VKY_CMAP_RDPU,
    VKY_CMAP_REDS,
    VKY_CMAP_YLGN,
    VKY_CMAP_YLGNBU,
    VKY_CMAP_YLORBR,
    VKY_CMAP_YLORRD,

    // matplotlib sequential 2

    VKY_CMAP_AFMHOT,
    VKY_CMAP_AUTUMN,
    VKY_CMAP_BONE,
    VKY_CMAP_COOL,
    VKY_CMAP_COPPER,
    VKY_CMAP_GIST_HEAT,
    VKY_CMAP_GRAY,
    VKY_CMAP_HOT,
    VKY_CMAP_PINK,
    VKY_CMAP_SPRING,
    VKY_CMAP_SUMMER,
    VKY_CMAP_WINTER,
    VKY_CMAP_WISTIA,

    // matplotlib diverging

    VKY_CMAP_BRBG,
    VKY_CMAP_BWR,
    VKY_CMAP_COOLWARM,
    VKY_CMAP_PIYG,
    VKY_CMAP_PRGN,
    VKY_CMAP_PUOR,
    VKY_CMAP_RDBU,
    VKY_CMAP_RDGY,
    VKY_CMAP_RDYLBU,
    VKY_CMAP_RDYLGN,
    VKY_CMAP_SEISMIC,
    VKY_CMAP_SPECTRAL,

    // matplotlib cyclic

    VKY_CMAP_TWILIGHT_SHIFTED,
    VKY_CMAP_TWILIGHT,

    // matplotlib misc

    VKY_CMAP_BRG,
    VKY_CMAP_CMRMAP,
    VKY_CMAP_CUBEHELIX,
    VKY_CMAP_FLAG,
    VKY_CMAP_GIST_EARTH,
    VKY_CMAP_GIST_NCAR,
    VKY_CMAP_GIST_RAINBOW,
    VKY_CMAP_GIST_STERN,
    VKY_CMAP_GNUPLOT2,
    VKY_CMAP_GNUPLOT,
    VKY_CMAP_JET,
    VKY_CMAP_NIPY_SPECTRAL,
    VKY_CMAP_OCEAN,
    VKY_CMAP_PRISM,
    VKY_CMAP_RAINBOW,
    VKY_CMAP_TERRAIN,

    // colorcet diverging

    VKY_CMAP_BKR,
    VKY_CMAP_BKY,
    VKY_CMAP_CET_D10,
    VKY_CMAP_CET_D11,
    VKY_CMAP_CET_D8,
    VKY_CMAP_CET_D13,
    VKY_CMAP_CET_D3,
    VKY_CMAP_CET_D1A,
    VKY_CMAP_BJY,
    VKY_CMAP_GWV,
    VKY_CMAP_BWY,
    VKY_CMAP_CET_D12,
    VKY_CMAP_CET_R3,
    VKY_CMAP_CET_D9,
    VKY_CMAP_CWR,

    // colorcet colorblind

    VKY_CMAP_CET_CBC1,
    VKY_CMAP_CET_CBC2,
    VKY_CMAP_CET_CBL1,
    VKY_CMAP_CET_CBL2,
    VKY_CMAP_CET_CBTC1,
    VKY_CMAP_CET_CBTC2,
    VKY_CMAP_CET_CBTL1,

    // colorcet others

    VKY_CMAP_BGY,
    VKY_CMAP_BGYW,
    VKY_CMAP_BMW,
    VKY_CMAP_CET_C1,
    VKY_CMAP_CET_C1S,
    VKY_CMAP_CET_C2,
    VKY_CMAP_CET_C4,
    VKY_CMAP_CET_C4S,
    VKY_CMAP_CET_C5,
    VKY_CMAP_CET_I1,
    VKY_CMAP_CET_I3,
    VKY_CMAP_CET_L10,
    VKY_CMAP_CET_L11,
    VKY_CMAP_CET_L12,
    VKY_CMAP_CET_L16,
    VKY_CMAP_CET_L17,
    VKY_CMAP_CET_L18,
    VKY_CMAP_CET_L19,
    VKY_CMAP_CET_L4,
    VKY_CMAP_CET_L7,
    VKY_CMAP_CET_L8,
    VKY_CMAP_CET_L9,
    VKY_CMAP_CET_R1,
    VKY_CMAP_CET_R2,
    VKY_CMAP_COLORWHEEL,
    VKY_CMAP_FIRE,
    VKY_CMAP_ISOLUM,
    VKY_CMAP_KB,
    VKY_CMAP_KBC,
    VKY_CMAP_KG,
    VKY_CMAP_KGY,
    VKY_CMAP_KR,

    // Moreland colormaps

    VKY_CMAP_BLACK_BODY,
    VKY_CMAP_KINDLMANN,
    VKY_CMAP_EXTENDED_KINDLMANN,

    // colorcet palettes with 256 colors

    VKY_CPAL256_GLASBEY = CPAL256_OFS,
    VKY_CPAL256_GLASBEY_COOL,
    VKY_CPAL256_GLASBEY_DARK,
    VKY_CPAL256_GLASBEY_HV,
    VKY_CPAL256_GLASBEY_LIGHT,
    VKY_CPAL256_GLASBEY_WARM,

    // matplotlib palettes with <= 32 colors

    VKY_CPAL032_ACCENT = CPAL032_OFS,
    VKY_CPAL032_DARK2,
    VKY_CPAL032_PAIRED,
    VKY_CPAL032_PASTEL1,
    VKY_CPAL032_PASTEL2,
    VKY_CPAL032_SET1,
    VKY_CPAL032_SET2,
    VKY_CPAL032_SET3,

    // (new row in the texture after 8 palettes)

    VKY_CPAL032_TAB10,
    VKY_CPAL032_TAB20,
    VKY_CPAL032_TAB20B,
    VKY_CPAL032_TAB20C,

    // bokeh palettes with <= 32 colors

    VKY_CPAL032_CATEGORY10_10,
    VKY_CPAL032_CATEGORY20_20,
    VKY_CPAL032_CATEGORY20B_20,
    VKY_CPAL032_CATEGORY20C_20,

    // (new row in the texture after 8 palettes)
    VKY_CPAL032_COLORBLIND8,

    // OS palettes

    // VKY_CPAL032_WINDOWS_16,
    // VKY_CPAL032_WINDOWS_20,
    // VKY_CPAL032_APPLE_16,
    // VKY_CPAL032_RISC_16,
    // VKY_CPAL032_WEB_16,

} VkyColormap;



/*************************************************************************************************/
/*  Color utils                                                                                  */
/*************************************************************************************************/

VKY_INLINE uint8_t get_cmap_bytes(VkyColormap cmap, uint8_t u_value)
{
    if (cmap >= CPAL032_OFS)
    {
        // For 32-color palettes, we need to alter the cmap and value.
        u_value = CPAL032_SIZ * ((cmap - CPAL032_OFS) % CPAL032_PER_ROW) + u_value;
        // NOTE: cmap has been transformed already in vky_set_color_context() before it is passed
        // to the GPU. So the GPU does not have to do the transformation of cmap. cmap =
        // (VkyColormap)(CPAL032_OFS + ((uint8_t)cmap - CPAL032_OFS) / CPAL032_PER_ROW);
    }
    return u_value;
}

VKY_INLINE uint8_t vky_cmap(VkyColormap cmap, float value, float vmin, float vmax)
{
    // Create the uint8_t instance to be fed to the GPU. Essentially Make the transformation
    // required only for 32-color palettes.

    if (vmin == vmax)
    {
        // log_warn("division by zero error in vky_cmap(): vmin = vmax = %.3f", vmin);
        return get_cmap_bytes(cmap, 0);
    }

    // Value byte is the tex row.
    value = CLIP(value, vmin, vmax);
    value = (vmax - value) / (vmax - vmin);
    uint8_t texcol = TO_BYTE(value);

    return get_cmap_bytes(cmap, texcol);
}

static bool _is_colormap_initialized = false;

VKY_EXPORT void vky_load_color_texture(void);

VKY_INLINE VkyColorBytes
vky_color(VkyColormap cmap, float value, float vmin, float vmax, float alpha)
{
    // Make sure the color texture is initialized.
    if (!_is_colormap_initialized)
    {
        vky_load_color_texture();
        _is_colormap_initialized = true;
    }
    uint8_t texcol = vky_cmap(cmap, value, vmin, vmax);

    int32_t cmap_idx = (int32_t)cmap;

    if (cmap_idx >= CPAL032_OFS)
    {
        cmap_idx = (CPAL032_OFS + (cmap_idx - CPAL032_OFS) / CPAL032_PER_ROW);
    }

    VkyColorBytes color = {0};
    color.r = VKY_COLOR_TEXTURE[cmap_idx * 256 * 4 + texcol * 4 + 0];
    color.g = VKY_COLOR_TEXTURE[cmap_idx * 256 * 4 + texcol * 4 + 1];
    color.b = VKY_COLOR_TEXTURE[cmap_idx * 256 * 4 + texcol * 4 + 2];
    color.a = TO_BYTE(alpha);

    return color;
}


VKY_EXPORT void vky_colormap_apply(
    VkyColormap cmap, double vmin, double vmax, uint32_t value_count, const double* values,
    VkyColorBytes* colors);



#endif
