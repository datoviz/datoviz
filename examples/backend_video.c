#include <visky/visky.h>

int main()
{
    log_set_level_env();

    VkyApp* app = vky_create_app(VKY_BACKEND_OFFSCREEN, NULL);
    VkyCanvas* canvas = vky_create_canvas(app, 1920, 1080);
    VkyScene* scene = vky_create_scene(canvas, VKY_CLEAR_COLOR_BLACK, 1, 1);

    // Set the panel controller.
    VkyPanel* panel = vky_get_panel(scene, 0, 0);
    vky_set_controller(panel, VKY_CONTROLLER_AUTOROTATE, NULL);

    // Cube mesh.
    VkyMeshParams params = vky_default_mesh_params(
        VKY_MESH_COLOR_RGBA, VKY_MESH_SHADING_BLINN_PHONG, (ivec2){0, 0}, 0);
    VkyVisual* visual = vky_visual(scene, VKY_VISUAL_MESH, &params, NULL);
    vky_add_visual_to_panel(visual, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);

    // Create the mesh.
    VkyMesh mesh = vky_create_mesh(100, 100);
    VkyColor color[36];
    for (uint32_t i = 0; i < 36; i++)
    {
        color[i] = vky_color(VKY_CMAP_HSV, i / 6, 0, 6, 1);
    }
    vky_mesh_cube(&mesh, color);
    vky_mesh_upload(&mesh, visual);
    vky_mesh_destroy(&mesh);

    // Run app.
    vky_create_video(canvas, "artifacts/cube.mp4", 5, 30, 4000000);
    vky_destroy_app(app);
    return 0;
}
