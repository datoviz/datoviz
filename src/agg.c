#include "../include/visky/visky.h"

// BEGIN_INCL_NO_WARN
// #include <stb_image.h>
// END_INCL_NO_WARN



/*************************************************************************************************/
/*  Path visual                                                                                  */
/*************************************************************************************************/

const uint32_t path_vertices_per_segment = 4;

static void add_path_point(
    VkyPathVertex* vertices, uint32_t vertex_offset, vec3 p0, vec3 p1, vec2 p2, vec3 p3,
    VkyColorBytes color)
{
    // Repeat the vertices path_vertices_per_segment times for the triangulation occurring in the
    // vertex shader.
    VkyPathVertex* vertex = &vertices[path_vertices_per_segment * vertex_offset];
    for (uint32_t k = 0; k < path_vertices_per_segment; k++)
    {
        // NOTE; the +1 is because of the very first vertex set above.
        vec3_copy(p0, vertex->p0);
        vec3_copy(p1, vertex->p1);
        vec3_copy(p2, vertex->p2);
        vec3_copy(p3, vertex->p3);
        vertex->color = color;
        vertex++;
    }
}

static VkyData vky_path_bake(VkyVisual* visual, VkyData data)
{
    // Determine the actual number of vertices and indices.
    uint32_t path_count = data.item_count;
    uint32_t vertex_count = 0; // NOT multiplied by path_vertices_per_segment
    if (data.items == NULL)
        return data;
    ASSERT(path_count > 0);
    const VkyPathData* paths = (const VkyPathData*)data.items;
    for (uint32_t i = 0; i < path_count; i++)
    {
        ASSERT(paths[i].point_count >= 2);
        vertex_count += paths[i].point_count - 1;
    }

    // Extra vertices to to hide joins between two paths.
    vertex_count += 2 * (path_count - 1);

    ASSERT(vertex_count > 0);

    data.vertex_count = vertex_count * path_vertices_per_segment;
    data.index_count = 0;

    VkyPathVertex* vertices = calloc(data.vertex_count, sizeof(VkyPathVertex));

    uint32_t vertex_offset = 0; // NOT multiplied by path_vertices_per_segment

    vec3 p0, p1, p2, p3;
    int32_t j0, j1, j2, j3;
    vec3* points = NULL;
    int32_t point_count_path = 0;
    VkyColorBytes color;

    for (uint32_t i = 0; i < path_count; i++)
    {
        points = paths[i].points;
        point_count_path = (int32_t)paths[i].point_count;

        // Make the path vertices.
        for (int32_t j = 0; j < point_count_path - 1; j++)
        {
            // Compute p0, p1, p2, p3.
            j0 = j - 1;
            j1 = j;
            j2 = j + 1;
            j3 = j + 2;
            if (paths[i].topology == VKY_PATH_OPEN)
            {
                j0 = j0 < 0 ? 0 : j0;
                j2 = j2 >= point_count_path ? (point_count_path - 1) : j2;
                j3 = j3 >= point_count_path ? (point_count_path - 1) : j3;
            }
            else if (paths[i].topology == VKY_PATH_CLOSED)
            {
                j0 = j0 < 0 ? (point_count_path - 2) : j0;
                j2 = j2 >= point_count_path ? 0 : j2;
                j3 = j3 >= point_count_path ? 1 : j3;
            }

            ASSERT(0 <= j0 && j0 < point_count_path);
            ASSERT(0 <= j1 && j1 < point_count_path);
            ASSERT(0 <= j2 && j2 < point_count_path);
            ASSERT(0 <= j3 && j3 < point_count_path);

            vec3_copy(points[j0], p0);
            vec3_copy(points[j1], p1);
            vec3_copy(points[j2], p2);
            vec3_copy(points[j3], p3);

            color = paths[i].colors[j];

            add_path_point(vertices, vertex_offset, p0, p1, p2, p3, color);
            ASSERT(vertex_offset < vertex_count);
            vertex_offset++;
        }

        // used to hide joins between paths
        if (i < path_count - 1)
        {
            // Add last point of current path.
            vec3_copy(paths[i].points[point_count_path - 1], p0);
            add_path_point(vertices, vertex_offset, p0, p0, p0, p0, (VkyColorBytes){0, 0, 0, 0});
            vertex_offset++;

            // Add first point of next path.
            vec3_copy(paths[i + 1].points[0], p0);
            add_path_point(vertices, vertex_offset, p0, p0, p0, p0, (VkyColorBytes){0, 0, 0, 0});
            vertex_offset++;

        } // end for loop on points in current path
    }     // end for loop on paths
    ASSERT(vertex_offset == vertex_count);

    data.vertices = vertices;
    data.indices = NULL;

    log_trace("finished baking the path data");
    return data;
}

