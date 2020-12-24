#include "../include/visky/builtin_visuals.h"
// #include "../include/visky/scene.h"



/*************************************************************************************************/
/*  Common utils                                                                                 */
/*************************************************************************************************/

static void _common_sources(VklVisual* visual)
{
    ASSERT(visual != NULL);

    // Binding #0: uniform buffer MVP
    vkl_visual_source( //
        visual, VKL_SOURCE_TYPE_MVP, VKL_PIPELINE_GRAPHICS, 0, 0, sizeof(VklMVP),
        VKL_SOURCE_FLAG_IMMEDIATE);

    // Binding #1: uniform buffer viewport
    vkl_visual_source(
        visual, VKL_SOURCE_TYPE_VIEWPORT, VKL_PIPELINE_GRAPHICS, 0, 1, sizeof(VklViewport), 0);
}

static void _common_props(VklVisual* visual)
{
    // MVP
    // Model.
    vkl_visual_prop(visual, VKL_PROP_MODEL, 0, VKL_DTYPE_MAT4, VKL_SOURCE_TYPE_MVP, 0);
    vkl_visual_prop_copy(
        visual, VKL_PROP_MODEL, 0, 0, offsetof(VklMVP, model), VKL_ARRAY_COPY_SINGLE, 1);

    // View.
    vkl_visual_prop(visual, VKL_PROP_VIEW, 0, VKL_DTYPE_MAT4, VKL_SOURCE_TYPE_MVP, 0);
    vkl_visual_prop_copy(
        visual, VKL_PROP_VIEW, 0, 1, offsetof(VklMVP, view), VKL_ARRAY_COPY_SINGLE, 1);

    // Proj.
    vkl_visual_prop(visual, VKL_PROP_PROJ, 0, VKL_DTYPE_MAT4, VKL_SOURCE_TYPE_MVP, 0);
    vkl_visual_prop_copy(
        visual, VKL_PROP_PROJ, 0, 2, offsetof(VklMVP, proj), VKL_ARRAY_COPY_SINGLE, 1);
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
    vkl_visual_source(
        visual, VKL_SOURCE_TYPE_VERTEX, VKL_PIPELINE_GRAPHICS, 0, 0, sizeof(VklVertex), 0);
    _common_sources(visual);
    vkl_visual_source(
        visual, VKL_SOURCE_TYPE_PARAM, VKL_PIPELINE_GRAPHICS, 0, VKL_USER_BINDING,
        sizeof(VklGraphicsPointParams), 0);

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
    _common_props(visual);

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
    _common_sources(visual);

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
    _common_props(visual);
}



/*************************************************************************************************/
/*  Axes 2D                                                                                      */
/*************************************************************************************************/

static cvec4 VKL_DEFAULT_AXES_COLOR[] = {
    {0, 0, 0, 255}, {0, 0, 0, 255}, {128, 128, 128, 255}, {0, 0, 0, 255}};
static vec4 VKL_DEFAULT_AXES_LINE_WIDTH = {2.0f, 4.0f, 1.0f, 4.0f};
static vec2 VKL_DEFAULT_AXES_TICK_LENGTH = {20.0f, 10.0f};
static float VKL_DEFAULT_AXES_FONT_SIZE = 14.0f;

static uint32_t _count_prop_items(
    VklVisual* visual, uint32_t prop_count, VklPropType* prop_types, //
    uint32_t idx_count)
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

