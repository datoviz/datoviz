#include "../include/visky/builtin_visuals.h"
#include "../include/visky/scene.h"



/*************************************************************************************************/
/*  Common utils                                                                                 */
/*************************************************************************************************/

static void _common_sources(VklVisual* visual, uint32_t pidx)
{
    ASSERT(visual != NULL);

    // Binding #0: uniform buffer MVP
    vkl_visual_source( //
        visual, VKL_SOURCE_TYPE_MVP, VKL_PIPELINE_GRAPHICS, pidx, 0, sizeof(VklMVP),
        VKL_SOURCE_FLAG_IMMEDIATE);

    // Binding #1: uniform buffer viewport
    vkl_visual_source(
        visual, VKL_SOURCE_TYPE_VIEWPORT, VKL_PIPELINE_GRAPHICS, pidx, 1, sizeof(VklViewport), 0);
}

static void _common_props(VklVisual* visual, uint32_t pidx)
{
    // MVP
    // Model.
    vkl_visual_prop(visual, VKL_PROP_MODEL, 0, VKL_DTYPE_MAT4, VKL_SOURCE_TYPE_MVP, pidx);
    vkl_visual_prop_copy(
        visual, VKL_PROP_MODEL, 0, 0, offsetof(VklMVP, model), VKL_ARRAY_COPY_SINGLE, 1);

    // View.
    vkl_visual_prop(visual, VKL_PROP_VIEW, 0, VKL_DTYPE_MAT4, VKL_SOURCE_TYPE_MVP, pidx);
    vkl_visual_prop_copy(
        visual, VKL_PROP_VIEW, 0, 1, offsetof(VklMVP, view), VKL_ARRAY_COPY_SINGLE, 1);

    // Proj.
    vkl_visual_prop(visual, VKL_PROP_PROJ, 0, VKL_DTYPE_MAT4, VKL_SOURCE_TYPE_MVP, pidx);
    vkl_visual_prop_copy(
        visual, VKL_PROP_PROJ, 0, 2, offsetof(VklMVP, proj), VKL_ARRAY_COPY_SINGLE, 1);
}

#define SOURCES_NOPARAMS(VERTEX_TYPE)                                                             \
    vkl_visual_source(                                                                            \
        visual, VKL_SOURCE_TYPE_VERTEX, VKL_PIPELINE_GRAPHICS, 0, 0, sizeof(VERTEX_TYPE), 0);     \
    _common_sources(visual, 0);

