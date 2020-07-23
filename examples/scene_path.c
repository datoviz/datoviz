#include <visky/visky.h>

int main()
{
    log_set_level_env();

    VkyApp* app = vky_create_app(VKY_DEFAULT_BACKEND);
    VkyCanvas* canvas = vky_create_canvas(app, VKY_DEFAULT_WIDTH, VKY_DEFAULT_HEIGHT);
    VkyScene* scene = vky_create_scene(canvas, VKY_CLEAR_COLOR_WHITE, 1, 1);

    vky_add_vertex_buffer(canvas->gpu, 1e5);
    vky_add_index_buffer(canvas->gpu, 1e5);

    // Set the panel controllers.
    VkyPanel* panel = vky_get_panel(scene, 0, 0);
    vky_set_controller(panel, VKY_CONTROLLER_PANZOOM, NULL);

    // Create the visual.
    VkyPathParams params = {20, 4., VKY_CAP_ROUND, VKY_JOIN_ROUND, VKY_DEPTH_DISABLE};
    VkyVisual* visual = vky_visual_path(scene, params);
    vky_add_visual_to_panel(visual, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);

    // Upload the data.
    const uint32_t n = 1000;
    vec3 points[n];
    VkyColorBytes color[n];
    double t = 0;
    for (uint32_t i = 0; i < n; i++)
    {
        t = (float)i / n;

        points[i][0] = -1 + 2 * t;
        points[i][0] *= .9;
        points[i][1] = .5 * sin(8 * M_PI * t);
        points[i][2] = 0;

        if (i >= n / 2)
            points[i][1] += .25;
        else
            points[i][1] -= .25;

        color[i] = vky_color(VKY_CMAP_JET, (i >= n / 2), 0, 1, 1);
    }

    VkyPathData path1 = {n / 2, points, color, VKY_PATH_OPEN};
    VkyPathData path2 = {n / 2, points + (n / 2), color + (n / 2), VKY_PATH_OPEN};
    // paths is an array of VkyPathData, each item represent 1 path.
    vky_visual_upload(visual, (VkyData){2, (VkyPathData[]){path1, path2}});

    vky_run_app(app);
    vky_destroy_scene(scene);
    vky_destroy_app(app);
    return 0;
}
