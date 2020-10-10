#include "../include/visky/triangulation.h"



/*************************************************************************************************/
/*  Graph visual bundle                                                                          */
/*************************************************************************************************/

void vky_graph_upload(
    VkyVisual* vb,                            //
    uint32_t node_count, VkyGraphNode* nodes, // nodes
    uint32_t edge_count, VkyGraphEdge* edges) // edges
{
    // Node data pass-through.
    VkyData node_data = {0};
    node_data.item_count = node_count;
    node_data.items = nodes;

    // Create the segment vertex array.
    VkySegmentVertex* edge_items = calloc(edge_count, sizeof(VkySegmentVertex));
    for (uint32_t i = 0; i < edge_count; i++)
    {
        ASSERT(edges[i].source_node < node_count);
        ASSERT(edges[i].target_node < node_count);

        vec3_copy(nodes[edges[i].source_node].pos, edge_items[i].P0);
        vec3_copy(nodes[edges[i].target_node].pos, edge_items[i].P1);
        edge_items[i].color = edges[i].color;

        edge_items[i].linewidth = edges[i].linewidth;
        edge_items[i].cap0 = (int32_t)edges[i].cap0;
        edge_items[i].cap1 = (int32_t)edges[i].cap1;
    }

    // Upload the node and edge data to the two visuals.
    VkyData edge_data = {0};
    edge_data.item_count = edge_count;
    edge_data.items = edge_items;

    vky_visual_upload(vb->children[0], edge_data);
    vky_visual_upload(vb->children[1], node_data);

    free(edge_items);
}

VkyVisual* vky_bundle_graph(VkyScene* scene, VkyGraphParams params)
{
    VkyVisual* vb = vky_create_visual(scene, VKY_VISUAL_EMPTY);

    VkyMarkersParams node_params = {0};
    vec4_copy(params.marker_edge_color, node_params.edge_color);
    node_params.edge_width = params.marker_edge_width;
    node_params.enable_depth = false;

    VkyVisual* edges = vky_visual_segment(scene);
    VkyVisual* nodes = vky_visual_marker(scene, &node_params);

    vky_visual_add_child(vb, edges);
    vky_visual_add_child(vb, nodes);

    return vb;
}



/*************************************************************************************************/
/*  Color bar visual bundle                                                                      */
/*************************************************************************************************/

#define TICK_DATA                                                                                 \
    {{TO_BYTE(VKY_AXES_LIM_COLOR_R), TO_BYTE(VKY_AXES_LIM_COLOR_G),                               \
      TO_BYTE(VKY_AXES_LIM_COLOR_B)},                                                             \
     TO_BYTE(VKY_AXES_LIM_COLOR_A)},                                                              \
        VKY_AXES_TICK_LINEWIDTH_LIM, VKY_CAP_SQUARE, VKY_CAP_SQUARE, true,

static VkyVisual* colorbar_visual(VkyScene* scene)
{
    VkyCanvas* canvas = scene->canvas;

    // Colorbar visual.
    VkyVisual* visual = vky_create_visual(scene, VKY_VISUAL_COLORBAR);

    // Shaders.
    VkyShaders shaders = vky_create_shaders(canvas->gpu);
    vky_add_shader(&shaders, VK_SHADER_STAGE_VERTEX_BIT, "colorbar.vert.spv");
    vky_add_shader(&shaders, VK_SHADER_STAGE_FRAGMENT_BIT, "colorbar.frag.spv");

    // Vertex layout.
    VkyVertexLayout vertex_layout =
        vky_create_vertex_layout(canvas->gpu, 0, sizeof(VkyColorbarVertex));
    vky_add_vertex_attribute(
        &vertex_layout, 0, VKY_DEFAULT_VERTEX_FORMAT_POS, offsetof(VkyColorbarVertex, pos));
    vky_add_vertex_attribute(
        &vertex_layout, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(VkyColorbarVertex, padding));
    vky_add_vertex_attribute(
        &vertex_layout, 2, VK_FORMAT_R8G8_UNORM, offsetof(VkyColorbarVertex, uv));

    // Resource layout.
    VkyResourceLayout resource_layout = vky_common_resource_layout(visual);

    // Pipeline.
    visual->pipeline = vky_create_graphics_pipeline(
        canvas, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, shaders, vertex_layout, resource_layout,
        (VkyGraphicsPipelineParams){true});

    vky_add_common_resources(visual);

    return visual;
}

static void colorbar_upload(VkyVisual* colorbar, VkyColorbarParams params)
{
    // Colorbar visual data.
    uint8_t cmap_row = (uint8_t)params.cmap;
    VkyColorbarVertex vertices[4];

    float z = params.z;

    // Position of the top-left and bottom-right corners, in relative coordinates.
    float x0 = params.pos_tl[0]; // in relative coordinates
    float y0 = params.pos_tl[1];
    float x1 = params.pos_br[0];
    float y1 = params.pos_br[1];

    float px0 = colorbar->scene->canvas->dpi_factor * params.pad_tl[0]; // in pixels
    float py0 = colorbar->scene->canvas->dpi_factor * params.pad_tl[1];
    float px1 = colorbar->scene->canvas->dpi_factor * params.pad_br[0];
    float py1 = colorbar->scene->canvas->dpi_factor * params.pad_br[1];

    vertices[0] = (VkyColorbarVertex){{x0, y1, z}, {px0, py1}, {255, cmap_row}};
    vertices[1] = (VkyColorbarVertex){{x1, y1, z}, {px1, py1}, {255, cmap_row}};
    vertices[2] = (VkyColorbarVertex){{x1, y0, z}, {px1, py0}, {0, cmap_row}};
    vertices[3] = (VkyColorbarVertex){{x0, y0, z}, {px0, py0}, {0, cmap_row}};

    if (params.position == VKY_COLORBAR_TOP || params.position == VKY_COLORBAR_BOTTOM)
    {
        vertices[0].uv[0] = 255;
        vertices[1].uv[0] = 0;
        vertices[2].uv[0] = 0;
        vertices[3].uv[0] = 255;
    }

    VkyIndex indices[] = {0, 1, 2, 2, 3, 0};

    vky_visual_upload(colorbar, (VkyData){0, NULL, 4, vertices, 6, indices});
}

