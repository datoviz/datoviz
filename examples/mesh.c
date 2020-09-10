#include <visky/visky.h>

#define SQUARE   0
#define DISC     1
#define CUBE     2
#define SPHERE   3
#define CYLINDER 4
#define CONE     5
#define SURFACE  6
#define HARMONIC 7
#define COMBINED 8

const char* mesh_types[] = {"square", "disc",    "cube",     "sphere",  "cylinder",
                            "cone",   "surface", "harmonic", "combined"};
const uint32_t mesh_type_count = 9;

#define N                200
#define MESH             COMBINED
#define CONTROLLER       VKY_CONTROLLER_ARCBALL
#define SHADING          VKY_MESH_SHADING_BLINN_PHONG
#define MAX_VERTEX_COUNT 10000000
#define MAX_INDEX_COUNT  10000000

uint32_t col_count = 0;
uint32_t row_count = 0;
uint32_t point_count = 0;

VkyMesh mesh;
int current_mesh_type = MESH;
int new_mesh_type = MESH;
VkyVisual* visual = NULL;
VkyVisualBundle* vbt;
VkyPanel* panel = NULL;

static void square(void)
{
    VkyColorBytes color[6];
    for (uint32_t i = 0; i < 6; i++)
    {
        color[i] = vky_color(VKY_CMAP_HSV, 0, 0, 6, 1);
    }
    vky_mesh_square(&mesh, color);
}

static void disc(void)
{
    VkyColorBytes color[N];
    color[0].a = 1;
    for (uint32_t i = 0; i < N; i++)
    {
        color[i] = vky_color(VKY_CMAP_HSV, i + 1, 0, N, 1);
    }
    vky_mesh_disc(&mesh, N, color);
}

static void cube(void)
{
    VkyColorBytes color[36];
    for (uint32_t i = 0; i < 36; i++)
    {
        color[i] = vky_color(VKY_CMAP_HSV, i / 6, 0, 6, 1);
    }
    vky_mesh_cube(&mesh, color);
}

static void sphere(void)
{
    uint32_t nv = N * N;
    VkyColorBytes* color = calloc(nv, sizeof(uint32_t));
    for (uint32_t i = 0; i < N; i++)
    {
        for (uint32_t j = 0; j < N; j++)
        {
            color[i * N + j] = vky_color(VKY_CMAP_HSV, i, 0, N - 1, 1);
        }
    }
    vky_mesh_sphere(&mesh, N, N, color);
    free(color);
}

static void cylinder(void)
{
    uint32_t nv = 2 * N;
    VkyColorBytes* color = calloc(nv, sizeof(VkyColorBytes));
    for (uint32_t i = 0; i < 2; i++)
    {
        for (uint32_t j = 0; j < N; j++)
        {
            color[i * N + j] = vky_color(VKY_CMAP_HSV, j, 0, N - 1, 1);
        }
    }
    vky_mesh_cylinder(&mesh, N, color);
    free(color);
}

static void cone(void)
{
    uint32_t nv = 2 * N;
    VkyColorBytes* color = calloc(nv, sizeof(VkyColorBytes));
    for (uint32_t i = 0; i < 2; i++)
    {
        for (uint32_t j = 0; j < N; j++)
        {
            color[i * N + j] = vky_color(VKY_CMAP_HSV, j, 0, N - 1, 1);
        }
    }
    vky_mesh_cone(&mesh, N, color);
    free(color);
}

static void surface(void)
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
            // z = exp(-5*(x*x+y*y));
            z = .25 * sin(10 * x) * cos(10 * y);
            heights[col_count * i + j] = z;
            color[col_count * i + j] = vky_color(VKY_CMAP_JET, 2 * (z + .25), 0, 1, 1);
        }
    }
    vec3 p00 = {-1, 0, -1}, p10 = {+1, 0, -1}, p01 = {-1, 0, +1};
    vky_mesh_grid_surface(&mesh, row_count, col_count, p00, p01, p10, heights, (cvec4*)color);
}

static void harmonic(void)
{
    float dphi, dtheta;
    dphi = dtheta = M_PI / N;
    float m0 = 4;
    float m1 = 3;
    float m2 = 2;
    float m3 = 3;
    float m4 = 6;
    float m5 = 2;
    float m6 = 6;
    float m7 = 4;
    float r, phi, theta, x, y, z;
    vec3* positions = calloc(point_count, sizeof(vec3));
    VkyColorBytes* color = calloc(point_count, sizeof(VkyColorBytes));
    ASSERT(sizeof(VkyColorBytes) == sizeof(cvec4));
    for (uint32_t i = 0; i < row_count; i++)
    {
        theta = dtheta * i;
        for (uint32_t j = 0; j < col_count; j++)
        {
            phi = dphi * j;
            r = .5 * (pow(sin(m0 * phi), m1) + pow(cos(m2 * phi), m3) + pow(sin(m4 * theta), m5) +
                      pow(cos(m6 * theta), m7));
            x = r * sin(phi) * cos(theta);
            y = r * cos(phi);
            z = r * sin(phi) * sin(theta);
            glm_vec3_copy((vec3){x, y, z}, positions[col_count * i + j]);
            color[col_count * i + j] = vky_color(VKY_CMAP_JET, j, 0, col_count, 1);
        }
    }
    vky_mesh_grid(
        &mesh, row_count, col_count, (const vec3*)positions, (const VkyColorBytes*)color);
}

