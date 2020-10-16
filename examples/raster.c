#include <visky/visky.h>

int main(int argc, char** argv)
{
    log_set_level_env();

    VkyApp* app = vky_create_app(VKY_DEFAULT_BACKEND);
    VkyCanvas* canvas = vky_create_canvas(app, VKY_DEFAULT_WIDTH, VKY_DEFAULT_HEIGHT);
    VkyScene* scene = vky_create_scene(canvas, VKY_CLEAR_COLOR_WHITE, 1, 1);
    VkyPanel* panel = vky_get_panel(scene, 0, 0);

    // Load the data.
    uint32_t size = 0;
    // char path[1024];

    if (argc < 4)
    {
        log_error("this example requires 3 command-line arguments: spike times, spike clusters, "
                  "spike depths");
        return 1;
    }


    // Load the data from disk.
    // Spike times.
    double* times = (double*)read_npy(argv[1], &size);
    ASSERT(times != NULL);
    ASSERT(size > 0);
    uint32_t n = size / 8; // 8 bytes per number
    ASSERT(times[n - 1] != 0);

    // Spike clusters.
    uint32_t* spike_clusters = (uint32_t*)read_npy(argv[2], &size);

    // Spike depths.
    double* depths = (double*)read_npy(argv[3], &size);
    dvec2s depth_min_max = vky_min_max(n, depths);

    // Set the axes controller.
    VkyAxes2DParams axparams = vky_default_axes_2D_params();
    axparams.xscale.vmin = 0;
    axparams.xscale.vmax = times[n - 1];
    axparams.yscale.vmin = depth_min_max.x;
    axparams.yscale.vmax = depth_min_max.y;
    vky_set_controller(panel, VKY_CONTROLLER_AXES_2D, &axparams);

    vky_add_vertex_buffer(canvas->gpu, 1e7);

    VkyMarkersRawParams params =
        (VkyMarkersRawParams){{2.0f, 10.0f}, VKY_SCALING_OFF, VKY_ALPHA_SCALING_ON};
    VkyVisual* visual = vky_visual(panel->scene, VKY_VISUAL_MARKER_RAW, &params, NULL);
    vky_add_visual_to_panel(visual, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);

    // Upload the data.
    VkyVertex* data = calloc(2 * n, sizeof(VkyVertex));
    float x, y;
    dvec2s dminmax = vky_min_max(n, depths);
    ASSERT(dminmax.x < dminmax.y);

    for (uint32_t i = 0; i < n; i++)
    {
        x = -1 + 2 * ((double)times[i] / times[n - 1]);
        y = -1 + 2 * (depths[i] - dminmax.x) / (dminmax.y - dminmax.x);
        data[i] = (VkyVertex){{x, y, 0}, {{0, 0, 0}, 4}};
        data[i].color = vky_color(VKY_CPAL256_GLASBEY, spike_clusters[i] % 32, 0, 32, .05);
    }
    visual->data.item_count = n;
    visual->data.items = data;
    vky_visual_data_raw(visual);
    FREE(times);
    FREE(depths);
    FREE(data);

    vky_run_app(app);
    vky_destroy_app(app);
    return 0;
}