static void colorbar_tick_upload(VkyVisual* text, VkyVisual* ticks, VkyColorbarParams params)
{
    // Colorbar ticks.
    bool normal_range =
        VKY_AXES_NORMAL_RANGE(params.scale.vmin) && VKY_AXES_NORMAL_RANGE(params.scale.vmax);

    // Colorbar tick margins.
    // NOTE: need to normalize by the glyph height, as the anchor is relative to the full
    // string
    float anchor_top = params.pad_tl[1] / VKY_AXES_FONT_SIZE;
    float anchor_bottom = params.pad_br[1] / VKY_AXES_FONT_SIZE;

    // Upload the data.
    uint32_t n = 5;
    VkyTextData* text_data = calloc(n, sizeof(VkyTextData));
    VkySegmentVertex* tick_data =
        calloc(n + 4, sizeof(VkySegmentVertex)); // ticks + colorbar outline (4 segments)
    double pos = 0;
    double t = 0;
    const uint32_t MAX_TICK_LEN = 16;
    char* tick = calloc(MAX_TICK_LEN * n, sizeof(char));
    char* cur_tick = tick;
    uint32_t tick_len = 0;
    float tick_length = VKY_COLORBAR_TICK_LENGTH;
    float anchor_y = 0;
    float hlw = VKY_ANTIALIAS / 2;
    for (uint32_t i = 0; i < n; i++)
    {
        // TODO: support right and horizontal colorbars
        t = (double)i / (n - 1);
        pos = 1.0 - 2.0 * t;
        anchor_y = (-anchor_bottom) + (anchor_top + anchor_bottom) * t;
        snprintf(
            cur_tick, MAX_TICK_LEN, normal_range ? "%.3f" : "%.3e",
            params.scale.vmin + (params.scale.vmax - params.scale.vmin) * t);
        tick_len = strlen(cur_tick);

        // Tick text.
        text_data[i] = (VkyTextData){
            {1, pos, 0},
            {VKY_AXES_MARGIN_RIGHT - params.pad_br[0], 0},
            {{TO_BYTE(VKY_AXES_TEXT_COLOR_R), //
              TO_BYTE(VKY_AXES_TEXT_COLOR_G), //
              TO_BYTE(VKY_AXES_TEXT_COLOR_B)},
             TO_BYTE(VKY_AXES_TEXT_COLOR_A)},
            VKY_AXES_FONT_SIZE,
            {+1, anchor_y},
            0,
            tick_len,
            cur_tick,
            true,
        };

        // Tick segment.
        anchor_y = -params.pad_br[1] + (params.pad_tl[1] + params.pad_br[1]) * t;
        tick_data[i] = (VkySegmentVertex){
            {1, pos, 0},
            {1, pos, 0},
            {-params.pad_br[0], anchor_y, -params.pad_br[0] + tick_length, anchor_y},
            TICK_DATA};

        cur_tick += MAX_TICK_LEN;
    }

    // Colorbar border.
    // Left
    tick_data[n] = (VkySegmentVertex){
        {+1, +1, 0},
        {+1, -1, 0},
        {-params.pad_tl[0], -params.pad_br[1] - hlw, -params.pad_tl[0], +params.pad_tl[1] + hlw},
        TICK_DATA};
    // Top
    tick_data[n + 1] = (VkySegmentVertex){
        {+1, -1, 0},
        {+1, -1, 0},
        {-params.pad_tl[0] + hlw, +params.pad_tl[1], -params.pad_br[0] - hlw, +params.pad_tl[1]},
        TICK_DATA};
    // Right
    tick_data[n + 2] = (VkySegmentVertex){
        {+1, +1, 0},
        {+1, -1, 0},
        {-params.pad_br[0], -params.pad_br[1] - hlw, -params.pad_br[0], +params.pad_tl[1] + hlw},
        TICK_DATA};
    // Bottom
    tick_data[n + 3] = (VkySegmentVertex){
        {+1, +1, 0},
        {+1, +1, 0},
        {-params.pad_tl[0] + hlw, -params.pad_br[1], -params.pad_br[0] - hlw, -params.pad_br[1]},
        TICK_DATA};

    vky_visual_upload(text, (VkyData){n, text_data});
    vky_visual_upload(ticks, (VkyData){n + 4, tick_data});

    free(tick);
    free(tick_data);
    free(text_data);
}

VkyVisual* vky_bundle_colorbar(VkyScene* scene, VkyColorbarParams params)
{
    VkyVisual* vb = vky_create_visual(scene, VKY_VISUAL_EMPTY);

    // Colorbar visual.
    VkyVisual* colorbar = colorbar_visual(scene);
    VkyVisual* text = vky_visual_text(scene);
    VkyVisual* ticks = vky_visual_segment(scene);

    // Add the visuals to the bundle.
    vky_visual_add_child(vb, colorbar);
    vky_visual_add_child(vb, text);
    vky_visual_add_child(vb, ticks);

    // Upload the colorbar data.
    colorbar_upload(colorbar, params);
    colorbar_tick_upload(text, ticks, params);

    return vb;
}



/*************************************************************************************************/
/*  Volume visual                                                                                */
/*************************************************************************************************/

