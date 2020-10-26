#include <visky/visky.h>

#define IMAGE_SIZE 1024

static void frame_callback(VkyCanvas* canvas, void* data)
{
    if (canvas->frame_count == 10)
    {
        vky_screenshot(canvas, "artifacts/graph.png");
        canvas->to_close = true;
    }
    return;
}

int main(void)
{
    log_set_level_env();

    VkyApp* app = vky_create_app(VKY_BACKEND_OFFSCREEN, NULL);
    VkyCanvas* canvas = vky_create_canvas(app, IMAGE_SIZE, IMAGE_SIZE);
    VkyScene* scene = vky_create_scene(canvas, VKY_CLEAR_COLOR_WHITE, 1, 1);

    vky_add_vertex_buffer(canvas->gpu, 1e5);
    vky_add_index_buffer(canvas->gpu, 1e5);

    // Set the panel controller.
    VkyPanel* panel = vky_get_panel(scene, 0, 0);
    vky_set_controller(panel, VKY_CONTROLLER_PANZOOM, NULL);
    vky_set_panel_aspect_ratio(panel, 1);
    VkyPanzoom* panzoom = (VkyPanzoom*)panel->controller;
    panzoom->zoom[0] = .75;
    panzoom->zoom[1] = .75;

    // Graph parameters.
    VkyGraphParams params = {0};
    params.marker_edge_width = 1.0f;
    glm_vec4_copy(GLM_VEC4_BLACK, params.marker_edge_color);

    // Create the graph visual.
    VkyVisual* graph = vky_visual_graph(scene, params);
    vky_add_visual_to_panel(graph, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);

    // Create the graph.
    const uint32_t nv = 50;
    const uint32_t ne = nv * (nv - 1) / 2;
    VkyGraphNode* nodes = calloc(nv, sizeof(VkyGraphNode));
    VkyGraphEdge* edges = calloc(ne, sizeof(VkyGraphEdge));

    // Nodes.
    for (uint32_t i = 0; i < nv; i++)
    {
        float angle = M_2PI * (float)i / nv;
        nodes[i] = (VkyGraphNode){
            {.75f * cos(angle), .75f * sin(angle), 0},
            vky_color(VKY_DEFAULT_COLORMAP, i, 0, nv, 1),
            10.0f + 20.0f * rand_float(),
            VKY_MARKER_DISC};
    }

    // Edges.
    uint32_t k = 0;
    for (uint32_t i = 0; i < nv; i++)
    {
        for (uint32_t j = i + 1; j < nv; j++)
        {
            ASSERT(k < ne);
            edges[k] = (VkyGraphEdge){
                i,
                j,
                {{0, 0, 0}, 255}, // TODO: black
                1,
                VKY_CAP_ROUND,
                VKY_CAP_ROUND};
            // Hide some edges.
            if (rand_float() < .85)
            {
                edges[k].color.alpha = 0;
            }
            k++;
        }
    }
    ASSERT(k == ne);

    vky_graph_upload(graph, nv, nodes, ne, edges);

    FREE(nodes);
    FREE(edges);

    ASSERT(scene->visual_count == 3);

    vky_add_frame_callback(canvas, frame_callback, NULL);

    vky_run_app(app);
    vky_destroy_app(app);
    return 0;
}