VkyVisual* vky_visual_path(VkyScene* scene, VkyPathParams params)
{
    VkyVisual* visual = vky_create_visual(scene, VKY_VISUAL_PATH);
    VkyCanvas* canvas = scene->canvas;

    // Shaders.
    VkyShaders shaders = vky_create_shaders(canvas->gpu);
    vky_add_shader(&shaders, VK_SHADER_STAGE_VERTEX_BIT, "path.vert.spv");
    vky_add_shader(&shaders, VK_SHADER_STAGE_FRAGMENT_BIT, "path.frag.spv");

    // Vertex layout.
    VkyVertexLayout vertex_layout =
        vky_create_vertex_layout(canvas->gpu, 0, sizeof(VkyPathVertex));
    vky_add_vertex_attribute(
        &vertex_layout, 0, VKY_DEFAULT_VERTEX_FORMAT_POS, offsetof(VkyPathVertex, p0));
    vky_add_vertex_attribute(
        &vertex_layout, 1, VKY_DEFAULT_VERTEX_FORMAT_POS, offsetof(VkyPathVertex, p1));
    vky_add_vertex_attribute(
        &vertex_layout, 2, VKY_DEFAULT_VERTEX_FORMAT_POS, offsetof(VkyPathVertex, p2));
    vky_add_vertex_attribute(
        &vertex_layout, 3, VKY_DEFAULT_VERTEX_FORMAT_POS, offsetof(VkyPathVertex, p3));
    vky_add_vertex_attribute(
        &vertex_layout, 4, VKY_DEFAULT_VERTEX_FORMAT_COLOR, offsetof(VkyPathVertex, color));

    // DPI scaling factor.
    params.linewidth *= canvas->dpi_factor;
    vky_visual_params(visual, sizeof(VkyPathParams), &params);

    // Resource layout.
    VkyResourceLayout resource_layout = vky_common_resource_layout(visual);

    // Pipeline.
    visual->pipeline = vky_create_graphics_pipeline(
        canvas, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, shaders, vertex_layout, resource_layout,
        (VkyGraphicsPipelineParams){params.enable_depth > 0});

    // Resources.
    vky_add_common_resources(visual);

    visual->cb_bake_data = vky_path_bake;

    return visual;
}



/*************************************************************************************************/
/*  Segment visual                                                                               */
/*************************************************************************************************/

static VkyData vky_segment_bake(VkyVisual* visual, VkyData data)
{
    uint32_t nv = 4 * data.item_count;
    uint32_t ni = 6 * data.item_count;

    ASSERT(nv > 0);
    ASSERT(ni > 0);

    data.vertex_count = nv;
    data.index_count = ni;

    if (data.items == NULL)
    {
        return data;
    }

    // Allocate the data buffer to be uploaded to the vertex buffer. Will be freed by visky.
    log_trace("allocating vertices and indices");
    VkySegmentVertex* vertices = calloc(nv, sizeof(VkySegmentVertex));
    VkyIndex* indices = calloc(ni, sizeof(VkyIndex));
    const VkySegmentVertex* items = (const VkySegmentVertex*)data.items;
    double dpi = visual->scene->canvas->dpi_factor;

    for (uint32_t i = 0; i < data.item_count; i++)
    {
        for (uint32_t j = 0; j < 4; j++)
        {
            vec3_copy(items[i].P0, vertices[4 * i + j].P0);
            vec3_copy(items[i].P1, vertices[4 * i + j].P1);
            vec4_scale(items[i].shift, (float)dpi, vertices[4 * i + j].shift);

            vertices[4 * i + j].color = items[i].color;
            vertices[4 * i + j].cap0 = items[i].cap0;
            vertices[4 * i + j].cap1 = items[i].cap1;
            vertices[4 * i + j].linewidth = items[i].linewidth * dpi;
            vertices[4 * i + j].is_static = items[i].is_static;

            ASSERT(4 * i + j < nv);
        }

        indices[6 * i + 0] = 4 * i + 0;
        indices[6 * i + 1] = 4 * i + 1;
        indices[6 * i + 2] = 4 * i + 2;
        indices[6 * i + 3] = 4 * i + 0;
        indices[6 * i + 4] = 4 * i + 2;
        indices[6 * i + 5] = 4 * i + 3;

        ASSERT(6 * i + 5 < ni);
    }

    data.vertices = vertices;
    data.indices = indices;

    return data;
}