VkyVisual*
vky_visual_volume(VkyScene* scene, const VkyTextureParams* tex_params, const void* pixels)
{
    VkyVisual* visual = vky_create_visual(scene, VKY_VISUAL_VOLUME);
    VkyCanvas* canvas = scene->canvas;

    // Shaders.
    VkyShaders shaders = vky_create_shaders(canvas->gpu);
    vky_add_shader(&shaders, VK_SHADER_STAGE_VERTEX_BIT, "volume.vert.spv");
    vky_add_shader(&shaders, VK_SHADER_STAGE_FRAGMENT_BIT, "volume.frag.spv");

    // Vertex layout.
    VkyVertexLayout vertex_layout = vky_create_vertex_layout(canvas->gpu, 0, sizeof(VkyVertexUV));
    vky_add_vertex_attribute(
        &vertex_layout, 0, VKY_DEFAULT_VERTEX_FORMAT_POS, offsetof(VkyVertexUV, pos));
    vky_add_vertex_attribute(
        &vertex_layout, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(VkyVertexUV, uv));

    // Params.
    VkyVolumeParams params = {0};
    glm_mat4_copy(GLM_MAT4_IDENTITY, params.inv_proj_view);
    glm_mat4_copy(GLM_MAT4_IDENTITY, params.normal_mat);
    vky_visual_params(visual, sizeof(VkyVolumeParams), &params);

    // Resource layout.
    VkyResourceLayout resource_layout = vky_common_resource_layout(visual);
    vky_add_resource_binding(&resource_layout, 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    // Pipeline.
    visual->pipeline = vky_create_graphics_pipeline(
        canvas, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, shaders, vertex_layout, resource_layout,
        (VkyGraphicsPipelineParams){true});

    // Texture.
    VkyTexture* tex = vky_add_texture(canvas->gpu, tex_params);
    vky_upload_texture(tex, pixels);

    // Resources.
    vky_add_common_resources(visual);
    vky_add_texture_resource(visual, tex);

    // Square.
    float x = 1.0f;
    VkyVertexUV vertices[] = {
        {{-x, -x, 0}, {0, 0}}, {{+x, -x, 0}, {1, 0}}, {{-x, +x, 0}, {0, 1}},
        {{-x, +x, 0}, {0, 1}}, {{+x, -x, 0}, {1, 0}}, {{+x, +x, 0}, {1, 1}},
    };

    vky_visual_upload(visual, (VkyData){0, NULL, 6, vertices, 0, NULL});

    return visual;
}



/*************************************************************************************************/
/*  Volume slicer visual                                                                         */
/*************************************************************************************************/

VkyVisual* vky_visual_volume_slicer(VkyScene* scene, VkyTexture* tex)
{
    // Create the visual.
    VkyVisual* visual = vky_create_visual(scene, VKY_VISUAL_VOLUME_SLICER);
    VkyCanvas* canvas = scene->canvas;

    // Shaders.
    VkyShaders shaders = vky_create_shaders(canvas->gpu);
    vky_add_shader(&shaders, VK_SHADER_STAGE_VERTEX_BIT, "volume_slice.vert.spv");
    vky_add_shader(&shaders, VK_SHADER_STAGE_FRAGMENT_BIT, "volume_slice.frag.spv");

    // Vertex layout.
    VkyVertexLayout vertex_layout =
        vky_create_vertex_layout(canvas->gpu, 0, sizeof(VkyTexturedVertex3D));
    vky_add_vertex_attribute(
        &vertex_layout, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VkyTexturedVertex3D, pos));
    vky_add_vertex_attribute(
        &vertex_layout, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VkyTexturedVertex3D, coords));

    // Resource layout.
    VkyResourceLayout resource_layout = vky_common_resource_layout(visual);
    vky_add_resource_binding(&resource_layout, 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    // Pipeline.
    visual->pipeline = vky_create_graphics_pipeline(
        canvas, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, shaders, vertex_layout, resource_layout,
        (VkyGraphicsPipelineParams){false});

    // Resources.
    vky_add_common_resources(visual);
    vky_add_texture_resource(visual, tex);

    return visual;
}



/*************************************************************************************************/
/*  Path visual                                                                                  */
/*************************************************************************************************/

const uint32_t path_vertices_per_segment = 4;

static void add_path_point(
    VkyPathVertex* vertices, uint32_t vertex_offset, vec3 p0, vec3 p1, vec2 p2, vec3 p3,
    VkyColor color)
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
    VkyColor color;

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
            add_path_point(vertices, vertex_offset, p0, p0, p0, p0, (VkyColor){{0, 0, 0}, 0});
            vertex_offset++;

            // Add first point of next path.
            vec3_copy(paths[i + 1].points[0], p0);
            add_path_point(vertices, vertex_offset, p0, p0, p0, p0, (VkyColor){{0, 0, 0}, 0});
            vertex_offset++;

        } // end for loop on points in current path
    }     // end for loop on paths
    ASSERT(vertex_offset == vertex_count);

    data.vertices = vertices;
    data.indices = NULL;

    log_trace("finished baking the path data");
    return data;
}

VkyVisual* vky_visual_path(VkyScene* scene, const VkyPathParams* params)
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

    // Params.
    VkyPathParams vparams = {0};
    if (params != NULL)
    {
        memcpy(&vparams, params, sizeof(VkyPathParams));
    }
    else
    {
        // TODO: constants
        vparams.linewidth = 1;
    }
    vparams.linewidth *= canvas->dpi_factor;
    ASSERT(vparams.linewidth > 0);
    vky_visual_params(visual, sizeof(VkyPathParams), &vparams);

    // Resource layout.
    VkyResourceLayout resource_layout = vky_common_resource_layout(visual);

    // Pipeline.
    visual->pipeline = vky_create_graphics_pipeline(
        canvas, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, shaders, vertex_layout, resource_layout,
        (VkyGraphicsPipelineParams){vparams.enable_depth > 0});

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
            vertices[4 * i + j].transform_mode = items[i].transform_mode;

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
        &vertex_layout, 7, VK_FORMAT_R8_UINT, offsetof(VkySegmentVertex, transform_mode));

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

VkyVisual* vky_visual_marker(VkyScene* scene, const VkyMarkersParams* params)
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
    VkyMarkersParams vparams = {0};
    if (params != NULL)
    {
        memcpy(&vparams, params, sizeof(VkyMarkersParams));
    }
    else
    {
        // Default.
        vparams.edge_width = 1;
    }
    vparams.edge_width *= canvas->dpi_factor;
    vky_visual_params(visual, sizeof(VkyMarkersParams), &vparams);

    // Resource layout.
    VkyResourceLayout resource_layout = vky_common_resource_layout(visual);

    // Pipeline.
    visual->pipeline = vky_create_graphics_pipeline(
        canvas, VK_PRIMITIVE_TOPOLOGY_POINT_LIST, shaders, vertex_layout, resource_layout,
        (VkyGraphicsPipelineParams){vparams.enable_depth > 0});

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
    VkyTextParams params = *((const VkyTextParams*)visual->params);
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
                (uint8_t)text[i].transform_mode,
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
        &vertex_layout, 7, VK_FORMAT_R8_UINT, offsetof(VkyTextVertex, transform_mode));

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



/*************************************************************************************************/
/*  Fake sphere visual                                                                           */
/*************************************************************************************************/

