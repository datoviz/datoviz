#include <visky/visky.h>

int main()
{
    log_set_level_env();

    VkyApp* app = vky_create_app(VKY_DEFAULT_BACKEND);
    VkyCanvas* canvas = vky_create_canvas(app, 1920, 1080);
    VkyScene* scene = vky_create_scene(canvas, VKY_CLEAR_COLOR_BLACK, 1, 1);

    // Set the panel controller.
    VkyPanel* panel = vky_get_panel(scene, 0, 0);
    vky_set_controller(panel, VKY_CONTROLLER_VOLUME, NULL);

    // Create the visual.
    VkyTextureParams params = {256,
                               256,
                               256,
                               1,
                               VK_FORMAT_R8_UNORM,
                               VK_FILTER_LINEAR,
                               VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                               0,
                               false};
    char path[1024];
    snprintf(path, sizeof(path), "%s/volume/skull.img", DATA_DIR);
    char* pixels = read_file(path, NULL);
    VkyVisual* visual = vky_visual_volume(scene, params, pixels);
    free(pixels);
    vky_add_visual_to_panel(visual, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);

    vky_run_app(app);
    vky_destroy_scene(scene);
    vky_destroy_app(app);
    return 0;
}
