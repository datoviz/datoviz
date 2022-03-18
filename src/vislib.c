#include "../include/datoviz/vislib.h"
#include "../include/datoviz/array.h"
#include "../include/datoviz/graphics.h"
#include "../include/datoviz/interact.h"
#include "../include/datoviz/mesh.h"
#include "axes.h"
#include "visuals_utils.h"



/*************************************************************************************************/
/*************************************************************************************************/
/*  Basic visuals                                                                                */
/*************************************************************************************************/
/*************************************************************************************************/

/*************************************************************************************************/
/*  Point                                                                                        */
/*************************************************************************************************/

static void _visual_point(DvzVisual* visual)
{
    ASSERT(visual != NULL);
    DvzCanvas* canvas = visual->canvas;
    ASSERT(canvas != NULL);
    DvzProp* prop = NULL;

    // Graphics.
    dvz_visual_graphics(visual, dvz_graphics_builtin(canvas, DVZ_GRAPHICS_POINT, visual->flags));

    // Sources
    dvz_visual_source(
        visual, DVZ_SOURCE_TYPE_VERTEX, 0, DVZ_PIPELINE_GRAPHICS, 0, 0, sizeof(DvzVertex), 0);
    _common_sources(visual);
    dvz_visual_source(
        visual, DVZ_SOURCE_TYPE_PARAM, 0, DVZ_PIPELINE_GRAPHICS, 0, DVZ_USER_BINDING,
        sizeof(DvzGraphicsPointParams), 0);

    // Props:

    // Vertex pos.
    prop = dvz_visual_prop(visual, DVZ_PROP_POS, 0, DVZ_DTYPE_DVEC3, DVZ_SOURCE_TYPE_VERTEX, 0);
    dvz_visual_prop_cast(
        prop, 0, offsetof(DvzVertex, pos), DVZ_DTYPE_VEC3, DVZ_ARRAY_COPY_SINGLE, 1);

    // Vertex color.
    prop = dvz_visual_prop(visual, DVZ_PROP_COLOR, 0, DVZ_DTYPE_CVEC4, DVZ_SOURCE_TYPE_VERTEX, 0);
    dvz_visual_prop_copy(prop, 1, offsetof(DvzVertex, color), DVZ_ARRAY_COPY_SINGLE, 1);
    cvec4 color = {200, 200, 200, 255};
    dvz_visual_prop_default(prop, &color);

    // Common props.
    _common_props(visual);

    // Param: marker size.
    prop = dvz_visual_prop(
        visual, DVZ_PROP_MARKER_SIZE, 0, DVZ_DTYPE_FLOAT, DVZ_SOURCE_TYPE_PARAM, 0);
    dvz_visual_prop_copy(
        prop, 0, offsetof(DvzGraphicsPointParams, point_size), DVZ_ARRAY_COPY_SINGLE, 1);
    dvz_visual_prop_dpi(prop, canvas->dpi_scaling);
    float size = 5;
    dvz_visual_prop_default(prop, &size);
}



/*************************************************************************************************/
/*  Line                                                                                         */
/*************************************************************************************************/

static void _visual_line(DvzVisual* visual)
{
    ASSERT(visual != NULL);
    DvzCanvas* canvas = visual->canvas;
    ASSERT(canvas != NULL);
    DvzProp* prop = NULL;

    // Graphics.
    dvz_visual_graphics(visual, dvz_graphics_builtin(canvas, DVZ_GRAPHICS_LINE, visual->flags));

    // Sources
    dvz_visual_source(
        visual, DVZ_SOURCE_TYPE_VERTEX, 0, DVZ_PIPELINE_GRAPHICS, 0, 0, sizeof(DvzVertex), 0);
    _common_sources(visual);

    // Props:

    // Vertex pos, segment start.
    prop = dvz_visual_prop(visual, DVZ_PROP_POS, 0, DVZ_DTYPE_DVEC3, DVZ_SOURCE_TYPE_VERTEX, 0);
    dvz_visual_prop_cast(
        prop, 0, offsetof(DvzVertex, pos), DVZ_DTYPE_VEC3, DVZ_ARRAY_COPY_SINGLE, 2);

    // Vertex pos, segment end.
    prop = dvz_visual_prop(visual, DVZ_PROP_POS, 1, DVZ_DTYPE_DVEC3, DVZ_SOURCE_TYPE_VERTEX, 0);
    dvz_visual_prop_cast(
        prop, 0, sizeof(DvzVertex) + offsetof(DvzVertex, pos), DVZ_DTYPE_VEC3,
        DVZ_ARRAY_COPY_SINGLE, 2);


    // Vertex color.
    prop = dvz_visual_prop(visual, DVZ_PROP_COLOR, 0, DVZ_DTYPE_CVEC4, DVZ_SOURCE_TYPE_VERTEX, 0);
    dvz_visual_prop_copy(prop, 1, offsetof(DvzVertex, color), DVZ_ARRAY_COPY_REPEAT, 2);

    // Common props.
    _common_props(visual);
}



/*************************************************************************************************/
/*  Line strip                                                                                   */
/*************************************************************************************************/

static void _line_strip_bake(DvzVisual* visual, DvzVisualDataEvent ev)
{
    ASSERT(visual != NULL);

    DvzProp* prop_pos = dvz_prop_get(visual, DVZ_PROP_POS, 0);
    DvzProp* prop_color = dvz_prop_get(visual, DVZ_PROP_COLOR, 0);

    // Input arrays.
    DvzArray* arr_pos = &prop_pos->arr_trans;
    if (arr_pos->item_size == 0)
        arr_pos = &prop_pos->arr_orig;
    DvzArray* arr_color = &prop_color->arr_trans;
    if (arr_color->item_size == 0)
        arr_color = &prop_color->arr_orig;

    // Length prop.
    DvzArray* arr_length = dvz_prop_array(visual, DVZ_PROP_LENGTH, 0); // uint

    // Number of vertices.
    uint32_t n_vertices = arr_pos->item_count;
    ASSERT(n_vertices > 0);

    // Number of line strips.
    uint32_t n_strips = arr_length->item_count;

    if (n_strips >= 2)
    {
        // New number of vertices, with the extra points.
        uint32_t n_vertices_new = n_vertices + 2 * (n_strips - 1);
        ASSERT(n_vertices_new > 0);

        // Create the staging arrays if needed.
        if (prop_pos->arr_staging.item_size == 0)
            prop_pos->arr_staging = dvz_array(arr_pos->item_count, arr_pos->dtype);
        if (prop_color->arr_staging.item_size == 0)
            prop_color->arr_staging = dvz_array(arr_color->item_count, arr_color->dtype);

        DvzArray* arr_pos_out = &prop_pos->arr_staging;
        DvzArray* arr_color_out = &prop_color->arr_staging;

        // Resize the pos and color transformed arrays.
        dvz_array_resize(arr_pos_out, n_vertices_new);
        dvz_array_resize(arr_color_out, n_vertices_new);

        // Lengths.
        uint32_t* lengths = (uint32_t*)arr_length->data; // length of each line strip

        // Data to insert.
        uint32_t src_offset = 0;
        uint32_t dst_offset = 0;
        uint32_t count = 0;
        dvec3* pos = NULL;
        // color of the invisible line joining two successive line strips:
        cvec4 color = {0, 0, 0, 0};

        for (uint32_t i = 0; i < n_strips; i++)
        {
            count = lengths[i];

            // Copy the position and color values of the current line strip.
            dvz_array_copy_region(arr_pos, arr_pos_out, src_offset, dst_offset, count);
            dvz_array_copy_region(arr_color, arr_color_out, src_offset, dst_offset, count);

            src_offset += count;
            dst_offset += count;

            // Add the extra elements.
            // Last position of the current line strip.
            pos = dvz_array_item(arr_pos, src_offset - 1);
            dvz_array_data(arr_pos_out, dst_offset, 1, 1, pos);
            dvz_array_data(arr_color_out, dst_offset, 1, 1, color);

            // First position of the next line strip.
            if (i < n_strips - 1)
            {
                pos = dvz_array_item(arr_pos, src_offset);
                dvz_array_data(arr_pos_out, dst_offset + 1, 1, 1, pos);
                dvz_array_data(arr_color_out, dst_offset + 1, 1, 1, color);
                dst_offset += 2;
            }
        }
        ASSERT(src_offset == n_vertices);
        ASSERT(dst_offset == n_vertices_new);
    }

    _default_visual_bake(visual, ev);
}

static void _visual_line_strip(DvzVisual* visual)
{
    ASSERT(visual != NULL);
    DvzCanvas* canvas = visual->canvas;
    ASSERT(canvas != NULL);
    DvzProp* prop = NULL;

    // Graphics.
    dvz_visual_graphics(
        visual, dvz_graphics_builtin(canvas, DVZ_GRAPHICS_LINE_STRIP, visual->flags));

    // Sources
    dvz_visual_source(
        visual, DVZ_SOURCE_TYPE_VERTEX, 0, DVZ_PIPELINE_GRAPHICS, 0, 0, sizeof(DvzVertex), 0);
    _common_sources(visual);

    // Props:

    // Vertex pos.
    prop = dvz_visual_prop(visual, DVZ_PROP_POS, 0, DVZ_DTYPE_DVEC3, DVZ_SOURCE_TYPE_VERTEX, 0);
    dvz_visual_prop_cast(
        prop, 0, offsetof(DvzVertex, pos), DVZ_DTYPE_VEC3, DVZ_ARRAY_COPY_SINGLE, 1);

    // Vertex color.
    prop = dvz_visual_prop(visual, DVZ_PROP_COLOR, 0, DVZ_DTYPE_CVEC4, DVZ_SOURCE_TYPE_VERTEX, 0);
    dvz_visual_prop_copy(prop, 1, offsetof(DvzVertex, color), DVZ_ARRAY_COPY_SINGLE, 1);

    // Line strip length.
    prop = dvz_visual_prop(visual, DVZ_PROP_LENGTH, 0, DVZ_DTYPE_UINT, DVZ_SOURCE_TYPE_NONE, 0);

    // Common props.
    _common_props(visual);

    // Baking function.
    dvz_visual_callback_bake(visual, _line_strip_bake);
}



/*************************************************************************************************/
/*  Triangle                                                                                     */
/*************************************************************************************************/

static void _visual_triangle(DvzVisual* visual)
{
    ASSERT(visual != NULL);
    DvzCanvas* canvas = visual->canvas;
    ASSERT(canvas != NULL);
    DvzProp* prop = NULL;

    // Graphics.
    dvz_visual_graphics(
        visual, dvz_graphics_builtin(canvas, DVZ_GRAPHICS_TRIANGLE, visual->flags));

    // Sources
    dvz_visual_source(
        visual, DVZ_SOURCE_TYPE_VERTEX, 0, DVZ_PIPELINE_GRAPHICS, 0, 0, sizeof(DvzVertex), 0);
    _common_sources(visual);

    // Props:

    // Vertex pos, triangle point.
    for (uint32_t i = 0; i < 3; i++)
    {
        prop =
            dvz_visual_prop(visual, DVZ_PROP_POS, i, DVZ_DTYPE_DVEC3, DVZ_SOURCE_TYPE_VERTEX, 0);
        dvz_visual_prop_cast(
            prop, 0, i * sizeof(DvzVertex) + offsetof(DvzVertex, pos), //
            DVZ_DTYPE_VEC3, DVZ_ARRAY_COPY_SINGLE, 3);
    }

    // Vertex color.
    prop = dvz_visual_prop(visual, DVZ_PROP_COLOR, 0, DVZ_DTYPE_CVEC4, DVZ_SOURCE_TYPE_VERTEX, 0);
    dvz_visual_prop_copy(prop, 1, offsetof(DvzVertex, color), DVZ_ARRAY_COPY_REPEAT, 3);

    // Common props.
    _common_props(visual);
}



/*************************************************************************************************/
/*  Triangle strip                                                                               */
/*************************************************************************************************/

static void _visual_triangle_strip(DvzVisual* visual)
{
    ASSERT(visual != NULL);
    DvzCanvas* canvas = visual->canvas;
    ASSERT(canvas != NULL);
    DvzProp* prop = NULL;

    // Graphics.
    dvz_visual_graphics(
        visual, dvz_graphics_builtin(canvas, DVZ_GRAPHICS_TRIANGLE_STRIP, visual->flags));

    // Sources
    dvz_visual_source(
        visual, DVZ_SOURCE_TYPE_VERTEX, 0, DVZ_PIPELINE_GRAPHICS, 0, 0, sizeof(DvzVertex), 0);
    _common_sources(visual);

    // Props:

    // Vertex pos.
    prop = dvz_visual_prop(visual, DVZ_PROP_POS, 0, DVZ_DTYPE_DVEC3, DVZ_SOURCE_TYPE_VERTEX, 0);
    dvz_visual_prop_cast(
        prop, 0, offsetof(DvzVertex, pos), DVZ_DTYPE_VEC3, DVZ_ARRAY_COPY_SINGLE, 1);

    // Vertex color.
    prop = dvz_visual_prop(visual, DVZ_PROP_COLOR, 0, DVZ_DTYPE_CVEC4, DVZ_SOURCE_TYPE_VERTEX, 0);
    dvz_visual_prop_copy(prop, 1, offsetof(DvzVertex, color), DVZ_ARRAY_COPY_SINGLE, 1);

    // Common props.
    _common_props(visual);
}



