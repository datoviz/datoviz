#include "../include/visky/builtin_visuals.h"
#include "../include/visky/interact.h"
#include "visuals_utils.h"



/*************************************************************************************************/
/*  Common utils                                                                                 */
/*************************************************************************************************/

static void _common_sources(VklVisual* visual)
{
    ASSERT(visual != NULL);

    // Binding #0: uniform buffer MVP
    vkl_visual_source( //
        visual, VKL_SOURCE_TYPE_MVP, 0, VKL_PIPELINE_GRAPHICS, 0, 0, sizeof(VklMVP),
        VKL_SOURCE_FLAG_MAPPABLE);

    // Binding #1: uniform buffer viewport
    vkl_visual_source(
        visual, VKL_SOURCE_TYPE_VIEWPORT, 0, VKL_PIPELINE_GRAPHICS, 0, 1, sizeof(VklViewport), 0);
}

static void _common_props(VklVisual* visual)
{
    VklProp* prop = NULL;

    // MVP
    // Model.
    prop = vkl_visual_prop(visual, VKL_PROP_MODEL, 0, VKL_DTYPE_MAT4, VKL_SOURCE_TYPE_MVP, 0);
    vkl_visual_prop_copy(prop, 0, offsetof(VklMVP, model), VKL_ARRAY_COPY_SINGLE, 1);

    // View.
    prop = vkl_visual_prop(visual, VKL_PROP_VIEW, 0, VKL_DTYPE_MAT4, VKL_SOURCE_TYPE_MVP, 0);
    vkl_visual_prop_copy(prop, 1, offsetof(VklMVP, view), VKL_ARRAY_COPY_SINGLE, 1);

    // Proj.
    prop = vkl_visual_prop(visual, VKL_PROP_PROJ, 0, VKL_DTYPE_MAT4, VKL_SOURCE_TYPE_MVP, 0);
    vkl_visual_prop_copy(prop, 2, offsetof(VklMVP, proj), VKL_ARRAY_COPY_SINGLE, 1);

    // Time.
    prop = vkl_visual_prop(visual, VKL_PROP_TIME, 0, VKL_DTYPE_FLOAT, VKL_SOURCE_TYPE_MVP, 0);
    vkl_visual_prop_copy(prop, 3, offsetof(VklMVP, time), VKL_ARRAY_COPY_SINGLE, 1);
}



/*************************************************************************************************/
/*************************************************************************************************/
/*  Basic visuals                                                                                */
/*************************************************************************************************/
/*************************************************************************************************/



/*************************************************************************************************/
/*  Point                                                                                        */
/*************************************************************************************************/

static void _visual_point(VklVisual* visual)
{
    ASSERT(visual != NULL);
    VklCanvas* canvas = visual->canvas;
    ASSERT(canvas != NULL);
    VklProp* prop = NULL;

    // Graphics.
    vkl_visual_graphics(visual, vkl_graphics_builtin(canvas, VKL_GRAPHICS_POINT, visual->flags));

    // Sources
    vkl_visual_source(
        visual, VKL_SOURCE_TYPE_VERTEX, 0, VKL_PIPELINE_GRAPHICS, 0, 0, sizeof(VklVertex), 0);
    _common_sources(visual);
    vkl_visual_source(
        visual, VKL_SOURCE_TYPE_PARAM, 0, VKL_PIPELINE_GRAPHICS, 0, VKL_USER_BINDING,
        sizeof(VklGraphicsPointParams), 0);

    // Props:

    // Vertex pos.
    prop = vkl_visual_prop(visual, VKL_PROP_POS, 0, VKL_DTYPE_DVEC3, VKL_SOURCE_TYPE_VERTEX, 0);
    vkl_visual_prop_cast(
        prop, 0, offsetof(VklVertex, pos), VKL_DTYPE_VEC3, VKL_ARRAY_COPY_SINGLE, 1);

    // Vertex color.
    prop = vkl_visual_prop(visual, VKL_PROP_COLOR, 0, VKL_DTYPE_CVEC4, VKL_SOURCE_TYPE_VERTEX, 0);
    vkl_visual_prop_copy(prop, 1, offsetof(VklVertex, color), VKL_ARRAY_COPY_SINGLE, 1);
    cvec4 color = {200, 200, 200, 255};
    vkl_visual_prop_default(prop, &color);

    // Common props.
    _common_props(visual);

    // Param: marker size.
    prop = vkl_visual_prop(
        visual, VKL_PROP_MARKER_SIZE, 0, VKL_DTYPE_FLOAT, VKL_SOURCE_TYPE_PARAM, 0);
    vkl_visual_prop_copy(
        prop, 0, offsetof(VklGraphicsPointParams, point_size), VKL_ARRAY_COPY_SINGLE, 1);
    float size = 5;
    vkl_visual_prop_default(prop, &size);
}



/*************************************************************************************************/
/*  Line                                                                                         */
/*************************************************************************************************/

static void _visual_line(VklVisual* visual)
{
    ASSERT(visual != NULL);
    VklCanvas* canvas = visual->canvas;
    ASSERT(canvas != NULL);
    VklProp* prop = NULL;

    // Graphics.
    vkl_visual_graphics(visual, vkl_graphics_builtin(canvas, VKL_GRAPHICS_LINE, 0));

    // Sources
    vkl_visual_source(
        visual, VKL_SOURCE_TYPE_VERTEX, 0, VKL_PIPELINE_GRAPHICS, 0, 0, sizeof(VklVertex), 0);
    _common_sources(visual);

    // Props:

    // Vertex pos, segment start.
    prop = vkl_visual_prop(visual, VKL_PROP_POS, 0, VKL_DTYPE_DVEC3, VKL_SOURCE_TYPE_VERTEX, 0);
    vkl_visual_prop_cast(
        prop, 0, offsetof(VklVertex, pos), VKL_DTYPE_VEC3, VKL_ARRAY_COPY_SINGLE, 2);

    // Vertex pos, segment end.
    prop = vkl_visual_prop(visual, VKL_PROP_POS, 1, VKL_DTYPE_DVEC3, VKL_SOURCE_TYPE_VERTEX, 0);
    vkl_visual_prop_cast(
        prop, 0, sizeof(VklVertex) + offsetof(VklVertex, pos), VKL_DTYPE_VEC3,
        VKL_ARRAY_COPY_SINGLE, 2);


    // Vertex color.
    prop = vkl_visual_prop(visual, VKL_PROP_COLOR, 0, VKL_DTYPE_CVEC4, VKL_SOURCE_TYPE_VERTEX, 0);
    vkl_visual_prop_copy(prop, 1, offsetof(VklVertex, color), VKL_ARRAY_COPY_REPEAT, 2);

    // Common props.
    _common_props(visual);
}



/*************************************************************************************************/
/*  Line strip                                                                                   */
/*************************************************************************************************/

