#include <visky/visky.h>

static void frame_callback(VkyCanvas* canvas)
{
    VkyScene* scene = canvas->scene;
    VkyPanel* panel = &scene->grid->panels[0];
    VkyPanzoom* panzoom = (VkyPanzoom*)panel->controller;

    vec3 axis = {0, 0, 1};
    glm_rotate_make(panzoom->model, vky_get_timer(), axis);
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

    // Create the visual.
    VkyVisual* visual = vky_visual_text(scene);
    vky_add_visual_to_panel(visual, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);

    // Upload the data.
    uint32_t n = 128;
    float t = 0;
    VkyTextData text[n];
    for (uint32_t i = 0; i < n; i++)
    {
        t = M_2PI * (float)i / n;
        text[i] = (VkyTextData){
            {.75 * cos(t), .75 * sin(t), 0},
            {0, 0},
            vky_color(VKY_CMAP_RAINBOW, i, 0, n, 1),
            10,
            {0, 0},
            M_2PI * (float)i / n,
            12,
            "Hello world!"};
    }
    vky_visual_upload(visual, (VkyData){n, text});

    vky_add_frame_callback(canvas, frame_callback);

    vky_run_app(app);
    vky_destroy_scene(scene);
    vky_destroy_app(app);
    return 0;
}