static void set_mesh()
{
    mesh = vky_create_mesh(MAX_VERTEX_COUNT, MAX_INDEX_COUNT);

    // The equation comes from: https://docs.enthought.com/mayavi/mayavi/mlab.html#a-demo
    switch (current_mesh_type)
    {
    case SQUARE:
        square();
        break;
    case DISC:
        disc();
        break;
    case CUBE:
        cube();
        break;
    case SPHERE:
        sphere();
        break;
    case CYLINDER:
        cylinder();
        break;
    case CONE:
        cone();
        break;
    case SURFACE:
        surface();
        break;
    case HARMONIC:
        harmonic();
        break;
    case COMBINED:

        vky_mesh_rotate(&mesh, M_PI / 4, (vec3){0, 0, 1});
        vky_mesh_translate(&mesh, (vec3){-.75, +.75, 0});
        cylinder();

        vky_mesh_transform_reset(&mesh);
        vky_mesh_rotate(&mesh, -M_PI / 2, (vec3){1, 0, 0});
        vky_mesh_rotate(&mesh, M_PI / 4, (vec3){0, 0, 1});
        vky_mesh_translate(&mesh, (vec3){-1.1, +1.1, 0});
        disc();

        // The transformations are cumulative until calls to vky_mesh_transform_reset()
        vky_mesh_transform_reset(&mesh);
        vky_mesh_translate(&mesh, (vec3){+.75, +.75, 0});
        sphere();

        vky_mesh_translate(&mesh, (vec3){0, -1.5, 0});
        cube();

        vky_mesh_transform_reset(&mesh);
        vky_mesh_scale(&mesh, (vec3){.5, .5, .5});
        vky_mesh_translate(&mesh, (vec3){-.75, -.75, 0});
        surface();

        break;
    default:
        break;
    }

    vky_mesh_upload(&mesh, visual);

    // if (vbt)
    //     vky_bundle_triangulation_upload(
    //         vbt,                                                           //
    //         mesh.vertex_offsets[mesh.object_count], sizeof(VkyMeshVertex), // vertices
    //         mesh.vertices,                                                 //
    //         mesh.index_offsets[mesh.object_count], mesh.indices);          // indices

    vky_mesh_destroy(&mesh);
}

static void mesh_selection(VkyCanvas* canvas)
{
    if (new_mesh_type != current_mesh_type)
    {
        current_mesh_type = new_mesh_type;
        set_mesh();
    }
}

int main()
{
    log_set_level_env();

    col_count = N + 1;
    row_count = 2 * N + 1;
    point_count = col_count * row_count;

    VkyApp* app = vky_create_app(VKY_DEFAULT_BACKEND);
    VkyCanvas* canvas = vky_create_canvas(app, VKY_DEFAULT_WIDTH, VKY_DEFAULT_HEIGHT);
    VkyScene* scene = vky_create_scene(canvas, VKY_CLEAR_COLOR_BLACK, 1, 1);

    // Set the panel controller.
    panel = vky_get_panel(scene, 0, 0);
    vky_set_controller(panel, CONTROLLER, NULL);

    // Mesh visual.
    VkyMeshParams params = vky_default_mesh_params(VKY_MESH_COLOR_RGBA, SHADING, (ivec2){0, 0}, 0);
    visual = vky_visual_mesh(scene, &params, NULL);
    vky_add_vertex_buffer(canvas->gpu, MAX_VERTEX_COUNT * sizeof(VkyMeshVertex));
    vky_add_index_buffer(canvas->gpu, MAX_INDEX_COUNT * sizeof(VkyIndex));
    vky_allocate_vertex_buffer(visual, MAX_VERTEX_COUNT * sizeof(VkyMeshVertex));
    vky_allocate_index_buffer(visual, MAX_INDEX_COUNT * sizeof(VkyIndex));
    vky_add_visual_to_panel(visual, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);

    // // Triangulation.
    // vbt = vky_bundle_triangulation(
    //     visual->scene, (VkyTriangulationParams){0.5, {255, 0, 0, 255}, {3, 3}, {0, 255, 0,
    //     255}});
    // vky_add_visual_bundle_to_panel(vbt, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);

    // Upload the mesh data.
    set_mesh();

    VkyGui* gui = vky_create_gui(canvas, (VkyGuiParams){VKY_GUI_STANDARD, 0});
    VkyGuiListParams gparams = {mesh_type_count, mesh_types};
    vky_gui_control(gui, VKY_GUI_LISTBOX, "", &gparams, &new_mesh_type);
    vky_add_frame_callback(canvas, mesh_selection);

    // Run app and quit.
    vky_run_app(app);
    vky_destroy_scene(scene);
    vky_destroy_app(app);
    return 0;
}
