#include <visky/visky.h>

#define MAX_VERTEX_COUNT 10000000
#define MAX_INDEX_COUNT  10000000

static void surface(VkyPanel* panel)
{
    const uint32_t N = 250;
    uint32_t col_count = N + 1;
    uint32_t row_count = 2 * N + 1;
    uint32_t point_count = col_count * row_count;

    VkyScene* scene = panel->scene;
    VkyCanvas* canvas = scene->canvas;

    // Mesh object.
    VkyMesh mesh = vky_create_mesh(MAX_VERTEX_COUNT, MAX_INDEX_COUNT);

    // Mesh visual.
    VkyMeshParams params = vky_default_mesh_params(
        VKY_MESH_COLOR_RGBA, VKY_MESH_SHADING_BLINN_PHONG, (ivec2){0, 0}, 0);
    VkyVisual* visual = vky_visual(scene, VKY_VISUAL_MESH, &params, NULL);
    vky_add_vertex_buffer(canvas->gpu, MAX_VERTEX_COUNT * sizeof(VkyMeshVertex));
    vky_add_index_buffer(canvas->gpu, MAX_INDEX_COUNT * sizeof(VkyIndex));
    vky_allocate_vertex_buffer(visual, MAX_VERTEX_COUNT * sizeof(VkyMeshVertex));
    vky_allocate_index_buffer(visual, MAX_INDEX_COUNT * sizeof(VkyIndex));
    vky_add_visual_to_panel(visual, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);

    float* heights = calloc(point_count, sizeof(float));
    VkyColor* color = calloc(point_count, sizeof(VkyColor));
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
    vky_mesh_grid_surface(&mesh, row_count, col_count, p00, p01, p10, heights, (cvec4*)color);

    // Mesh visual.
    vky_mesh_upload(&mesh, visual);
    vky_mesh_destroy(&mesh);
}

static void spheres(VkyPanel* panel)
{
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
    VkyVisual* visual = vky_visual(panel->scene, VKY_VISUAL_FAKE_SPHERE, &params, NULL);
    vky_add_visual_to_panel(visual, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);
    vky_visual_data(visual, (VkyData){n, vertices});
}

static void volume(VkyPanel* panel)
{
    VkyScene* scene = panel->scene;
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
    VkyVisual* visual = vky_visual(scene, VKY_VISUAL_VOLUME, &params, pixels);
    free(pixels);
    vky_add_visual_to_panel(visual, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);
}

static void brain(VkyPanel* panel)
{
    // Mesh visual.
    VkyMeshParams params = vky_default_mesh_params(
        VKY_MESH_COLOR_RGBA, VKY_MESH_SHADING_BLINN_PHONG, (ivec2){0, 0}, 0);
    VkyVisual* visual = vky_visual(panel->scene, VKY_VISUAL_MESH, &params, NULL);
    vky_add_visual_to_panel(visual, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);

    // Load the mesh object.
    VkyMesh mesh = vky_create_mesh(1e6, 1e6);
    char path[1024];
    snprintf(path, sizeof(path), "%s/mesh/brain.obj", DATA_DIR);
    vky_mesh_obj(&mesh, path);
    vky_mesh_upload(&mesh, visual);
    vky_mesh_destroy(&mesh);
}