VkyVisual* vky_visual_segment(VkyScene* scene)
{
    VkyVisual* visual = vky_create_visual(scene, VKY_VISUAL_SEGMENT);
    VkyCanvas* canvas = scene->canvas;

    // Shaders.
    VkyShaders shaders = vky_create_shaders(canvas->gpu);
    vky_add_shader(&shaders, VK_SHADER_STAGE_VERTEX_BIT, "segment.vert.spv");
    vky_add_shader(&shaders, VK_SHADER_STAGE_FRAGMENT_BIT, "segment.frag.spv");

    // Vertex layout.
    VkyVertexLayout vertex_layout =
        vky_create_vertex_layout(canvas->gpu, 0, sizeof(VkySegmentVertex));
    vky_add_vertex_attribute(
        &vertex_layout, 0, VKY_DEFAULT_VERTEX_FORMAT_POS, offsetof(VkySegmentVertex, P0));
    vky_add_vertex_attribute(
        &vertex_layout, 1, VKY_DEFAULT_VERTEX_FORMAT_POS, offsetof(VkySegmentVertex, P1));
    vky_add_vertex_attribute(
        &vertex_layout, 2, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(VkySegmentVertex, shift));
    vky_add_vertex_attribute(
        &vertex_layout, 3, VKY_DEFAULT_VERTEX_FORMAT_COLOR, offsetof(VkySegmentVertex, color));
    vky_add_vertex_attribute(
        &vertex_layout, 4, VK_FORMAT_R32_SFLOAT, offsetof(VkySegmentVertex, linewidth));
    vky_add_vertex_attribute(
        &vertex_layout, 5, VK_FORMAT_R32_SINT, offsetof(VkySegmentVertex, cap0));
    vky_add_vertex_attribute(
        &vertex_layout, 6, VK_FORMAT_R32_SINT, offsetof(VkySegmentVertex, cap1));
    vky_add_vertex_attribute(
        &vertex_layout, 7, VK_FORMAT_R8_UINT, offsetof(VkySegmentVertex, is_static));

    // Resource layout.
    VkyResourceLayout resource_layout = vky_common_resource_layout(visual);

    // Pipeline.
    visual->pipeline = vky_create_graphics_pipeline(
        canvas, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, shaders, vertex_layout, resource_layout,
        (VkyGraphicsPipelineParams){false});

    // Resources.
    vky_add_common_resources(visual);

    visual->cb_bake_data = vky_segment_bake;

    return visual;
}



/*************************************************************************************************/
/*  Markers visual                                                                               */
/*************************************************************************************************/

static VkyData vky_marker_bake(VkyVisual* visual, VkyData data)
{
    data.vertex_count = data.item_count;

    if (data.items == NULL)
    {
        return data;
    }

    // Allocate the data buffer to be uploaded to the vertex buffer. Will be freed by visky.
    log_trace("allocating vertices and indices");
    VkyMarkersVertex* vertices = calloc(data.vertex_count, sizeof(VkyMarkersVertex));
    const VkyMarkersVertex* items = (const VkyMarkersVertex*)data.items;

    // TODO: avoid copying and multiply the marker size in the shader instead, adding
    // the dpi factor in the params
    for (uint32_t i = 0; i < data.item_count; i++)
    {
        vertices[i] = items[i];
        vertices[i].size *= visual->scene->canvas->dpi_factor;
    }

    data.vertices = vertices;

    return data;
}