static uint32_t _count_chars(VklProp* prop)
{
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

static void _tick_pos(float x, VklAxisLevel level, VklAxisCoord coord, vec3 P0, vec3 P1)
{
    vec2 lim = {0};
    lim[0] = -1;

    switch (level)
    {
    case VKL_AXES_LEVEL_MINOR:
    case VKL_AXES_LEVEL_MAJOR:
        lim[1] = -1;
        break;
    case VKL_AXES_LEVEL_GRID:
    case VKL_AXES_LEVEL_LIM:
        lim[1] = +1;
        break;
    default:
        break;
    }

    if (coord == VKL_AXES_COORD_X)
    {
        P0[0] = x;
        P0[1] = lim[0];
        P1[0] = x;
        P1[1] = lim[1];
    }
    else if (coord == VKL_AXES_COORD_Y)
    {
        P0[1] = x;
        P0[0] = lim[0];
        P1[1] = x;
        P1[0] = lim[1];
    }
}

static void _tick_shift(
    uint32_t i, uint32_t n, float s, vec2 tick_length, VklAxisLevel level, VklAxisCoord coord,
    vec4 shift)
{
    if (level <= VKL_AXES_LEVEL_MAJOR)
        shift[3 - coord] = tick_length[level];

    if (level == VKL_AXES_LEVEL_LIM)
    {
        if (coord == VKL_AXES_COORD_X)
        {
            // Prevent half of the first and last lines to be cut off by viewport clipping.
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
        else if (coord == VKL_AXES_COORD_Y)
        {
            // Prevent half of the first and last lines to be cut off by viewport clipping.
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
}

static void _tick_linewidth(VklAxisLevel level, float* lw)
{
    switch (level)
    {

    case VKL_AXES_LEVEL_MINOR:
        *lw = 2;
        break;

    case VKL_AXES_LEVEL_MAJOR:
        *lw = 5;
        break;

    case VKL_AXES_LEVEL_GRID:
        *lw = 1;
        break;

    case VKL_AXES_LEVEL_LIM:
        *lw = 3;
        break;

    default:
        break;
    }
}

static void _add_ticks(
    VklProp* tick_prop, VklGraphicsData* data, VklAxisLevel level, VklAxisCoord coord, cvec4 color,
    float lw, vec2 tick_length)
{
    ASSERT(tick_prop != NULL);
    ASSERT(data != NULL);

    float* x = NULL;
    vec3 P0 = {0};
    vec3 P1 = {0};
    VklCapType cap = VKL_CAP_TYPE_NONE;
    VklTransformAxis transform =
        level == VKL_AXES_LEVEL_LIM ? VKL_TRANSFORM_AXIS_NONE : VKL_TRANSFORM_AXIS_DEFAULT;

    uint32_t n = tick_prop->arr_orig.item_count;
    ASSERT(n > 0);
    float s = 0 + .5 * lw;
    VklGraphicsSegmentVertex vertex = {0};
    for (uint32_t i = 0; i < n; i++)
    {
        // TODO: transformation
        x = vkl_bake_prop_item(tick_prop, i);
        ASSERT(x != NULL);

        _tick_shift(i, n, s, tick_length, level, coord, vertex.shift);
        _tick_pos(*x, level, coord, P0, P1);

        glm_vec3_copy(P0, vertex.P0);
        glm_vec3_copy(P1, vertex.P1);
        memcpy(vertex.color, color, sizeof(cvec4));
        vertex.cap0 = vertex.cap1 = cap;
        vertex.linewidth = lw;
        vertex.transform = transform;
        vkl_graphics_append(data, &vertex);
    }
}

#define PARAM(t, x, n, i)                                                                         \
    {                                                                                             \
        t* item = vkl_bake_prop_item(vkl_bake_prop(visual, VKL_PROP_##n, i), 0);                  \
        if (item != NULL)                                                                         \
            memcpy(&x, item, sizeof(t));                                                          \
    }

static void _visual_axes_2D_bake(VklVisual* visual, VklVisualDataEvent ev)
{
    ASSERT(visual != NULL);

    // Data sources.
    VklSource* seg_vert_src = vkl_bake_source(visual, VKL_SOURCE_TYPE_VERTEX, 0);
    VklSource* seg_index_src = vkl_bake_source(visual, VKL_SOURCE_TYPE_INDEX, 0);
    VklSource* text_vert_src = vkl_bake_source(visual, VKL_SOURCE_TYPE_VERTEX, 1);

    // Count the total number of segments.
    uint32_t count = _count_prop_items(visual, 1, (VklPropType[]){VKL_PROP_POS}, 4);


    // Segment graphics.
    // -----------------

    VklGraphicsData seg_data =
        vkl_graphics_data(visual->graphics[0], &seg_vert_src->arr, &seg_index_src->arr, visual);
    vkl_graphics_alloc(&seg_data, count);

    // Visual coordinate.
    VklAxisCoord coord = (VklAxisCoord)visual->flags;
    ASSERT(coord < 2);

    // Params
    cvec4 color = {0};
    float lw = 0;
    float tick_length_minor = 0, tick_length_major = 0;
    PARAM(float, tick_length_minor, LENGTH, VKL_AXES_LEVEL_MINOR)
    PARAM(float, tick_length_major, LENGTH, VKL_AXES_LEVEL_MAJOR)
    vec2 tick_length = {tick_length_minor, tick_length_major};

    VklProp* prop = NULL;
    uint32_t tick_count = 0;
    for (uint32_t level = 0; level < VKL_AXES_LEVEL_COUNT; level++)
    {
        // Take the tick positions.
        prop = vkl_bake_prop(visual, VKL_PROP_POS, level);
        ASSERT(prop != NULL);
        tick_count = prop->arr_orig.item_count; // number of ticks for this level.
        if (tick_count == 0)
            continue;
        ASSERT(tick_count > 0);

        PARAM(cvec4, color, COLOR, level)
        PARAM(float, lw, LINE_WIDTH, level)

        // ticks
        _tick_linewidth(level, &lw);
        _add_ticks(prop, &seg_data, (VklAxisLevel)level, coord, color, lw, tick_length);
    }

    // Labels: one for each major tick.
    VklGraphicsData text_data =
        vkl_graphics_data(visual->graphics[1], &text_vert_src->arr, NULL, visual);

    // Text prop.
    prop = vkl_bake_prop(visual, VKL_PROP_TEXT, 0);
    ASSERT(prop != NULL);

    // Major tick prop.
    VklProp* prop_major = vkl_bake_prop(visual, VKL_PROP_POS, VKL_AXES_LEVEL_MAJOR);
    uint32_t n_major = prop_major->arr_orig.item_count;
    uint32_t n_text = prop->arr_orig.item_count;
    uint32_t count_chars = _count_chars(prop);

    // Skip text graphics if no text.
    if (n_text == 0 || count_chars == 0 || n_major == 0)
    {
        log_warn("skip text graphics in axes visual as MAJOR pos or TEXT not set");
        return;
    }

    // Text graphics.
    // --------------

    ASSERT(count_chars > 0);
    ASSERT(n_major > 0);
    ASSERT(n_text <= count_chars);
    n_text = MIN(n_text, n_major);
    ASSERT(n_text > 0);

    // Allocate the text vertex array.
    vkl_graphics_alloc(&text_data, count_chars);

    char* text = NULL;
    VklGraphicsTextItem str_item = {0};
    float* x = NULL;
    vec3 P = {0};
    if (coord == VKL_AXES_COORD_X)
    {
        str_item.vertex.anchor[1] = 1;
        str_item.vertex.shift[1] = -10;
    }
    else if (coord == VKL_AXES_COORD_Y)
    {
        str_item.vertex.anchor[0] = -1;
        str_item.vertex.shift[0] = -10;
    }
    str_item.vertex.color[3] = 255;
    for (uint32_t i = 0; i < n_text; i++)
    {
        // Add text.
        text = ((char**)prop->arr_orig.data)[i];
        ASSERT(text != NULL);
        ASSERT(strlen(text) > 0);
        str_item.font_size = 12;
        str_item.string = text;

        // Position of the text corresponds to position of the major tick.
        x = vkl_bake_prop_item(prop_major, i);
        ASSERT(x != NULL);
        _tick_pos(*x, VKL_AXES_LEVEL_MAJOR, coord, str_item.vertex.pos, P);

        vkl_graphics_append(&text_data, &str_item);
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
    }

    // Text graphics.
    {
        // Vertex buffer.
        vkl_visual_source(
            visual, VKL_SOURCE_TYPE_VERTEX, VKL_PIPELINE_GRAPHICS, 1, //
            0, sizeof(VklGraphicsTextVertex), 0);

        // Parameters.
        vkl_visual_source(
            visual, VKL_SOURCE_TYPE_PARAM, VKL_PIPELINE_GRAPHICS, 1, //
            VKL_USER_BINDING, sizeof(VklGraphicsTextParams), 0);

        // Font atlas texture.
        vkl_visual_source(
            visual, VKL_SOURCE_TYPE_FONT_ATLAS, VKL_PIPELINE_GRAPHICS, 1, //
            VKL_USER_BINDING + 1, sizeof(cvec4), 0);
    }

    // Uniform buffers.
    _common_sources(visual);

    // Share the MVP source with the second visual.
    vkl_visual_source_share(visual, VKL_SOURCE_TYPE_MVP, 0, 1);

    // NOTE: the viewport source is not shared, as we want different clipping for both graphics.
    vkl_visual_source(
        visual, VKL_SOURCE_TYPE_VIEWPORT, VKL_PIPELINE_GRAPHICS, 1, 1, sizeof(VklViewport), 0);

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
            vkl_visual_prop_default(visual, VKL_PROP_COLOR, level, &VKL_DEFAULT_AXES_COLOR[level]);

            // line width
            vkl_visual_prop(
                visual, VKL_PROP_LINE_WIDTH, level, VKL_DTYPE_FLOAT, VKL_SOURCE_TYPE_VERTEX, 0);
            vkl_visual_prop_default(
                visual, VKL_PROP_LINE_WIDTH, level, &VKL_DEFAULT_AXES_LINE_WIDTH[level]);
        }

        // minor tick length
        vkl_visual_prop(
            visual, VKL_PROP_LENGTH, VKL_AXES_LEVEL_MINOR, VKL_DTYPE_FLOAT, VKL_SOURCE_TYPE_VERTEX,
            0);
        vkl_visual_prop_default(
            visual, VKL_PROP_LENGTH, VKL_AXES_LEVEL_MINOR, &VKL_DEFAULT_AXES_TICK_LENGTH[0]);

        // major tick length
        vkl_visual_prop(
            visual, VKL_PROP_LENGTH, VKL_AXES_LEVEL_MAJOR, VKL_DTYPE_FLOAT, VKL_SOURCE_TYPE_VERTEX,
            0);
        vkl_visual_prop_default(
            visual, VKL_PROP_LENGTH, VKL_AXES_LEVEL_MAJOR, &VKL_DEFAULT_AXES_TICK_LENGTH[1]);

        // tick h margin
        // vkl_visual_prop(visual, VKL_PROP_MARGIN, 0, VKL_DTYPE_FLOAT, VKL_SOURCE_TYPE_VERTEX, 0);
        // TODO: default
    }

    _common_props(visual); // segment graphics

    // Text graphics props.
    {
        // tick text
        vkl_visual_prop(visual, VKL_PROP_TEXT, 0, VKL_DTYPE_STR, VKL_SOURCE_TYPE_VERTEX, 1);

        // tick text size
        vkl_visual_prop(visual, VKL_PROP_TEXT_SIZE, 0, VKL_DTYPE_FLOAT, VKL_SOURCE_TYPE_VERTEX, 1);
        vkl_visual_prop_default(visual, VKL_PROP_TEXT_SIZE, 0, &VKL_DEFAULT_AXES_FONT_SIZE);
    }

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
