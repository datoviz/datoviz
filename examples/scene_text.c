#include <visky/visky.h>

#define STRING "Hello world!"
const uint32_t n_chars = strlen(STRING);

static void upload_text(VkyVisual* visual)
{
    // Upload the data.
    uint32_t n = 64;
    float t = visual->scene->canvas->local_time;
    VkyTextData text[n];
    for (uint32_t i = 0; i < n; i++)
    {
        // t = M_2PI * (float)i / n;
        text[i] = (VkyTextData){
            .pos = {.5 * cos(M_2PI * (float)i / n), .5 * sin(M_2PI * (float)i / n), 0},
            .shift = {0, 0},
            .color = vky_color(VKY_CMAP_HSV, fmod(i + t, n), 0, n, .75),
            .glyph_size = 40 + 20 * cos(3 * t + i),
            .anchor = {0, 0},
            .angle = -.67 * t + M_2PI * (float)i / n,
            .string = STRING,
            .string_len = n_chars,
            .is_static = false,
        };
    }
    vky_visual_upload(visual, (VkyData){n, text});
}

static void frame_callback(VkyCanvas* canvas)
{
    VkyScene* scene = canvas->scene;
    VkyPanel* panel = &scene->grid->panels[0];
    VkyPanzoom* panzoom = (VkyPanzoom*)panel->controller;
    VkyVisual* visual = &scene->visuals[0];
    upload_text(visual);

    vec3 axis = {0, 0, 1};
    glm_rotate_make(panzoom->model, .25 * canvas->local_time, axis);
}

int main()
{
    log_set_level_env();

    VkyApp* app = vky_create_app(VKY_DEFAULT_BACKEND);
    VkyCanvas* canvas = vky_create_canvas(app, VKY_DEFAULT_WIDTH, VKY_DEFAULT_HEIGHT);
    VkyScene* scene = vky_create_scene(canvas, VKY_CLEAR_COLOR_BLACK, 1, 1);

    // Set the panel controllers.
    VkyPanel* panel = vky_get_panel(scene, 0, 0);
    vky_set_controller(panel, VKY_CONTROLLER_PANZOOM, NULL);
    vky_set_panel_aspect_ratio(panel, 1);

    // Create the visual.
    VkyVisual* visual = vky_visual_text(scene);
    vky_add_visual_to_panel(visual, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);

    upload_text(visual);
    vky_add_frame_callback(canvas, frame_callback);

    vky_run_app(app);
    vky_destroy_scene(scene);
    vky_destroy_app(app);
    return 0;
}
