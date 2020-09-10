#include "../include/visky/visky.h"



/*************************************************************************************************/
/*  Graph visual bundle                                                                          */
/*************************************************************************************************/

void vky_graph_upload(
    VkyVisualBundle* vb,                      //
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

    vky_visual_upload(vb->visuals[0], edge_data);
    vky_visual_upload(vb->visuals[1], node_data);

    free(edge_items);
}

VkyVisualBundle* vky_bundle_graph(VkyScene* scene, VkyGraphParams params)
{
    VkyVisualBundle* vb = vky_create_visual_bundle(scene);

    VkyMarkersParams node_params = {0};
    vec4_copy(params.marker_edge_color, node_params.edge_color);
    node_params.edge_width = params.marker_edge_width;
    node_params.enable_depth = false;

    VkyVisual* edges = vky_visual_segment(scene);
    VkyVisual* nodes = vky_visual_marker(scene, &node_params);

    ASSERT(vb->visual_count == 0);
    vky_add_visual_to_bundle(vb, edges);
    vky_add_visual_to_bundle(vb, nodes);
    ASSERT(vb->visual_count == 2);

    return vb;
}



/*************************************************************************************************/
/*  Color bar visual bundle                                                                      */
/*************************************************************************************************/

#define TICK_DATA                                                                                 \
    {TO_BYTE(VKY_AXES_LIM_COLOR_R), TO_BYTE(VKY_AXES_LIM_COLOR_G), TO_BYTE(VKY_AXES_LIM_COLOR_B), \
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
            {TO_BYTE(VKY_AXES_TEXT_COLOR_R), TO_BYTE(VKY_AXES_TEXT_COLOR_G),
             TO_BYTE(VKY_AXES_TEXT_COLOR_B), TO_BYTE(VKY_AXES_TEXT_COLOR_A)},
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

VkyVisualBundle* vky_bundle_colorbar(VkyScene* scene, VkyColorbarParams params)
{
    VkyVisualBundle* vb = vky_create_visual_bundle(scene);

    // Colorbar visual.
    VkyVisual* colorbar = colorbar_visual(scene);
    VkyVisual* text = vky_visual_text(scene);
    VkyVisual* ticks = vky_visual_segment(scene);

    // Add the visuals to the bundle.
    vky_add_visual_to_bundle(vb, colorbar);
    vky_add_visual_to_bundle(vb, text);
    vky_add_visual_to_bundle(vb, ticks);

    // Upload the colorbar data.
    colorbar_upload(colorbar, params);
    colorbar_tick_upload(text, ticks, params);

    return vb;
}