#define SOURCES_PARAMS(VERTEX_TYPE, PARAMS_TYPE)                                                  \
    {                                                                                             \
        vkl_visual_source(                                                                        \
            visual, VKL_SOURCE_TYPE_VERTEX, VKL_PIPELINE_GRAPHICS, 0, 0, sizeof(VERTEX_TYPE), 0); \
        _common_sources(visual, 0);                                                               \
        vkl_visual_source(                                                                        \
            visual, VKL_SOURCE_TYPE_PARAM, VKL_PIPELINE_GRAPHICS, 0, VKL_USER_BINDING,            \
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
    SOURCES_PARAMS(VklVertex, VklGraphicsPointParams)

    // Props:

    // Vertex pos.
    vkl_visual_prop(visual, VKL_PROP_POS, 0, VKL_DTYPE_VEC3, VKL_SOURCE_TYPE_VERTEX, 0);
    vkl_visual_prop_copy(
        visual, VKL_PROP_POS, 0, 0, offsetof(VklVertex, pos), VKL_ARRAY_COPY_SINGLE, 1);

    // Vertex color.
    vkl_visual_prop(visual, VKL_PROP_COLOR, 0, VKL_DTYPE_CVEC4, VKL_SOURCE_TYPE_VERTEX, 0);
    vkl_visual_prop_copy(
        visual, VKL_PROP_COLOR, 0, 1, offsetof(VklVertex, color), VKL_ARRAY_COPY_SINGLE, 1);

    // Common props.
    _common_props(visual, 0);

    // Param: marker size.
    vkl_visual_prop(visual, VKL_PROP_MARKER_SIZE, 0, VKL_DTYPE_FLOAT, VKL_SOURCE_TYPE_PARAM, 0);
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
        visual, VKL_SOURCE_TYPE_VERTEX, VKL_PIPELINE_GRAPHICS, 0, 0, sizeof(VklVertex), 0);
    _common_sources(visual, 0);

    // Props:

    // Vertex pos, segment start.
    vkl_visual_prop(visual, VKL_PROP_POS, 0, VKL_DTYPE_VEC3, VKL_SOURCE_TYPE_VERTEX, 0);
    vkl_visual_prop_copy(
        visual, VKL_PROP_POS, 0, 0, offsetof(VklVertex, pos), VKL_ARRAY_COPY_SINGLE, 2);

    // Vertex pos, segment end.
    vkl_visual_prop(visual, VKL_PROP_POS, 1, VKL_DTYPE_VEC3, VKL_SOURCE_TYPE_VERTEX, 0);
    vkl_visual_prop_copy(
        visual, VKL_PROP_POS, 1, 0, sizeof(VklVertex) + offsetof(VklVertex, pos),
        VKL_ARRAY_COPY_SINGLE, 2);


    // Vertex color.
    vkl_visual_prop(visual, VKL_PROP_COLOR, 0, VKL_DTYPE_CVEC4, VKL_SOURCE_TYPE_VERTEX, 0);
    vkl_visual_prop_copy(
        visual, VKL_PROP_COLOR, 0, 1, offsetof(VklVertex, color), VKL_ARRAY_COPY_REPEAT, 2);

    // Common props.
    _common_props(visual, 0);
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

static uint32_t _count_chars(VklVisual* visual)
{
    ASSERT(visual != NULL);
    VklProp* prop = vkl_bake_prop(visual, VKL_PROP_TEXT, 0);
    ASSERT(prop != NULL);
    uint32_t n_text = prop->arr_orig.item_count;
    uint32_t char_count = 0;
    char* str = NULL;
    uint32_t slen = 0;
    for (uint32_t i = 0; i < n_text; i++)
    {
        str = ((char**)prop->arr_orig.data)[i];
        slen = strlen(str);
        ASSERT(slen > 0);
        char_count += slen;
    }
    return char_count;
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
    VklTransformAxis transform =
        level == VKL_AXES_LEVEL_LIM ? VKL_TRANSFORM_AXIS_NONE : VKL_TRANSFORM_AXIS_DEFAULT;

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

        _graphics_segment_add(
            vertices, indices, offset + i, P0, P1, color, lw, shift, cap, cap, transform);
    }
}