/*************************************************************************************************/
/*  Triangle fan                                                                                 */
/*************************************************************************************************/

static void _visual_triangle_fan(DvzVisual* visual)
{
    ASSERT(visual != NULL);
    DvzCanvas* canvas = visual->canvas;
    ASSERT(canvas != NULL);
    DvzProp* prop = NULL;

    // Graphics.
    dvz_visual_graphics(
        visual, dvz_graphics_builtin(canvas, DVZ_GRAPHICS_TRIANGLE_FAN, visual->flags));

    // Sources
    dvz_visual_source(
        visual, DVZ_SOURCE_TYPE_VERTEX, 0, DVZ_PIPELINE_GRAPHICS, 0, 0, sizeof(DvzVertex), 0);
    _common_sources(visual);

    // Props:

    // Vertex pos.
    prop = dvz_visual_prop(visual, DVZ_PROP_POS, 0, DVZ_DTYPE_DVEC3, DVZ_SOURCE_TYPE_VERTEX, 0);
    dvz_visual_prop_cast(
        prop, 0, offsetof(DvzVertex, pos), DVZ_DTYPE_VEC3, DVZ_ARRAY_COPY_SINGLE, 1);

    // Vertex color.
    prop = dvz_visual_prop(visual, DVZ_PROP_COLOR, 0, DVZ_DTYPE_CVEC4, DVZ_SOURCE_TYPE_VERTEX, 0);
    dvz_visual_prop_copy(prop, 1, offsetof(DvzVertex, color), DVZ_ARRAY_COPY_SINGLE, 1);

    // Common props.
    _common_props(visual);
}



/*************************************************************************************************/
/*  Rectangle                                                                                    */
/*************************************************************************************************/

static void _rectangle_bake(DvzVisual* visual, DvzVisualDataEvent ev)
{
    ASSERT(visual != NULL);

    // First, we obtain the array instances holding the prop data as specified by the user.
    DvzArray* arr_p0 = dvz_prop_array(visual, DVZ_PROP_POS, 0);
    DvzArray* arr_p1 = dvz_prop_array(visual, DVZ_PROP_POS, 1);
    DvzArray* arr_color = dvz_prop_array(visual, DVZ_PROP_COLOR, 0);

    // We also get the array of the vertex buffer, which we'll need to fill with the triangulation.
    DvzArray* arr_vertex = dvz_source_array(visual, DVZ_SOURCE_TYPE_VERTEX, 0);

    // The number of rows in the 1D position array (set by the user) is the number of rectangles
    // requested by the user.
    uint32_t rectangle_count = arr_p0->item_count;

    // We resize the vertex buffer array so that it holds six vertices per rectangle (two
    // triangles).
    dvz_array_resize(arr_vertex, 6 * rectangle_count);

    // Pointers to the input data.
    dvec3* p0 = NULL;
    dvec3* p1 = NULL;
    cvec4* color = NULL;

    // Pointer to the output vertex.
    DvzVertex* vertex = (DvzVertex*)arr_vertex->data;

    // Here, we triangulate each rectangle by computing the position of each rectangle corner.
    for (uint32_t i = 0; i < rectangle_count; i++)
    {
        // We get a pointer to the current item in each prop array.
        p0 = dvz_array_item(arr_p0, i);
        p1 = dvz_array_item(arr_p1, i);
        color = dvz_array_item(arr_color, i);

        // First triangle:

        // Bottom-left corner.
        vertex[6 * i + 0].pos[0] = p0[0][0];
        vertex[6 * i + 0].pos[1] = p0[0][1];

        // Bottom-right corner.
        vertex[6 * i + 1].pos[0] = p1[0][0];
        vertex[6 * i + 1].pos[1] = p0[0][1];

        // Top-right corner.
        vertex[6 * i + 2].pos[0] = p1[0][0];
        vertex[6 * i + 2].pos[1] = p1[0][1];

        // Second triangle:

        // Top-right corner again.
        vertex[6 * i + 3].pos[0] = p1[0][0];
        vertex[6 * i + 3].pos[1] = p1[0][1];

        // Top-left corner.
        vertex[6 * i + 4].pos[0] = p0[0][0];
        vertex[6 * i + 4].pos[1] = p1[0][1];

        // Bottom-left corner (again).
        vertex[6 * i + 5].pos[0] = p0[0][0];
        vertex[6 * i + 5].pos[1] = p0[0][1];

        // We copy the rectangle color to each of the six vertices making the current rectangle.
        // This is a choice made in this example, and it is up to the custom visual creator
        // to define how the user data, passed via props, will be used to fill in the vertices.
        for (uint32_t j = 0; j < 6; j++)
            memcpy(vertex[6 * i + j].color, color, sizeof(cvec4));
    }
}