static void _visual_line_strip(VklVisual* visual)
{
    ASSERT(visual != NULL);
    VklCanvas* canvas = visual->canvas;
    ASSERT(canvas != NULL);
    VklProp* prop = NULL;

    // Graphics.
    vkl_visual_graphics(visual, vkl_graphics_builtin(canvas, VKL_GRAPHICS_LINE_STRIP, 0));

    // Sources
    vkl_visual_source(
        visual, VKL_SOURCE_TYPE_VERTEX, 0, VKL_PIPELINE_GRAPHICS, 0, 0, sizeof(VklVertex), 0);
    _common_sources(visual);

    // Props:

    // Vertex pos.
    prop = vkl_visual_prop(visual, VKL_PROP_POS, 0, VKL_DTYPE_DVEC3, VKL_SOURCE_TYPE_VERTEX, 0);
    vkl_visual_prop_cast(
        prop, 0, offsetof(VklVertex, pos), VKL_DTYPE_VEC3, VKL_ARRAY_COPY_SINGLE, 1);

    // Vertex color.
    prop = vkl_visual_prop(visual, VKL_PROP_COLOR, 0, VKL_DTYPE_CVEC4, VKL_SOURCE_TYPE_VERTEX, 0);
    vkl_visual_prop_copy(prop, 1, offsetof(VklVertex, color), VKL_ARRAY_COPY_REPEAT, 1);

    // Common props.
    _common_props(visual);
}



/*************************************************************************************************/
/*  Triangle                                                                                     */
/*************************************************************************************************/

static void _visual_triangle(VklVisual* visual)
{
    ASSERT(visual != NULL);
    VklCanvas* canvas = visual->canvas;
    ASSERT(canvas != NULL);
    VklProp* prop = NULL;

    // Graphics.
    vkl_visual_graphics(visual, vkl_graphics_builtin(canvas, VKL_GRAPHICS_TRIANGLE, 0));

    // Sources
    vkl_visual_source(
        visual, VKL_SOURCE_TYPE_VERTEX, 0, VKL_PIPELINE_GRAPHICS, 0, 0, sizeof(VklVertex), 0);
    _common_sources(visual);

    // Props:

    // Vertex pos, triangle point.
    for (uint32_t i = 0; i < 3; i++)
    {
        prop =
            vkl_visual_prop(visual, VKL_PROP_POS, i, VKL_DTYPE_DVEC3, VKL_SOURCE_TYPE_VERTEX, 0);
        vkl_visual_prop_cast(
            prop, 0, i * sizeof(VklVertex) + offsetof(VklVertex, pos), //
            VKL_DTYPE_VEC3, VKL_ARRAY_COPY_SINGLE, 3);
    }

    // Vertex color.
    prop = vkl_visual_prop(visual, VKL_PROP_COLOR, 0, VKL_DTYPE_CVEC4, VKL_SOURCE_TYPE_VERTEX, 0);
    vkl_visual_prop_copy(prop, 1, offsetof(VklVertex, color), VKL_ARRAY_COPY_REPEAT, 3);

    // Common props.
    _common_props(visual);
}



/*************************************************************************************************/
/*  Triangle strip                                                                               */
/*************************************************************************************************/

static void _visual_triangle_strip(VklVisual* visual)
{
    ASSERT(visual != NULL);
    VklCanvas* canvas = visual->canvas;
    ASSERT(canvas != NULL);
    VklProp* prop = NULL;

    // Graphics.
    vkl_visual_graphics(visual, vkl_graphics_builtin(canvas, VKL_GRAPHICS_TRIANGLE_STRIP, 0));

    // Sources
    vkl_visual_source(
        visual, VKL_SOURCE_TYPE_VERTEX, 0, VKL_PIPELINE_GRAPHICS, 0, 0, sizeof(VklVertex), 0);
    _common_sources(visual);

    // Props:

    // Vertex pos.
    prop = vkl_visual_prop(visual, VKL_PROP_POS, 0, VKL_DTYPE_DVEC3, VKL_SOURCE_TYPE_VERTEX, 0);
    vkl_visual_prop_cast(
        prop, 0, offsetof(VklVertex, pos), VKL_DTYPE_VEC3, VKL_ARRAY_COPY_SINGLE, 1);

    // Vertex color.
    prop = vkl_visual_prop(visual, VKL_PROP_COLOR, 0, VKL_DTYPE_CVEC4, VKL_SOURCE_TYPE_VERTEX, 0);
    vkl_visual_prop_copy(prop, 1, offsetof(VklVertex, color), VKL_ARRAY_COPY_REPEAT, 1);

    // Common props.
    _common_props(visual);
}



/*************************************************************************************************/
/*  Triangle fan                                                                                 */
/*************************************************************************************************/

static void _visual_triangle_fan(VklVisual* visual)
{
    ASSERT(visual != NULL);
    VklCanvas* canvas = visual->canvas;
    ASSERT(canvas != NULL);
    VklProp* prop = NULL;

    // Graphics.
    vkl_visual_graphics(visual, vkl_graphics_builtin(canvas, VKL_GRAPHICS_TRIANGLE_FAN, 0));

    // Sources
    vkl_visual_source(
        visual, VKL_SOURCE_TYPE_VERTEX, 0, VKL_PIPELINE_GRAPHICS, 0, 0, sizeof(VklVertex), 0);
    _common_sources(visual);

    // Props:

    // Vertex pos.
    prop = vkl_visual_prop(visual, VKL_PROP_POS, 0, VKL_DTYPE_DVEC3, VKL_SOURCE_TYPE_VERTEX, 0);
    vkl_visual_prop_cast(
        prop, 0, offsetof(VklVertex, pos), VKL_DTYPE_VEC3, VKL_ARRAY_COPY_SINGLE, 1);

    // Vertex color.
    prop = vkl_visual_prop(visual, VKL_PROP_COLOR, 0, VKL_DTYPE_CVEC4, VKL_SOURCE_TYPE_VERTEX, 0);
    vkl_visual_prop_copy(prop, 1, offsetof(VklVertex, color), VKL_ARRAY_COPY_REPEAT, 1);

    // Common props.
    _common_props(visual);
}



/*************************************************************************************************/
/*************************************************************************************************/
/*  Antialiased 2D visuals                                                                       */
/*************************************************************************************************/
/*************************************************************************************************/



/*************************************************************************************************/
/*  Marker                                                                                       */
/*************************************************************************************************/

