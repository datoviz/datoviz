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
    vkl_visual_prop(visual, VKL_PROP_MODEL, 0, VKL_DTYPE_MAT4, VKL_SOURCE_UNIFORM, 0);
    vkl_visual_prop_copy(
        visual, VKL_PROP_MODEL, 0, 0, offsetof(VklMVP, model), VKL_ARRAY_COPY_SINGLE, 1);

    // View.
    vkl_visual_prop(visual, VKL_PROP_VIEW, 0, VKL_DTYPE_MAT4, VKL_SOURCE_UNIFORM, 0);
    vkl_visual_prop_copy(
        visual, VKL_PROP_VIEW, 0, 1, offsetof(VklMVP, view), VKL_ARRAY_COPY_SINGLE, 1);

    // Proj.
    vkl_visual_prop(visual, VKL_PROP_PROJ, 0, VKL_DTYPE_MAT4, VKL_SOURCE_UNIFORM, 0);
    vkl_visual_prop_copy(
        visual, VKL_PROP_PROJ, 0, 2, offsetof(VklMVP, proj), VKL_ARRAY_COPY_SINGLE, 1);

    // Colormap texture.
    vkl_visual_prop(visual, VKL_PROP_COLOR_TEXTURE, 0, VKL_DTYPE_CVEC4, VKL_SOURCE_TEXTURE_2D, 0);
    vkl_visual_prop_copy(visual, VKL_PROP_COLOR_TEXTURE, 0, 0, 0, VKL_ARRAY_COPY_SINGLE, 1);
}

#define SOURCES(VERTEX_TYPE, PARAMS_TYPE)                                                         \
    {                                                                                             \
        vkl_visual_source(                                                                        \
            visual, VKL_SOURCE_VERTEX, 0, VKL_PIPELINE_GRAPHICS, 0, 0, sizeof(VERTEX_TYPE), 0);   \
        _common_sources(visual);                                                                  \
        vkl_visual_source(                                                                        \
            visual, VKL_SOURCE_UNIFORM, 2, VKL_PIPELINE_GRAPHICS, 0, VKL_USER_BINDING,            \
            sizeof(PARAMS_TYPE), 0);                                                              \
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
    vkl_visual_prop(visual, VKL_PROP_POS, 0, VKL_DTYPE_VEC3, VKL_SOURCE_VERTEX, 0);
    vkl_visual_prop_copy(
        visual, VKL_PROP_POS, 0, 0, offsetof(VklVertex, pos), VKL_ARRAY_COPY_SINGLE, 1);

    // Vertex color.
    vkl_visual_prop(visual, VKL_PROP_COLOR, 0, VKL_DTYPE_CVEC4, VKL_SOURCE_VERTEX, 0);
    vkl_visual_prop_copy(
        visual, VKL_PROP_COLOR, 0, 1, offsetof(VklVertex, color), VKL_ARRAY_COPY_SINGLE, 1);

    // Common props.
    _common_props(visual);

    // Param: marker size.
    vkl_visual_prop(visual, VKL_PROP_MARKER_SIZE, 0, VKL_DTYPE_FLOAT, VKL_SOURCE_UNIFORM, 2);
    vkl_visual_prop_copy(
        visual, VKL_PROP_MARKER_SIZE, 0, 0, offsetof(VklGraphicsPointParams, point_size),
        VKL_ARRAY_COPY_SINGLE, 1);
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
    vkl_visual_prop(visual, VKL_PROP_POS, 0, VKL_DTYPE_VEC3, VKL_SOURCE_VERTEX, 0);
    vkl_visual_prop_copy(
        visual, VKL_PROP_POS, 0, 0, offsetof(VklVertex, pos), VKL_ARRAY_COPY_SINGLE, 2);

    // Vertex pos, segment end.
    vkl_visual_prop(visual, VKL_PROP_POS, 1, VKL_DTYPE_VEC3, VKL_SOURCE_VERTEX, 0);
    vkl_visual_prop_copy(
        visual, VKL_PROP_POS, 1, 0, sizeof(VklVertex) + offsetof(VklVertex, pos),
        VKL_ARRAY_COPY_SINGLE, 2);


    // Vertex color.
    vkl_visual_prop(visual, VKL_PROP_COLOR, 0, VKL_DTYPE_CVEC4, VKL_SOURCE_VERTEX, 0);
    vkl_visual_prop_copy(
        visual, VKL_PROP_COLOR, 0, 1, offsetof(VklVertex, color), VKL_ARRAY_COPY_REPEAT, 2);

    // Common props.
    _common_props(visual);
}



/*************************************************************************************************/
/*  Axes 2D                                                                                      */
/*************************************************************************************************/

