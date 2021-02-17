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
    ASSERT(canvas->dpi_scaling > 0);                                                              \
    x *= canvas->dpi_scaling;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

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
    DVZ_VISUAL_IMAGE_CMAP,

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



// Axis levels.
typedef enum
{
    DVZ_AXES_LEVEL_MINOR,
    DVZ_AXES_LEVEL_MAJOR,
    DVZ_AXES_LEVEL_GRID,
    DVZ_AXES_LEVEL_LIM,
    DVZ_AXES_LEVEL_COUNT,
} DvzAxisLevel;



// Axis flags.
typedef enum
{
    DVZ_AXES_FLAGS_DEFAULT = 0x0000,
    DVZ_AXES_FLAGS_HIDE_MINOR = 0x0400,
    DVZ_AXES_FLAGS_HIDE_GRID = 0x0800,
} DvzAxesFlags;



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



/*************************************************************************************************/
/*  Common utils                                                                                 */
/*************************************************************************************************/

static void _common_sources(DvzVisual* visual)
{
    ASSERT(visual != NULL);

    // Binding #0: uniform buffer MVP
    dvz_visual_source( //
        visual, DVZ_SOURCE_TYPE_MVP, 0, DVZ_PIPELINE_GRAPHICS, 0, 0, sizeof(DvzMVP),
        DVZ_SOURCE_FLAG_MAPPABLE);

    // Binding #1: uniform buffer viewport
    dvz_visual_source(
        visual, DVZ_SOURCE_TYPE_VIEWPORT, 0, DVZ_PIPELINE_GRAPHICS, 0, 1, sizeof(DvzViewport), 0);
}

static void _common_props(DvzVisual* visual)
{
    DvzProp* prop = NULL;

    // MVP
    // Model.
    prop = dvz_visual_prop(visual, DVZ_PROP_MODEL, 0, DVZ_DTYPE_MAT4, DVZ_SOURCE_TYPE_MVP, 0);
    dvz_visual_prop_copy(prop, 0, offsetof(DvzMVP, model), DVZ_ARRAY_COPY_SINGLE, 1);

    // View.
    prop = dvz_visual_prop(visual, DVZ_PROP_VIEW, 0, DVZ_DTYPE_MAT4, DVZ_SOURCE_TYPE_MVP, 0);
    dvz_visual_prop_copy(prop, 1, offsetof(DvzMVP, view), DVZ_ARRAY_COPY_SINGLE, 1);

    // Proj.
    prop = dvz_visual_prop(visual, DVZ_PROP_PROJ, 0, DVZ_DTYPE_MAT4, DVZ_SOURCE_TYPE_MVP, 0);
    dvz_visual_prop_copy(prop, 2, offsetof(DvzMVP, proj), DVZ_ARRAY_COPY_SINGLE, 1);

    // Time.
    prop = dvz_visual_prop(visual, DVZ_PROP_TIME, 0, DVZ_DTYPE_FLOAT, DVZ_SOURCE_TYPE_MVP, 0);
    dvz_visual_prop_copy(prop, 3, offsetof(DvzMVP, time), DVZ_ARRAY_COPY_SINGLE, 1);
}



#endif