static void _visual_marker(VklVisual* visual)
{
    ASSERT(visual != NULL);
    VklCanvas* canvas = visual->canvas;
    ASSERT(canvas != NULL);
    VklProp* prop = NULL;

    // Graphics.
    vkl_visual_graphics(visual, vkl_graphics_builtin(canvas, VKL_GRAPHICS_MARKER, visual->flags));
    vkl_graphics_depth_test(visual->graphics[0], VKL_DEPTH_TEST_DISABLE);

    // Sources
    vkl_visual_source(
        visual, VKL_SOURCE_TYPE_VERTEX, 0, VKL_PIPELINE_GRAPHICS, 0, 0,
        sizeof(VklGraphicsMarkerVertex), 0);
    _common_sources(visual);
    vkl_visual_source(
        visual, VKL_SOURCE_TYPE_PARAM, 0, VKL_PIPELINE_GRAPHICS, 0, VKL_USER_BINDING,
        sizeof(VklGraphicsMarkerParams), 0);

    // Props:

    // Marker pos.
    prop = vkl_visual_prop(visual, VKL_PROP_POS, 0, VKL_DTYPE_DVEC3, VKL_SOURCE_TYPE_VERTEX, 0);
    vkl_visual_prop_cast(
        prop, 0, offsetof(VklGraphicsMarkerVertex, pos), VKL_DTYPE_VEC3, VKL_ARRAY_COPY_SINGLE, 1);

    // Marker color.
    prop = vkl_visual_prop(visual, VKL_PROP_COLOR, 0, VKL_DTYPE_CVEC4, VKL_SOURCE_TYPE_VERTEX, 0);
    vkl_visual_prop_copy(
        prop, 1, offsetof(VklGraphicsMarkerVertex, color), VKL_ARRAY_COPY_SINGLE, 1);
    cvec4 color = {200, 200, 200, 255};
    vkl_visual_prop_default(prop, &color);

    // Marker size.
    prop = vkl_visual_prop(
        visual, VKL_PROP_MARKER_SIZE, 0, VKL_DTYPE_FLOAT, VKL_SOURCE_TYPE_VERTEX, 0);
    vkl_visual_prop_copy(
        prop, 1, offsetof(VklGraphicsMarkerVertex, size), VKL_ARRAY_COPY_SINGLE, 1);
    float size = 20;
    vkl_visual_prop_default(prop, &size);

    // Marker type.
    prop = vkl_visual_prop(
        visual, VKL_PROP_MARKER_TYPE, 0, VKL_DTYPE_CHAR, VKL_SOURCE_TYPE_VERTEX, 0);
    vkl_visual_prop_copy(
        prop, 1, offsetof(VklGraphicsMarkerVertex, marker), VKL_ARRAY_COPY_SINGLE, 1);
    VklMarkerType marker = VKL_MARKER_DISC;
    vkl_visual_prop_default(prop, &marker);

    // Marker angle.
    prop = vkl_visual_prop(visual, VKL_PROP_ANGLE, 0, VKL_DTYPE_CHAR, VKL_SOURCE_TYPE_VERTEX, 0);
    vkl_visual_prop_copy(
        prop, 1, offsetof(VklGraphicsMarkerVertex, angle), VKL_ARRAY_COPY_SINGLE, 1);
    float angle = 0;
    vkl_visual_prop_default(prop, &angle);

    // Marker transform.
    prop =
        vkl_visual_prop(visual, VKL_PROP_TRANSFORM, 0, VKL_DTYPE_CHAR, VKL_SOURCE_TYPE_VERTEX, 0);
    vkl_visual_prop_copy(
        prop, 1, offsetof(VklGraphicsMarkerVertex, transform), VKL_ARRAY_COPY_SINGLE, 1);

    // Common props.
    _common_props(visual);

    // Param: edge color.
    prop = vkl_visual_prop(visual, VKL_PROP_COLOR, 1, VKL_DTYPE_VEC4, VKL_SOURCE_TYPE_PARAM, 0);
    vkl_visual_prop_copy(
        prop, 0, offsetof(VklGraphicsMarkerParams, edge_color), VKL_ARRAY_COPY_SINGLE, 1);
    vec4 edge_color = {0, 0, 0, 1};
    vkl_visual_prop_default(prop, &edge_color);

    // Param: edge width.
    prop =
        vkl_visual_prop(visual, VKL_PROP_LINE_WIDTH, 0, VKL_DTYPE_FLOAT, VKL_SOURCE_TYPE_PARAM, 0);
    vkl_visual_prop_copy(
        prop, 0, offsetof(VklGraphicsMarkerParams, edge_width), VKL_ARRAY_COPY_SINGLE, 1);
    float edge_width = 1;
    vkl_visual_prop_default(prop, &edge_width);
}



/*************************************************************************************************/
/*  Axes 2D                                                                                      */
/*************************************************************************************************/

// TODO: customizable params
static cvec4 VKL_DEFAULT_AXES_COLOR[] = {
    {0, 0, 0, 255}, {0, 0, 0, 255}, {128, 128, 128, 255}, {0, 0, 0, 255}};
static vec4 VKL_DEFAULT_AXES_LINE_WIDTH = {2.0f, 4.0f, 1.0f, 2.0f};
static vec2 VKL_DEFAULT_AXES_TICK_LENGTH = {10.0f, 15.0f};
static float VKL_DEFAULT_AXES_FONT_SIZE = 12.0f;

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
            count += vkl_prop_size(vkl_prop_get(visual, prop_types[i], j));
        }
    }
    return count;
}

static uint32_t _count_chars(VklArray* arr_text)
{
    ASSERT(arr_text != NULL);
    uint32_t n_text = arr_text->item_count;
    uint32_t char_count = 0;
    char* str = NULL;
    uint32_t slen = 0;
    for (uint32_t i = 0; i < n_text; i++)
    {
        str = ((char**)arr_text->data)[i];
        slen = strlen(str);
        ASSERT(slen > 0);
        char_count += slen;
    }
    return char_count;
}

static void _tick_pos(double x, VklAxisLevel level, VklAxisCoord coord, vec3 P0, vec3 P1)
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

static void _add_ticks(
    VklProp* tick_prop, VklGraphicsData* data, VklAxisLevel level, VklAxisCoord coord, cvec4 color,
    float lw, vec2 tick_length)
{
    ASSERT(tick_prop != NULL);
    ASSERT(data != NULL);

    double* x = NULL;
    vec3 P0 = {0};
    vec3 P1 = {0};
    VklCapType cap = VKL_CAP_TYPE_NONE;
    int32_t interact_axis = level == VKL_AXES_LEVEL_LIM ? VKL_INTERACT_FIXED_AXIS_ALL
                                                        : VKL_INTERACT_FIXED_AXIS_DEFAULT;
    interact_axis = interact_axis >> 12;
    ASSERT(0 <= interact_axis && interact_axis <= 8);

    uint32_t n = tick_prop->arr_orig.item_count;
    ASSERT(n > 0);
    float s = 0 + .5 * lw;
    VklGraphicsSegmentVertex vertex = {0};
    for (uint32_t i = 0; i < n; i++)
    {
        // TODO: transformation
        x = vkl_prop_item(tick_prop, i);
        ASSERT(x != NULL);

        _tick_shift(i, n, s, tick_length, level, coord, vertex.shift);
        _tick_pos(*x, level, coord, P0, P1);

        glm_vec3_copy(P0, vertex.P0);
        glm_vec3_copy(P1, vertex.P1);
        memcpy(vertex.color, color, sizeof(cvec4));
        vertex.cap0 = vertex.cap1 = cap;
        vertex.linewidth = lw;
        vertex.transform = interact_axis;
        vkl_graphics_append(data, &vertex);
    }
}