VkyVisual* vky_visual_fake_sphere(VkyScene* scene, const VkyFakeSphereParams* params)
{
    VkyVisual* visual = vky_create_visual(scene, VKY_VISUAL_FAKE_SPHERE);
    VkyCanvas* canvas = scene->canvas;

    // Shaders.
    VkyShaders shaders = vky_create_shaders(canvas->gpu);
    vky_add_shader(&shaders, VK_SHADER_STAGE_VERTEX_BIT, "fake_sphere.vert.spv");
    vky_add_shader(&shaders, VK_SHADER_STAGE_FRAGMENT_BIT, "fake_sphere.frag.spv");

    // Vertex layout.
    VkyVertexLayout vertex_layout =
        vky_create_vertex_layout(canvas->gpu, 0, sizeof(VkyFakeSphereVertex));
    vky_add_vertex_attribute(
        &vertex_layout, 0, VKY_DEFAULT_VERTEX_FORMAT_POS, offsetof(VkyFakeSphereVertex, pos));
    vky_add_vertex_attribute(
        &vertex_layout, 1, VKY_DEFAULT_VERTEX_FORMAT_COLOR, offsetof(VkyFakeSphereVertex, color));
    vky_add_vertex_attribute(
        &vertex_layout, 2, VK_FORMAT_R32_SFLOAT, offsetof(VkyFakeSphereVertex, radius));

    // Params.
    vky_visual_params(visual, sizeof(VkyFakeSphereParams), params);

    // Resource layout.
    VkyResourceLayout resource_layout = vky_common_resource_layout(visual);

    // Pipeline.
    visual->pipeline = vky_create_graphics_pipeline(
        canvas, VK_PRIMITIVE_TOPOLOGY_POINT_LIST, shaders, vertex_layout, resource_layout,
        (VkyGraphicsPipelineParams){true});

    // Resources.
    vky_add_common_resources(visual);

    return visual;
}



/*************************************************************************************************/
/*  Rectangle visual                                                                             */
/*************************************************************************************************/

static VkyData vky_visual_rectangle_bake(VkyVisual* visual, VkyData data)
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
    VkyVertex* vertices = calloc(nv, sizeof(VkyVertex));
    VkyIndex* indices = calloc(ni, sizeof(VkyIndex));

    const VkyRectangleData* items = (const VkyRectangleData*)data.items;

    vec3 origin, u, v, w;
    VkyRectangleParams params = {0};
    memcpy(&params, visual->params, sizeof(VkyRectangleParams));
    glm_vec3_copy(params.origin, origin);
    glm_vec3_copy(params.u, u);
    glm_vec3_copy(params.v, v);

    for (uint32_t i = 0; i < data.item_count; i++)
    {
        // p00
        glm_vec3_scale(u, items[i].p[0], w);
        glm_vec3_add(w, origin, vertices[4 * i + 0].pos);
        glm_vec3_scale(v, items[i].p[1], w);
        glm_vec3_add(w, vertices[4 * i + 0].pos, vertices[4 * i + 0].pos);

        // p01
        glm_vec3_scale(u, items[i].size[0], w);
        glm_vec3_add(vertices[4 * i + 0].pos, w, vertices[4 * i + 1].pos);

        // p10
        glm_vec3_scale(v, items[i].size[1], w);
        glm_vec3_add(vertices[4 * i + 0].pos, w, vertices[4 * i + 3].pos);

        // p11
        glm_vec3_add(vertices[4 * i + 1].pos, w, vertices[4 * i + 2].pos);

        // Copy the color.
        for (uint32_t j = 0; j < 4; j++)
        {
            memcpy(&vertices[4 * i + j].color, &items[i].color, sizeof(items[i].color));
        }

        indices[6 * i + 0] = 4 * i + 0;
        indices[6 * i + 1] = 4 * i + 1;
        indices[6 * i + 2] = 4 * i + 2;
        indices[6 * i + 3] = 4 * i + 2;
        indices[6 * i + 4] = 4 * i + 3;
        indices[6 * i + 5] = 4 * i + 0;

        ASSERT(6 * i + 5 < ni);
    }

    data.vertices = vertices;
    data.indices = indices;

    return data;
}



VkyVisual* vky_visual_rectangle(VkyScene* scene, const VkyRectangleParams* params)
{
    VkyVisual* visual = vky_create_visual(scene, VKY_VISUAL_RECTANGLE);
    VkyCanvas* canvas = scene->canvas;

    // Shaders.
    VkyShaders shaders = vky_create_shaders(canvas->gpu);
    vky_add_shader(&shaders, VK_SHADER_STAGE_VERTEX_BIT, "mesh_raw.vert.spv");
    vky_add_shader(&shaders, VK_SHADER_STAGE_FRAGMENT_BIT, "mesh_raw.frag.spv");

    // Vertex layout.
    VkyVertexLayout vertex_layout = vky_create_vertex_layout(canvas->gpu, 0, sizeof(VkyVertex));
    vky_add_vertex_attribute(
        &vertex_layout, 0, VKY_DEFAULT_VERTEX_FORMAT_POS, offsetof(VkyVertex, pos));
    vky_add_vertex_attribute(
        &vertex_layout, 1, VK_FORMAT_R8G8B8A8_UNORM, offsetof(VkyVertex, color));

    // Resource layout.
    VkyResourceLayout resource_layout = vky_common_resource_layout(visual);

    // Pipeline.
    visual->pipeline = vky_create_graphics_pipeline(
        canvas, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, shaders, vertex_layout, resource_layout,
        (VkyGraphicsPipelineParams){true});

    vky_visual_params(visual, sizeof(VkyRectangleParams), params);
    vky_add_common_resources(visual);
    visual->cb_bake_data = vky_visual_rectangle_bake;
    return visual;
}



/*************************************************************************************************/
/*  Area visual                                                                                  */
/*************************************************************************************************/

static VkyData vky_visual_area_bake(VkyVisual* visual, VkyData data)
{
    uint32_t nv = 2 * data.item_count;

    ASSERT(nv > 0);

    data.vertex_count = nv;
    data.index_count = 0;

    if (data.items == NULL)
        return data;

    // Allocate the data buffer to be uploaded to the vertex buffer. Will be freed by visky.
    log_trace("allocating vertices");
    VkyAreaVertex* vertices = calloc(nv, sizeof(VkyAreaVertex));

    const VkyAreaData* items = (const VkyAreaData*)data.items;

    vec3 origin, u, v, w;
    VkyAreaParams params = {0};
    memcpy(&params, visual->params, sizeof(VkyAreaParams));
    glm_vec3_copy(params.origin, origin);
    glm_vec3_copy(params.u, u);
    glm_vec3_copy(params.v, v);

    for (uint32_t i = 0; i < data.item_count; i++)
    {
        // pos_bottom
        glm_vec3_scale(u, items[i].p[0], w);                               // w = u * x
        glm_vec3_add(w, origin, vertices[2 * i + 0].pos);                  // pos = origin + w
        glm_vec3_scale(v, items[i].p[1], w);                               // w = v * y
        glm_vec3_add(w, vertices[2 * i + 0].pos, vertices[2 * i + 0].pos); // pos += w;

        // pos_top
        glm_vec3_copy(vertices[2 * i + 0].pos, vertices[2 * i + 1].pos);   // pos = pos_prev
        glm_vec3_scale(v, items[i].h, w);                                  // w = v * h
        glm_vec3_add(vertices[2 * i + 1].pos, w, vertices[2 * i + 1].pos); // pos += w

        // Copy the color.
        for (uint32_t j = 0; j < 2; j++)
        {
            memcpy(&vertices[2 * i + j].color, &items[i].color, sizeof(items[i].color));
            vertices[2 * i + j].area_idx = items[i].area_idx;
        }
    }

    data.vertices = vertices;
    data.indices = NULL;

    return data;
}



