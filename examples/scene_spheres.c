#include <visky/visky.h>

int main()
{
    log_set_level_env();

    // Create app.
    VkyApp* app = vky_create_app(VKY_DEFAULT_BACKEND);
    VkyCanvas* canvas = vky_create_canvas(app, VKY_DEFAULT_WIDTH, VKY_DEFAULT_HEIGHT);
    VkyScene* scene = vky_create_scene(canvas, VKY_CLEAR_COLOR_BLACK, 1, 1);

    // Set the panel controller.
    VkyPanel* panel = vky_get_panel(scene, 0, 0);
    vky_set_controller(panel, VKY_CONTROLLER_AUTOROTATE, NULL);

    // 3D data.
    const uint32_t n = 1000;
    VkyFakeSphereVertex* vertices = calloc(n, sizeof(VkyFakeSphereVertex));
    for (uint32_t i = 0; i < n; i++)
    {
        vertices[i] = (VkyFakeSphereVertex){
            RAND_POS_3D,
            vky_color(VKY_DEFAULT_COLORMAP, i, 0, n, 1),
            .01 + .1 * rand_float(),
        };
    }

    // Visual.
    VkyFakeSphereParams params = {0};
    glm_vec4_copy((vec4){2, 2, -2, 1}, params.light_pos);
    VkyVisual* visual = vky_visual_fake_sphere(scene, params);
    vky_add_visual_to_panel(visual, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);
    vky_visual_upload(visual, (VkyData){n, vertices});

    // Run app and exit.
    vky_run_app(app);
    vky_destroy_scene(scene);
    vky_destroy_app(app);

    return 0;
}