static void _visual_axes_2D_bake(VklVisual* visual, VklVisualDataEvent ev)
{
    ASSERT(visual != NULL);

    // Data sources.
    VklSource* seg_vert_src = vkl_source_get(visual, VKL_SOURCE_TYPE_VERTEX, 0);
    VklSource* seg_index_src = vkl_source_get(visual, VKL_SOURCE_TYPE_INDEX, 0);
    VklSource* text_vert_src = vkl_source_get(visual, VKL_SOURCE_TYPE_VERTEX, 1);

    // HACK: mark the index buffer to be updated.
    seg_index_src->obj.request = VKL_VISUAL_REQUEST_UPLOAD;

    // Count the total number of segments.
    // NOTE: the number of segments is determined by the POS prop.
    uint32_t count = _count_prop_items(visual, 1, (VklPropType[]){VKL_PROP_POS}, 4);


    // Segment graphics.
    // -----------------

    VklGraphicsData seg_data =
        vkl_graphics_data(visual->graphics[0], &seg_vert_src->arr, &seg_index_src->arr, visual);
    vkl_graphics_alloc(&seg_data, count);

    // Visual coordinate.
    VklAxisCoord coord = (VklAxisCoord)(visual->flags & 0xF);
    ASSERT(coord < 2);

    // Params
    cvec4 color = {0};
    float lw = 0;

    float tick_length_minor = 0, tick_length_major = 0;
    PARAM(float, tick_length_minor, LENGTH, VKL_AXES_LEVEL_MINOR)
    PARAM(float, tick_length_major, LENGTH, VKL_AXES_LEVEL_MAJOR)
    DPI_SCALE(tick_length_minor)
    DPI_SCALE(tick_length_major)

    vec2 tick_length = {tick_length_minor, tick_length_major};

    VklProp* prop = NULL;
    uint32_t tick_count = 0;
    for (uint32_t level = 0; level < VKL_AXES_LEVEL_COUNT; level++)
    {
        // Take the tick positions.
        prop = vkl_prop_get(visual, VKL_PROP_POS, level);
        ASSERT(prop != NULL);
        tick_count = prop->arr_orig.item_count; // number of ticks for this level.
        if (tick_count == 0)
            continue;
        ASSERT(tick_count > 0);

        PARAM(cvec4, color, COLOR, level)
        PARAM(float, lw, LINE_WIDTH, level)
        DPI_SCALE(lw)

        _add_ticks(prop, &seg_data, (VklAxisLevel)level, coord, color, lw, tick_length);
    }

    // Labels: one for each major tick.
    VklGraphicsData text_data =
        vkl_graphics_data(visual->graphics[1], &text_vert_src->arr, NULL, visual);

    // Text prop.
    VklArray* arr_text = _prop_array(vkl_prop_get(visual, VKL_PROP_TEXT, 0));
    ASSERT(prop != NULL);

    // Major tick prop.
    VklProp* prop_major = vkl_prop_get(visual, VKL_PROP_POS, VKL_AXES_LEVEL_MAJOR);
    uint32_t n_major = prop_major->arr_orig.item_count;
    uint32_t n_text = arr_text->item_count;
    uint32_t count_chars = _count_chars(arr_text);

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
    double* x = NULL;
    vec3 P = {0};
    float font_size = 0;

    PARAM(float, font_size, TEXT_SIZE, 0)
    DPI_SCALE(font_size)

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
        text = ((char**)arr_text->data)[i];
        ASSERT(text != NULL);
        ASSERT(strlen(text) > 0);
        str_item.font_size = font_size;
        str_item.string = text;

        // Position of the text corresponds to position of the major tick.
        x = vkl_prop_item(prop_major, i);
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
    VklProp* prop = NULL;

    // Graphics.
    vkl_visual_graphics(visual, vkl_graphics_builtin(canvas, VKL_GRAPHICS_SEGMENT, 0));
    vkl_visual_graphics(visual, vkl_graphics_builtin(canvas, VKL_GRAPHICS_TEXT, 0));

    // Segment graphics.
    {
        // Vertex buffer.
        vkl_visual_source(
            visual, VKL_SOURCE_TYPE_VERTEX, 0, VKL_PIPELINE_GRAPHICS, 0, //
            0, sizeof(VklGraphicsSegmentVertex), 0);

        // Index buffer.
        vkl_visual_source(
            visual, VKL_SOURCE_TYPE_INDEX, 0, VKL_PIPELINE_GRAPHICS, 0, //
            0, sizeof(VklIndex), 0);
    }

    // Text graphics.
    {
        // Vertex buffer.
        vkl_visual_source(
            visual, VKL_SOURCE_TYPE_VERTEX, 1, VKL_PIPELINE_GRAPHICS, 1, //
            0, sizeof(VklGraphicsTextVertex), 0);

        // Parameters.
        vkl_visual_source(
            visual, VKL_SOURCE_TYPE_PARAM, 0, VKL_PIPELINE_GRAPHICS, 1, //
            VKL_USER_BINDING, sizeof(VklGraphicsTextParams), 0);

        // Font atlas texture.
        vkl_visual_source(
            visual, VKL_SOURCE_TYPE_FONT_ATLAS, 0, VKL_PIPELINE_GRAPHICS, 1, //
            VKL_USER_BINDING + 1, sizeof(cvec4), 0);
    }

    // Uniform buffers. // set MVP and Viewport sources for pipeline #0
    _common_sources(visual);

    // Binding #0: uniform buffer MVP
    vkl_visual_source( //
        visual, VKL_SOURCE_TYPE_MVP, 1, VKL_PIPELINE_GRAPHICS, 1, 0, sizeof(VklMVP),
        VKL_SOURCE_FLAG_MAPPABLE);
    // Share the MVP source with the second graphics pipeline.
    vkl_visual_source_share(visual, VKL_SOURCE_TYPE_MVP, 0, 1);

    // NOTE: the viewport source is not shared, as we want different clipping for both graphics.
    vkl_visual_source(
        visual, VKL_SOURCE_TYPE_VIEWPORT, 1, VKL_PIPELINE_GRAPHICS, 1, 1, sizeof(VklViewport), 0);

    // Segment graphics props.
    {
        for (uint32_t level = 0; level < VKL_AXES_LEVEL_COUNT; level++)
        {
            // xticks
            prop = vkl_visual_prop(
                visual, VKL_PROP_POS, level, VKL_DTYPE_DOUBLE, VKL_SOURCE_TYPE_VERTEX, 0);

            // color
            prop = vkl_visual_prop(
                visual, VKL_PROP_COLOR, level, VKL_DTYPE_CVEC4, VKL_SOURCE_TYPE_VERTEX, 0);
            vkl_visual_prop_default(prop, &VKL_DEFAULT_AXES_COLOR[level]);

            // line width
            prop = vkl_visual_prop(
                visual, VKL_PROP_LINE_WIDTH, level, VKL_DTYPE_FLOAT, VKL_SOURCE_TYPE_VERTEX, 0);
            vkl_visual_prop_default(prop, &VKL_DEFAULT_AXES_LINE_WIDTH[level]);
        }

        // minor tick length
        prop = vkl_visual_prop(
            visual, VKL_PROP_LENGTH, VKL_AXES_LEVEL_MINOR, VKL_DTYPE_FLOAT, VKL_SOURCE_TYPE_VERTEX,
            0);
        vkl_visual_prop_default(prop, &VKL_DEFAULT_AXES_TICK_LENGTH[0]);

        // major tick length
        prop = vkl_visual_prop(
            visual, VKL_PROP_LENGTH, VKL_AXES_LEVEL_MAJOR, VKL_DTYPE_FLOAT, VKL_SOURCE_TYPE_VERTEX,
            0);
        vkl_visual_prop_default(prop, &VKL_DEFAULT_AXES_TICK_LENGTH[1]);

        // tick h margin
        // vkl_visual_prop(visual, VKL_PROP_MARGIN, 0, VKL_DTYPE_FLOAT, VKL_SOURCE_TYPE_VERTEX, 0);
        // TODO: default
    }

    _common_props(visual); // segment graphics

    // Text graphics props.
    {
        // tick text
        prop = vkl_visual_prop(visual, VKL_PROP_TEXT, 0, VKL_DTYPE_STR, VKL_SOURCE_TYPE_VERTEX, 1);

        // tick text size
        prop = vkl_visual_prop(
            visual, VKL_PROP_TEXT_SIZE, 0, VKL_DTYPE_FLOAT, VKL_SOURCE_TYPE_VERTEX, 1);
        vkl_visual_prop_default(prop, &VKL_DEFAULT_AXES_FONT_SIZE);
    }

    vkl_visual_callback_bake(visual, _visual_axes_2D_bake);
}