static void _visual_axes_2D_bake(VklVisual* visual, VklVisualDataEvent ev)
{
    ASSERT(visual != NULL);
    // segment graphics vertex buffer
    VklSource* seg_vert_src = vkl_bake_source(visual, VKL_SOURCE_VERTEX, 0);
    VklSource* seg_index_src = vkl_bake_source(visual, VKL_SOURCE_INDEX, 0);

    // TODO: multiple levels
    uint32_t level = 0;
    VklProp* xpos = vkl_bake_prop(visual, VKL_PROP_XPOS, level);
    uint32_t xtick_count = xpos->arr_orig.item_count; // number of ticks for this level.

    vkl_bake_source_alloc(visual, seg_vert_src, 4 * xtick_count);
    vkl_bake_source_alloc(visual, seg_index_src, 6 * xtick_count);

    // TODO: params
    cvec4 color = {0, 0, 0, 255};
    VklCapType cap = VKL_CAP_SQUARE;
    float lw = 2;
    float x = 0;

    VklGraphicsSegmentVertex* vertices = seg_vert_src->arr.data;
    ASSERT(seg_vert_src->arr.item_count == 4 * xtick_count);

    VklIndex* indices = seg_index_src->arr.data;
    ASSERT(seg_index_src->arr.item_count == 6 * xtick_count);

    for (uint32_t i = 0; i < xtick_count; i++)
    {
        // TODO: transformation
        x = ((float*)xpos->arr_orig.data)[i];
        // TODO: params
        _graphics_segment_add(
            vertices, indices, i, (vec3){x, -1, 0}, (vec3){x, +1, 0}, color, lw, cap, cap);
    }
}

static void _visual_axes_2D(VklVisual* visual)
{
    ASSERT(visual != NULL);
    VklCanvas* canvas = visual->canvas;
    ASSERT(canvas != NULL);

    // Graphics.
    vkl_visual_graphics(visual, vkl_graphics_builtin(canvas, VKL_GRAPHICS_SEGMENT, 0));

    // Sources
    // Segment graphics, vertex buffer.
    vkl_visual_source(
        visual, VKL_SOURCE_VERTEX, 0, VKL_PIPELINE_GRAPHICS, 0, //
        0, sizeof(VklGraphicsSegmentVertex), 0);
    // Segment graphics, index buffer.
    vkl_visual_source(
        visual, VKL_SOURCE_INDEX, 0, VKL_PIPELINE_GRAPHICS, 0, 0, sizeof(VklIndex), 0);

    // Uniform buffers.
    _common_sources(visual);

    // Props
    for (uint32_t level = 0; level < VKL_AXES_LEVEL_COUNT; level++)
    {
        // xticks
        vkl_visual_prop(visual, VKL_PROP_XPOS, level, VKL_DTYPE_FLOAT, VKL_SOURCE_VERTEX, 0);
        // yticks
        vkl_visual_prop(visual, VKL_PROP_YPOS, level, VKL_DTYPE_FLOAT, VKL_SOURCE_VERTEX, 0);
        // color
        vkl_visual_prop(visual, VKL_PROP_COLOR, level, VKL_DTYPE_CVEC4, VKL_SOURCE_VERTEX, 0);
        // line width
        vkl_visual_prop(visual, VKL_PROP_LINE_WIDTH, level, VKL_DTYPE_FLOAT, VKL_SOURCE_VERTEX, 0);
    }

    // tick length
    vkl_visual_prop(
        visual, VKL_PROP_LENGTH, VKL_AXES_LEVEL_MINOR, VKL_DTYPE_FLOAT, VKL_SOURCE_VERTEX, 0);
    // tick length
    vkl_visual_prop(
        visual, VKL_PROP_LENGTH, VKL_AXES_LEVEL_MAJOR, VKL_DTYPE_FLOAT, VKL_SOURCE_VERTEX, 0);
    // tick h margin
    vkl_visual_prop(visual, VKL_PROP_HMARGIN, 0, VKL_DTYPE_FLOAT, VKL_SOURCE_VERTEX, 0);
    // tick v margin
    vkl_visual_prop(visual, VKL_PROP_VMARGIN, 0, VKL_DTYPE_FLOAT, VKL_SOURCE_VERTEX, 0);
    // tick text size
    vkl_visual_prop(visual, VKL_PROP_TEXT_SIZE, 0, VKL_DTYPE_FLOAT, VKL_SOURCE_VERTEX, 0);

    // Common props.
    _common_props(visual);

    vkl_visual_callback_bake(visual, _visual_axes_2D_bake);
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

    case VKL_VISUAL_AXES_2D:
        _visual_axes_2D(&visual);
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