VkyVisual* vky_visual_marker(VkyScene* scene, VkyMarkersParams params, bool enable_depth)
{
    VkyVisual* visual = vky_create_visual(scene, VKY_VISUAL_MARKER);
    VkyCanvas* canvas = scene->canvas;

    // Shaders.
    VkyShaders shaders = vky_create_shaders(canvas->gpu);
    vky_add_shader(&shaders, VK_SHADER_STAGE_VERTEX_BIT, "markers.vert.spv");
    vky_add_shader(&shaders, VK_SHADER_STAGE_FRAGMENT_BIT, "markers.frag.spv");

    // Vertex layout.
    VkyVertexLayout vertex_layout =
        vky_create_vertex_layout(canvas->gpu, 0, sizeof(VkyMarkersVertex));
    vky_add_vertex_attribute(
        &vertex_layout, 0, VKY_DEFAULT_VERTEX_FORMAT_POS, offsetof(VkyMarkersVertex, pos));
    // NOTE: UNORM means that uint8 values are converted to float in the shaders (division by 255)
    vky_add_vertex_attribute(
        &vertex_layout, 1, VKY_DEFAULT_VERTEX_FORMAT_COLOR, offsetof(VkyMarkersVertex, color));
    vky_add_vertex_attribute(
        &vertex_layout, 2, VK_FORMAT_R8_UINT, offsetof(VkyMarkersVertex, size));
    vky_add_vertex_attribute(
        &vertex_layout, 3, VK_FORMAT_R8_UINT, offsetof(VkyMarkersVertex, marker));
    vky_add_vertex_attribute(
        &vertex_layout, 4, VK_FORMAT_R8_UNORM, offsetof(VkyMarkersVertex, angle));

    // Params.
    params.edge_width *= canvas->dpi_factor;
    vky_visual_params(visual, sizeof(VkyMarkersParams), &params);

    // Resource layout.
    VkyResourceLayout resource_layout = vky_common_resource_layout(visual);

    // Pipeline.
    visual->pipeline = vky_create_graphics_pipeline(
        canvas, VK_PRIMITIVE_TOPOLOGY_POINT_LIST, shaders, vertex_layout, resource_layout,
        (VkyGraphicsPipelineParams){enable_depth});

    // Resources.
    vky_add_common_resources(visual);

    visual->cb_bake_data = vky_marker_bake;

    return visual;
}



/*************************************************************************************************/
/*  Text visual                                                                                  */
/*************************************************************************************************/

static VkyData vky_text_bake(VkyVisual* visual, VkyData data)
{
    ASSERT(data.items != NULL); // TODO: support allocation with no upload by specifying a max
                                // number of glyphs per string

    // Input text as array of VkyTextData.
    const VkyTextData* text = (const VkyTextData*)data.items;

    // Count the number of glyphs.
    uint32_t glyph_count = 0;
    for (uint32_t i = 0; i < data.item_count; i++)
    { // vertex_count is the number of strings here
        glyph_count += text[i].string_len;
    }

    uint32_t nv = glyph_count;
    ASSERT(nv > 0);

    data.vertex_count = nv * 4;
    data.index_count = 0; // we don't use the index buffer

    // Allocate the data buffer to be uploaded to the vertex buffer. Will be freed by visky.
    VkyTextVertex* vertices = calloc(data.vertex_count, sizeof(VkyTextVertex));

    // Glyph aspect ratio and size.
    VkyTextParams params = *((VkyTextParams*)visual->params);
    float glyph_width = params.tex_size[0] / (float)params.grid_size[1];
    float glyph_height = params.tex_size[1] / (float)params.grid_size[0];

    uint32_t k = 0;
    VkyTextVertex vertex = {0};
    double dpi = visual->scene->canvas->dpi_factor;
    // Go through all strings.
    for (uint32_t i = 0; i < data.item_count; i++)
    {
        uint32_t str_len = text[i].string_len;
        // For each string, go through the chars.
        for (uint32_t j = 0; j < str_len; j++)
        {
            char c[] = {text[i].string[j]};
            uint32_t ci = strcspn(VKY_TEXT_CHARS, c);
            vertex = (VkyTextVertex){
                {text[i].pos[0], text[i].pos[1], text[i].pos[2]},
                {text[i].shift[0] * dpi, text[i].shift[1] * dpi},
                text[i].color,
                {text[i].glyph_size / glyph_height * glyph_width * dpi, text[i].glyph_size * dpi},
                {text[i].anchor[0], text[i].anchor[1]},
                text[i].angle,
                {ci, j, str_len, i}, // char, charIdx, strLen, strIdx
                (uint8_t)text[i].is_static,
            };
            // Duplicate the vertex for the 2 triangles composing the rectangle.
            for (uint32_t u = 0; u < 4; u++)
            {
                vertices[4 * k + u] = vertex;
            }
            k++;
        }
    }

    data.vertices = vertices;
    ASSERT(data.vertices != NULL);
    data.indices = NULL;

    return data;
}