/*************************************************************************************************/
/*  Polygon                                                                                      */
/*************************************************************************************************/

static void _polygon_bake(VklVisual* visual, VklVisualDataEvent ev)
{
    ASSERT(visual != NULL);

    VklProp* prop_pos = vkl_prop_get(visual, VKL_PROP_POS, 0);       // dvec3
    VklProp* prop_length = vkl_prop_get(visual, VKL_PROP_LENGTH, 0); // uint
    VklProp* prop_color = vkl_prop_get(visual, VKL_PROP_COLOR, 0);   // cvec4

    VklArray* arr_pos = _prop_array(prop_pos);
    VklArray* arr_length = _prop_array(prop_length);
    VklArray* arr_color = _prop_array(prop_color);

    VklSource* src_vertex = vkl_source_get(visual, VKL_SOURCE_TYPE_VERTEX, 0);
    VklSource* src_index = vkl_source_get(visual, VKL_SOURCE_TYPE_INDEX, 0);

    // The baking function doesn't run if the VERTEX source is handled by the user.
    if (src_vertex->origin != VKL_SOURCE_ORIGIN_LIB)
        return;
    if (src_vertex->obj.request != VKL_VISUAL_REQUEST_UPLOAD)
    {
        log_trace(
            "skip bake source for source %d that doesn't need updating", src_vertex->source_kind);
        return;
    }

    // Source arrays.
    VklArray* arr_vertex = &src_vertex->arr;
    VklArray* arr_index = &src_index->arr;

    // Number of points and polygons.
    uint32_t n_points = arr_pos->item_count;
    uint32_t n_polys = arr_length->item_count;

    ASSERT(n_points > 0);
    ASSERT(n_polys > 0);

    dvec3* points = (dvec3*)arr_pos->data;
    uint32_t* poly_lengths = (uint32_t*)arr_length->data;

    // Triangulate the polygons.
    uint32_t index_count = 0;
    uint32_t total_index_count = 0;
    uint32_t* indices = NULL;
    uint32_t* index_count_list = (uint32_t*)calloc(n_polys, sizeof(uint32_t));
    uint32_t** indices_list = (uint32_t**)calloc(n_polys, sizeof(uint32_t*));
    uint32_t offset = 0;

    // Triangulate all polygons.
    for (uint32_t i = 0; i < n_polys; i++)
    {
        vkl_triangulate_polygon(
            poly_lengths[i], (const dvec3*)&points[offset], &index_count, &indices);
        ASSERT(indices != NULL);
        ASSERT(index_count > 0);
        total_index_count += index_count;
        // Save each triangulation's indices in order to concatenate them afterwards.
        index_count_list[i] = index_count;
        indices_list[i] = indices;
        offset += poly_lengths[i];
    }

    // Concatenate all triangulations.
    uint32_t* total_indices = (uint32_t*)calloc(total_index_count, sizeof(uint32_t));
    offset = 0;
    uint32_t voffset = 0;
    for (uint32_t i = 0; i < n_polys; i++)
    {
        for (uint32_t j = 0; j < index_count_list[i]; j++)
        {
            total_indices[offset + j] = voffset + indices_list[i][j];
        }
        offset += index_count_list[i];
        voffset += poly_lengths[i];
        FREE(indices_list[i]);
    }

    // Reesize and fill the vertex buffer.
    vkl_array_resize(arr_vertex, n_points);
    // Copy the positions from the pos prop to the vertex buffer.
    _prop_copy(visual, prop_pos);

    // Resize and fill the index buffer.
    vkl_array_resize(arr_index, total_index_count);
    vkl_array_data(arr_index, 0, total_index_count, total_index_count, total_indices);

    // Copy the polygon colors to the vertices.
    cvec4* color = NULL;
    // Go through the polygons.
    uint32_t k = 0;
    for (uint32_t i = 0; i < n_polys; i++)
    {
        // Color prop for the current polygon.
        color = (cvec4*)vkl_array_item(arr_color, i);
        // Copy the color to the vertex buffer, repeating it for each vertex in the polygon.
        vkl_array_column(
            arr_vertex, offsetof(VklVertex, color), sizeof(cvec4), k, poly_lengths[i], 1, color,
            VKL_DTYPE_NONE, VKL_DTYPE_NONE, VKL_ARRAY_COPY_SINGLE, 1);
        k += poly_lengths[i];
    }

    FREE(index_count_list);
    FREE(indices_list);
    FREE(total_indices);
}

