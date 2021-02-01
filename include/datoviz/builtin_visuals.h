/*************************************************************************************************/
/*  Library of builtin visuals                                                                   */
/*************************************************************************************************/

#ifndef DVZ_BUILTIN_VISUALS_HEADER
#define DVZ_BUILTIN_VISUALS_HEADER

#include "graphics.h"
#include "visuals.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define PARAM(t, x, n, i)                                                                         \
    {                                                                                             \
        t* item = dvz_prop_item(dvz_prop_get(visual, DVZ_PROP_##n, i), 0);                        \
        if (item != NULL)                                                                         \
            memcpy(&x, item, sizeof(t));                                                          \
    }

#define DPI_SCALE(x)                                                                              \
    ASSERT(ev.viewport.dpi_scaling > 0);                                                          \
    x *= ev.viewport.dpi_scaling;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// // Variants.
// typedef enum
// {
//     DVZ_VISUAL_VARIANT_NONE = 0,

//     DVZ_VISUAL_VARIANT_RAW = 0x0001,
//     DVZ_VISUAL_VARIANT_AGG = 0x0002,
//     DVZ_VISUAL_VARIANT_SHADED = 0x0004,

//     DVZ_VISUAL_VARIANT_TEXTURED = 0x0010,
//     DVZ_VISUAL_VARIANT_TEXTURED_MULTI = 0x0020,

// } DvzVisualVariant;



// Visual types.
typedef enum
{
    DVZ_VISUAL_NONE,

    // Basic visuals.
    DVZ_VISUAL_POINT,
    DVZ_VISUAL_LINE,
    DVZ_VISUAL_LINE_STRIP,
    DVZ_VISUAL_TRIANGLE,
    DVZ_VISUAL_TRIANGLE_STRIP,
    DVZ_VISUAL_TRIANGLE_FAN,

    DVZ_VISUAL_RECTANGLE,

    DVZ_VISUAL_MARKER,
    DVZ_VISUAL_SEGMENT,
    DVZ_VISUAL_ARROW,
    DVZ_VISUAL_PATH,

    DVZ_VISUAL_TEXT,
    DVZ_VISUAL_IMAGE,

    DVZ_VISUAL_DISC,
    DVZ_VISUAL_SECTOR,
    DVZ_VISUAL_MESH,
    DVZ_VISUAL_POLYGON,
    DVZ_VISUAL_PSLG,
    DVZ_VISUAL_HISTOGRAM,
    DVZ_VISUAL_AREA,
    DVZ_VISUAL_CANDLE,

    DVZ_VISUAL_GRAPH,

    DVZ_VISUAL_SURFACE,
    DVZ_VISUAL_VOLUME_SLICE,
    DVZ_VISUAL_VOLUME,

    DVZ_VISUAL_FAKE_SPHERE,
    DVZ_VISUAL_AXES_2D,
    DVZ_VISUAL_AXES_3D,
    DVZ_VISUAL_COLORMAP,

    DVZ_VISUAL_COUNT,

    DVZ_VISUAL_CUSTOM,
} DvzVisualType;



// Axis levels
typedef enum
{
    DVZ_AXES_LEVEL_MINOR,
    DVZ_AXES_LEVEL_MAJOR,
    DVZ_AXES_LEVEL_GRID,
    DVZ_AXES_LEVEL_LIM,
    DVZ_AXES_LEVEL_COUNT,
} DvzAxisLevel;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Make a builtin visual out of an existing visual.
 *
 * @param visual the visual to update
 * @param type the type of builtin visual
 * @param flags the creation flags for the builtin visual
 */
DVZ_EXPORT void dvz_visual_builtin(DvzVisual* visual, DvzVisualType type, int flags);


/**
 * Create the common sources and props for a custom visual.
 *
 * This function *must* be called *after* setting at least 1 graphics pipeline.
 *
 * @param visual the visual to update
 */
DVZ_EXPORT void dvz_visual_custom(DvzVisual* visual);



#endif
