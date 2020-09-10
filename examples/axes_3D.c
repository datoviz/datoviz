#include <visky/visky.h>


#define N                100
#define MAX_VERTEX_COUNT 10000000
#define MAX_INDEX_COUNT  10000000

static const uint32_t col_count = N + 1;
static const uint32_t row_count = 2 * N + 1;
static const uint32_t point_count = col_count * row_count;

static const char* items[] = {"mesh", "spiral"};
static const char* citems[] = {"axes 3D", "FPS", "fly"};
static const VkyControllerType ctypes[] = {
    VKY_CONTROLLER_AXES_3D, VKY_CONTROLLER_FPS, VKY_CONTROLLER_FLY};
static int previous_item = 1;
static int current_item = 1;
static int cprevious_item = 0;
static int ccurrent_item = 0;
static VkyVisual* surface_visual = NULL;
static VkyVisual* spiral_visual = NULL;


static void generate_surface(VkyMesh* mesh)
{
    float* heights = calloc(point_count, sizeof(float));
    VkyColorBytes* color = calloc(point_count, sizeof(VkyColorBytes));
    float w = 1;
    float x, y, z;
    for (uint32_t i = 0; i < row_count; i++)
    {
        x = (float)i / (row_count - 1);
        x = -w + 2 * w * x;
        for (uint32_t j = 0; j < col_count; j++)
        {
            y = (float)j / (col_count - 1);
            y = -w + 2 * w * y;
            z = .25 * sin(10 * x) * cos(10 * y);
            heights[col_count * i + j] = z;
            color[col_count * i + j] = vky_color(VKY_CMAP_JET, 2 * (z + .25), 0, 1, 1);
        }
    }
    vec3 p00 = {-1, 0, -1}, p10 = {+1, 0, -1}, p01 = {-1, 0, +1};
    vky_mesh_grid_surface(mesh, row_count, col_count, p00, p01, p10, heights, (cvec4*)color);
}


static void spiral(VkyPanel* panel)
{
    // Create the path visual.
    VkyPathParams params = {10, 2., VKY_CAP_ROUND, VKY_JOIN_ROUND, VKY_DEPTH_ENABLE};
    spiral_visual = vky_visual_path(panel->scene, &params);
    vky_add_visual_to_panel(spiral_visual, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);
    const uint32_t N_path = 1000;
    vec3 points[1000];
    VkyColorBytes color[1000];
    double t = 0;
    for (uint32_t i = 0; i < N_path; i++)
    {
        t = (float)i / N_path;

        points[i][0] = t * cos(8 * M_2PI * t);
        points[i][1] = 2 * (.5 - t);
        points[i][2] = t * sin(8 * M_2PI * t);

        color[i] = vky_color(VKY_CMAP_JET, i, 0, N_path, 1);
    }
    VkyPathData path = {N_path, points, color, VKY_PATH_OPEN};
    vky_visual_upload(spiral_visual, (VkyData){1, (VkyPathData[]){path}});
}


static void surface(VkyPanel* panel)
{
    surface_visual =
        vky_visual_mesh(panel->scene, VKY_MESH_COLOR_RGBA, VKY_MESH_SHADING_BLINN_PHONG, 0, NULL);
    vky_allocate_vertex_buffer(surface_visual, MAX_VERTEX_COUNT * sizeof(VkyMeshVertex));
    vky_allocate_index_buffer(surface_visual, MAX_INDEX_COUNT * sizeof(VkyIndex));
    vky_add_visual_to_panel(surface_visual, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);
    VkyMesh mesh = vky_create_mesh(1e6, 1e6);
    generate_surface(&mesh);
    vky_mesh_upload(&mesh, surface_visual);
    vky_mesh_destroy(&mesh);
}


static void callback(VkyCanvas* canvas)
{
    if (current_item != previous_item)
    {
        vky_toggle_visual_visibility(current_item == 0 ? surface_visual : spiral_visual, true);
        vky_toggle_visual_visibility(current_item == 1 ? surface_visual : spiral_visual, false);
        previous_item = current_item;
    }
    if (ccurrent_item != cprevious_item)
    {
        vky_set_controller(&canvas->scene->grid->panels[0], ctypes[ccurrent_item], NULL);
        cprevious_item = ccurrent_item;
    }
}


int main()
{
    log_set_level_env();

    VkyApp* app = vky_create_app(VKY_DEFAULT_BACKEND);
    VkyCanvas* canvas = vky_create_canvas(app, VKY_DEFAULT_WIDTH, VKY_DEFAULT_HEIGHT);
    VkyScene* scene = vky_create_scene(canvas, VKY_CLEAR_COLOR_WHITE, 1, 1);
    vky_add_vertex_buffer(canvas->gpu, MAX_VERTEX_COUNT * sizeof(VkyMeshVertex));
    vky_add_index_buffer(canvas->gpu, MAX_INDEX_COUNT * sizeof(VkyIndex));

    // Set the panel controllers.
    VkyPanel* panel = vky_get_panel(scene, 0, 0);
    vky_set_controller(panel, VKY_CONTROLLER_AXES_3D, NULL);

    // axes(panel);
    surface(panel);
    vky_toggle_visual_visibility(surface_visual, false);
    spiral(panel);

    // GUI.
    VkyGui* gui = vky_create_gui(canvas, (VkyGuiParams){0, 0});

    VkyGuiListParams params = {2, items};
    vky_gui_control(gui, VKY_GUI_LISTBOX, "object", &params, &current_item);

    VkyGuiListParams cparams = {3, citems};
    vky_gui_control(gui, VKY_GUI_LISTBOX, "controller", &cparams, &ccurrent_item);
    vky_add_frame_callback(canvas, callback);

    vky_run_app(app);
    vky_destroy_scene(scene);
    vky_destroy_app(app);

    return 0;
}