static void _visual_polygon(VklVisual* visual)
{
    ASSERT(visual != NULL);
    VklCanvas* canvas = visual->canvas;
    ASSERT(canvas != NULL);
    VklProp* prop = NULL;

    // Graphics.
    vkl_visual_graphics(visual, vkl_graphics_builtin(canvas, VKL_GRAPHICS_TRIANGLE, 0));

    // Sources
    vkl_visual_source(
        visual, VKL_SOURCE_TYPE_VERTEX, 0, VKL_PIPELINE_GRAPHICS, 0, 0, sizeof(VklVertex), 0);

    vkl_visual_source(
        visual, VKL_SOURCE_TYPE_INDEX, 0, VKL_PIPELINE_GRAPHICS, 0, 0, sizeof(VklIndex), 0);

    _common_sources(visual);

    // Props:

    // Polygon points, 1 position per point.
    prop = vkl_visual_prop(visual, VKL_PROP_POS, 0, VKL_DTYPE_DVEC3, VKL_SOURCE_TYPE_VERTEX, 0);
    // Copy the polygon points directly to the vertex buffer, as the triangulation only sets the
    // vertex indices and does not change the vertices themselves.
    vkl_visual_prop_cast(
        prop, 0, offsetof(VklVertex, pos), VKL_DTYPE_VEC3, VKL_ARRAY_COPY_SINGLE, 1);

    // Polygon lengths, 1 length per polygon.
    prop = vkl_visual_prop(visual, VKL_PROP_LENGTH, 0, VKL_DTYPE_UINT, VKL_SOURCE_TYPE_VERTEX, 0);

    // Polygon colors, 1 color per polygon.
    prop = vkl_visual_prop(visual, VKL_PROP_COLOR, 0, VKL_DTYPE_CVEC4, VKL_SOURCE_TYPE_VERTEX, 0);

    // Common props.
    _common_props(visual);

    vkl_visual_callback_bake(visual, _polygon_bake);
}



/*************************************************************************************************/
/*************************************************************************************************/
/*  3D visuals                                                                                   */
/*************************************************************************************************/
/*************************************************************************************************/



/*************************************************************************************************/
/*  Mesh                                                                                         */
/*************************************************************************************************/

static void _mesh_bake(VklVisual* visual, VklVisualDataEvent ev)
{
    // Take the color prop and override the tex coords and alpha if needed.
    VklProp* prop_color = vkl_prop_get(visual, VKL_PROP_COLOR, 0);         // cvec4
    VklProp* prop_texcoords = vkl_prop_get(visual, VKL_PROP_TEXCOORDS, 0); // vec2
    VklProp* prop_alpha = vkl_prop_get(visual, VKL_PROP_ALPHA, 0);         // uint8_t

    VklArray* arr_color = _prop_array(prop_color);
    VklArray* arr_texcoords = _prop_array(prop_texcoords);
    VklArray* arr_alpha = _prop_array(prop_alpha);

    // If colors have been specified, we need to override texcoords.
    uint32_t N = arr_color->item_count;
    if (N > 0)
    {
        vkl_array_resize(arr_texcoords, N);
        vkl_array_resize(arr_alpha, N);

        ASSERT(arr_texcoords->item_count == N);
        ASSERT(arr_alpha->item_count == N);

        cvec4* color = NULL;
        cvec3* rgb = NULL;
        vec2* uv = NULL;
        uint8_t* alpha = NULL;

        for (uint32_t i = 0; i < N; i++)
        {
            color = vkl_array_item(arr_color, i);
            uv = vkl_array_item(arr_texcoords, i);
            alpha = vkl_array_item(arr_alpha, i);

            // Copy the color RGB components to the UV tex coords by packing 3 char into a float
            rgb = (cvec3*)color;
            vkl_colormap_packuv(*rgb, *uv);

            // Copy the alpha component to the ALPHA prop
            *alpha = (*color)[3];
        }
    }

    _default_visual_bake(visual, ev);
}