VkyVisual* vky_visual_area(VkyScene* scene, const VkyAreaParams* params)
{
    VkyVisual* visual = vky_create_visual(scene, VKY_VISUAL_AREA);
    VkyCanvas* canvas = scene->canvas;

    // Shaders.
    VkyShaders shaders = vky_create_shaders(canvas->gpu);
    vky_add_shader(&shaders, VK_SHADER_STAGE_VERTEX_BIT, "area.vert.spv");
    vky_add_shader(&shaders, VK_SHADER_STAGE_FRAGMENT_BIT, "area.frag.spv");

    // Vertex layout.
    VkyVertexLayout vertex_layout =
        vky_create_vertex_layout(canvas->gpu, 0, sizeof(VkyAreaVertex));
    vky_add_vertex_attribute(
        &vertex_layout, 0, VKY_DEFAULT_VERTEX_FORMAT_POS, offsetof(VkyAreaVertex, pos));
    vky_add_vertex_attribute(
        &vertex_layout, 1, VK_FORMAT_R8G8B8A8_UNORM, offsetof(VkyAreaVertex, color));
    vky_add_vertex_attribute(
        &vertex_layout, 2, VK_FORMAT_R32_UINT, offsetof(VkyAreaVertex, area_idx));

    // Resource layout.
    VkyResourceLayout resource_layout = vky_common_resource_layout(visual);

    // Pipeline.
    visual->pipeline = vky_create_graphics_pipeline(
        canvas, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, shaders, vertex_layout, resource_layout,
        (VkyGraphicsPipelineParams){true});

    vky_visual_params(visual, sizeof(VkyAreaParams), params);
    vky_add_common_resources(visual);
    visual->cb_bake_data = vky_visual_area_bake;
    return visual;
}



/*************************************************************************************************/
/*  Axis rectangle visual                                                                        */
/*************************************************************************************************/

static VkyData vky_visual_rectangle_axis_bake(VkyVisual* visual, VkyData data)
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
    VkyVertex* vertices = calloc(nv, sizeof(VkyVertex));
    VkyIndex* indices = calloc(ni, sizeof(VkyIndex));

    const VkyRectangleAxisData* items = (const VkyRectangleAxisData*)data.items;

    float span_axis = 0;
    for (uint32_t i = 0; i < data.item_count; i++)
    {
        span_axis = (float)items[i].span_axis;
        if (items[i].span_axis == 0)
        {
            glm_vec3_copy((vec3){-1, items[i].ab[0], span_axis}, vertices[4 * i + 0].pos);
            glm_vec3_copy((vec3){+1, items[i].ab[0], span_axis}, vertices[4 * i + 1].pos);
            glm_vec3_copy((vec3){+1, items[i].ab[1], span_axis}, vertices[4 * i + 2].pos);
            glm_vec3_copy((vec3){-1, items[i].ab[1], span_axis}, vertices[4 * i + 3].pos);
        }
        else if (items[i].span_axis == 1)
        {
            glm_vec3_copy((vec3){items[i].ab[0], -1, span_axis}, vertices[4 * i + 0].pos);
            glm_vec3_copy((vec3){items[i].ab[1], -1, span_axis}, vertices[4 * i + 1].pos);
            glm_vec3_copy((vec3){items[i].ab[1], +1, span_axis}, vertices[4 * i + 2].pos);
            glm_vec3_copy((vec3){items[i].ab[0], +1, span_axis}, vertices[4 * i + 3].pos);
        }

        // Copy the color.
        for (uint32_t j = 0; j < 4; j++)
        {
            memcpy(&vertices[4 * i + j].color, &items[i].color, sizeof(items[i].color));
        }

        indices[6 * i + 0] = 4 * i + 0;
        indices[6 * i + 1] = 4 * i + 1;
        indices[6 * i + 2] = 4 * i + 2;
        indices[6 * i + 3] = 4 * i + 2;
        indices[6 * i + 4] = 4 * i + 3;
        indices[6 * i + 5] = 4 * i + 0;

        ASSERT(6 * i + 5 < ni);
    }

    data.vertices = vertices;
    data.indices = indices;

    return data;
}



VkyVisual* vky_visual_rectangle_axis(VkyScene* scene)
{
    VkyVisual* visual = vky_create_visual(scene, VKY_VISUAL_RECTANGLE_AXIS);
    VkyCanvas* canvas = scene->canvas;

    // Shaders.
    VkyShaders shaders = vky_create_shaders(canvas->gpu);
    vky_add_shader(&shaders, VK_SHADER_STAGE_VERTEX_BIT, "axrect.vert.spv");
    vky_add_shader(&shaders, VK_SHADER_STAGE_FRAGMENT_BIT, "axrect.frag.spv");

    // Vertex layout.
    VkyVertexLayout vertex_layout = vky_create_vertex_layout(canvas->gpu, 0, sizeof(VkyVertex));
    vky_add_vertex_attribute(
        &vertex_layout, 0, VKY_DEFAULT_VERTEX_FORMAT_POS, offsetof(VkyVertex, pos));
    vky_add_vertex_attribute(
        &vertex_layout, 1, VK_FORMAT_R8G8B8A8_UNORM, offsetof(VkyVertex, color));

    // Resource layout.
    VkyResourceLayout resource_layout = vky_common_resource_layout(visual);

    // Pipeline.
    visual->pipeline = vky_create_graphics_pipeline(
        canvas, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, shaders, vertex_layout, resource_layout,
        (VkyGraphicsPipelineParams){true});

    vky_add_common_resources(visual);
    visual->cb_bake_data = vky_visual_rectangle_axis_bake;
    return visual;
}



