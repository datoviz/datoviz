#include <visky/visky.h>

int main()
{
    log_set_level_env();

    VkyApp* app = vky_create_app(VKY_DEFAULT_BACKEND);
    VkyCanvas* canvas = vky_create_canvas(app, VKY_DEFAULT_WIDTH, VKY_DEFAULT_HEIGHT);
    VkyScene* scene = vky_create_scene(canvas, VKY_CLEAR_COLOR_BLACK, 1, 1);

    // Set the panel controllers.
    VkyPanel* panel = vky_get_panel(scene, 0, 0);
    vky_set_controller(panel, VKY_CONTROLLER_PANZOOM, NULL);

    // Create the visual.
    VkyVisual* visual = vky_visual_mesh_raw(scene);
    vky_add_visual_to_panel(visual, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);

    // Upload the data.
    float x = .5;
    VkyVertex vertices[] = {
        {{-x, -x, 0}, {255, 0, 0, 255}},  {{+x, -x, 0}, {0, 255, 0, 255}},
        {{+x, x, 0}, {0, 0, 255, 255}},   {{+x, x, 0}, {0, 0, 255, 255}},
        {{-x, x, 0}, {255, 0, 255, 255}}, {{-x, -x, 0}, {255, 0, 0, 255}},
    };
    vky_visual_upload(visual, (VkyData){0, NULL, 6, vertices, 0, NULL});

    vky_run_app(app);
    vky_destroy_scene(scene);
    vky_destroy_app(app);
    return 0;
}
