#include <visky/visky.h>


int main()
{
    log_set_level_env();

    VkyApp* app = vky_create_app(VKY_DEFAULT_BACKEND);
    VkyCanvas* canvas = vky_create_canvas(app, VKY_DEFAULT_WIDTH, VKY_DEFAULT_HEIGHT);
    VkyScene* scene = vky_create_scene(canvas, VKY_CLEAR_COLOR_WHITE, 1, 1);

    // Set the panel controller.
    VkyPanel* panel = vky_get_panel(scene, 0, 0);
    vky_set_controller(panel, VKY_CONTROLLER_AXES_3D, NULL);

    // Mesh visual.
    VkyVisual* visual =
        vky_visual_mesh(scene, VKY_MESH_COLOR_RGBA, VKY_MESH_SHADING_BLINN_PHONG, 0, NULL);
    vky_add_visual_to_panel(visual, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);

    // Load the mesh object.
    VkyMesh mesh = vky_create_mesh(1e6, 1e6);
    char path[1024];
    snprintf(path, sizeof(path), "%s/mesh/brain.obj", DATA_DIR);
    vky_mesh_obj(&mesh, path);
    vky_mesh_upload(&mesh, visual);
    vky_mesh_destroy(&mesh);

    // Run app and quit.
    vky_run_app(app);
    vky_destroy_scene(scene);
    vky_destroy_app(app);

    return 0;
}