/*************************************************************************************************/
/*  Image visual                                                                                 */
/*************************************************************************************************/

static VkyData vky_visual_image_bake(VkyVisual* visual, VkyData data)
{
    // Determine the actual number of vertices and indices.
    uint32_t image_count = data.item_count; // 1 data item per image
    uint32_t nv = 4 * image_count;          // total number of points
    uint32_t ni = 6 * image_count;

    ASSERT(nv > 0);
    ASSERT(ni > 0);

    data.vertex_count = nv;
    data.index_count = ni;

    if (data.items == NULL)
    {
        return data;
    }

    const VkyImageData* items = (const VkyImageData*)data.items;
    VkyTextureVertex* vertices = calloc(nv, sizeof(VkyTextureVertex));
    VkyIndex* indices = calloc(ni, sizeof(VkyIndex));

    for (uint32_t i = 0; i < image_count; i++)
    {
        // Vertices.
        vertices[4 * i + 0].pos[0] = items[i].p0[0];
        vertices[4 * i + 0].pos[1] = items[i].p0[1];
        vertices[4 * i + 0].pos[2] = items[i].p0[2];
        vertices[4 * i + 0].uv[0] = items[i].uv0[0];
        vertices[4 * i + 0].uv[1] = items[i].uv0[1];

        vertices[4 * i + 1].pos[0] = items[i].p1[0];
        vertices[4 * i + 1].pos[1] = items[i].p0[1];
        vertices[4 * i + 1].pos[2] = items[i].p0[2];
        vertices[4 * i + 1].uv[0] = items[i].uv1[0];
        vertices[4 * i + 1].uv[1] = items[i].uv0[1];

        vertices[4 * i + 2].pos[0] = items[i].p1[0];
        vertices[4 * i + 2].pos[1] = items[i].p1[1];
        vertices[4 * i + 2].pos[2] = items[i].p0[2];
        vertices[4 * i + 2].uv[0] = items[i].uv1[0];
        vertices[4 * i + 2].uv[1] = items[i].uv1[1];

        vertices[4 * i + 3].pos[0] = items[i].p0[0];
        vertices[4 * i + 3].pos[1] = items[i].p1[1];
        vertices[4 * i + 3].pos[2] = items[i].p0[2];
        vertices[4 * i + 3].uv[0] = items[i].uv0[0];
        vertices[4 * i + 3].uv[1] = items[i].uv1[1];

        // Indices.
        indices[6 * i + 0] = 4 * i + 0;
        indices[6 * i + 1] = 4 * i + 1;
        indices[6 * i + 2] = 4 * i + 2;
        indices[6 * i + 3] = 4 * i + 2;
        indices[6 * i + 4] = 4 * i + 3;
        indices[6 * i + 5] = 4 * i + 0;
    }

    // Pass-through for the vertices.
    data.vertices = vertices;
    data.indices = indices;

    return data;
}

