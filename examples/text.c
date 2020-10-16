#include <visky/visky.h>

#define STRING  "Hello world!"
#define N       64
#define N_CHARS (strlen(STRING))

static void upload_text(VkyVisual* visual)
{
    // Upload the data.
    float t = visual->scene->canvas->local_time;

    vec3 pos = {0};
    VkyColor color = {0};
    float size = {0};
    float angle = {0};

    for (uint32_t i = 0; i < N; i++)
    {
        pos[0] = .5 * cos(M_2PI * (float)i / N);
        pos[1] = .5 * sin(M_2PI * (float)i / N);
        color = vky_color(VKY_CMAP_HSV, fmod(i + t, N), 0, N, .75);
        size = 30 + 15 * cos(3 * t + i);
        angle = -.67 * t + M_2PI * (float)i / N;

        vky_visual_data_group(visual, VKY_VISUAL_PROP_POS_GPU, 0, i, 1, pos);
        vky_visual_data_group(visual, VKY_VISUAL_PROP_COLOR, 0, i, 1, &color);
        vky_visual_data_group(visual, VKY_VISUAL_PROP_SIZE, 0, i, 1, &size);
        vky_visual_data_group(visual, VKY_VISUAL_PROP_ANGLE, 0, i, 1, &angle);
    }
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
    VkyVisual* visual = vky_visual(scene, VKY_VISUAL_TEXT, NULL, NULL);
    vky_add_visual_to_panel(visual, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);

    // Set the groups.
    uint32_t* group_size = calloc(N, sizeof(uint32_t));
    for (uint32_t i = 0; i < N; i++)
        group_size[i] = N_CHARS;
    vky_visual_data_set_size(visual, N * N_CHARS, N, group_size, NULL);
    FREE(group_size);

    // Set the text.
    const char* str = STRING;
    for (uint32_t i = 0; i < N; i++)
        vky_visual_data_group(visual, VKY_VISUAL_PROP_TEXT, 0, i, strlen(str), str);

    upload_text(visual);
    vky_add_frame_callback(canvas, frame_callback);

    vky_run_app(app);
    vky_destroy_app(app);
    return 0;
}
