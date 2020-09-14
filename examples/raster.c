#include <visky/visky.h>

int main()
{
    log_set_level_env();

    VkyApp* app = vky_create_app(VKY_DEFAULT_BACKEND);
    VkyCanvas* canvas = vky_create_canvas(app, VKY_DEFAULT_WIDTH, VKY_DEFAULT_HEIGHT);
    VkyScene* scene = vky_create_scene(canvas, VKY_CLEAR_COLOR_WHITE, 1, 1);
    VkyPanel* panel = vky_get_panel(scene, 0, 0);

    // Load the data.
    uint32_t size = 0;
    char path[1024];

    // Spike samples.
    snprintf(path, sizeof(path), "%s/misc/spikes.samples.npy", DATA_DIR);
    uint64_t* samples = (uint64_t*)read_npy(path, &size);
    ASSERT(samples != NULL);
    ASSERT(size > 0);
    uint32_t n = size / 8; // 8 bytes per number
    ASSERT(samples[n - 1] != 0);

    // Spike depths.
    snprintf(path, sizeof(path), "%s/misc/spikes.depths.npy", DATA_DIR);
    double* depths = (double*)read_npy(path, &size);

    // Set the axes controller.
    VkyAxes2DParams axparams = vky_default_axes_2D_params();
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
        x = -1 + 2 * ((double)samples[i] / samples[n - 1]);
        y = -1 + 2 * (depths[i] - dminmax.x) / (dminmax.y - dminmax.x);
        data[i] = (VkyVertex){{x, y, 0}, {0, 0, 0, 4}};
    }
    vky_visual_upload(visual, (VkyData){n, data});
    free(samples);
    free(depths);
    free(data);

    vky_run_app(app);
    vky_destroy_app(app);
    return 0;
}
