
/*************************************************************************************************/
/*  Colormap enums                                                                               */
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

// Colormaps.
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