static void _visual_axes_2D_bake(VklVisual* visual, VklVisualDataEvent ev)
{
    ASSERT(visual != NULL);
    VklScene* scene = visual->canvas->scene;

    // segment graphics vertex buffer
    VklSource* seg_vert_src = vkl_bake_source(visual, VKL_SOURCE_TYPE_VERTEX, 0);
    VklSource* seg_index_src = vkl_bake_source(visual, VKL_SOURCE_TYPE_INDEX, 0);
    // text graphics vertex buffer
    VklSource* text_vert_src = vkl_bake_source(visual, VKL_SOURCE_TYPE_VERTEX, 1);

    // Count the total number of segments.
    uint32_t count =
        _count_prop_items(visual, 1, (VklPropType[]){VKL_PROP_POS}, 4, (uint32_t[]){0, 1, 2, 3});

    // Allocate the vertex and index buffer.
    vkl_bake_source_alloc(visual, seg_vert_src, 4 * count);
    vkl_bake_source_alloc(visual, seg_index_src, 6 * count);

    // Allocate the text vertex buffer.
    uint32_t count_chars = _count_chars(visual);
    if (count_chars > 0)
        vkl_bake_source_alloc(visual, text_vert_src, 4 * count_chars);

    // Vertices arrays.
    VklGraphicsSegmentVertex* vertices = seg_vert_src->arr.data;
    ASSERT(vertices != NULL);
    ASSERT(seg_vert_src->arr.item_count == 4 * count);

    // Indices arrays.
    VklIndex* indices = seg_index_src->arr.data;
    ASSERT(indices != NULL);
    ASSERT(seg_index_src->arr.item_count == 6 * count);

    // Text array.
    VklGraphicsTextVertex* text_vertices = text_vert_src->arr.data;
    ASSERT(text_vert_src->arr.item_count == 4 * count_chars);

    // Visual coordinate.
    VklAxisCoord coord = (VklAxisCoord)visual->flags;
    ASSERT(coord < 2);

    // TODO: params
    cvec4 color = {0, 0, 0, 255};
    float lw = 2;
    float minor_tick_length = 20;
    float major_tick_length = 10;
    vec4 shift = {0};
    vec2 lim = {0};

    VklProp* prop = NULL;
    uint32_t tick_count = 0;
    uint32_t offset = 0; // tick offset
    for (uint32_t level = 0; level < VKL_AXES_LEVEL_COUNT; level++)
    {
        // Take the tick positions.
        prop = vkl_bake_prop(visual, VKL_PROP_POS, level);
        ASSERT(prop != NULL);
        tick_count = prop->arr_orig.item_count; // number of ticks for this level.
        if (tick_count == 0)
            continue;
        ASSERT(tick_count > 0);

        memset(color, 0, 3);
        glm_vec4_zero(shift);
        glm_vec2_zero(lim);

        switch (level)
        {

        case VKL_AXES_LEVEL_MINOR:
            color[0] = 255;
            lim[0] = -1;
            lim[1] = -1;
            shift[3 - coord] = minor_tick_length;
            lw = 2;
            break;

        case VKL_AXES_LEVEL_MAJOR:
            lim[0] = -1;
            lim[1] = -1;
            shift[3 - coord] = major_tick_length;
            lw = 5;
            break;

        case VKL_AXES_LEVEL_GRID:
            lim[0] = -1;
            lim[1] = +1;
            lw = 1;
            break;

        case VKL_AXES_LEVEL_LIM:
            lim[0] = -1;
            lim[1] = +1;
            lw = 3;
            break;

        default:
            break;
        }

        // ticks
        _add_ticks(
            prop, vertices, indices, (VklAxisLevel)level, offset, coord, lim, color, lw, shift);
        offset += tick_count;
        ASSERT(offset <= count);
    }

    // Labels: one for each major tick.
    prop = vkl_bake_prop(visual, VKL_PROP_TEXT, 0);
    uint32_t slen = 0;
    char* text = NULL;
    vec2 anchor = {0};
    vec3 pos = {0};
    offset = 0;
    for (uint32_t i = 0; i < prop->arr_orig.item_count; i++)
    {
        // Add text.
        text = ((char**)prop->arr_orig.data)[i];
        ASSERT(text != NULL);
        slen = strlen(text);
        _graphics_text_string(
            &scene->font_atlas, i, text, pos, shift, anchor, 0, 50, (const cvec4*)color,
            &text_vertices[offset]);
        offset += 4 * slen;
    }
}

