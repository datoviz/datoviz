/*************************************************************************************************/
/*  Offscreen example                                                                            */
/*************************************************************************************************/

/// We import the library public header.
#include <datoviz.h>

// Entry point.
int main(int argc, char** argv)
{
    // Create app object.
    DvzApp* app = dvz_app(DVZ_APP_FLAGS_OFFSCREEN);
    DvzBatch* batch = dvz_app_batch(app);

    // Create a scene.
    DvzScene* scene = dvz_scene(batch);

    // Create a figure.
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);

    // Create a panel.
    DvzPanel* panel = dvz_panel_default(figure);

    // Panzoom.
    DvzPanzoom* pz = dvz_panel_panzoom(panel, 0);

    // Create a visual.
    DvzVisual* visual = dvz_point(batch, 0);

    // Allocate a number of points.
    const uint32_t n = 10000;
    dvz_point_alloc(visual, n);

    // Set the point positions.
    vec3* pos = dvz_mock_pos_2D(n, 0.25);
    dvz_point_position(visual, 0, n, pos, 0);
    FREE(pos);

    // Set the point RGBA colors.
    DvzColor* color = dvz_mock_color(n, 128);
    dvz_point_color(visual, 0, n, color, 0);
    FREE(color);

    // Set the point sizes.
    float* size = dvz_mock_uniform(n, 25, 50);
    dvz_point_size(visual, 0, n, size, 0);
    FREE(size);

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(panel, visual, 0);

    // Run the app.
    dvz_scene_run(scene, app, 0);

    // Screenshot.
    dvz_app_screenshot(app, dvz_figure_id(figure), "offscreen_example.png");

    // Cleanup.
    dvz_scene_destroy(scene);
    dvz_app_destroy(app);

    return 0;
}
