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

static cvec4 VKL_DEFAULT_AXES_COLOR = {0, 0, 0, 255};
static float VKL_DEFAULT_AXES_LINE_WIDTH = 2.0f;

static uint32_t _count_prop_items(
    VklVisual* visual, uint32_t prop_count, VklPropType* prop_types, //
    uint32_t idx_count, uint32_t* indices)
{
    ASSERT(visual != NULL);
    uint32_t count = 0;
    for (uint32_t i = 0; i < prop_count; i++)
    {
        for (uint32_t j = 0; j < idx_count; j++)
        {
            count += vkl_bake_prop(visual, prop_types[i], j)->arr_orig.item_count;
        }
    }
    return count;
}

static void _add_ticks(
    VklProp* tick_prop, VklGraphicsSegmentVertex* vertices, VklIndex* indices, //
    VklAxisLevel level, uint32_t offset, VklAxisCoord coord, vec2 lim, cvec4 color, float lw,
    vec4 shift)
{
    ASSERT(tick_prop != NULL);
    ASSERT(vertices != NULL);
    ASSERT(indices != NULL);

    float* x = NULL;

    vec3 P0 = {0};
    vec3 P1 = {0};
    vec4 shift0 = {0};
    glm_vec4_copy(shift, shift0);

    VklCapType cap = VKL_CAP_TYPE_NONE;

    uint32_t n = tick_prop->arr_orig.item_count;
    ASSERT(n > 0);
    float s = 0 + .5 * lw;
    for (uint32_t i = 0; i < n; i++)
    {
        glm_vec4_copy(shift0, shift);

        // TODO: transformation
        x = vkl_bake_prop_item(tick_prop, i);
        ASSERT(x != NULL);

        if (coord == VKL_AXES_COORD_X)
        {
            P0[0] = *x;
            P1[0] = *x;
            P0[1] = lim[0];
            P1[1] = lim[1];

            // Prevent half of the first and last lines to be cut off by viewport clipping.
            if (level == VKL_AXES_LEVEL_LIM)
            {
                if (i == 0)
                {
                    shift[0] += s;
                    shift[2] += s;
                }
                else if (i == n - 1)
                {
                    shift[0] -= s;
                    shift[2] -= s;
                }
            }
        }
        else if (coord == VKL_AXES_COORD_Y)
        {
            P0[1] = *x;
            P1[1] = *x;
            P0[0] = lim[0];
            P1[0] = lim[1];

            // Prevent half of the first and last lines to be cut off by viewport clipping.
            if (level == VKL_AXES_LEVEL_LIM)
            {
                if (i == 0)
                {
                    shift[1] += s;
                    shift[3] += s;
                }
                else if (i == n - 1)
                {
                    shift[1] -= s;
                    shift[3] -= s;
                }
            }
        }

        _graphics_segment_add(vertices, indices, offset + i, P0, P1, color, lw, shift, cap, cap);
    }
}

static void _visual_axes_2D_bake(VklVisual* visual, VklVisualDataEvent ev)
{
    ASSERT(visual != NULL);

    // segment graphics vertex buffer
    VklSource* seg_vert_src = vkl_bake_source(visual, VKL_SOURCE_VERTEX, 0);
    VklSource* seg_index_src = vkl_bake_source(visual, VKL_SOURCE_INDEX, 0);

    // Count the total number of segments.
    uint32_t count =
        _count_prop_items(visual, 1, (VklPropType[]){VKL_PROP_POS}, 4, (uint32_t[]){0, 1, 2, 3});

    // Allocate the vertex and index buffer.
    vkl_bake_source_alloc(visual, seg_vert_src, 4 * count);
    vkl_bake_source_alloc(visual, seg_index_src, 6 * count);

    // Vertices and indices arrays.
    VklGraphicsSegmentVertex* vertices = seg_vert_src->arr.data;
    ASSERT(vertices != NULL);
    ASSERT(seg_vert_src->arr.item_count == 4 * count);

    VklIndex* indices = seg_index_src->arr.data;
    ASSERT(indices != NULL);
    ASSERT(seg_index_src->arr.item_count == 6 * count);

    // TODO: params
    cvec4 color = {0, 0, 0, 255};
    float lw = 2;
    vec4 shift = {0};
    vec2 lim = {0};

    VklProp* pos = NULL;
    uint32_t tick_count = 0;

    uint32_t offset = 0; // tick offset
    for (uint32_t level = 0; level < VKL_AXES_LEVEL_COUNT; level++)
    {
        // Take the tick positions.
        pos = vkl_bake_prop(visual, VKL_PROP_POS, level);
        ASSERT(pos != NULL);
        tick_count = pos->arr_orig.item_count; // number of ticks for this level.
        if (tick_count == 0)
            continue;
        ASSERT(tick_count > 0);

        glm_vec4_zero(shift);
        glm_vec2_zero(lim);

        switch (level)
        {

        case VKL_AXES_LEVEL_MINOR:
            // shift[0];
            break;

        case VKL_AXES_LEVEL_MAJOR:
            break;

        case VKL_AXES_LEVEL_GRID:
            lim[0] = -1;
            lim[1] = +1;
            break;

        case VKL_AXES_LEVEL_LIM:
            break;

        default:
            break;
        }

        // ticks
        _add_ticks(
            pos, vertices, indices, (VklAxisLevel)level, offset, //
            (VklAxisCoord)visual->flags, lim, color, lw, shift);
        offset += tick_count;
        ASSERT(offset <= count);
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
        vkl_visual_prop(visual, VKL_PROP_POS, level, VKL_DTYPE_FLOAT, VKL_SOURCE_VERTEX, 0);

        // color
        vkl_visual_prop(visual, VKL_PROP_COLOR, level, VKL_DTYPE_CVEC4, VKL_SOURCE_VERTEX, 0);
        vkl_visual_prop_default(visual, VKL_PROP_COLOR, level, VKL_DEFAULT_AXES_COLOR);

        // line width
        vkl_visual_prop(visual, VKL_PROP_LINE_WIDTH, level, VKL_DTYPE_FLOAT, VKL_SOURCE_VERTEX, 0);
        vkl_visual_prop_default(visual, VKL_PROP_COLOR, level, &VKL_DEFAULT_AXES_LINE_WIDTH);
    }

    // minor tick length
    vkl_visual_prop(
        visual, VKL_PROP_LENGTH, VKL_AXES_LEVEL_MINOR, VKL_DTYPE_FLOAT, VKL_SOURCE_VERTEX, 0);

    // major tick length
    vkl_visual_prop(
        visual, VKL_PROP_LENGTH, VKL_AXES_LEVEL_MAJOR, VKL_DTYPE_FLOAT, VKL_SOURCE_VERTEX, 0);

    // tick h margin
    vkl_visual_prop(visual, VKL_PROP_MARGIN, 0, VKL_DTYPE_FLOAT, VKL_SOURCE_VERTEX, 0);

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
    visual.flags = flags;
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
