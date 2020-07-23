#include <visky/visky.h>

static inline dvec2s vky_min_max(size_t size, double* points)
{
    const double INF = 1e9;
    double min = +INF, max = -INF;
    for (uint32_t i = 0; i < size; i++)
    {
        if (points[i] < min)
            min = points[i];
        if (points[i] > max)
            max = points[i];
    }
    return (dvec2s){min, max};
}

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
    snprintf(path, sizeof(path), "%s/misc/spike_samples.bin", DATA_DIR);
    uint64_t* samples = (uint64_t*)read_file(path, &size);
    ASSERT(samples != NULL);
    ASSERT(size > 0);
    uint32_t n = size / 8;

    // Spike depths.
    snprintf(path, sizeof(path), "%s/misc/spike_depths.bin", DATA_DIR);
    double* depths = (double*)read_file(path, &size);

    // Set the axes controller.
    VkyAxes2DParams axparams = vky_default_axes_2D_params();
    // TODO: fix initial scale
    vky_set_controller(panel, VKY_CONTROLLER_AXES_2D, &axparams);

    vky_add_vertex_buffer(canvas->gpu, 1e7);

    VkyMarkersRawParams params = (VkyMarkersRawParams){{2.0f, 10.0f}, VKY_SCALING_OFF};
    VkyVisual* visual = vky_visual_marker_raw(panel->scene, params);
    vky_add_visual_to_panel(visual, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);

    // Upload the data.
    VkyVertex* data = calloc(2 * n, sizeof(VkyVertex));
    float x, y;
    dvec2s dminmax = vky_min_max(n, depths);

    for (uint32_t i = 0; i < n; i++)
    {
        x = -1 + 2 * ((double)samples[i] / samples[n - 1]);
        y = -1 + 2 * (depths[i] - dminmax.x) / (dminmax.y - dminmax.x);
        data[i] = (VkyVertex){{x, y, 0}, {0, 0, 0, 8}};
    }
    vky_visual_upload(visual, (VkyData){n, data});
    free(samples);
    free(depths);
    free(data);

    vky_run_app(app);
    vky_destroy_scene(scene);
    vky_destroy_app(app);
    return 0;
}