VkyVisual* vky_visual_text(VkyScene* scene)
{
    VkyVisual* visual = vky_create_visual(scene, VKY_VISUAL_TEXT);
    VkyCanvas* canvas = scene->canvas;

    // Shaders.
    VkyShaders shaders = vky_create_shaders(canvas->gpu);
    vky_add_shader(&shaders, VK_SHADER_STAGE_VERTEX_BIT, "text.vert.spv");
    vky_add_shader(&shaders, VK_SHADER_STAGE_FRAGMENT_BIT, "text.frag.spv");

    // Vertex layout.
    VkyVertexLayout vertex_layout =
        vky_create_vertex_layout(canvas->gpu, 0, sizeof(VkyTextVertex));
    vky_add_vertex_attribute(
        &vertex_layout, 0, VKY_DEFAULT_VERTEX_FORMAT_POS, offsetof(VkyTextVertex, pos));
    vky_add_vertex_attribute(
        &vertex_layout, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(VkyTextVertex, shift));
    vky_add_vertex_attribute(
        &vertex_layout, 2, VKY_DEFAULT_VERTEX_FORMAT_COLOR, offsetof(VkyTextVertex, color));
    vky_add_vertex_attribute(
        &vertex_layout, 3, VK_FORMAT_R32G32_SFLOAT, offsetof(VkyTextVertex, glyph_size));
    vky_add_vertex_attribute(
        &vertex_layout, 4, VK_FORMAT_R32G32_SFLOAT, offsetof(VkyTextVertex, anchor));
    vky_add_vertex_attribute(
        &vertex_layout, 5, VK_FORMAT_R32_SFLOAT, offsetof(VkyTextVertex, angle));
    vky_add_vertex_attribute(
        &vertex_layout, 6, VK_FORMAT_R16G16B16A16_UINT, offsetof(VkyTextVertex, glyph));
    vky_add_vertex_attribute(
        &vertex_layout, 7, VK_FORMAT_R8_UINT, offsetof(VkyTextVertex, is_static));

    // Font texture.
    VkyTexture* texture = vky_get_font_texture(canvas->gpu);
    int width = (int)texture->params.width;
    int height = (int)texture->params.height;

    VkyTextParams params = {VKY_FONT_TEXTURE_SHAPE, {width, height}};
    vky_visual_params(visual, sizeof(VkyTextParams), &params);

    // Resource layout.
    VkyResourceLayout resource_layout = vky_common_resource_layout(visual);
    vky_add_resource_binding(&resource_layout, 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    // Pipeline.
    visual->pipeline = vky_create_graphics_pipeline(
        canvas, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, shaders, vertex_layout, resource_layout,
        (VkyGraphicsPipelineParams){false});

    // Resources.
    vky_add_common_resources(visual);
    vky_add_texture_resource(visual, texture); // font texture

    visual->cb_bake_data = vky_text_bake;

    return visual;
}



/*************************************************************************************************/
/*  Arrow visual                                                                                 */
/*************************************************************************************************/

static VkyData vky_arrow_bake(VkyVisual* visual, VkyData data)
{
    uint32_t nv = 4 * data.item_count;
    uint32_t ni = 6 * data.item_count;

    ASSERT(nv > 0);
    ASSERT(ni > 0);

    data.vertex_count = nv;
    data.index_count = ni;

    if (data.items == NULL)
    {
        return data;
    }

    // Allocate the data buffer to be uploaded to the vertex buffer. Will be freed by visky.
    log_trace("allocating vertices and indices");
    VkyArrowVertex* vertices = calloc(nv, sizeof(VkyArrowVertex));
    VkyIndex* indices = calloc(ni, sizeof(VkyIndex));
    const VkyArrowVertex* items = (const VkyArrowVertex*)data.items;

    for (uint32_t i = 0; i < data.item_count; i++)
    {
        for (uint32_t j = 0; j < 4; j++)
        {
            ASSERT(4 * i + j < nv);
            vec3_copy(items[i].P0, vertices[4 * i + j].P0);
            vec3_copy(items[i].P1, vertices[4 * i + j].P1);
            memcpy(&vertices[4 * i + j].color, &items[i].color, sizeof(items[i].color));

            vertices[4 * i + j].head = items[i].head * visual->scene->canvas->dpi_factor;
            vertices[4 * i + j].linewidth = items[i].linewidth * visual->scene->canvas->dpi_factor;
            vertices[4 * i + j].arrow_type = items[i].arrow_type;

            ASSERT(4 * i + j < nv);
        }

        indices[6 * i + 0] = 4 * i + 0;
        indices[6 * i + 1] = 4 * i + 1;
        indices[6 * i + 2] = 4 * i + 2;
        indices[6 * i + 3] = 4 * i + 0;
        indices[6 * i + 4] = 4 * i + 2;
        indices[6 * i + 5] = 4 * i + 3;

        ASSERT(6 * i + 5 < ni);
    }

    data.vertices = vertices;
    data.indices = indices;

    return data;
}

VkyVisual* vky_visual_arrow(VkyScene* scene)
{
    VkyVisual* visual = vky_create_visual(scene, VKY_VISUAL_ARROW);
    VkyCanvas* canvas = scene->canvas;

    // Shaders.
    VkyShaders shaders = vky_create_shaders(canvas->gpu);
    vky_add_shader(&shaders, VK_SHADER_STAGE_VERTEX_BIT, "arrow.vert.spv");
    vky_add_shader(&shaders, VK_SHADER_STAGE_FRAGMENT_BIT, "arrow.frag.spv");

    // Vertex layout.
    VkyVertexLayout vertex_layout =
        vky_create_vertex_layout(canvas->gpu, 0, sizeof(VkyArrowVertex));
    vky_add_vertex_attribute(
        &vertex_layout, 0, VKY_DEFAULT_VERTEX_FORMAT_POS, offsetof(VkyArrowVertex, P0));
    vky_add_vertex_attribute(
        &vertex_layout, 1, VKY_DEFAULT_VERTEX_FORMAT_POS, offsetof(VkyArrowVertex, P1));
    vky_add_vertex_attribute(
        &vertex_layout, 2, VKY_DEFAULT_VERTEX_FORMAT_COLOR, offsetof(VkyArrowVertex, color));
    vky_add_vertex_attribute(
        &vertex_layout, 3, VK_FORMAT_R32_SFLOAT, offsetof(VkyArrowVertex, head));
    vky_add_vertex_attribute(
        &vertex_layout, 4, VK_FORMAT_R32_SFLOAT, offsetof(VkyArrowVertex, linewidth));
    vky_add_vertex_attribute(
        &vertex_layout, 5, VK_FORMAT_R32_SFLOAT, offsetof(VkyArrowVertex, arrow_type));

    // Resource layout.
    VkyResourceLayout resource_layout = vky_common_resource_layout(visual);

    // Pipeline.
    visual->pipeline = vky_create_graphics_pipeline(
        canvas, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, shaders, vertex_layout, resource_layout,
        (VkyGraphicsPipelineParams){false});

    // Resources.
    vky_add_common_resources(visual);

    visual->cb_bake_data = vky_arrow_bake;

    return visual;
}