static void _visual_rectangle(DvzVisual* visual)
{
    ASSERT(visual != NULL);
    DvzCanvas* canvas = visual->canvas;
    ASSERT(canvas != NULL);
    DvzProp* prop = NULL;

    // Graphics.
    dvz_visual_graphics(
        visual, dvz_graphics_builtin(canvas, DVZ_GRAPHICS_TRIANGLE, visual->flags));

    // Sources
    dvz_visual_source(
        visual, DVZ_SOURCE_TYPE_VERTEX, 0, DVZ_PIPELINE_GRAPHICS, 0, 0, sizeof(DvzVertex), 0);
    _common_sources(visual);

    // Props:
    // Add some props.
    dvz_visual_prop(visual, DVZ_PROP_POS, 0, DVZ_DTYPE_DVEC3, DVZ_SOURCE_TYPE_VERTEX, 0);
    dvz_visual_prop(visual, DVZ_PROP_POS, 1, DVZ_DTYPE_DVEC3, DVZ_SOURCE_TYPE_VERTEX, 0);

    // Custom baking functions.
    dvz_visual_callback_bake(visual, _rectangle_bake);

    // Vertex color.
    prop = dvz_visual_prop(visual, DVZ_PROP_COLOR, 0, DVZ_DTYPE_CVEC4, DVZ_SOURCE_TYPE_VERTEX, 0);
    dvz_visual_prop_copy(prop, 1, offsetof(DvzVertex, color), DVZ_ARRAY_COPY_REPEAT, 3);

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

static void _visual_marker(DvzVisual* visual)
{
    ASSERT(visual != NULL);
    DvzCanvas* canvas = visual->canvas;
    ASSERT(canvas != NULL);
    DvzProp* prop = NULL;

    // Graphics.
    dvz_visual_graphics(visual, dvz_graphics_builtin(canvas, DVZ_GRAPHICS_MARKER, visual->flags));
    dvz_graphics_depth_test(visual->graphics[0], DVZ_DEPTH_TEST_DISABLE);

    // Sources
    dvz_visual_source(
        visual, DVZ_SOURCE_TYPE_VERTEX, 0, DVZ_PIPELINE_GRAPHICS, 0, 0,
        sizeof(DvzGraphicsMarkerVertex), 0);
    _common_sources(visual);
    dvz_visual_source(
        visual, DVZ_SOURCE_TYPE_PARAM, 0, DVZ_PIPELINE_GRAPHICS, 0, DVZ_USER_BINDING,
        sizeof(DvzGraphicsMarkerParams), 0);

    // Props:

    // Marker pos.
    prop = dvz_visual_prop(visual, DVZ_PROP_POS, 0, DVZ_DTYPE_DVEC3, DVZ_SOURCE_TYPE_VERTEX, 0);
    dvz_visual_prop_cast(
        prop, 0, offsetof(DvzGraphicsMarkerVertex, pos), DVZ_DTYPE_VEC3, DVZ_ARRAY_COPY_SINGLE, 1);

    // Marker color.
    prop = dvz_visual_prop(visual, DVZ_PROP_COLOR, 0, DVZ_DTYPE_CVEC4, DVZ_SOURCE_TYPE_VERTEX, 0);
    dvz_visual_prop_copy(
        prop, 1, offsetof(DvzGraphicsMarkerVertex, color), DVZ_ARRAY_COPY_SINGLE, 1);
    cvec4 color = {200, 200, 200, 255};
    dvz_visual_prop_default(prop, &color);

    // Marker size.
    prop = dvz_visual_prop(
        visual, DVZ_PROP_MARKER_SIZE, 0, DVZ_DTYPE_FLOAT, DVZ_SOURCE_TYPE_VERTEX, 0);
    dvz_visual_prop_copy(
        prop, 1, offsetof(DvzGraphicsMarkerVertex, size), DVZ_ARRAY_COPY_SINGLE, 1);
    dvz_visual_prop_dpi(prop, canvas->dpi_scaling);
    float size = 20;
    dvz_visual_prop_default(prop, &size);

    // Marker type.
    prop = dvz_visual_prop(
        visual, DVZ_PROP_MARKER_TYPE, 0, DVZ_DTYPE_CHAR, DVZ_SOURCE_TYPE_VERTEX, 0);
    dvz_visual_prop_copy(
        prop, 1, offsetof(DvzGraphicsMarkerVertex, marker), DVZ_ARRAY_COPY_SINGLE, 1);
    DvzMarkerType marker = DVZ_MARKER_DISC;
    dvz_visual_prop_default(prop, &marker);

    // Marker angle.
    prop = dvz_visual_prop(visual, DVZ_PROP_ANGLE, 0, DVZ_DTYPE_CHAR, DVZ_SOURCE_TYPE_VERTEX, 0);
    dvz_visual_prop_copy(
        prop, 1, offsetof(DvzGraphicsMarkerVertex, angle), DVZ_ARRAY_COPY_SINGLE, 1);
    float angle = 0;
    dvz_visual_prop_default(prop, &angle);

    // Marker transform.
    prop =
        dvz_visual_prop(visual, DVZ_PROP_TRANSFORM, 0, DVZ_DTYPE_CHAR, DVZ_SOURCE_TYPE_VERTEX, 0);
    dvz_visual_prop_copy(
        prop, 1, offsetof(DvzGraphicsMarkerVertex, transform), DVZ_ARRAY_COPY_SINGLE, 1);

    // Common props.
    _common_props(visual);

    // Param: edge color.
    prop = dvz_visual_prop(visual, DVZ_PROP_COLOR, 1, DVZ_DTYPE_VEC4, DVZ_SOURCE_TYPE_PARAM, 0);
    dvz_visual_prop_copy(
        prop, 0, offsetof(DvzGraphicsMarkerParams, edge_color), DVZ_ARRAY_COPY_SINGLE, 1);
    vec4 edge_color = {0, 0, 0, 1};
    dvz_visual_prop_default(prop, &edge_color);

    // Param: edge width.
    prop =
        dvz_visual_prop(visual, DVZ_PROP_LINE_WIDTH, 0, DVZ_DTYPE_FLOAT, DVZ_SOURCE_TYPE_PARAM, 0);
    dvz_visual_prop_copy(
        prop, 0, offsetof(DvzGraphicsMarkerParams, edge_width), DVZ_ARRAY_COPY_SINGLE, 1);
    dvz_visual_prop_dpi(prop, canvas->dpi_scaling);
    float edge_width = 1;
    dvz_visual_prop_default(prop, &edge_width);
}



/*************************************************************************************************/
/*  Polygon                                                                                      */
/*************************************************************************************************/

static void _polygon_bake(DvzVisual* visual, DvzVisualDataEvent ev)
{
    ASSERT(visual != NULL);

    DvzProp* prop_pos = dvz_prop_get(visual, DVZ_PROP_POS, 0);       // dvec3
    DvzProp* prop_length = dvz_prop_get(visual, DVZ_PROP_LENGTH, 0); // uint
    DvzProp* prop_color = dvz_prop_get(visual, DVZ_PROP_COLOR, 0);   // cvec4

    DvzArray* arr_pos = _prop_array(prop_pos, DVZ_PROP_ARRAY_DEFAULT);
    DvzArray* arr_length = _prop_array(prop_length, DVZ_PROP_ARRAY_DEFAULT);
    DvzArray* arr_color = _prop_array(prop_color, DVZ_PROP_ARRAY_DEFAULT);

    DvzSource* src_vertex = dvz_source_get(visual, DVZ_SOURCE_TYPE_VERTEX, 0);
    DvzSource* src_index = dvz_source_get(visual, DVZ_SOURCE_TYPE_INDEX, 0);

    // The baking function doesn't run if the VERTEX source is handled by the user.
    if (src_vertex->origin != DVZ_SOURCE_ORIGIN_LIB)
        return;
    if (src_vertex->obj.request != DVZ_VISUAL_REQUEST_UPLOAD)
    {
        log_trace(
            "skip bake source for source %d that doesn't need updating", src_vertex->source_kind);
        return;
    }

    // Source arrays.
    DvzArray* arr_vertex = &src_vertex->arr;
    DvzArray* arr_index = &src_index->arr;

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
        dvz_triangulate_polygon(
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
    dvz_array_resize(arr_vertex, n_points);
    // Copy the positions from the pos prop to the vertex buffer.
    _prop_copy(visual, prop_pos);

    // Resize and fill the index buffer.
    dvz_array_resize(arr_index, total_index_count);
    dvz_array_data(arr_index, 0, total_index_count, total_index_count, total_indices);

    // Copy the polygon colors to the vertices.
    cvec4* color = NULL;
    // Go through the polygons.
    uint32_t k = 0;
    for (uint32_t i = 0; i < n_polys; i++)
    {
        // Color prop for the current polygon.
        color = (cvec4*)dvz_array_item(arr_color, i);
        // Copy the color to the vertex buffer, repeating it for each vertex in the polygon.
        dvz_array_column(
            arr_vertex, offsetof(DvzVertex, color), sizeof(cvec4), k, poly_lengths[i], 1, color,
            DVZ_DTYPE_NONE, DVZ_DTYPE_NONE, DVZ_ARRAY_COPY_SINGLE, 1);
        k += poly_lengths[i];
    }

    FREE(index_count_list);
    FREE(indices_list);
    FREE(total_indices);
}

static void _visual_polygon(DvzVisual* visual)
{
    ASSERT(visual != NULL);
    DvzCanvas* canvas = visual->canvas;
    ASSERT(canvas != NULL);
    DvzProp* prop = NULL;

    // Graphics.
    dvz_visual_graphics(
        visual, dvz_graphics_builtin(canvas, DVZ_GRAPHICS_TRIANGLE, visual->flags));

    // Sources
    dvz_visual_source(
        visual, DVZ_SOURCE_TYPE_VERTEX, 0, DVZ_PIPELINE_GRAPHICS, 0, 0, sizeof(DvzVertex), 0);

    dvz_visual_source(
        visual, DVZ_SOURCE_TYPE_INDEX, 0, DVZ_PIPELINE_GRAPHICS, 0, 0, sizeof(DvzIndex), 0);

    _common_sources(visual);

    // Props:

    // Polygon points, 1 position per point.
    prop = dvz_visual_prop(visual, DVZ_PROP_POS, 0, DVZ_DTYPE_DVEC3, DVZ_SOURCE_TYPE_VERTEX, 0);
    // Copy the polygon points directly to the vertex buffer, as the triangulation only sets the
    // vertex indices and does not change the vertices themselves.
    dvz_visual_prop_cast(
        prop, 0, offsetof(DvzVertex, pos), DVZ_DTYPE_VEC3, DVZ_ARRAY_COPY_SINGLE, 1);

    // Polygon lengths, 1 length per polygon.
    prop = dvz_visual_prop(visual, DVZ_PROP_LENGTH, 0, DVZ_DTYPE_UINT, DVZ_SOURCE_TYPE_VERTEX, 0);

    // Polygon colors, 1 color per polygon.
    prop = dvz_visual_prop(visual, DVZ_PROP_COLOR, 0, DVZ_DTYPE_CVEC4, DVZ_SOURCE_TYPE_VERTEX, 0);

    // Common props.
    _common_props(visual);

    dvz_visual_callback_bake(visual, _polygon_bake);
}



/*************************************************************************************************/
/*  Path                                                                                         */
/*************************************************************************************************/

static void _path_bake(DvzVisual* visual, DvzVisualDataEvent ev)
{
    ASSERT(visual != NULL);

    DvzProp* prop_pos = dvz_prop_get(visual, DVZ_PROP_POS, 0);     // dvec3
    DvzProp* prop_color = dvz_prop_get(visual, DVZ_PROP_COLOR, 0); // cvec4

    DvzProp* prop_length = dvz_prop_get(visual, DVZ_PROP_LENGTH, 0);     // uint
    DvzProp* prop_topology = dvz_prop_get(visual, DVZ_PROP_TOPOLOGY, 0); // int

    DvzArray* arr_pos = _prop_array(prop_pos, DVZ_PROP_ARRAY_DEFAULT);
    DvzArray* arr_color = _prop_array(prop_color, DVZ_PROP_ARRAY_DEFAULT);
    DvzArray* arr_length = _prop_array(prop_length, DVZ_PROP_ARRAY_DEFAULT);
    DvzArray* arr_topology = _prop_array(prop_topology, DVZ_PROP_ARRAY_DEFAULT);

    DvzSource* src_vertex = dvz_source_get(visual, DVZ_SOURCE_TYPE_VERTEX, 0);

    // The baking function doesn't run if the VERTEX source is handled by the user.
    if (src_vertex->origin != DVZ_SOURCE_ORIGIN_LIB)
        return;
    if (src_vertex->obj.request != DVZ_VISUAL_REQUEST_UPLOAD)
    {
        log_trace(
            "skip bake source for source %d that doesn't need updating", src_vertex->source_kind);
        return;
    }

    // Source arrays.
    DvzArray* arr_vertex = &src_vertex->arr;

    // Number of points and paths.
    uint32_t n_points = arr_pos->item_count; // number of points
    if (n_points == 0)
    {
        log_debug("empty path visual");
        return;
    }
    uint32_t n_paths = arr_length->item_count; // number of paths
    if (n_paths == 0)
        n_paths = 1;
    // number of points, incl invisible join points
    uint32_t n_points_tot = n_points + 0 * (n_paths);

    ASSERT(n_points > 0);
    ASSERT(n_paths > 0);

    dvec3* point = NULL;
    cvec4* color = NULL;
    uint32_t* path_length = NULL;
    int32_t* is_closed = NULL;

    // Reesize and fill the vertex buffer.
    dvz_array_resize(arr_vertex, n_points);
    // Copy the positions from the pos prop to the vertex buffer.
    _prop_copy(visual, prop_pos);

    // Graphics data.
    DvzGraphicsData data = dvz_graphics_data(visual->graphics[0], arr_vertex, NULL, NULL);
    dvz_graphics_alloc(&data, n_points_tot);

    DvzGraphicsPathVertex item = {0};
    int32_t path_size = 0;
    bool closed = false;
    int32_t j0, j1, j2, j3;
    int32_t idx = 0; // index of the first point in the current path

    for (uint32_t i = 0; i < n_paths; i++)
    {
        // log_info("path #%d", i);

        // Per-path data.
        path_length = dvz_array_item(arr_length, i);
        path_size = path_length != NULL ? (int32_t)*path_length : (int32_t)n_points;

        is_closed = dvz_array_item(arr_topology, i);
        closed = is_closed != NULL ? *is_closed : false;

        // Add join point at the beginning of each path.
        {
            point = dvz_array_item(arr_pos, (uint32_t)idx);

            _vec3_cast((const dvec3*)point, &item.p0);
            _vec3_cast((const dvec3*)point, &item.p1);
            _vec3_cast((const dvec3*)point, &item.p2);
            _vec3_cast((const dvec3*)point, &item.p3);

            memset(item.color, 0, sizeof(cvec4));

            // dvz_graphics_append(&data, &item);
        }

        // Add all points in the path.
        for (int32_t j = 0; j < (int32_t)path_size; j++)
        {
            // Compute p0, p1, p2, p3.
            j0 = j - 1;
            j1 = j;
            j2 = j + 1;
            j3 = j + 2;

            if (!closed)
            {
                j0 = j0 < 0 ? 0 : j0;
                j2 = j2 >= path_size ? (path_size - 1) : j2;
                j3 = j3 >= path_size ? (path_size - 1) : j3;
            }
            else
            {
                j0 = j0 < 0 ? (path_size - 2) : j0;
                j2 = j2 >= path_size ? 0 : j2;
                j3 = j3 >= path_size ? 1 : j3;
            }

            ASSERT(0 <= j0 && j0 < path_size);
            ASSERT(0 <= j1 && j1 < path_size);
            ASSERT(0 <= j2 && j2 < path_size);
            ASSERT(0 <= j3 && j3 < path_size);

            point = dvz_array_item(arr_pos, (uint32_t)(idx + j0));
            _vec3_cast((const dvec3*)point, &item.p0);

            point = dvz_array_item(arr_pos, (uint32_t)(idx + j1));
            _vec3_cast((const dvec3*)point, &item.p1);

            point = dvz_array_item(arr_pos, (uint32_t)(idx + j2));
            _vec3_cast((const dvec3*)point, &item.p2);

            point = dvz_array_item(arr_pos, (uint32_t)(idx + j3));
            _vec3_cast((const dvec3*)point, &item.p3);

            color = dvz_array_item(arr_color, (uint32_t)(idx + j1));
            memcpy(item.color, color, sizeof(cvec4));

            dvz_graphics_append(&data, &item);
        }

        // Add join point at the end of each path.
        {
            point = dvz_array_item(arr_pos, (uint32_t)(idx + path_size - 1));

            _vec3_cast((const dvec3*)point, &item.p0);
            _vec3_cast((const dvec3*)point, &item.p1);
            _vec3_cast((const dvec3*)point, &item.p2);
            _vec3_cast((const dvec3*)point, &item.p3);

            memset(item.color, 0, sizeof(cvec4));

            // dvz_graphics_append(&data, &item);
        }

        idx += path_size;
    }
    ASSERT(idx == (int32_t)n_points);
}

static void _visual_path(DvzVisual* visual)
{
    ASSERT(visual != NULL);
    DvzCanvas* canvas = visual->canvas;
    ASSERT(canvas != NULL);
    DvzProp* prop = NULL;

    // Graphics.
    dvz_visual_graphics(visual, dvz_graphics_builtin(canvas, DVZ_GRAPHICS_PATH, visual->flags));

    // Sources
    dvz_visual_source(
        visual, DVZ_SOURCE_TYPE_VERTEX, 0, DVZ_PIPELINE_GRAPHICS, 0, 0,
        sizeof(DvzGraphicsPathVertex), 0);

    _common_sources(visual);

    dvz_visual_source(                                              // params
        visual, DVZ_SOURCE_TYPE_PARAM, 0, DVZ_PIPELINE_GRAPHICS, 0, //
        DVZ_USER_BINDING, sizeof(DvzGraphicsPathParams), 0);        //

    // Props:

    // Path points, 1 position per point.
    prop = dvz_visual_prop(visual, DVZ_PROP_POS, 0, DVZ_DTYPE_DVEC3, DVZ_SOURCE_TYPE_VERTEX, 0);

    // Path colors, 1 color per point.
    prop = dvz_visual_prop(visual, DVZ_PROP_COLOR, 0, DVZ_DTYPE_CVEC4, DVZ_SOURCE_TYPE_VERTEX, 0);
    dvz_visual_prop_default(prop, (cvec4[]){{255, 0, 0, 255}});

    // Path lengths, 1 length per path.
    prop = dvz_visual_prop(visual, DVZ_PROP_LENGTH, 0, DVZ_DTYPE_UINT, DVZ_SOURCE_TYPE_VERTEX, 0);

    // Path topology, 1 value per path.
    prop = dvz_visual_prop(visual, DVZ_PROP_TOPOLOGY, 0, DVZ_DTYPE_INT, DVZ_SOURCE_TYPE_VERTEX, 0);
    dvz_visual_prop_default(prop, (int32_t[]){DVZ_PATH_OPEN});

    // Common props.
    _common_props(visual);

    // Line width.
    prop =
        dvz_visual_prop(visual, DVZ_PROP_LINE_WIDTH, 0, DVZ_DTYPE_FLOAT, DVZ_SOURCE_TYPE_PARAM, 0);
    dvz_visual_prop_copy(
        prop, 0, offsetof(DvzGraphicsPathParams, linewidth), DVZ_ARRAY_COPY_SINGLE, 1);
    dvz_visual_prop_dpi(prop, canvas->dpi_scaling);
    dvz_visual_prop_default(prop, (float[]){5.0f});

    // Miter limit.
    prop = dvz_visual_prop(
        visual, DVZ_PROP_MITER_LIMIT, 0, DVZ_DTYPE_FLOAT, DVZ_SOURCE_TYPE_PARAM, 0);
    dvz_visual_prop_copy(
        prop, 1, offsetof(DvzGraphicsPathParams, miter_limit), DVZ_ARRAY_COPY_SINGLE, 1);
    dvz_visual_prop_default(prop, (float[]){4.0f});

    // Cap type.
    prop = dvz_visual_prop(visual, DVZ_PROP_CAP_TYPE, 0, DVZ_DTYPE_INT, DVZ_SOURCE_TYPE_PARAM, 0);
    dvz_visual_prop_copy(
        prop, 2, offsetof(DvzGraphicsPathParams, cap_type), DVZ_ARRAY_COPY_SINGLE, 1);
    dvz_visual_prop_default(prop, (int32_t[]){DVZ_CAP_ROUND});

    // Join type.
    prop = dvz_visual_prop(visual, DVZ_PROP_JOIN_TYPE, 0, DVZ_DTYPE_INT, DVZ_SOURCE_TYPE_PARAM, 0);
    dvz_visual_prop_copy(
        prop, 3, offsetof(DvzGraphicsPathParams, round_join), DVZ_ARRAY_COPY_SINGLE, 1);
    dvz_visual_prop_default(prop, (int32_t[]){DVZ_JOIN_ROUND});

    dvz_visual_callback_bake(visual, _path_bake);
}



/*************************************************************************************************/
/*  Text                                                                                         */
/*************************************************************************************************/

static void _text_bake(DvzVisual* visual, DvzVisualDataEvent ev)
{
    ASSERT(visual != NULL);

    DvzProp* prop_pos = dvz_prop_get(visual, DVZ_PROP_POS, 0);        // dvec3
    DvzProp* prop_text = dvz_prop_get(visual, DVZ_PROP_TEXT, 0);      // str
    DvzProp* prop_glyph = dvz_prop_get(visual, DVZ_PROP_GLYPH, 0);    // char
    DvzProp* prop_length = dvz_prop_get(visual, DVZ_PROP_LENGTH, 0);  // uint
    DvzProp* prop_color = dvz_prop_get(visual, DVZ_PROP_COLOR, 0);    // cvec4
    DvzProp* prop_size = dvz_prop_get(visual, DVZ_PROP_TEXT_SIZE, 0); // float
    DvzProp* prop_anchor = dvz_prop_get(visual, DVZ_PROP_ANCHOR, 0);  // vec2
    DvzProp* prop_angle = dvz_prop_get(visual, DVZ_PROP_ANGLE, 0);    // float

    DvzArray* arr_pos = _prop_array(prop_pos, DVZ_PROP_ARRAY_DEFAULT);
    DvzArray* arr_text = _prop_array(prop_text, DVZ_PROP_ARRAY_DEFAULT);
    DvzArray* arr_glyph = _prop_array(prop_glyph, DVZ_PROP_ARRAY_DEFAULT);
    DvzArray* arr_length = _prop_array(prop_length, DVZ_PROP_ARRAY_DEFAULT);
    DvzArray* arr_color = _prop_array(prop_color, DVZ_PROP_ARRAY_DEFAULT);
    DvzArray* arr_size = _prop_array(prop_size, DVZ_PROP_ARRAY_DEFAULT);
    DvzArray* arr_anchor = _prop_array(prop_anchor, DVZ_PROP_ARRAY_DEFAULT);
    DvzArray* arr_angle = _prop_array(prop_angle, DVZ_PROP_ARRAY_DEFAULT);

    DvzSource* src_vertex = dvz_source_get(visual, DVZ_SOURCE_TYPE_VERTEX, 0);

    // The baking function doesn't run if the VERTEX source is handled by the user.
    if (src_vertex->origin != DVZ_SOURCE_ORIGIN_LIB)
        return;
    if (src_vertex->obj.request != DVZ_VISUAL_REQUEST_UPLOAD)
    {
        log_trace(
            "skip bake source for source %d that doesn't need updating", src_vertex->source_kind);
        return;
    }

    // Source arrays.
    DvzArray* arr_vertex = &src_vertex->arr;

    // Number of strings.
    uint32_t n_strings = arr_text->item_count; // number of strings
    // Total number of characters, so that we can resize the vertex source array.
    uint32_t n_chars = 0;

    // Alternatively, look at the GLYPH prop instead of TEXT.
    bool glyphs = false; // whether the user has set glyphs directly instead of text

    // Glyphs are set directly:
    if (n_strings == 0)
    {
        glyphs = true;
        // NOTE: when setting glyphs directly, there is no notion of \0 null-termination of strings
        n_chars = arr_glyph->item_count;
        // NOTE: must compute n_strings
        n_strings = arr_length->item_count;
    }

    // Or strings are used instead:
    else
    {
        // NOTE: must compute n_chars
        char* str = NULL;
        for (uint32_t i = 0; i < n_strings; i++)
        {
            str = *(char**)dvz_array_item(arr_text, i);
            ASSERT(str != NULL);
            // WARNING: not safe
            n_chars += strlen(str);
        }
    }

    if (n_strings == 0 || n_chars == 0)
    {
        log_debug("empty text visual");
        return;
    }

    ASSERT(n_strings > 0);
    ASSERT(n_chars > 0);
    log_debug("found %d string(s) in text visual, for a total of %d chars", n_strings, n_chars);

    // Graphics data.
    DvzGraphicsData data = dvz_graphics_data(visual->graphics[0], arr_vertex, NULL, NULL);
    dvz_graphics_alloc(&data, n_chars);

    DvzGraphicsTextItem item = {0};
    // Add all of the strings.
    cvec4* colors = calloc(n_chars, sizeof(cvec4));
    cvec4* color = NULL;
    uint32_t string_len = 0;
    uint32_t k = 0;
    for (uint32_t i = 0; i < n_strings; i++)
    {
        // String.
        if (glyphs)
        {
            string_len = *(uint32_t*)dvz_array_item(arr_length, i);
            item.strlen = string_len;
            ASSERT(item.strlen > 0);
            // NOTE: pointer to the glyph array, increasing of strlen at every string.
            item.glyphs = (uint16_t*)dvz_array_item(arr_glyph, k);
            k += item.strlen;
        }
        else
        {
            item.string = *(char**)dvz_array_item(arr_text, i);
            string_len = strlen(item.string);
        }

        // Font size for this string.
        item.font_size = *(float*)dvz_array_item(arr_size, i);

        // String position.
        _vec3_cast(dvz_array_item(arr_pos, i), &item.vertex.pos);
        // Anchor.
        memcpy(item.vertex.anchor, dvz_array_item(arr_anchor, i), sizeof(vec2));

        // Angle.
        item.vertex.angle = *(float*)dvz_array_item(arr_angle, i);

        // Repeat the color for each glyph.
        color = (cvec4*)dvz_array_item(arr_color, i);
        for (uint32_t j = 0; j < string_len; j++)
        {
            memcpy(colors[j], color, sizeof(cvec4));
        }
        item.glyph_colors = colors;

        dvz_graphics_append(&data, &item);
    }
    FREE(colors);
}

static void _visual_text(DvzVisual* visual)
{
    ASSERT(visual != NULL);
    DvzCanvas* canvas = visual->canvas;
    ASSERT(canvas != NULL);
    DvzProp* prop = NULL;

    // Graphics.
    dvz_visual_graphics(visual, dvz_graphics_builtin(canvas, DVZ_GRAPHICS_TEXT, visual->flags));

    // Sources.

    // Vertex buffer.
    dvz_visual_source(
        visual, DVZ_SOURCE_TYPE_VERTEX, 0, DVZ_PIPELINE_GRAPHICS, 0, 0,
        sizeof(DvzGraphicsTextVertex), 0);

    // Common sources.
    _common_sources(visual);

    // Params source.
    dvz_visual_source(                                              //
        visual, DVZ_SOURCE_TYPE_PARAM, 0, DVZ_PIPELINE_GRAPHICS, 0, //
        DVZ_USER_BINDING, sizeof(DvzGraphicsTextParams), 0);        //

    // Font atlas source.
    dvz_visual_source(                                                   //
        visual, DVZ_SOURCE_TYPE_FONT_ATLAS, 0, DVZ_PIPELINE_GRAPHICS, 0, //
        DVZ_USER_BINDING + 1, sizeof(cvec4), 0);                         //


    // Props:

    // Text position, 1 per string.
    prop = dvz_visual_prop(visual, DVZ_PROP_POS, 0, DVZ_DTYPE_DVEC3, DVZ_SOURCE_TYPE_VERTEX, 0);
    dvz_visual_prop_default(prop, (dvec3[]){{0, 0, 0}});

    // Text strings.
    // Each element is a pointer to a char buffer.
    // WARNING: these pointers must not be freed during the lifetime of the visual!
    prop = dvz_visual_prop(visual, DVZ_PROP_TEXT, 0, DVZ_DTYPE_STR, DVZ_SOURCE_TYPE_VERTEX, 0);

    // Alternatively to setting text strings, one can directly set the glyph index within the font
    // atlas (useful for wrappers).
    prop = dvz_visual_prop(visual, DVZ_PROP_GLYPH, 0, DVZ_DTYPE_USHORT, DVZ_SOURCE_TYPE_VERTEX, 0);
    // Length of each string (glyphs are grouped per string).
    prop = dvz_visual_prop(visual, DVZ_PROP_LENGTH, 0, DVZ_DTYPE_UINT, DVZ_SOURCE_TYPE_VERTEX, 0);

    // Text colors, 1 per string.
    prop = dvz_visual_prop(visual, DVZ_PROP_COLOR, 0, DVZ_DTYPE_CVEC4, DVZ_SOURCE_TYPE_VERTEX, 0);
    dvz_visual_prop_default(prop, (cvec4[]){{127, 127, 127, 255}});

    // Text size, 1 per string.
    prop =
        dvz_visual_prop(visual, DVZ_PROP_TEXT_SIZE, 0, DVZ_DTYPE_FLOAT, DVZ_SOURCE_TYPE_VERTEX, 0);
    dvz_visual_prop_default(prop, (float[]){16.0f});

    // String anchor.
    prop = dvz_visual_prop(visual, DVZ_PROP_ANCHOR, 0, DVZ_DTYPE_VEC2, DVZ_SOURCE_TYPE_VERTEX, 0);
    dvz_visual_prop_default(prop, (vec2[]){{0, 0}});

    // String angle.
    prop = dvz_visual_prop(visual, DVZ_PROP_ANGLE, 0, DVZ_DTYPE_FLOAT, DVZ_SOURCE_TYPE_VERTEX, 0);
    dvz_visual_prop_default(prop, (float[]){0});

    // Common props.
    _common_props(visual);

    dvz_visual_callback_bake(visual, _text_bake);


    // Connect the font atlas to the FONT_ATLAS and PARAM sources of the text visual.
    // Text params.
    ASSERT(canvas->gpu != NULL);
    ASSERT(canvas->gpu->context != NULL);
    DvzFontAtlas* atlas = &canvas->gpu->context->font_atlas;
    ASSERT(atlas != NULL);
    ASSERT(strlen(atlas->font_str) > 0);
    dvz_visual_texture(visual, DVZ_SOURCE_TYPE_FONT_ATLAS, 0, atlas->texture);

    DvzGraphicsTextParams params = {0};
    params.grid_size[0] = (int32_t)atlas->rows;
    params.grid_size[1] = (int32_t)atlas->cols;
    params.tex_size[0] = (int32_t)atlas->width;
    params.tex_size[1] = (int32_t)atlas->height;
    dvz_visual_data_source(visual, DVZ_SOURCE_TYPE_PARAM, 0, 0, 1, 1, &params);
}



/*************************************************************************************************/
/*  Image                                                                                        */
/*************************************************************************************************/

static void _visual_image_bake(DvzVisual* visual, DvzVisualDataEvent ev)
{
    ASSERT(visual != NULL);

    // Vertex buffer source.
    DvzSource* source = dvz_source_get(visual, DVZ_SOURCE_TYPE_VERTEX, 0);
    ASSERT(source->arr.item_size == sizeof(DvzGraphicsImageVertex));

    // Get props.
    DvzProp* pos0 = dvz_prop_get(visual, DVZ_PROP_POS, 0);
    DvzProp* pos1 = dvz_prop_get(visual, DVZ_PROP_POS, 1);
    DvzProp* pos2 = dvz_prop_get(visual, DVZ_PROP_POS, 2);
    DvzProp* pos3 = dvz_prop_get(visual, DVZ_PROP_POS, 3);

    DvzProp* uv0 = dvz_prop_get(visual, DVZ_PROP_TEXCOORDS, 0);
    DvzProp* uv1 = dvz_prop_get(visual, DVZ_PROP_TEXCOORDS, 1);
    DvzProp* uv2 = dvz_prop_get(visual, DVZ_PROP_TEXCOORDS, 2);
    DvzProp* uv3 = dvz_prop_get(visual, DVZ_PROP_TEXCOORDS, 3);

    ASSERT(pos0 != NULL);
    ASSERT(pos1 != NULL);
    ASSERT(pos2 != NULL);
    ASSERT(pos3 != NULL);

    ASSERT(uv0 != NULL);
    ASSERT(uv1 != NULL);
    ASSERT(uv2 != NULL);
    ASSERT(uv3 != NULL);

    // Number of images
    uint32_t img_count = dvz_prop_size(pos0);
    if (img_count == 0)
        return;
    ASSERT(dvz_prop_size(pos1) == img_count);
    ASSERT(dvz_prop_size(pos2) == img_count);
    ASSERT(dvz_prop_size(pos3) == img_count);

    // Graphics data.
    DvzGraphicsData data = dvz_graphics_data(visual->graphics[0], &source->arr, NULL, NULL);
    dvz_graphics_alloc(&data, img_count);

    DvzGraphicsImageItem item = {0};
    for (uint32_t i = 0; i < img_count; i++)
    {
        _vec3_cast((const dvec3*)dvz_prop_item(pos0, i), &item.pos0);
        _vec3_cast((const dvec3*)dvz_prop_item(pos1, i), &item.pos1);
        _vec3_cast((const dvec3*)dvz_prop_item(pos2, i), &item.pos2);
        _vec3_cast((const dvec3*)dvz_prop_item(pos3, i), &item.pos3);

        memcpy(&item.uv0, dvz_prop_item(uv0, i), sizeof(vec2));
        memcpy(&item.uv1, dvz_prop_item(uv1, i), sizeof(vec2));
        memcpy(&item.uv2, dvz_prop_item(uv2, i), sizeof(vec2));
        memcpy(&item.uv3, dvz_prop_item(uv3, i), sizeof(vec2));

        dvz_graphics_append(&data, &item);
    }
}

static void _visual_image(DvzVisual* visual)
{
    ASSERT(visual != NULL);
    DvzCanvas* canvas = visual->canvas;
    ASSERT(canvas != NULL);
    DvzProp* prop = NULL;

    // TODO: customizable dtype for the image

    // Graphics.
    dvz_visual_graphics(visual, dvz_graphics_builtin(canvas, DVZ_GRAPHICS_IMAGE, visual->flags));

    // Sources
    dvz_visual_source(                                               // vertex buffer
        visual, DVZ_SOURCE_TYPE_VERTEX, 0, DVZ_PIPELINE_GRAPHICS, 0, //
        0, sizeof(DvzGraphicsImageVertex), 0);                       //

    _common_sources(visual); // common sources

    dvz_visual_source(                                              // params
        visual, DVZ_SOURCE_TYPE_PARAM, 0, DVZ_PIPELINE_GRAPHICS, 0, //
        DVZ_USER_BINDING, sizeof(DvzGraphicsImageParams), 0);       //

    for (uint32_t i = 0; i < 4; i++)
        dvz_visual_source(                                              // textures
            visual, DVZ_SOURCE_TYPE_IMAGE, i, DVZ_PIPELINE_GRAPHICS, 0, //
            DVZ_USER_BINDING + i + 1, sizeof(uint8_t), 0);              //

    // Props:

    // Point positions.
    // Top left, top right, bottom right, bottom left
    for (uint32_t i = 0; i < 4; i++)
        dvz_visual_prop(visual, DVZ_PROP_POS, i, DVZ_DTYPE_DVEC3, DVZ_SOURCE_TYPE_VERTEX, 0);

    // Tex coords.
    for (uint32_t i = 0; i < 4; i++)
        dvz_visual_prop(visual, DVZ_PROP_TEXCOORDS, i, DVZ_DTYPE_VEC2, DVZ_SOURCE_TYPE_VERTEX, 0);

    // Common props.
    _common_props(visual);

    // Params.

    // Texture coefficients.
    prop = dvz_visual_prop(visual, DVZ_PROP_TEXCOEFS, 0, DVZ_DTYPE_VEC4, DVZ_SOURCE_TYPE_PARAM, 0);
    dvz_visual_prop_copy(
        prop, 0, offsetof(DvzGraphicsImageParams, tex_coefs), DVZ_ARRAY_COPY_SINGLE, 1);
    dvz_visual_prop_default(prop, (vec4[]){{1, 0, 0, 0}});

    // // Texture props.
    // for (uint32_t i = 0; i < 4; i++)
    //     dvz_visual_prop(visual, DVZ_PROP_IMAGE, i, DVZ_DTYPE_CHAR, DVZ_SOURCE_TYPE_IMAGE, i);

    // Baking function.
    dvz_visual_callback_bake(visual, _visual_image_bake);
}

static void _visual_image_cmap(DvzVisual* visual)
{
    ASSERT(visual != NULL);
    DvzCanvas* canvas = visual->canvas;
    ASSERT(canvas != NULL);
    DvzProp* prop = NULL;

    // TODO: customizable dtype for the image

    // Graphics.
    dvz_visual_graphics(
        visual, dvz_graphics_builtin(canvas, DVZ_GRAPHICS_IMAGE_CMAP, visual->flags));

    // Sources
    dvz_visual_source(                                               // vertex buffer
        visual, DVZ_SOURCE_TYPE_VERTEX, 0, DVZ_PIPELINE_GRAPHICS, 0, //
        0, sizeof(DvzGraphicsImageVertex), 0);                       //

    _common_sources(visual); // common sources

    dvz_visual_source(                                              // params
        visual, DVZ_SOURCE_TYPE_PARAM, 0, DVZ_PIPELINE_GRAPHICS, 0, //
        DVZ_USER_BINDING, sizeof(DvzGraphicsImageCmapParams), 0);   //

    dvz_visual_source(                                                      // colormap texture
        visual, DVZ_SOURCE_TYPE_COLOR_TEXTURE, 0, DVZ_PIPELINE_GRAPHICS, 0, //
        DVZ_USER_BINDING + 1, sizeof(uint8_t), 0);                          //

    dvz_visual_source(                                              // image
        visual, DVZ_SOURCE_TYPE_IMAGE, 0, DVZ_PIPELINE_GRAPHICS, 0, //
        DVZ_USER_BINDING + 2, sizeof(uint8_t), 0);                  //

    // Props:

    // Point positions.
    // Top left, top right, bottom right, bottom left
    for (uint32_t i = 0; i < 4; i++)
        dvz_visual_prop(visual, DVZ_PROP_POS, i, DVZ_DTYPE_DVEC3, DVZ_SOURCE_TYPE_VERTEX, 0);

    // Tex coords.
    for (uint32_t i = 0; i < 4; i++)
        dvz_visual_prop(visual, DVZ_PROP_TEXCOORDS, i, DVZ_DTYPE_VEC2, DVZ_SOURCE_TYPE_VERTEX, 0);

    // Common props.
    _common_props(visual);

    // Params.

    // Range.
    prop = dvz_visual_prop(visual, DVZ_PROP_RANGE, 0, DVZ_DTYPE_VEC2, DVZ_SOURCE_TYPE_PARAM, 0);
    dvz_visual_prop_copy(
        prop, 0, offsetof(DvzGraphicsImageCmapParams, vrange), DVZ_ARRAY_COPY_SINGLE, 1);
    dvz_visual_prop_default(prop, (vec2){0, 1});

    // Colormap value.
    prop = dvz_visual_prop(visual, DVZ_PROP_COLORMAP, 0, DVZ_DTYPE_INT, DVZ_SOURCE_TYPE_PARAM, 0);
    dvz_visual_prop_copy(
        prop, 1, offsetof(DvzGraphicsImageCmapParams, cmap), DVZ_ARRAY_COPY_SINGLE, 1);
    DvzColormap cmap = DVZ_CMAP_VIRIDIS;
    dvz_visual_prop_default(prop, &cmap);

    // // Texture prop.
    // dvz_visual_prop(visual, DVZ_PROP_IMAGE, 0, DVZ_DTYPE_CHAR, DVZ_SOURCE_TYPE_IMAGE, 0);

    // Baking function.
    dvz_visual_callback_bake(visual, _visual_image_bake);
}



/*************************************************************************************************/
/*  Axes 2D                                                                                      */
/*************************************************************************************************/

static uint32_t _count_prop_items(
    DvzVisual* visual, uint32_t prop_count, DvzPropType* prop_types, //
    uint32_t idx_count)
{
    ASSERT(visual != NULL);
    uint32_t count = 0;
    for (uint32_t i = 0; i < prop_count; i++)
    {
        for (uint32_t j = 0; j < idx_count; j++)
        {
            count += dvz_prop_size(dvz_prop_get(visual, prop_types[i], j));
        }
    }
    return count;
}

static uint32_t _count_chars(DvzArray* arr_text)
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

static void _tick_pos(double x, DvzAxisLevel level, DvzAxisCoord coord, vec3 P0, vec3 P1)
{
    vec2 lim = {0};
    lim[0] = -1;

    switch (level)
    {
    case DVZ_AXES_LEVEL_MINOR:
    case DVZ_AXES_LEVEL_MAJOR:
        lim[1] = -1;
        break;
    case DVZ_AXES_LEVEL_GRID:
    case DVZ_AXES_LEVEL_LIM:
        lim[1] = +1;
        break;
    default:
        break;
    }

    if (coord == DVZ_AXES_COORD_X)
    {
        P0[0] = x;
        P0[1] = lim[0];
        P1[0] = x;
        P1[1] = lim[1];
    }
    else if (coord == DVZ_AXES_COORD_Y)
    {
        P0[1] = x;
        P0[0] = lim[0];
        P1[1] = x;
        P1[0] = lim[1];
    }
}

static void _tick_shift(
    uint32_t i, uint32_t n, float s, vec2 tick_length, DvzAxisLevel level, DvzAxisCoord coord,
    vec4 shift)
{
    if (level <= DVZ_AXES_LEVEL_MAJOR)
        shift[3 - coord] = tick_length[level];

    if (level == DVZ_AXES_LEVEL_LIM)
    {
        if (coord == DVZ_AXES_COORD_X)
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
        else if (coord == DVZ_AXES_COORD_Y)
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
    DvzProp* tick_prop, DvzGraphicsData* data, DvzAxisLevel level, DvzAxisCoord coord, cvec4 color,
    float lw, vec2 tick_length)
{
    ASSERT(tick_prop != NULL);
    ASSERT(data != NULL);

    double* x = NULL;
    vec3 P0 = {0};
    vec3 P1 = {0};
    DvzCapType cap = DVZ_CAP_TYPE_NONE;
    int32_t interact_axis = level == DVZ_AXES_LEVEL_LIM ? DVZ_INTERACT_FIXED_AXIS_ALL
                                                        : DVZ_INTERACT_FIXED_AXIS_DEFAULT;
    interact_axis = interact_axis >> 12;
    ASSERT(0 <= interact_axis && interact_axis <= 8);

    uint32_t n = tick_prop->arr_orig.item_count;
    ASSERT(n > 0);
    float s = 0 + .5 * lw;
    DvzGraphicsSegmentVertex vertex = {0};
    for (uint32_t i = 0; i < n; i++)
    {
        // TODO: transformation
        x = dvz_prop_item(tick_prop, i);
        ASSERT(x != NULL);

        _tick_shift(i, n, s, tick_length, level, coord, vertex.shift);
        _tick_pos(*x, level, coord, P0, P1);

        glm_vec3_copy(P0, vertex.P0);
        glm_vec3_copy(P1, vertex.P1);
        memcpy(vertex.color, color, sizeof(cvec4));
        vertex.cap0 = vertex.cap1 = cap;
        vertex.linewidth = lw;
        vertex.transform = interact_axis;
        dvz_graphics_append(data, &vertex);
    }
}

static void _visual_axes_2D_bake(DvzVisual* visual, DvzVisualDataEvent ev)
{
    ASSERT(visual != NULL);
    DvzCanvas* canvas = visual->canvas;

    // Data sources.
    DvzSource* seg_vert_src = dvz_source_get(visual, DVZ_SOURCE_TYPE_VERTEX, 0);
    DvzSource* seg_index_src = dvz_source_get(visual, DVZ_SOURCE_TYPE_INDEX, 0);
    DvzSource* text_vert_src = dvz_source_get(visual, DVZ_SOURCE_TYPE_VERTEX, 1);

    // HACK: mark the index buffer to be updated.
    seg_index_src->obj.request = DVZ_VISUAL_REQUEST_UPLOAD;

    // Count the total number of segments.
    // NOTE: the number of segments is determined by the POS prop.
    uint32_t count = _count_prop_items(visual, 1, (DvzPropType[]){DVZ_PROP_POS}, 4);
    if (count == 0)
        return;

    // Segment graphics.
    // -----------------

    DvzGraphicsData seg_data =
        dvz_graphics_data(visual->graphics[0], &seg_vert_src->arr, &seg_index_src->arr, visual);
    dvz_graphics_alloc(&seg_data, count);

    // Visual coordinate.
    DvzAxisCoord coord = (DvzAxisCoord)(visual->flags & 0x1);
    ASSERT(coord < 2);

    // Params
    cvec4 color = {0};
    float lw = 0;

    float tick_length_minor = 0, tick_length_major = 0;
    PARAM(float, tick_length_minor, LENGTH, DVZ_AXES_LEVEL_MINOR)
    PARAM(float, tick_length_major, LENGTH, DVZ_AXES_LEVEL_MAJOR)
    DPI_SCALE(tick_length_minor)
    DPI_SCALE(tick_length_major)

    vec2 tick_length = {tick_length_minor, tick_length_major};

    DvzProp* prop = NULL;
    uint32_t tick_count = 0;
    int flags = ((visual->flags >> 2) & 0x0003);
    bool hide_minor = flags & 0x1;
    bool hide_grid = flags & 0x2;

    for (uint32_t level = 0; level < DVZ_AXES_LEVEL_COUNT; level++)
    {
        // Take the tick positions.
        prop = dvz_prop_get(visual, DVZ_PROP_POS, level);
        ASSERT(prop != NULL);
        tick_count = prop->arr_orig.item_count; // number of ticks for this level.
        if (tick_count == 0)
            continue;
        ASSERT(tick_count > 0);

        PARAM(cvec4, color, COLOR, level)
        PARAM(float, lw, LINE_WIDTH, level)
        DPI_SCALE(lw)

        // Hide minor and/or grid depending on the visual flags.
        if ((level == DVZ_AXES_LEVEL_MINOR && hide_minor) ||
            (level == DVZ_AXES_LEVEL_GRID && hide_grid))
            color[3] = 0;

        _add_ticks(prop, &seg_data, (DvzAxisLevel)level, coord, color, lw, tick_length);
    }

    // Labels: one for each major tick.
    DvzGraphicsData text_data =
        dvz_graphics_data(visual->graphics[1], &text_vert_src->arr, NULL, visual);

    // Text prop.
    DvzArray* arr_text =
        _prop_array(dvz_prop_get(visual, DVZ_PROP_TEXT, 0), DVZ_PROP_ARRAY_DEFAULT);
    ASSERT(prop != NULL);

    // Major tick prop.
    DvzProp* prop_major = dvz_prop_get(visual, DVZ_PROP_POS, DVZ_AXES_LEVEL_MAJOR);
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
    dvz_graphics_alloc(&text_data, count_chars);

    char* text = NULL;
    DvzGraphicsTextItem str_item = {0};
    double* x = NULL;
    vec3 P = {0};
    float font_size = 0;

    PARAM(float, font_size, TEXT_SIZE, 0)
    DPI_SCALE(font_size)

    if (coord == DVZ_AXES_COORD_X)
    {
        str_item.vertex.anchor[1] = 1;
        str_item.vertex.shift[1] = -10;
    }
    else if (coord == DVZ_AXES_COORD_Y)
    {
        str_item.vertex.anchor[0] = -1;
        str_item.vertex.shift[0] = -10;
    }

    // Text color.
    PARAM(cvec4, str_item.vertex.color, COLOR, 4)

    for (uint32_t i = 0; i < n_text; i++)
    {
        // Add text.
        text = ((char**)arr_text->data)[i];
        ASSERT(text != NULL);
        ASSERT(strlen(text) > 0);
        str_item.font_size = font_size;
        str_item.string = text;

        // Position of the text corresponds to position of the major tick.
        x = dvz_prop_item(prop_major, i);
        ASSERT(x != NULL);
        _tick_pos(*x, DVZ_AXES_LEVEL_MAJOR, coord, str_item.vertex.pos, P);

        dvz_graphics_append(&text_data, &str_item);
    }
}

static void _visual_axes_2D(DvzVisual* visual)
{
    ASSERT(visual != NULL);
    DvzCanvas* canvas = visual->canvas;
    ASSERT(canvas != NULL);
    DvzProp* prop = NULL;

    // Graphics.
    dvz_visual_graphics(visual, dvz_graphics_builtin(canvas, DVZ_GRAPHICS_SEGMENT, visual->flags));
    dvz_visual_graphics(visual, dvz_graphics_builtin(canvas, DVZ_GRAPHICS_TEXT, visual->flags));

    // Segment graphics: sources.
    {
        // Vertex buffer.
        dvz_visual_source(
            visual, DVZ_SOURCE_TYPE_VERTEX, 0, DVZ_PIPELINE_GRAPHICS, 0, //
            0, sizeof(DvzGraphicsSegmentVertex), 0);

        // Index buffer.
        dvz_visual_source(
            visual, DVZ_SOURCE_TYPE_INDEX, 0, DVZ_PIPELINE_GRAPHICS, 0, //
            0, sizeof(DvzIndex), 0);
    }

    // Text graphics: sources.
    {
        // Vertex buffer.
        dvz_visual_source(
            visual, DVZ_SOURCE_TYPE_VERTEX, 1, DVZ_PIPELINE_GRAPHICS, 1, //
            0, sizeof(DvzGraphicsTextVertex), 0);

        // Parameters.
        dvz_visual_source(
            visual, DVZ_SOURCE_TYPE_PARAM, 0, DVZ_PIPELINE_GRAPHICS, 1, //
            DVZ_USER_BINDING, sizeof(DvzGraphicsTextParams), 0);

        // Font atlas texture.
        dvz_visual_source(
            visual, DVZ_SOURCE_TYPE_FONT_ATLAS, 0, DVZ_PIPELINE_GRAPHICS, 1, //
            DVZ_USER_BINDING + 1, sizeof(cvec4), 0);
    }

    // Uniform buffers.
    // Set MVP and Viewport sources for pipeline #0.
    _common_sources(visual);

    // Set MVP and Viewport sources for pipeline #1:

    // Binding #0: uniform buffer MVP, shared from pipeline #0
    dvz_visual_source( //
        visual, DVZ_SOURCE_TYPE_MVP, 1, DVZ_PIPELINE_GRAPHICS, 1, 0, sizeof(DvzMVP),
        DVZ_SOURCE_FLAG_MAPPABLE);
    // Share the MVP source with the second graphics pipeline.
    dvz_visual_source_share(visual, DVZ_SOURCE_TYPE_MVP, 0, 1);

    // Binding #1: viewport source, NOT shared, as we want different clipping for both graphics.
    dvz_visual_source(
        visual, DVZ_SOURCE_TYPE_VIEWPORT, 1, DVZ_PIPELINE_GRAPHICS, 1, 1, sizeof(DvzViewport), 0);

    // Segment graphics props.
    {
        vec4 line_widths = {
            DVZ_DEFAULT_AXES_LINE_WIDTH_MINOR, DVZ_DEFAULT_AXES_LINE_WIDTH_MAJOR,
            DVZ_DEFAULT_AXES_LINE_WIDTH_GRID, DVZ_DEFAULT_AXES_LINE_WIDTH_LIM};
        for (uint32_t level = 0; level < DVZ_AXES_LEVEL_COUNT; level++)
        {
            // xticks
            prop = dvz_visual_prop(
                visual, DVZ_PROP_POS, level, DVZ_DTYPE_DOUBLE, DVZ_SOURCE_TYPE_VERTEX, 0);

            // color
            prop = dvz_visual_prop(
                visual, DVZ_PROP_COLOR, level, DVZ_DTYPE_CVEC4, DVZ_SOURCE_TYPE_VERTEX, 0);
            dvz_visual_prop_default(
                prop, level == 2 ? DVZ_DEFAULT_AXES_COLOR_GRAY : DVZ_DEFAULT_AXES_COLOR_BLACK);

            // line width
            prop = dvz_visual_prop(
                visual, DVZ_PROP_LINE_WIDTH, level, DVZ_DTYPE_FLOAT, DVZ_SOURCE_TYPE_VERTEX, 0);
            dvz_visual_prop_default(prop, &line_widths[level]);
        }

        // minor tick length
        prop = dvz_visual_prop(
            visual, DVZ_PROP_LENGTH, DVZ_AXES_LEVEL_MINOR, DVZ_DTYPE_FLOAT, DVZ_SOURCE_TYPE_VERTEX,
            0);
        dvz_visual_prop_default(prop, (float[]){DVZ_DEFAULT_AXES_TICK_LENGTH_MINOR});

        // major tick length
        prop = dvz_visual_prop(
            visual, DVZ_PROP_LENGTH, DVZ_AXES_LEVEL_MAJOR, DVZ_DTYPE_FLOAT, DVZ_SOURCE_TYPE_VERTEX,
            0);
        dvz_visual_prop_default(prop, (float[]){DVZ_DEFAULT_AXES_TICK_LENGTH_MAJOR});

        // tick h margin
        // dvz_visual_prop(visual, DVZ_PROP_MARGIN, 0, DVZ_DTYPE_FLOAT, DVZ_SOURCE_TYPE_VERTEX, 0);
        // TODO: default
    }

    _common_props(visual); // segment graphics

    // Text graphics props.
    {
        // tick text
        prop = dvz_visual_prop(visual, DVZ_PROP_TEXT, 0, DVZ_DTYPE_STR, DVZ_SOURCE_TYPE_VERTEX, 1);

        // tick text size
        prop = dvz_visual_prop(
            visual, DVZ_PROP_TEXT_SIZE, 0, DVZ_DTYPE_FLOAT, DVZ_SOURCE_TYPE_VERTEX, 1);
        dvz_visual_prop_default(prop, (float[]){DVZ_DEFAULT_AXES_FONT_SIZE});

        // text color
        prop =
            dvz_visual_prop(visual, DVZ_PROP_COLOR, 4, DVZ_DTYPE_CVEC4, DVZ_SOURCE_TYPE_VERTEX, 1);
        dvz_visual_prop_default(prop, DVZ_DEFAULT_AXES_COLOR_BLACK);

        // Viewport.
        prop = dvz_visual_prop(
            visual, DVZ_PROP_VIEWPORT, 1, DVZ_DTYPE_CUSTOM, DVZ_SOURCE_TYPE_VIEWPORT, 1);
        dvz_visual_prop_size(prop, sizeof(DvzViewport));
        dvz_visual_prop_copy(prop, 0, 0, DVZ_ARRAY_COPY_SINGLE, 1);
    }

    dvz_visual_callback_bake(visual, _visual_axes_2D_bake);
}



/*************************************************************************************************/
/*************************************************************************************************/
/*  3D visuals                                                                                   */
/*************************************************************************************************/
/*************************************************************************************************/



/*************************************************************************************************/
/*  Mesh                                                                                         */
/*************************************************************************************************/

static void _mesh_bake(DvzVisual* visual, DvzVisualDataEvent ev)
{
    // Take the color prop and override the tex coords and alpha if needed.
    DvzProp* prop_color = dvz_prop_get(visual, DVZ_PROP_COLOR, 0);         // cvec4
    DvzProp* prop_texcoords = dvz_prop_get(visual, DVZ_PROP_TEXCOORDS, 0); // vec2
    DvzProp* prop_alpha = dvz_prop_get(visual, DVZ_PROP_ALPHA, 0);         // uint8_t

    DvzArray* arr_color = _prop_array(prop_color, DVZ_PROP_ARRAY_DEFAULT);
    DvzArray* arr_texcoords = _prop_array(prop_texcoords, DVZ_PROP_ARRAY_DEFAULT);
    DvzArray* arr_alpha = _prop_array(prop_alpha, DVZ_PROP_ARRAY_DEFAULT);

    // If colors have been specified, we need to override texcoords.
    uint32_t N = arr_color->item_count;
    if (N > 0)
    {
        dvz_array_resize(arr_texcoords, N);
        dvz_array_resize(arr_alpha, N);

        ASSERT(arr_texcoords->item_count == N);
        ASSERT(arr_alpha->item_count == N);

        cvec4* color = NULL;
        cvec3* rgb = NULL;
        vec2* uv = NULL;
        uint8_t* alpha = NULL;

        for (uint32_t i = 0; i < N; i++)
        {
            color = dvz_array_item(arr_color, i);
            uv = dvz_array_item(arr_texcoords, i);
            alpha = dvz_array_item(arr_alpha, i);

            // Copy the color RGB components to the UV tex coords by packing 3 char into a float
            rgb = (cvec3*)color;
            dvz_colormap_packuv(*rgb, *uv);

            // Copy the alpha component to the ALPHA prop
            *alpha = (*color)[3];
        }
    }

    // The default baking takes care of the other props beyond color/texcoords: normal, params,
    // etc.
    _default_visual_bake(visual, ev);

    // Check the normal data.
    DvzArray* arr_vertex = dvz_source_array(visual, DVZ_SOURCE_TYPE_VERTEX, 0);
    DvzArray* arr_index = dvz_source_array(visual, DVZ_SOURCE_TYPE_INDEX, 0);

    // Check if the first normal is 00
    vec4* normal = &((DvzGraphicsMeshVertex*)dvz_array_item(arr_vertex, 0))->normal;
    if (normal[0][0] == 0 && normal[0][1] == 0 && normal[0][2] == 0)
    {
        // Compute the normal from the faces.
        DvzMesh mesh = {0};
        mesh.vertices = *arr_vertex; // (DvzGraphicsMeshVertex*)arr_vertex->data;
        mesh.indices = *arr_index;   // (DvzIndex*)arr_index->data;
        dvz_mesh_normals(&mesh);
    }
    // The first normal should no longer be empty.
    ASSERT(!(normal[0][0] == 0 && normal[0][1] == 0 && normal[0][2] == 0));
}

static void _visual_mesh(DvzVisual* visual)
{
    ASSERT(visual != NULL);
    DvzCanvas* canvas = visual->canvas;
    ASSERT(canvas != NULL);
    DvzProp* prop = NULL;

    // Graphics.
    dvz_visual_graphics(visual, dvz_graphics_builtin(canvas, DVZ_GRAPHICS_MESH, visual->flags));

    // Sources
    dvz_visual_source(                                               // vertex buffer
        visual, DVZ_SOURCE_TYPE_VERTEX, 0, DVZ_PIPELINE_GRAPHICS, 0, //
        0, sizeof(DvzGraphicsMeshVertex), 0);                        //

    dvz_visual_source(                                              // index buffer
        visual, DVZ_SOURCE_TYPE_INDEX, 0, DVZ_PIPELINE_GRAPHICS, 0, //
        0, sizeof(DvzIndex), 0);                                    //

    _common_sources(visual); // common sources

    dvz_visual_source(                                              // params
        visual, DVZ_SOURCE_TYPE_PARAM, 0, DVZ_PIPELINE_GRAPHICS, 0, //
        DVZ_USER_BINDING, sizeof(DvzGraphicsMeshParams), 0);        //

    for (uint32_t i = 0; i < 4; i++)                                    // texture sources
        dvz_visual_source(                                              //
            visual, DVZ_SOURCE_TYPE_IMAGE, i, DVZ_PIPELINE_GRAPHICS, 0, //
            DVZ_USER_BINDING + i + 1, sizeof(cvec4), 0);                //

    // Props:

    // Vertex pos.
    prop = dvz_visual_prop(visual, DVZ_PROP_POS, 0, DVZ_DTYPE_DVEC3, DVZ_SOURCE_TYPE_VERTEX, 0);
    dvz_visual_prop_cast(
        prop, 0, offsetof(DvzGraphicsMeshVertex, pos), DVZ_DTYPE_VEC3, DVZ_ARRAY_COPY_SINGLE, 1);

    // Vertex normal.
    prop = dvz_visual_prop(visual, DVZ_PROP_NORMAL, 0, DVZ_DTYPE_VEC3, DVZ_SOURCE_TYPE_VERTEX, 0);
    dvz_visual_prop_copy(
        prop, 0, offsetof(DvzGraphicsMeshVertex, normal), DVZ_ARRAY_COPY_SINGLE, 1);

    // Vertex tex coords.
    prop =
        dvz_visual_prop(visual, DVZ_PROP_TEXCOORDS, 0, DVZ_DTYPE_VEC2, DVZ_SOURCE_TYPE_VERTEX, 0);
    dvz_visual_prop_copy(prop, 0, offsetof(DvzGraphicsMeshVertex, uv), DVZ_ARRAY_COPY_SINGLE, 1);

    // Vertex color: override tex coords by packing 3 bytes into a float
    prop = dvz_visual_prop(visual, DVZ_PROP_COLOR, 0, DVZ_DTYPE_CVEC4, DVZ_SOURCE_TYPE_VERTEX, 0);

    // Vertex alpha.
    prop = dvz_visual_prop(visual, DVZ_PROP_ALPHA, 0, DVZ_DTYPE_CHAR, DVZ_SOURCE_TYPE_VERTEX, 0);
    dvz_visual_prop_copy(
        prop, 0, offsetof(DvzGraphicsMeshVertex, alpha), DVZ_ARRAY_COPY_SINGLE, 1);
    uint8_t alpha = 255;
    dvz_visual_prop_default(prop, &alpha);

    // Index.
    prop = dvz_visual_prop(visual, DVZ_PROP_INDEX, 0, DVZ_DTYPE_UINT, DVZ_SOURCE_TYPE_INDEX, 0);
    dvz_visual_prop_copy(prop, 0, 0, DVZ_ARRAY_COPY_SINGLE, 1);

    // Common props.
    _common_props(visual);

    // Params.
    // Default values.
    DvzGraphicsMeshParams params = default_graphics_mesh_params(DVZ_CAMERA_EYE);

    // Light positions.
    prop =
        dvz_visual_prop(visual, DVZ_PROP_LIGHT_POS, 0, DVZ_DTYPE_MAT4, DVZ_SOURCE_TYPE_PARAM, 0);
    dvz_visual_prop_copy(
        prop, 0, offsetof(DvzGraphicsMeshParams, lights_pos_0), DVZ_ARRAY_COPY_SINGLE, 1);
    dvz_visual_prop_default(prop, &params.lights_pos_0);

    // Light params.
    prop = dvz_visual_prop(
        visual, DVZ_PROP_LIGHT_PARAMS, 0, DVZ_DTYPE_MAT4, DVZ_SOURCE_TYPE_PARAM, 0);
    dvz_visual_prop_copy(
        prop, 1, offsetof(DvzGraphicsMeshParams, lights_params_0), DVZ_ARRAY_COPY_SINGLE, 1);
    dvz_visual_prop_default(prop, &params.lights_params_0);

    // Texture coefficients.
    prop = dvz_visual_prop(visual, DVZ_PROP_TEXCOEFS, 0, DVZ_DTYPE_VEC4, DVZ_SOURCE_TYPE_PARAM, 0);
    dvz_visual_prop_copy(
        prop, 3, offsetof(DvzGraphicsMeshParams, tex_coefs), DVZ_ARRAY_COPY_SINGLE, 1);
    dvz_visual_prop_default(prop, &params.tex_coefs);

    // Clipping coefficients.
    prop = dvz_visual_prop(visual, DVZ_PROP_CLIP, 0, DVZ_DTYPE_VEC4, DVZ_SOURCE_TYPE_PARAM, 0);
    dvz_visual_prop_copy(
        prop, 4, offsetof(DvzGraphicsMeshParams, clip_coefs), DVZ_ARRAY_COPY_SINGLE, 1);

    // // Texture props.
    // for (uint32_t i = 0; i < 4; i++)
    //     dvz_visual_prop(visual, DVZ_PROP_IMAGE, i, DVZ_DTYPE_UINT, DVZ_SOURCE_TYPE_IMAGE, i);

    dvz_visual_callback_bake(visual, _mesh_bake);
}



/*************************************************************************************************/
/*  Volume                                                                                       */
/*************************************************************************************************/

static void _visual_volume_bake(DvzVisual* visual, DvzVisualDataEvent ev)
{
    ASSERT(visual != NULL);

    // Vertex buffer source.
    DvzSource* source = dvz_source_get(visual, DVZ_SOURCE_TYPE_VERTEX, 0);
    ASSERT(source->arr.item_size == sizeof(DvzGraphicsVolumeVertex));

    // Get props.
    DvzProp* pos0 = dvz_prop_get(visual, DVZ_PROP_POS, 0);
    DvzProp* pos1 = dvz_prop_get(visual, DVZ_PROP_POS, 1);

    ASSERT(pos0 != NULL);
    ASSERT(pos1 != NULL);

    // Number of images
    uint32_t img_count = dvz_prop_size(pos0);
    if (img_count == 0)
        return;
    ASSERT(dvz_prop_size(pos1) == img_count);

    // Graphics data.
    DvzGraphicsData data = dvz_graphics_data(visual->graphics[0], &source->arr, NULL, NULL);
    dvz_graphics_alloc(&data, img_count);

    DvzGraphicsVolumeItem item = {0};
    for (uint32_t i = 0; i < img_count; i++)
    {
        _vec3_cast((const dvec3*)dvz_prop_item(pos0, i), &item.pos0);
        _vec3_cast((const dvec3*)dvz_prop_item(pos1, i), &item.pos1);

        dvz_graphics_append(&data, &item);
    }
}

static void _visual_volume(DvzVisual* visual)
{
    ASSERT(visual != NULL);
    DvzCanvas* canvas = visual->canvas;
    ASSERT(canvas != NULL);
    DvzProp* prop = NULL;

    // Graphics.
    dvz_visual_graphics(visual, dvz_graphics_builtin(canvas, DVZ_GRAPHICS_VOLUME, visual->flags));

    // Sources
    dvz_visual_source(                                               // vertex buffer
        visual, DVZ_SOURCE_TYPE_VERTEX, 0, DVZ_PIPELINE_GRAPHICS, 0, //
        0, sizeof(DvzGraphicsVolumeVertex), 0);                      //

    _common_sources(visual); // common sources

    dvz_visual_source(                                              // params
        visual, DVZ_SOURCE_TYPE_PARAM, 0, DVZ_PIPELINE_GRAPHICS, 0, //
        DVZ_USER_BINDING, sizeof(DvzGraphicsVolumeParams), 0);      //

    // dvz_visual_source(                                                      // colormap texture
    //     visual, DVZ_SOURCE_TYPE_COLOR_TEXTURE, 0, DVZ_PIPELINE_GRAPHICS, 0, //
    //     DVZ_USER_BINDING + 1, 0, 0);                                        //

    dvz_visual_source(                                               // 3D volume with density
        visual, DVZ_SOURCE_TYPE_VOLUME, 0, DVZ_PIPELINE_GRAPHICS, 0, //
        DVZ_USER_BINDING + 1, 0, 0);                                 //

    dvz_visual_source(                                               // 3D volume with vox color
        visual, DVZ_SOURCE_TYPE_VOLUME, 1, DVZ_PIPELINE_GRAPHICS, 0, //
        DVZ_USER_BINDING + 2, 0, 0);                                 //

    dvz_visual_source(                                                 // transfer function
        visual, DVZ_SOURCE_TYPE_TRANSFER, 0, DVZ_PIPELINE_GRAPHICS, 0, //
        DVZ_USER_BINDING + 3, 0, 0);                                   //

    // Props:

    // Point positions.
    // Top left, top right, bottom right, bottom left
    for (uint32_t i = 0; i < 2; i++)
        dvz_visual_prop(visual, DVZ_PROP_POS, i, DVZ_DTYPE_DVEC3, DVZ_SOURCE_TYPE_VERTEX, 0);

    // Common props.
    _common_props(visual);


    // Params.

    // Box size.
    prop = dvz_visual_prop(visual, DVZ_PROP_LENGTH, 0, DVZ_DTYPE_VEC3, DVZ_SOURCE_TYPE_PARAM, 0);
    dvz_visual_prop_copy(
        prop, 0, offsetof(DvzGraphicsVolumeParams, box_size), DVZ_ARRAY_COPY_SINGLE, 1);
    dvz_visual_prop_default(prop, (vec3){2, 2, 2});

    // 3D texture coordinates of the first corner.
    prop =
        dvz_visual_prop(visual, DVZ_PROP_TEXCOORDS, 0, DVZ_DTYPE_VEC3, DVZ_SOURCE_TYPE_PARAM, 0);
    dvz_visual_prop_copy(
        prop, 1, offsetof(DvzGraphicsVolumeParams, uvw0), DVZ_ARRAY_COPY_SINGLE, 1);
    dvz_visual_prop_default(prop, (vec3[]){{0, 0, 0}});

    // 3D texture coordinates of the second corner.
    prop =
        dvz_visual_prop(visual, DVZ_PROP_TEXCOORDS, 1, DVZ_DTYPE_VEC3, DVZ_SOURCE_TYPE_PARAM, 0);
    dvz_visual_prop_copy(
        prop, 2, offsetof(DvzGraphicsVolumeParams, uvw1), DVZ_ARRAY_COPY_SINGLE, 1);
    dvz_visual_prop_default(prop, (vec3[]){{1, 1, 1}});

    // Clip vector.
    prop = dvz_visual_prop(visual, DVZ_PROP_CLIP, 0, DVZ_DTYPE_VEC4, DVZ_SOURCE_TYPE_PARAM, 0);
    dvz_visual_prop_copy(
        prop, 3, offsetof(DvzGraphicsVolumeParams, clip), DVZ_ARRAY_COPY_SINGLE, 1);
    // If both values are equal, disable the transfer function on the GPU.
    dvz_visual_prop_default(prop, (vec4){0, 0, 0, 0});

    // Transfer xrange.
    prop =
        dvz_visual_prop(visual, DVZ_PROP_TRANSFER_X, 0, DVZ_DTYPE_VEC2, DVZ_SOURCE_TYPE_PARAM, 0);
    dvz_visual_prop_copy(
        prop, 4, offsetof(DvzGraphicsVolumeParams, transfer_xrange), DVZ_ARRAY_COPY_SINGLE, 1);
    // If both values are equal, disable the transfer function on the GPU.
    dvz_visual_prop_default(prop, (vec2){0, 0});

    // // Colormap value.
    // prop = dvz_visual_prop(visual, DVZ_PROP_COLORMAP, 0, DVZ_DTYPE_INT, DVZ_SOURCE_TYPE_PARAM,
    // 0); dvz_visual_prop_copy(
    //     prop, 5, offsetof(DvzGraphicsVolumeParams, cmap), DVZ_ARRAY_COPY_SINGLE, 1);
    // dvz_visual_prop_default(prop, (DvzColormap[]){DVZ_CMAP_BONE});

    // Color coefficient.
    prop = dvz_visual_prop(visual, DVZ_PROP_SCALE, 0, DVZ_DTYPE_FLOAT, DVZ_SOURCE_TYPE_PARAM, 0);
    dvz_visual_prop_copy(
        prop, 5, offsetof(DvzGraphicsVolumeParams, color_coef), DVZ_ARRAY_COPY_SINGLE, 1);
    dvz_visual_prop_default(prop, (float[]){.01});


    // Baking function.
    dvz_visual_callback_bake(visual, _visual_volume_bake);
}



/*************************************************************************************************/
/*  Volume image                                                                                 */
/*************************************************************************************************/

static void _visual_volume_slice_bake(DvzVisual* visual, DvzVisualDataEvent ev)
{
    ASSERT(visual != NULL);

    // Vertex buffer source.
    DvzSource* source = dvz_source_get(visual, DVZ_SOURCE_TYPE_VERTEX, 0);
    ASSERT(source->arr.item_size == sizeof(DvzGraphicsVolumeSliceVertex));

    // Get props.
    DvzProp* pos0 = dvz_prop_get(visual, DVZ_PROP_POS, 0);
    DvzProp* pos1 = dvz_prop_get(visual, DVZ_PROP_POS, 1);
    DvzProp* pos2 = dvz_prop_get(visual, DVZ_PROP_POS, 2);
    DvzProp* pos3 = dvz_prop_get(visual, DVZ_PROP_POS, 3);

    DvzProp* uvw0 = dvz_prop_get(visual, DVZ_PROP_TEXCOORDS, 0);
    DvzProp* uvw1 = dvz_prop_get(visual, DVZ_PROP_TEXCOORDS, 1);
    DvzProp* uvw2 = dvz_prop_get(visual, DVZ_PROP_TEXCOORDS, 2);
    DvzProp* uvw3 = dvz_prop_get(visual, DVZ_PROP_TEXCOORDS, 3);

    ASSERT(pos0 != NULL);
    ASSERT(pos1 != NULL);
    ASSERT(pos2 != NULL);
    ASSERT(pos3 != NULL);

    ASSERT(uvw0 != NULL);
    ASSERT(uvw1 != NULL);
    ASSERT(uvw2 != NULL);
    ASSERT(uvw3 != NULL);

    // Number of images
    uint32_t img_count = dvz_prop_size(pos0);
    if (img_count == 0)
        return;
    ASSERT(dvz_prop_size(pos1) == img_count);
    ASSERT(dvz_prop_size(pos2) == img_count);
    ASSERT(dvz_prop_size(pos3) == img_count);

    // Graphics data.
    DvzGraphicsData data = dvz_graphics_data(visual->graphics[0], &source->arr, NULL, NULL);
    dvz_graphics_alloc(&data, img_count);

    DvzGraphicsVolumeSliceItem item = {0};
    for (uint32_t i = 0; i < img_count; i++)
    {
        _vec3_cast((const dvec3*)dvz_prop_item(pos0, i), &item.pos0);
        _vec3_cast((const dvec3*)dvz_prop_item(pos1, i), &item.pos1);
        _vec3_cast((const dvec3*)dvz_prop_item(pos2, i), &item.pos2);
        _vec3_cast((const dvec3*)dvz_prop_item(pos3, i), &item.pos3);

        // memcpy(&item.pos0, dvz_prop_item(pos0, i), sizeof(vec3));
        // memcpy(&item.pos1, dvz_prop_item(pos1, i), sizeof(vec3));
        // memcpy(&item.pos2, dvz_prop_item(pos2, i), sizeof(vec3));
        // memcpy(&item.pos3, dvz_prop_item(pos3, i), sizeof(vec3));

        memcpy(&item.uvw0, dvz_prop_item(uvw0, i), sizeof(vec3));
        memcpy(&item.uvw1, dvz_prop_item(uvw1, i), sizeof(vec3));
        memcpy(&item.uvw2, dvz_prop_item(uvw2, i), sizeof(vec3));
        memcpy(&item.uvw3, dvz_prop_item(uvw3, i), sizeof(vec3));

        dvz_graphics_append(&data, &item);
    }
}

static void _visual_volume_slice(DvzVisual* visual)
{
    ASSERT(visual != NULL);
    DvzCanvas* canvas = visual->canvas;
    ASSERT(canvas != NULL);
    DvzProp* prop = NULL;

    // TODO: customizable dtype for the image

    // Graphics.
    dvz_visual_graphics(
        visual, dvz_graphics_builtin(canvas, DVZ_GRAPHICS_VOLUME_SLICE, visual->flags));

    // Sources
    dvz_visual_source(                                               // vertex buffer
        visual, DVZ_SOURCE_TYPE_VERTEX, 0, DVZ_PIPELINE_GRAPHICS, 0, //
        0, sizeof(DvzGraphicsVolumeSliceVertex), 0);                 //

    _common_sources(visual); // common sources

    dvz_visual_source(                                              // params
        visual, DVZ_SOURCE_TYPE_PARAM, 0, DVZ_PIPELINE_GRAPHICS, 0, //
        DVZ_USER_BINDING, sizeof(DvzGraphicsVolumeSliceParams), 0); //

    dvz_visual_source(                                                      // colormap texture
        visual, DVZ_SOURCE_TYPE_COLOR_TEXTURE, 0, DVZ_PIPELINE_GRAPHICS, 0, //
        DVZ_USER_BINDING + 1, sizeof(uint8_t), 0);                          //

    dvz_visual_source(                                               // texture source
        visual, DVZ_SOURCE_TYPE_VOLUME, 0, DVZ_PIPELINE_GRAPHICS, 0, //
        DVZ_USER_BINDING + 2, sizeof(uint8_t), 0);                   //

    // Props:

    // Point positions.
    // Top left, top right, bottom right, bottom left
    for (uint32_t i = 0; i < 4; i++)
        dvz_visual_prop(visual, DVZ_PROP_POS, i, DVZ_DTYPE_DVEC3, DVZ_SOURCE_TYPE_VERTEX, 0);

    // Tex coords.
    for (uint32_t i = 0; i < 4; i++)
        dvz_visual_prop(visual, DVZ_PROP_TEXCOORDS, i, DVZ_DTYPE_VEC3, DVZ_SOURCE_TYPE_VERTEX, 0);

    // Common props.
    _common_props(visual);

    // Params.

    // Colormap transfer function.
    vec4 vec = {0, .333333, .666666, 1};
    prop = dvz_visual_prop(                                                                 //
        visual, DVZ_PROP_TRANSFER_X, 0, DVZ_DTYPE_VEC4, DVZ_SOURCE_TYPE_PARAM, 0);          //
    dvz_visual_prop_copy(                                                                   //
        prop, 0, offsetof(DvzGraphicsVolumeSliceParams, x_cmap), DVZ_ARRAY_COPY_SINGLE, 1); //
    dvz_visual_prop_default(prop, vec);                                                     //

    prop = dvz_visual_prop(                                                                 //
        visual, DVZ_PROP_TRANSFER_Y, 0, DVZ_DTYPE_VEC4, DVZ_SOURCE_TYPE_PARAM, 0);          //
    dvz_visual_prop_copy(                                                                   //
        prop, 1, offsetof(DvzGraphicsVolumeSliceParams, y_cmap), DVZ_ARRAY_COPY_SINGLE, 1); //
    dvz_visual_prop_default(prop, vec);                                                     //

    // Alpha transfer function.
    prop = dvz_visual_prop(                                                                  //
        visual, DVZ_PROP_TRANSFER_X, 1, DVZ_DTYPE_VEC4, DVZ_SOURCE_TYPE_PARAM, 0);           //
    dvz_visual_prop_copy(                                                                    //
        prop, 2, offsetof(DvzGraphicsVolumeSliceParams, x_alpha), DVZ_ARRAY_COPY_SINGLE, 1); //
    dvz_visual_prop_default(prop, vec);                                                      //

    prop = dvz_visual_prop(                                                                  //
        visual, DVZ_PROP_TRANSFER_Y, 1, DVZ_DTYPE_VEC4, DVZ_SOURCE_TYPE_PARAM, 0);           //
    dvz_visual_prop_copy(                                                                    //
        prop, 3, offsetof(DvzGraphicsVolumeSliceParams, y_alpha), DVZ_ARRAY_COPY_SINGLE, 1); //
    dvz_visual_prop_default(prop, vec);                                                      //

    // Colormap value.
    prop = dvz_visual_prop(visual, DVZ_PROP_COLORMAP, 0, DVZ_DTYPE_INT, DVZ_SOURCE_TYPE_PARAM, 0);
    dvz_visual_prop_copy(
        prop, 4, offsetof(DvzGraphicsVolumeSliceParams, cmap), DVZ_ARRAY_COPY_SINGLE, 1);
    dvz_visual_prop_default(prop, (DvzColormap[]){DVZ_CMAP_BONE}); //

    // Scaling factor.
    prop = dvz_visual_prop(visual, DVZ_PROP_SCALE, 0, DVZ_DTYPE_FLOAT, DVZ_SOURCE_TYPE_PARAM, 0);
    dvz_visual_prop_copy(
        prop, 5, offsetof(DvzGraphicsVolumeSliceParams, scale), DVZ_ARRAY_COPY_SINGLE, 1);
    dvz_visual_prop_default(prop, (float[]){1}); //



    // // Colormap texture prop.
    // dvz_visual_prop(
    //     visual, DVZ_PROP_COLOR_TEXTURE, 0, DVZ_DTYPE_CHAR, DVZ_SOURCE_TYPE_COLOR_TEXTURE, 0);

    // // 3D texture prop.
    // dvz_visual_prop(visual, DVZ_PROP_VOLUME, 0, DVZ_DTYPE_CHAR, DVZ_SOURCE_TYPE_VOLUME, 0);

    // Baking function.
    dvz_visual_callback_bake(visual, _visual_volume_slice_bake);
    // dvz_visual_callback_transform(visual, _visual_normalize);
}



/*************************************************************************************************/
/*  Main function                                                                                */
/*************************************************************************************************/

void dvz_visual_builtin(DvzVisual* visual, DvzVisualType type, int flags)
{
    ASSERT(visual != NULL);
    visual->flags = flags;
    switch (type)
    {

    case DVZ_VISUAL_POINT:
        _visual_point(visual);
        break;

    case DVZ_VISUAL_LINE:
        _visual_line(visual);
        break;

    case DVZ_VISUAL_LINE_STRIP:
        _visual_line_strip(visual);
        break;

    case DVZ_VISUAL_TRIANGLE:
        _visual_triangle(visual);
        break;

    case DVZ_VISUAL_TRIANGLE_STRIP:
        _visual_triangle_strip(visual);
        break;

    case DVZ_VISUAL_TRIANGLE_FAN:
        _visual_triangle_fan(visual);
        break;

    case DVZ_VISUAL_RECTANGLE:
        _visual_rectangle(visual);
        break;



    case DVZ_VISUAL_MARKER:
        _visual_marker(visual);
        break;

    case DVZ_VISUAL_POLYGON:
        _visual_polygon(visual);
        break;

    case DVZ_VISUAL_PATH:
        _visual_path(visual);
        break;

    case DVZ_VISUAL_IMAGE:
        _visual_image(visual);
        break;

    case DVZ_VISUAL_IMAGE_CMAP:
        _visual_image_cmap(visual);
        break;

    case DVZ_VISUAL_TEXT:
        _visual_text(visual);
        break;

    case DVZ_VISUAL_AXES_2D:
        _visual_axes_2D(visual);
        break;



    case DVZ_VISUAL_MESH:
        _visual_mesh(visual);
        break;

    case DVZ_VISUAL_VOLUME:
        _visual_volume(visual);
        break;

    case DVZ_VISUAL_VOLUME_SLICE:
        _visual_volume_slice(visual);
        break;


    case DVZ_VISUAL_CUSTOM:
    case DVZ_VISUAL_NONE:
        break;

    default:
        log_error("no builtin visual found for type %d", type);
        break;
    }
}
