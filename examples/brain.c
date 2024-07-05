/*************************************************************************************************/
/*  Scatter plot example                                                                         */
/*************************************************************************************************/

/// We import the library public header.
#include <datoviz.h>


// Callback function called at every mouse event (mouse, click, drag...)
static void show_arcball_angles(DvzApp* app, DvzId window_id, DvzMouseEvent ev)
{
    ANN(app);

    // We only run the callback function when mouse drag stops (button down, move, button up).
    if (ev.type != DVZ_MOUSE_EVENT_DRAG_STOP)
        return;

    // The user data is passed as last argument in dvz_app_onmouse().
    DvzArcball* arcball = (DvzArcball*)ev.user_data;
    ANN(arcball);

    // Get the arcball angles and display them.
    vec3 angles = {0};
    dvz_arcball_angles(arcball, angles);
    printf("Arcball angles: %.02f, %.02f, %.02f\n", angles[0], angles[1], angles[2]);
}


// Entry point.
int main(int argc, char** argv)
{
    // Create app object.
    DvzApp* app = dvz_app(0);
    DvzBatch* batch = dvz_app_batch(app);

    // Create a scene.
    DvzScene* scene = dvz_scene(batch);

    // Create a figure.
    DvzFigure* figure = dvz_figure(scene, 800, 600, DVZ_CANVAS_FLAGS_FPS);

    // Create a panel.
    DvzPanel* panel = dvz_panel_default(figure);

    // Arcball.
    DvzArcball* arcball = dvz_panel_arcball(scene, panel);

    // Set the initial arcball angles.
    dvz_arcball_initial(arcball, (vec3){+0.6, -1.2, +3.0});
    dvz_panel_update(panel); // IMPORTANT after changing the interactivity parameters

    // Perspective camera.
    DvzCamera* camera = dvz_panel_camera(panel);

    // File path to a .obj file.
    // This is a 3D mesh reconstruction of a mouse brain, provided by the Allen Institute.
    char path[1024];
    snprintf(path, sizeof(path), "data/mesh/brain.obj");

    // Load the obj file.
    DvzShape shape = dvz_shape_obj(path);
    if (!shape.vertex_count)
    {
        dvz_shape_destroy(&shape);
        return 0;
    }

    // Set the color of every vertex (the shape comes with an already allocated color array).
    for (uint32_t i = 0; i < shape.vertex_count; i++)
    {
        // Generate colors using the "bwr" colormap, in reverse (blue -> red).
        dvz_colormap_scale(
            DVZ_CMAP_BWR, shape.vertex_count - 1 - i, 0, shape.vertex_count, shape.color[i]);
    }

    // Create a mesh visual with basic lightingsupport.
    DvzVisual* visual = dvz_mesh_shape(batch, &shape, DVZ_MESH_FLAGS_LIGHTING);

    // Mesh lighting parameters.
    dvz_mesh_light_pos(visual, (vec4){-1, +1, +10, 0});    // Light position
    dvz_mesh_light_params(visual, (vec4){.5, .5, .5, 16}); // Ambient, diffuse, specular, exponent.

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(panel, visual);

    // Print the arcball angles in the terminal.
    dvz_app_onmouse(app, show_arcball_angles, arcball);

    // Run the app.
    dvz_scene_run(scene, app, 0);

    // Cleanup.
    dvz_scene_destroy(scene);
    dvz_app_destroy(app);

    return 0;
}