VkyVisual* vky_visual_image(VkyScene* scene, const VkyTextureParams* params)
{
    ASSERT(params != NULL);
    VkyVisual* visual = vky_create_visual(scene, VKY_VISUAL_IMAGE);
    VkyCanvas* canvas = scene->canvas;

    // Shaders.
    VkyShaders shaders = vky_create_shaders(canvas->gpu);
    vky_add_shader(&shaders, VK_SHADER_STAGE_VERTEX_BIT, "image.vert.spv");
    vky_add_shader(&shaders, VK_SHADER_STAGE_FRAGMENT_BIT, "image.frag.spv");

    // Vertex layout.
    VkyVertexLayout vertex_layout =
        vky_create_vertex_layout(canvas->gpu, 0, sizeof(VkyTextureVertex));
    vky_add_vertex_attribute(
        &vertex_layout, 0, VKY_DEFAULT_VERTEX_FORMAT_POS, offsetof(VkyTextureVertex, pos));
    vky_add_vertex_attribute(
        &vertex_layout, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(VkyTextureVertex, uv));

    // Resource layout.
    VkyResourceLayout resource_layout = vky_common_resource_layout(visual);
    vky_add_resource_binding(&resource_layout, 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    // Pipeline.
    visual->pipeline = vky_create_graphics_pipeline(
        canvas, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, shaders, vertex_layout, resource_layout,
        (VkyGraphicsPipelineParams){true});

    // Add the texture.
    VkyTexture* tex = vky_add_texture(canvas->gpu, params);
    // HACK: to avoid empty texture, use an empty texture.
    void* pixels = calloc(params->width * params->height * params->depth, params->format_bytes);
    vky_upload_texture(tex, pixels);
    free(pixels);

    // Resources.
    vky_add_common_resources(visual);
    vky_add_texture_resource(visual, tex);

    visual->cb_bake_data = vky_visual_image_bake;

    return visual;
}

void vky_visual_image_upload(VkyVisual* visual, const void* pixels)
{
    ASSERT(visual->visual_type == VKY_VISUAL_IMAGE);
    ASSERT(visual->resource_count == 3); // MVP, color texture, visual's image
    VkyTexture* texture = (VkyTexture*)visual->resources[2];
    ASSERT(texture != NULL);
    vky_upload_texture(texture, pixels);
}



/*************************************************************************************************/
/*  Raw path visual                                                                              */
/*************************************************************************************************/

VkyVisual* vky_visual_path_raw(VkyScene* scene)
{
    VkyVisual* visual = vky_create_visual(scene, VKY_VISUAL_PATH_RAW);
    VkyCanvas* canvas = scene->canvas;

    // Shaders.
    VkyShaders shaders = vky_create_shaders(canvas->gpu);
    vky_add_shader(&shaders, VK_SHADER_STAGE_VERTEX_BIT, "raw_path.vert.spv");
    vky_add_shader(&shaders, VK_SHADER_STAGE_FRAGMENT_BIT, "raw_path.frag.spv");

    // Vertex layout.
    VkyVertexLayout vertex_layout = vky_create_vertex_layout(canvas->gpu, 0, sizeof(VkyVertex));
    vky_add_vertex_attribute(
        &vertex_layout, 0, VKY_DEFAULT_VERTEX_FORMAT_POS, offsetof(VkyVertex, pos));
    vky_add_vertex_attribute(
        &vertex_layout, 1, VKY_DEFAULT_VERTEX_FORMAT_COLOR, offsetof(VkyVertex, color));

    // Resource layout.
    VkyResourceLayout resource_layout = vky_common_resource_layout(visual);

    // Pipeline.
    visual->pipeline = vky_create_graphics_pipeline(
        canvas, VK_PRIMITIVE_TOPOLOGY_LINE_STRIP, shaders, vertex_layout, resource_layout,
        (VkyGraphicsPipelineParams){true});

    // Resources.
    vky_add_common_resources(visual);

    return visual;
}



/*************************************************************************************************/
/*  Raw multi path visual                                                                        */
/*************************************************************************************************/

static VkyData vky_path_raw_multi_bake(VkyVisual* visual, VkyData data)
{
    const VkyMultiRawPathParams* params = (const VkyMultiRawPathParams*)visual->params;

    // Determine the actual number of vertices and indices.
    uint32_t nv = data.item_count; // total number of points
    uint32_t path_count = (uint32_t)params->info[0];
    ASSERT(nv % path_count == 0);
    uint32_t vertex_count_per_path = nv / path_count;
    uint32_t ni = nv;

    ASSERT(nv > 0);
    ASSERT(ni > 0);

    data.vertex_count = nv;
    data.index_count = ni;

    if (data.items == NULL)
    {
        return data;
    }

    // We don't allocate data for the vertices, we pass through the user data.
    data.no_vertices_alloc = true;
    // Allocate the data buffer to be uploaded to the vertex buffer. Will be freed by visky.
    VkyIndex* indices = calloc(ni, sizeof(VkyIndex));

    uint32_t offset = 0;
    for (uint32_t path_idx = 0; path_idx < path_count; path_idx++)
    {
        for (uint32_t i = 0; i < vertex_count_per_path; i++)
        {
            indices[offset + i] = path_idx + i * path_count;
        }
        offset += vertex_count_per_path;
    }
    ASSERT(offset == ni);

    // Pass-through for the vertices.
    data.vertices = data.items;
    data.indices = indices;

    return data;
}

VkyVisual* vky_visual_path_raw_multi(VkyScene* scene, const VkyMultiRawPathParams* params)
{
    VkyVisual* visual = vky_create_visual(scene, VKY_VISUAL_PATH_RAW_MULTI);
    VkyCanvas* canvas = scene->canvas;

    // Shaders.
    VkyShaders shaders = vky_create_shaders(canvas->gpu);
    vky_add_shader(&shaders, VK_SHADER_STAGE_VERTEX_BIT, "raw_multi_path.vert.spv");
    vky_add_shader(&shaders, VK_SHADER_STAGE_FRAGMENT_BIT, "raw_multi_path.frag.spv");

    // Vertex layout.
    // TODO: customizable data format.
    VkyVertexLayout vertex_layout = vky_create_vertex_layout(canvas->gpu, 0, sizeof(int16_t));
    vky_add_vertex_attribute(&vertex_layout, 0, VK_FORMAT_R16_SSCALED, 0);
    // TODO: if Metal, need to use R32_SFLOAT and conversion from int16 to float in baking

    // Params.
    vky_visual_params(visual, sizeof(VkyMultiRawPathParams), params);

    // Resource layout.
    // VkyResourceLayout resource_layout = vky_create_resource_layout(canvas->gpu,
    // canvas->image_count);
    VkyResourceLayout resource_layout = vky_common_resource_layout(visual);

    // Pipeline.
    visual->pipeline = vky_create_graphics_pipeline(
        canvas, VK_PRIMITIVE_TOPOLOGY_LINE_STRIP, shaders, vertex_layout, resource_layout,
        (VkyGraphicsPipelineParams){true});

    // Resources.
    vky_add_common_resources(visual);

    visual->cb_bake_data = vky_path_raw_multi_bake;

    return visual;
}



/*************************************************************************************************/
/*  Raw markers visual                                                                           */
/*************************************************************************************************/

VkyVisual* vky_visual_marker_raw(VkyScene* scene, const VkyMarkersRawParams* params)
{
    VkyVisual* visual = vky_create_visual(scene, VKY_VISUAL_MARKER_RAW);
    VkyCanvas* canvas = scene->canvas;

    // Shaders.
    VkyShaders shaders = vky_create_shaders(canvas->gpu);
    vky_add_shader(&shaders, VK_SHADER_STAGE_VERTEX_BIT, "raw_markers.vert.spv");
    vky_add_shader(&shaders, VK_SHADER_STAGE_FRAGMENT_BIT, "raw_markers.frag.spv");

    // Vertex layout.
    VkyVertexLayout vertex_layout = vky_create_vertex_layout(canvas->gpu, 0, sizeof(VkyVertex));
    vky_add_vertex_attribute(
        &vertex_layout, 0, VKY_DEFAULT_VERTEX_FORMAT_POS, offsetof(VkyVertex, pos));
    vky_add_vertex_attribute(
        &vertex_layout, 1, VKY_DEFAULT_VERTEX_FORMAT_COLOR, offsetof(VkyVertex, color));



    // Default params.
    VkyMarkersRawParams vparams = {0};
    if (params != NULL)
    {
        memcpy(&vparams, params, sizeof(VkyMarkersRawParams));
    }
    else
    {
        // TODO: constants
        vparams.marker_size[0] = 10;
        vparams.marker_size[1] = 10;
    }
    // DPI scaling factor.
    vparams.marker_size[0] *= canvas->dpi_factor;
    vparams.marker_size[1] *= canvas->dpi_factor;
    vky_visual_params(visual, sizeof(VkyMarkersRawParams), &vparams);
    // Resource layout.
    VkyResourceLayout resource_layout = vky_common_resource_layout(visual);

    // Pipeline.
    visual->pipeline = vky_create_graphics_pipeline(
        canvas, VK_PRIMITIVE_TOPOLOGY_POINT_LIST, shaders, vertex_layout, resource_layout,
        (VkyGraphicsPipelineParams){false});

    // Resources.
    vky_add_common_resources(visual);

    return visual;
}



/*************************************************************************************************/
/*  Mesh visual                                                                                  */
/*************************************************************************************************/

VkyMeshParams vky_default_mesh_params(
    VkyMeshColorType color_type, VkyMeshShading shading, ivec2 tex_size, float wire_linewidth)
{
    VkyMeshParams params = {
        VKY_DEFAULT_LIGHT_POSITION, VKY_DEFAULT_LIGHT_COEFFS, {tex_size[0], tex_size[1]},
        (int32_t)color_type,        (int32_t)shading,         wire_linewidth,
    };
    return params;
}


VkyVisual*
vky_visual_mesh(VkyScene* scene, const VkyMeshParams* params, const VkyTextureParams* tparams)
{
    VkyVisual* visual = vky_create_visual(scene, VKY_VISUAL_MESH);
    VkyCanvas* canvas = scene->canvas;

    // Shaders.
    VkyShaders shaders = vky_create_shaders(canvas->gpu);
    vky_add_shader(&shaders, VK_SHADER_STAGE_VERTEX_BIT, "mesh.vert.spv");
    vky_add_shader(&shaders, VK_SHADER_STAGE_FRAGMENT_BIT, "mesh.frag.spv");

    // Vertex layout.
    VkyVertexLayout vertex_layout =
        vky_create_vertex_layout(canvas->gpu, 0, sizeof(VkyMeshVertex));
    vky_add_vertex_attribute(
        &vertex_layout, 0, VKY_DEFAULT_VERTEX_FORMAT_POS, offsetof(VkyMeshVertex, pos));
    vky_add_vertex_attribute(
        &vertex_layout, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VkyMeshVertex, normal));
    vky_add_vertex_attribute(
        &vertex_layout, 2, VK_FORMAT_R8G8B8A8_UINT, offsetof(VkyMeshVertex, color));

    // Params.
    if (tparams != NULL && (params->tex_size[0] != (int32_t)tparams->width ||
                            params->tex_size[1] != (int32_t)tparams->height))
    {
        log_error("texture size specified in mesh params does not match texture params");
        tparams = NULL;
    }
    vky_visual_params(visual, sizeof(VkyMeshParams), params);

    // Resource layout.
    VkyResourceLayout resource_layout = vky_common_resource_layout(visual);
    vky_add_resource_binding(&resource_layout, 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    // Pipeline.
    visual->pipeline = vky_create_graphics_pipeline(
        canvas, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, shaders, vertex_layout, resource_layout,
        (VkyGraphicsPipelineParams){true});

    // Resources.
    vky_add_common_resources(visual);
    // Add the texture.
    VkyTexture* tex = NULL;
    if (tparams != NULL)
    {
        tex = vky_add_texture(canvas->gpu, tparams);
        // HACK: to avoid empty texture, use an empty texture.
        {
            void* pixels =
                calloc(tparams->width * tparams->height * tparams->depth, tparams->format_bytes);
            vky_upload_texture(tex, pixels);
            free(pixels);
        }
    }
    else
    {
        // HACK: use an existing texture if none is provided in order to prevent Vulkan errors
        tex = &canvas->gpu->textures[0];
    }
    ASSERT(tex != NULL);
    vky_add_texture_resource(visual, tex);

    return visual;
}


void vky_visual_mesh_upload(VkyVisual* visual, const void* pixels)
{
    ASSERT(visual->visual_type == VKY_VISUAL_MESH);
    ASSERT(visual->resource_count == 3); // MVP, color texture, visual's image
    VkyTexture* texture = (VkyTexture*)visual->resources[2];
    ASSERT(texture != NULL);
    vky_upload_texture(texture, pixels);
}



/*************************************************************************************************/
/*  Raw mesh visual                                                                              */
/*************************************************************************************************/

VkyVisual* vky_visual_mesh_raw(VkyScene* scene)
{
    VkyVisual* visual = vky_create_visual(scene, VKY_VISUAL_MESH_RAW);
    VkyCanvas* canvas = scene->canvas;

    // Shaders.
    VkyShaders shaders = vky_create_shaders(canvas->gpu);
    vky_add_shader(&shaders, VK_SHADER_STAGE_VERTEX_BIT, "mesh_raw.vert.spv");
    vky_add_shader(&shaders, VK_SHADER_STAGE_FRAGMENT_BIT, "mesh_raw.frag.spv");

    // Vertex layout.
    VkyVertexLayout vertex_layout = vky_create_vertex_layout(canvas->gpu, 0, sizeof(VkyVertex));
    vky_add_vertex_attribute(
        &vertex_layout, 0, VKY_DEFAULT_VERTEX_FORMAT_POS, offsetof(VkyVertex, pos));
    vky_add_vertex_attribute(
        &vertex_layout, 1, VK_FORMAT_R8G8B8A8_UNORM, offsetof(VkyVertex, color));

    // Resource layout.
    VkyResourceLayout resource_layout = vky_common_resource_layout(visual);

    // Pipeline.
    visual->pipeline = vky_create_graphics_pipeline(
        canvas, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, shaders, vertex_layout, resource_layout,
        (VkyGraphicsPipelineParams){true});

    vky_add_common_resources(visual);
    return visual;
}

VkyVisual* vky_visual_mesh_flat(VkyScene* scene)
{
    VkyVisual* visual = vky_create_visual(scene, VKY_VISUAL_MESH_RAW);
    VkyCanvas* canvas = scene->canvas;

    // Shaders.
    VkyShaders shaders = vky_create_shaders(canvas->gpu);
    vky_add_shader(&shaders, VK_SHADER_STAGE_VERTEX_BIT, "mesh_flat.vert.spv");
    vky_add_shader(&shaders, VK_SHADER_STAGE_FRAGMENT_BIT, "mesh_flat.frag.spv");

    // Vertex layout.
    VkyVertexLayout vertex_layout = vky_create_vertex_layout(canvas->gpu, 0, sizeof(VkyVertex));
    vky_add_vertex_attribute(
        &vertex_layout, 0, VKY_DEFAULT_VERTEX_FORMAT_POS, offsetof(VkyVertex, pos));
    vky_add_vertex_attribute(
        &vertex_layout, 1, VK_FORMAT_R8G8B8A8_UNORM, offsetof(VkyVertex, color));

    // Resource layout.
    VkyResourceLayout resource_layout = vky_common_resource_layout(visual);

    // Pipeline.
    visual->pipeline = vky_create_graphics_pipeline(
        canvas, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, shaders, vertex_layout, resource_layout,
        (VkyGraphicsPipelineParams){true});

    vky_add_common_resources(visual);
    return visual;
}



void vky_mesh_upload(VkyMesh* mesh, VkyVisual* visual)
{
    ASSERT(visual->visual_type == VKY_VISUAL_MESH || visual->visual_type == VKY_VISUAL_MESH_RAW);
    VkyData data = vky_mesh_data(mesh);
    vky_visual_upload(visual, data);
}