static void _visual_axes_2D(VklVisual* visual)
{
    ASSERT(visual != NULL);
    VklCanvas* canvas = visual->canvas;
    ASSERT(canvas != NULL);

    // Graphics.
    vkl_visual_graphics(visual, vkl_graphics_builtin(canvas, VKL_GRAPHICS_SEGMENT, 0));
    vkl_visual_graphics(visual, vkl_graphics_builtin(canvas, VKL_GRAPHICS_TEXT, 0));

    // Segment graphics.
    {
        // Vertex buffer.
        vkl_visual_source(
            visual, VKL_SOURCE_TYPE_VERTEX, VKL_PIPELINE_GRAPHICS, 0, //
            0, sizeof(VklGraphicsSegmentVertex), 0);

        // Index buffer.
        vkl_visual_source(
            visual, VKL_SOURCE_TYPE_INDEX, VKL_PIPELINE_GRAPHICS, 0, 0, sizeof(VklIndex), 0);

        // Uniform buffers.
        _common_sources(visual, 0); // segment visual
    }

    // Text graphics.
    {
        // Vertex buffer.
        vkl_visual_source(
            visual, VKL_SOURCE_TYPE_VERTEX, VKL_PIPELINE_GRAPHICS, 1, //
            0, sizeof(VklGraphicsTextVertex), 0);

        // Uniform buffers.
        _common_sources(visual, 1); // text visual

        // Parameters.
        vkl_visual_source(
            visual, VKL_SOURCE_TYPE_PARAM, VKL_PIPELINE_GRAPHICS, 1, //
            VKL_USER_BINDING, sizeof(VklGraphicsTextParams), 0);

        // Font atlas texture.
        vkl_visual_source(
            visual, VKL_SOURCE_TYPE_FONT_ATLAS, VKL_PIPELINE_GRAPHICS, 1, //
            VKL_USER_BINDING + 1, sizeof(cvec4), 0);
    }


    // Segment graphics props.
    {
        for (uint32_t level = 0; level < VKL_AXES_LEVEL_COUNT; level++)
        {
            // xticks
            vkl_visual_prop(
                visual, VKL_PROP_POS, level, VKL_DTYPE_FLOAT, VKL_SOURCE_TYPE_VERTEX, 0);

            // color
            vkl_visual_prop(
                visual, VKL_PROP_COLOR, level, VKL_DTYPE_CVEC4, VKL_SOURCE_TYPE_VERTEX, 0);
            vkl_visual_prop_default(visual, VKL_PROP_COLOR, level, VKL_DEFAULT_AXES_COLOR);

            // line width
            vkl_visual_prop(
                visual, VKL_PROP_LINE_WIDTH, level, VKL_DTYPE_FLOAT, VKL_SOURCE_TYPE_VERTEX, 0);
            vkl_visual_prop_default(visual, VKL_PROP_COLOR, level, &VKL_DEFAULT_AXES_LINE_WIDTH);
        }

        // minor tick length
        vkl_visual_prop(
            visual, VKL_PROP_LENGTH, VKL_AXES_LEVEL_MINOR, VKL_DTYPE_FLOAT, VKL_SOURCE_TYPE_VERTEX,
            0);
        // TODO: default

        // major tick length
        vkl_visual_prop(
            visual, VKL_PROP_LENGTH, VKL_AXES_LEVEL_MAJOR, VKL_DTYPE_FLOAT, VKL_SOURCE_TYPE_VERTEX,
            0);
        // TODO: default

        // tick h margin
        vkl_visual_prop(visual, VKL_PROP_MARGIN, 0, VKL_DTYPE_FLOAT, VKL_SOURCE_TYPE_VERTEX, 0);
        // TODO: default
    }

    // Text graphics props.
    {
        // tick text size
        vkl_visual_prop(visual, VKL_PROP_TEXT, 0, VKL_DTYPE_STR, VKL_SOURCE_TYPE_VERTEX, 1);
        // TODO: default

        // tick text size
        vkl_visual_prop(visual, VKL_PROP_TEXT_SIZE, 0, VKL_DTYPE_FLOAT, VKL_SOURCE_TYPE_VERTEX, 1);
    }

    // Common props.
    _common_props(visual, 0); // segment graphics
    _common_props(visual, 1); // text graphics

    vkl_visual_callback_bake(visual, _visual_axes_2D_bake);
}



/*************************************************************************************************/
/*  Main function                                                                                */
/*************************************************************************************************/

void vkl_visual_builtin(VklVisual* visual, VklVisualType type, int flags)
{
    ASSERT(visual != NULL);
    visual->flags = flags;
    switch (type)
    {

    case VKL_VISUAL_MARKER:
        // TODO: raw/agg
        _visual_marker_raw(visual);
        break;

    case VKL_VISUAL_SEGMENT:
        // TODO: raw/agg
        _visual_segment_raw(visual);
        break;

    case VKL_VISUAL_AXES_2D:
        _visual_axes_2D(visual);
        break;

    case VKL_VISUAL_CUSTOM:
    case VKL_VISUAL_NONE:
        break;

    default:
        log_error("no builtin visual found for type %d", type);
        break;
    }
}