static void _visual_mesh(VklVisual* visual)
{
    ASSERT(visual != NULL);
    VklCanvas* canvas = visual->canvas;
    ASSERT(canvas != NULL);
    VklProp* prop = NULL;

    // Graphics.
    vkl_visual_graphics(visual, vkl_graphics_builtin(canvas, VKL_GRAPHICS_MESH, 0));

    // Sources
    vkl_visual_source(                                               // vertex buffer
        visual, VKL_SOURCE_TYPE_VERTEX, 0, VKL_PIPELINE_GRAPHICS, 0, //
        0, sizeof(VklGraphicsMeshVertex), 0);                        //

    vkl_visual_source(                                              // index buffer
        visual, VKL_SOURCE_TYPE_INDEX, 0, VKL_PIPELINE_GRAPHICS, 0, //
        0, sizeof(VklIndex), 0);                                    //

    _common_sources(visual); // common sources

    vkl_visual_source(                                              // params
        visual, VKL_SOURCE_TYPE_PARAM, 0, VKL_PIPELINE_GRAPHICS, 0, //
        VKL_USER_BINDING, sizeof(VklGraphicsMeshParams), 0);        //

    for (uint32_t i = 0; i < 4; i++)                                    // texture sources
        vkl_visual_source(                                              //
            visual, VKL_SOURCE_TYPE_IMAGE, i, VKL_PIPELINE_GRAPHICS, 0, //
            VKL_USER_BINDING + i + 1, sizeof(cvec4), 0);                //

    // Props:

    // Vertex pos.
    prop = vkl_visual_prop(visual, VKL_PROP_POS, 0, VKL_DTYPE_DVEC3, VKL_SOURCE_TYPE_VERTEX, 0);
    vkl_visual_prop_cast(
        prop, 0, offsetof(VklGraphicsMeshVertex, pos), VKL_DTYPE_VEC3, VKL_ARRAY_COPY_SINGLE, 1);

    // Vertex normal.
    prop = vkl_visual_prop(visual, VKL_PROP_NORMAL, 0, VKL_DTYPE_VEC3, VKL_SOURCE_TYPE_VERTEX, 0);
    vkl_visual_prop_copy(
        prop, 0, offsetof(VklGraphicsMeshVertex, normal), VKL_ARRAY_COPY_SINGLE, 1);

    // Vertex tex coords.
    prop =
        vkl_visual_prop(visual, VKL_PROP_TEXCOORDS, 0, VKL_DTYPE_VEC2, VKL_SOURCE_TYPE_VERTEX, 0);
    vkl_visual_prop_copy(prop, 0, offsetof(VklGraphicsMeshVertex, uv), VKL_ARRAY_COPY_SINGLE, 1);

    // Vertex color: override tex coords by packing 3 bytes into a float
    prop = vkl_visual_prop(visual, VKL_PROP_COLOR, 0, VKL_DTYPE_CVEC4, VKL_SOURCE_TYPE_VERTEX, 0);

    // Vertex alpha.
    prop = vkl_visual_prop(visual, VKL_PROP_ALPHA, 0, VKL_DTYPE_CHAR, VKL_SOURCE_TYPE_VERTEX, 0);
    vkl_visual_prop_copy(
        prop, 0, offsetof(VklGraphicsMeshVertex, alpha), VKL_ARRAY_COPY_SINGLE, 1);
    uint8_t alpha = 255;
    vkl_visual_prop_default(prop, &alpha);

    // Index.
    prop = vkl_visual_prop(visual, VKL_PROP_INDEX, 0, VKL_DTYPE_UINT, VKL_SOURCE_TYPE_INDEX, 0);
    vkl_visual_prop_copy(prop, 0, 0, VKL_ARRAY_COPY_SINGLE, 1);

    // Common props.
    _common_props(visual);

    // Params.
    // Default values.
    VklGraphicsMeshParams params = default_graphics_mesh_params(VKL_CAMERA_EYE);

    // Light positions.
    prop =
        vkl_visual_prop(visual, VKL_PROP_LIGHT_POS, 0, VKL_DTYPE_MAT4, VKL_SOURCE_TYPE_PARAM, 0);
    vkl_visual_prop_copy(
        prop, 0, offsetof(VklGraphicsMeshParams, lights_pos_0), VKL_ARRAY_COPY_SINGLE, 1);
    vkl_visual_prop_default(prop, &params.lights_pos_0);

    // Light params.
    prop = vkl_visual_prop(
        visual, VKL_PROP_LIGHT_PARAMS, 0, VKL_DTYPE_MAT4, VKL_SOURCE_TYPE_PARAM, 0);
    vkl_visual_prop_copy(
        prop, 1, offsetof(VklGraphicsMeshParams, lights_params_0), VKL_ARRAY_COPY_SINGLE, 1);
    vkl_visual_prop_default(prop, &params.lights_params_0);

    // View pos.
    prop = vkl_visual_prop(visual, VKL_PROP_VIEW_POS, 0, VKL_DTYPE_VEC4, VKL_SOURCE_TYPE_PARAM, 0);
    vkl_visual_prop_copy(
        prop, 2, offsetof(VklGraphicsMeshParams, view_pos), VKL_ARRAY_COPY_SINGLE, 1);
    vkl_visual_prop_default(prop, &params.view_pos);

    // Texture coefficients.
    prop = vkl_visual_prop(visual, VKL_PROP_TEXCOEFS, 0, VKL_DTYPE_VEC4, VKL_SOURCE_TYPE_PARAM, 0);
    vkl_visual_prop_copy(
        prop, 3, offsetof(VklGraphicsMeshParams, tex_coefs), VKL_ARRAY_COPY_SINGLE, 1);
    vkl_visual_prop_default(prop, &params.tex_coefs);

    // Clipping coefficients.
    prop = vkl_visual_prop(visual, VKL_PROP_CLIP, 0, VKL_DTYPE_VEC4, VKL_SOURCE_TYPE_PARAM, 0);
    vkl_visual_prop_copy(
        prop, 4, offsetof(VklGraphicsMeshParams, clip_coefs), VKL_ARRAY_COPY_SINGLE, 1);

    // Texture props.
    for (uint32_t i = 0; i < 4; i++)
        vkl_visual_prop(visual, VKL_PROP_IMAGE, i, VKL_DTYPE_UINT, VKL_SOURCE_TYPE_IMAGE, i);

    vkl_visual_callback_bake(visual, _mesh_bake);
}



/*************************************************************************************************/
/*  Volume image                                                                                 */
/*************************************************************************************************/

static void _visual_volume_slice_bake(VklVisual* visual, VklVisualDataEvent ev)
{
    ASSERT(visual != NULL);

    // Vertex buffer source.
    VklSource* source = vkl_source_get(visual, VKL_SOURCE_TYPE_VERTEX, 0);
    ASSERT(source->arr.item_size == sizeof(VklGraphicsVolumeSliceVertex));

    // Get props.
    VklProp* pos0 = vkl_prop_get(visual, VKL_PROP_POS, 0);
    VklProp* pos1 = vkl_prop_get(visual, VKL_PROP_POS, 1);
    VklProp* pos2 = vkl_prop_get(visual, VKL_PROP_POS, 2);
    VklProp* pos3 = vkl_prop_get(visual, VKL_PROP_POS, 3);

    VklProp* uvw0 = vkl_prop_get(visual, VKL_PROP_TEXCOORDS, 0);
    VklProp* uvw1 = vkl_prop_get(visual, VKL_PROP_TEXCOORDS, 1);
    VklProp* uvw2 = vkl_prop_get(visual, VKL_PROP_TEXCOORDS, 2);
    VklProp* uvw3 = vkl_prop_get(visual, VKL_PROP_TEXCOORDS, 3);

    ASSERT(pos0 != NULL);
    ASSERT(pos1 != NULL);
    ASSERT(pos2 != NULL);
    ASSERT(pos3 != NULL);

    ASSERT(uvw0 != NULL);
    ASSERT(uvw1 != NULL);
    ASSERT(uvw2 != NULL);
    ASSERT(uvw3 != NULL);

    // Number of images
    uint32_t img_count = vkl_prop_size(pos0);
    ASSERT(vkl_prop_size(pos1) == img_count);
    ASSERT(vkl_prop_size(pos2) == img_count);
    ASSERT(vkl_prop_size(pos3) == img_count);

    // Graphics data.
    VklGraphicsData data = vkl_graphics_data(visual->graphics[0], &source->arr, NULL, NULL);
    vkl_graphics_alloc(&data, img_count);

    VklGraphicsVolumeSliceItem item = {0};
    for (uint32_t i = 0; i < img_count; i++)
    {
        _vec3_cast((const dvec3*)vkl_prop_item(pos0, i), &item.pos0);
        _vec3_cast((const dvec3*)vkl_prop_item(pos1, i), &item.pos1);
        _vec3_cast((const dvec3*)vkl_prop_item(pos2, i), &item.pos2);
        _vec3_cast((const dvec3*)vkl_prop_item(pos3, i), &item.pos3);

        // memcpy(&item.pos0, vkl_prop_item(pos0, i), sizeof(vec3));
        // memcpy(&item.pos1, vkl_prop_item(pos1, i), sizeof(vec3));
        // memcpy(&item.pos2, vkl_prop_item(pos2, i), sizeof(vec3));
        // memcpy(&item.pos3, vkl_prop_item(pos3, i), sizeof(vec3));

        memcpy(&item.uvw0, vkl_prop_item(uvw0, i), sizeof(vec3));
        memcpy(&item.uvw1, vkl_prop_item(uvw1, i), sizeof(vec3));
        memcpy(&item.uvw2, vkl_prop_item(uvw2, i), sizeof(vec3));
        memcpy(&item.uvw3, vkl_prop_item(uvw3, i), sizeof(vec3));

        vkl_graphics_append(&data, &item);
    }
}

