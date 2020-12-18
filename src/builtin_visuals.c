#include "../include/visky/builtin_visuals.h"



/*************************************************************************************************/
/*  Common utils                                                                                 */
/*************************************************************************************************/

static void _common_sources(VklVisual* visual)
{
    ASSERT(visual != NULL);

    // Binding #0: uniform buffer MVP
    vkl_visual_source( //
        visual, VKL_SOURCE_UNIFORM, 0, VKL_PIPELINE_GRAPHICS, 0, 0, sizeof(VklMVP),
        VKL_SOURCE_FLAG_IMMEDIATE);

    // Binding #1: uniform buffer viewport
    vkl_visual_source(
        visual, VKL_SOURCE_UNIFORM, 1, VKL_PIPELINE_GRAPHICS, 0, 1, sizeof(VklViewport), 0);

    // Binding #2: color texture
    vkl_visual_source( //
        visual, VKL_SOURCE_TEXTURE_2D, 0, VKL_PIPELINE_GRAPHICS, 0, 2, sizeof(cvec4), 0);
}

static void _common_props(VklVisual* visual)
{

    // MVP
    // Model.
    vkl_visual_prop(
        visual, VKL_PROP_MODEL, 0, VKL_SOURCE_UNIFORM, 0, //
        0, VKL_DTYPE_MAT4, offsetof(VklMVP, model));

    // View.
    vkl_visual_prop(
        visual, VKL_PROP_VIEW, 0, VKL_SOURCE_UNIFORM, 0, //
        1, VKL_DTYPE_MAT4, offsetof(VklMVP, view));

    // Proj.
    vkl_visual_prop(
        visual, VKL_PROP_PROJ, 0, VKL_SOURCE_UNIFORM, 0, //
        2, VKL_DTYPE_MAT4, offsetof(VklMVP, proj));


    // Colormap texture.
    vkl_visual_prop(
        visual, VKL_PROP_COLOR_TEXTURE, 0, VKL_SOURCE_TEXTURE_2D, 0, //
        0, VKL_DTYPE_CVEC4, 0);
}

#define SOURCES(VERTEX_TYPE, PARAMS_TYPE)                                                         \
    {                                                                                             \
        vkl_visual_source(                                                                        \
            visual, VKL_SOURCE_VERTEX, 0, VKL_PIPELINE_GRAPHICS, 0, 0, sizeof(VERTEX_TYPE), 0);   \
        _common_sources(visual);                                                                  \
        vkl_visual_source(                                                                        \
            visual, VKL_SOURCE_UNIFORM, 2, VKL_PIPELINE_GRAPHICS, 0, 3, sizeof(PARAMS_TYPE), 0);  \
    }



/*************************************************************************************************/
/*  Scatter raw                                                                                  */
/*************************************************************************************************/

static void _visual_marker_raw(VklVisual* visual)
{
    // TODO: flags variant

    ASSERT(visual != NULL);
    VklCanvas* canvas = visual->canvas;
    ASSERT(canvas != NULL);

    // Graphics.
    vkl_visual_graphics(visual, vkl_graphics_builtin(canvas, VKL_GRAPHICS_POINTS, 0));

    // Sources
    SOURCES(VklVertex, VklGraphicsPointParams)

    // Props:

    // Vertex pos.
    vkl_visual_prop(                                   //
        visual, VKL_PROP_POS, 0, VKL_SOURCE_VERTEX, 0, //
        0, VKL_DTYPE_VEC3, offsetof(VklVertex, pos));  //

    // Vertex color.
    vkl_visual_prop(                                     //
        visual, VKL_PROP_COLOR, 0, VKL_SOURCE_VERTEX, 0, //
        1, VKL_DTYPE_CVEC4, offsetof(VklVertex, color)); //

    // Common props.
    _common_props(visual);

    // Param: marker size.
    vkl_visual_prop(
        visual, VKL_PROP_MARKER_SIZE, 0, VKL_SOURCE_UNIFORM, 2, //
        0, VKL_DTYPE_FLOAT, offsetof(VklGraphicsPointParams, point_size));
}



/*************************************************************************************************/
/*  Segment raw                                                                                  */
/*************************************************************************************************/

static void _visual_segment_raw(VklVisual* visual)
{
    // TODO: flags variant

    ASSERT(visual != NULL);
    VklCanvas* canvas = visual->canvas;
    ASSERT(canvas != NULL);

    // Graphics.
    vkl_visual_graphics(visual, vkl_graphics_builtin(canvas, VKL_GRAPHICS_LINES, 0));

    // Sources
    vkl_visual_source(
        visual, VKL_SOURCE_VERTEX, 0, VKL_PIPELINE_GRAPHICS, 0, 0, sizeof(VklVertex), 0);
    _common_sources(visual);

    // Props:

    // Vertex pos, segment start.
    vkl_visual_prop(                                   //
        visual, VKL_PROP_POS, 0, VKL_SOURCE_VERTEX, 0, //
        0, VKL_DTYPE_VEC3, offsetof(VklVertex, pos));  //
    vkl_visual_prop_copy(visual, VKL_PROP_POS, 0, VKL_ARRAY_COPY_SINGLE, 2);

    // Vertex pos, segment end.
    vkl_visual_prop(                                                      //
        visual, VKL_PROP_POS, 1, VKL_SOURCE_VERTEX, 0,                    //
        0, VKL_DTYPE_VEC3, sizeof(VklVertex) + offsetof(VklVertex, pos)); //
    vkl_visual_prop_copy(visual, VKL_PROP_POS, 1, VKL_ARRAY_COPY_SINGLE, 2);


    // Vertex color.
    vkl_visual_prop(                                     //
        visual, VKL_PROP_COLOR, 0, VKL_SOURCE_VERTEX, 0, //
        1, VKL_DTYPE_CVEC4, offsetof(VklVertex, color)); //
    vkl_visual_prop_copy(visual, VKL_PROP_COLOR, 0, VKL_ARRAY_COPY_REPEAT, 2);

    // Common props.
    _common_props(visual);
}



/*************************************************************************************************/
/*  Main function                                                                                */
/*************************************************************************************************/

VklVisual vkl_visual_builtin(VklCanvas* canvas, VklVisualType type, int flags)
{
    ASSERT(canvas != NULL);
    VklVisual visual = vkl_visual(canvas);
    switch (type)
    {

    case VKL_VISUAL_MARKER:
        // TODO: raw/agg
        _visual_marker_raw(&visual);
        break;

    case VKL_VISUAL_SEGMENT:
        // TODO: raw/agg
        _visual_segment_raw(&visual);
        break;

    case VKL_VISUAL_CUSTOM:
    case VKL_VISUAL_NONE:
        break;

    default:
        log_error("no builtin visual found for type %d", type);
        break;
    }

    return visual;
}
