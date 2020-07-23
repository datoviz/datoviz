#include <visky/visky.h>



int main()
{
    log_set_level_env();

    const uint32_t point_count = 31244;
    dvec2* points = NULL;
    uint32_t poly_count = 1;

    // Load the France points.
    char path[1024];
    snprintf(path, sizeof(path), "%s/misc/departements.polypoints.bin", DATA_DIR);
    points = (dvec2*)read_file(path, NULL);

    // Load the polygon lengths.
    snprintf(path, sizeof(path), "%s/misc/departements.polylengths.bin", DATA_DIR);
    uint32_t* poly_lengths = (uint32_t*)read_file(path, NULL);
    poly_count = 131;

    // Convert longitudes/latitudes to pixels.
    vky_earth_to_pixels(point_count, points);

    // Polygon colors
    VkyColorBytes* poly_colors = calloc(poly_count, sizeof(VkyColorBytes));
    for (uint32_t i = 0; i < poly_count; i++)
    {
        poly_colors[i] = vky_color(VKY_CPAL256_GLASBEY, i % 256, 0, poly_count, 1);
    }

    // Create the canvas.
    VkyApp* app = vky_create_app(VKY_DEFAULT_BACKEND);
    VkyCanvas* canvas = vky_create_canvas(app, VKY_DEFAULT_WIDTH, VKY_DEFAULT_HEIGHT);
    VkyScene* scene = vky_create_scene(canvas, VKY_CLEAR_COLOR_WHITE, 1, 1);

    // Set the panel controllers.
    VkyPanel* panel = vky_get_panel(scene, 0, 0);
    vky_set_controller(panel, VKY_CONTROLLER_PANZOOM, NULL);
    vky_set_panel_aspect_ratio(panel, 1);

    // Create the polygon visual bundle.
    VkyVisualBundle* vb = vky_bundle_polygon(scene, (VkyPolygonParams){2, {0, 0, 0, 255}});
    vky_add_visual_bundle_to_panel(vb, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);

    vky_bundle_polygon_upload(
        vb,                                        // visual bundle
        point_count, (const dvec2*)points,         // points
        poly_count, (const uint32_t*)poly_lengths, // polygons
        (const VkyColorBytes*)poly_colors          // polygon colors
    );

    // Run app.
    vky_run_app(app);

    // Cleanup
    vky_destroy_scene(scene);
    vky_destroy_app(app);
    free(points);
    free(poly_lengths);

    return 0;
}