static void _visual_volume_slice(VklVisual* visual)
{
    ASSERT(visual != NULL);
    VklCanvas* canvas = visual->canvas;
    ASSERT(canvas != NULL);
    VklProp* prop = NULL;

    // TODO: customizable dtype for the image

    // Graphics.
    vkl_visual_graphics(visual, vkl_graphics_builtin(canvas, VKL_GRAPHICS_VOLUME_SLICE, 0));

    // Sources
    vkl_visual_source(                                               // vertex buffer
        visual, VKL_SOURCE_TYPE_VERTEX, 0, VKL_PIPELINE_GRAPHICS, 0, //
        0, sizeof(VklGraphicsVolumeSliceVertex), 0);                 //

    _common_sources(visual); // common sources

    vkl_visual_source(                                              // params
        visual, VKL_SOURCE_TYPE_PARAM, 0, VKL_PIPELINE_GRAPHICS, 0, //
        VKL_USER_BINDING, sizeof(VklGraphicsVolumeSliceParams), 0); //

    vkl_visual_source(                                                      // colormap texture
        visual, VKL_SOURCE_TYPE_COLOR_TEXTURE, 0, VKL_PIPELINE_GRAPHICS, 0, //
        VKL_USER_BINDING + 1, sizeof(uint8_t), 0);                          //

    vkl_visual_source(                                               // texture source
        visual, VKL_SOURCE_TYPE_VOLUME, 0, VKL_PIPELINE_GRAPHICS, 0, //
        VKL_USER_BINDING + 2, sizeof(uint8_t), 0);                   //

    // Props:

    // Point positions.
    // Top left, top right, bottom right, bottom left
    for (uint32_t i = 0; i < 4; i++)
        vkl_visual_prop(visual, VKL_PROP_POS, i, VKL_DTYPE_DVEC3, VKL_SOURCE_TYPE_VERTEX, 0);

    // Tex coords.
    for (uint32_t i = 0; i < 4; i++)
        vkl_visual_prop(visual, VKL_PROP_TEXCOORDS, i, VKL_DTYPE_VEC3, VKL_SOURCE_TYPE_VERTEX, 0);

    // Common props.
    _common_props(visual);

    // Params.

    // Colormap transfer function.
    vec4 vec = {0, .333333, .666666, 1};
    prop = vkl_visual_prop(                                                                 //
        visual, VKL_PROP_TRANSFER_X, 0, VKL_DTYPE_VEC4, VKL_SOURCE_TYPE_PARAM, 0);          //
    vkl_visual_prop_copy(                                                                   //
        prop, 0, offsetof(VklGraphicsVolumeSliceParams, x_cmap), VKL_ARRAY_COPY_SINGLE, 1); //
    vkl_visual_prop_default(prop, vec);                                                     //

    prop = vkl_visual_prop(                                                                 //
        visual, VKL_PROP_TRANSFER_Y, 0, VKL_DTYPE_VEC4, VKL_SOURCE_TYPE_PARAM, 0);          //
    vkl_visual_prop_copy(                                                                   //
        prop, 1, offsetof(VklGraphicsVolumeSliceParams, y_cmap), VKL_ARRAY_COPY_SINGLE, 1); //
    vkl_visual_prop_default(prop, vec);                                                     //

    // Alpha transfer function.
    prop = vkl_visual_prop(                                                                  //
        visual, VKL_PROP_TRANSFER_X, 1, VKL_DTYPE_VEC4, VKL_SOURCE_TYPE_PARAM, 0);           //
    vkl_visual_prop_copy(                                                                    //
        prop, 2, offsetof(VklGraphicsVolumeSliceParams, x_alpha), VKL_ARRAY_COPY_SINGLE, 1); //
    vkl_visual_prop_default(prop, vec);                                                      //

    prop = vkl_visual_prop(                                                                  //
        visual, VKL_PROP_TRANSFER_Y, 1, VKL_DTYPE_VEC4, VKL_SOURCE_TYPE_PARAM, 0);           //
    vkl_visual_prop_copy(                                                                    //
        prop, 3, offsetof(VklGraphicsVolumeSliceParams, y_alpha), VKL_ARRAY_COPY_SINGLE, 1); //
    vkl_visual_prop_default(prop, vec);                                                      //

    // Colormap value.
    prop = vkl_visual_prop(visual, VKL_PROP_COLORMAP, 0, VKL_DTYPE_INT, VKL_SOURCE_TYPE_PARAM, 0);
    vkl_visual_prop_copy(
        prop, 4, offsetof(VklGraphicsVolumeSliceParams, cmap), VKL_ARRAY_COPY_SINGLE, 1);

    // Scaling factor.
    prop = vkl_visual_prop(visual, VKL_PROP_SCALE, 0, VKL_DTYPE_FLOAT, VKL_SOURCE_TYPE_PARAM, 0);
    vkl_visual_prop_copy(
        prop, 5, offsetof(VklGraphicsVolumeSliceParams, scale), VKL_ARRAY_COPY_SINGLE, 1);



    // Colormap texture prop.
    vkl_visual_prop(
        visual, VKL_PROP_COLOR_TEXTURE, 0, VKL_DTYPE_CHAR, VKL_SOURCE_TYPE_COLOR_TEXTURE, 0);

    // 3D texture prop.
    vkl_visual_prop(visual, VKL_PROP_VOLUME, 0, VKL_DTYPE_CHAR, VKL_SOURCE_TYPE_VOLUME, 0);

    // Baking function.
    vkl_visual_callback_bake(visual, _visual_volume_slice_bake);
    // vkl_visual_callback_transform(visual, _visual_normalize);
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

    case VKL_VISUAL_POINT:
        _visual_point(visual);
        break;

    case VKL_VISUAL_LINE:
        _visual_line(visual);
        break;

    case VKL_VISUAL_LINE_STRIP:
        _visual_line_strip(visual);
        break;

    case VKL_VISUAL_TRIANGLE:
        _visual_triangle(visual);
        break;

    case VKL_VISUAL_TRIANGLE_STRIP:
        _visual_triangle_strip(visual);
        break;

    case VKL_VISUAL_TRIANGLE_FAN:
        _visual_triangle_fan(visual);
        break;



    case VKL_VISUAL_MARKER:
        _visual_marker(visual);
        break;

    case VKL_VISUAL_POLYGON:
        _visual_polygon(visual);
        break;

    case VKL_VISUAL_AXES_2D:
        _visual_axes_2D(visual);
        break;



    case VKL_VISUAL_MESH:
        _visual_mesh(visual);
        break;

    case VKL_VISUAL_VOLUME_SLICE:
        _visual_volume_slice(visual);
        break;


    case VKL_VISUAL_CUSTOM:
    case VKL_VISUAL_NONE:
        break;

    default:
        log_error("no builtin visual found for type %d", type);
        break;
    }
}
