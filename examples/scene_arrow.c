#include <visky/visky.h>

int main()
{
    log_set_level_env();

    VkyApp* app = vky_create_app(VKY_DEFAULT_BACKEND);
    VkyCanvas* canvas = vky_create_canvas(app, VKY_DEFAULT_WIDTH, VKY_DEFAULT_HEIGHT);
    VkyScene* scene = vky_create_scene(canvas, VKY_CLEAR_COLOR_WHITE, 1, 1);

    // Set the panel controllers.
    VkyPanel* panel = vky_get_panel(scene, 0, 0);
    vky_set_controller(panel, VKY_CONTROLLER_PANZOOM, NULL);

    // Create the visual.
    VkyVisual* visual = vky_visual_arrow(scene);
    vky_add_visual_to_panel(visual, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);

    // Upload the data.
    const uint32_t n = 20;
    VkyArrowVertex data[n];
    double t = 0;
    double r = .25;
    double R = .75;
    for (uint32_t i = 0; i < n; i++)
    {
        t = 2 * M_PI * (float)i / n;
        data[i] = (VkyArrowVertex){
            {R * cos(t), R * sin(t), 0},
            {r * cos(t), r * sin(t), 0},
            vky_color(VKY_DEFAULT_COLORMAP, i, 0, n, 1),
            15,
            5,
            VKY_ARROW_STEALTH};
    }
    vky_visual_upload(visual, (VkyData){n, data});

    vky_reset_all_mvp(scene);
    vky_run_app(app);
    vky_destroy_scene(scene);
    vky_destroy_app(app);
    return 0;
}
