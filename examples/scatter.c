#include "../include/visky/visky.h"

#define RANDOM_POS true

int main()
{
    log_set_level_env();
    // vky_set_constant(VKY_DPI_SCALING_FACTOR_ID, 2);

    VkyApp* app = vky_create_app(VKY_DEFAULT_BACKEND);
    VkyCanvas* canvas = vky_create_canvas(app, VKY_DEFAULT_WIDTH, VKY_DEFAULT_HEIGHT);
    VkyScene* scene = vky_create_scene(canvas, VKY_CLEAR_COLOR_WHITE, 1, 1);

    vky_add_vertex_buffer(canvas->gpu, 1e7);
    vky_add_index_buffer(canvas->gpu, 1e7);

    VkyPanel* panel = vky_get_panel(scene, 0, 0);

    // Create the visual.
    VkyMarkersParams params = (VkyMarkersParams){{0, 0, 0, 1}, 1.0f, false};
    VkyVisual* visual = vky_visual(scene, VKY_VISUAL_MARKER, &params, NULL);
    vky_add_visual_to_panel(visual, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);

    // Set the panel controller.
    VkyAxes2DParams axparams = vky_default_axes_2D_params();

    // Set the yscale.
    axparams.yscale.vmin = -25;
    axparams.yscale.vmax = +75;

    // x label
    strcpy(axparams.xlabel.label, "Scatter plot");
    axparams.xlabel.axis = VKY_AXIS_X;
    axparams.xlabel.color.a = TO_BYTE(VKY_AXES_LABEL_COLOR_A);
    axparams.xlabel.font_size = 12;

    // y label
    strcpy(axparams.ylabel.label, "Vertical axis");
    axparams.ylabel.axis = VKY_AXIS_Y;
    axparams.ylabel.color.a = TO_BYTE(VKY_AXES_LABEL_COLOR_A);
    axparams.ylabel.font_size = 12;

    axparams.colorbar.cmap = VKY_CMAP_VIRIDIS;
    vky_set_controller(panel, VKY_CONTROLLER_AXES_2D, &axparams);

    // Upload the data.
    const uint32_t n0 = 100;
    const uint32_t n = n0 * n0;
    VkyMarkersVertex* data = calloc(n, sizeof(VkyMarkersVertex));
    for (uint32_t i = 0; i < n; i++)
    {
        data[i] = (VkyMarkersVertex)
        {
#if RANDOM_POS
            {.25f * randn(), -.5 + .25f * randn(), 0.0f},
#else
            {-1 + .02 * (i % n0), -1 + .02 * (i / n0), 0},
#endif
                vky_color(VKY_CMAP_VIRIDIS, i % n0, 0, n0, .5 + .5 * rand_float()),
                RAND_MARKER_SIZE, VKY_MARKER_ARROW, i % 256
        };
    }
    vky_visual_upload(visual, (VkyData){n, data});
    free(data);

    vky_run_app(app);
    vky_destroy_scene(scene);
    vky_destroy_app(app);
    return 0;
}
