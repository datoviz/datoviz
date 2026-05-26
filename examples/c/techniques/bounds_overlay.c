/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* bounds_overlay - display retained visual bounds in 2D and 3D panels.
 *
 * Build:  just example-c bounds_overlay
 * Run:    ./build/examples/c/techniques/bounds_overlay
 */

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "datoviz/app.h"
#include "datoviz/gui.h"
#include "datoviz/scene.h"
#include "example_common.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define POINT_COUNT 256
#define SPHERE_COUNT 32



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct BoundsOverlayState
{
    DvzPanel* panel_2d;
    DvzPanel* panel_3d;
    bool show_bounds;
} BoundsOverlayState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Fill a deterministic 2D spiral point cloud.
 *
 * @param positions output positions
 * @param colors output colors
 * @param sizes output point diameters
 */
static void _fill_points(vec3 positions[POINT_COUNT], DvzColor colors[POINT_COUNT],
                         float sizes[POINT_COUNT])
{
    for (uint32_t i = 0; i < POINT_COUNT; i++)
    {
        float t = (float)i / (float)(POINT_COUNT - 1);
        float angle = 9.0f * 3.14159265358979323846f * t;
        float radius = 0.08f + 0.82f * t;
        positions[i][0] = radius * cosf(angle);
        positions[i][1] = radius * sinf(angle);
        positions[i][2] = 0.0f;

        colors[i] = dvz_color_rgba(
            (uint8_t)(40.0f + 180.0f * t), (uint8_t)(210.0f - 70.0f * t),
            (uint8_t)(255.0f - 170.0f * t), 230);
        sizes[i] = 5.0f + 9.0f * t;
    }
}



/**
 * Fill deterministic 3D sphere centers and radii.
 *
 * @param positions output centers
 * @param colors output colors
 * @param radii output radii
 */
static void _fill_spheres(vec3 positions[SPHERE_COUNT], DvzColor colors[SPHERE_COUNT],
                          float radii[SPHERE_COUNT])
{
    for (uint32_t i = 0; i < SPHERE_COUNT; i++)
    {
        uint32_t ix = i % 4;
        uint32_t iy = (i / 4) % 4;
        uint32_t iz = i / 16;
        float jx = 0.035f * sinf(1.7f * (float)i);
        float jy = 0.035f * cosf(2.1f * (float)i);
        float jz = 0.055f * sinf(0.9f * (float)i);
        positions[i][0] = -0.54f + 0.36f * (float)ix + jx;
        positions[i][1] = -0.54f + 0.36f * (float)iy + jy;
        positions[i][2] = -0.36f + 0.72f * (float)iz + jz;

        float t = (float)i / (float)(SPHERE_COUNT - 1);
        colors[i] = dvz_color_rgb(
            (uint8_t)(230.0f - 120.0f * t), (uint8_t)(80.0f + 120.0f * t),
            (uint8_t)(120.0f + 100.0f * t));
        const float radius_classes[3] = {0.045f, 0.075f, 0.120f};
        uint32_t radius_class = (i * 7u + iz) % 3u;
        radii[i] = radius_classes[radius_class] + 0.006f * sinf(0.8f * (float)i);
    }
}



/**
 * Apply the GUI-controlled bounds visibility to both panels.
 *
 * @param state example state
 * @return whether both panels were updated
 */
static bool _apply_bounds_visibility(BoundsOverlayState* state)
{
    if (state == NULL)
        return false;
    int rc = dvz_panel_set_bounds_visible(state->panel_2d, state->show_bounds);
    if (rc != 0)
        return false;
    rc = dvz_panel_set_bounds_visible(state->panel_3d, state->show_bounds);
    return rc == 0;
}



/**
 * Render the example GUI controls.
 *
 * @param gui GUI overlay
 * @param win app view
 * @param user_data example state
 */
static void _bounds_overlay_gui(DvzGui* gui, DvzView* win, void* user_data)
{
    (void)win;
    BoundsOverlayState* state = (BoundsOverlayState*)user_data;
    if (gui == NULL || state == NULL)
        return;

    if (dvz_gui_begin(gui, "Bounds Overlay", NULL, 0))
    {
        bool changed = dvz_gui_checkbox(gui, "Show bounds", &state->show_bounds);
        if (changed)
            (void)_apply_bounds_visibility(state);
    }
    dvz_gui_end(gui);
}



/*************************************************************************************************/
/*  Main                                                                                         */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, 1200, 720, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel_2d = dvz_panel(figure, (DvzPanelDesc){0.04f, 0.08f, 0.44f, 0.84f});
    EXAMPLE_CHECK(panel_2d != NULL, "dvz_panel() failed for 2D panel");
    DvzPanel* panel_3d = dvz_panel(figure, (DvzPanelDesc){0.52f, 0.08f, 0.44f, 0.84f});
    EXAMPLE_CHECK(panel_3d != NULL, "dvz_panel() failed for 3D panel");
    dvz_panel_set_background_color(panel_2d, 0.06f, 0.07f, 0.09f, 1.0f);
    dvz_panel_set_background_color(panel_3d, 0.07f, 0.065f, 0.075f, 1.0f);

    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.eye[0] = 0.0f;
    camera_desc.eye[1] = -2.9f;
    camera_desc.eye[2] = 2.1f;
    camera_desc.target[0] = 0.0f;
    camera_desc.target[1] = 0.0f;
    camera_desc.target[2] = 0.0f;
    camera_desc.up[0] = 0.0f;
    camera_desc.up[1] = 0.0f;
    camera_desc.up[2] = 1.0f;
    camera_desc.fov_y = 0.72f;
    camera_desc.near = 0.05f;
    camera_desc.far = 100.0f;
    DvzCamera* camera = dvz_panel_set_camera(panel_3d, &camera_desc);
    EXAMPLE_CHECK(camera != NULL, "dvz_panel_set_camera() failed for 3D panel");

    DvzVisual* points = dvz_point(scene, 0);
    EXAMPLE_CHECK(points != NULL, "dvz_point() failed");
    vec3 point_positions[POINT_COUNT] = {0};
    DvzColor point_colors[POINT_COUNT] = {0};
    float point_sizes[POINT_COUNT] = {0};
    _fill_points(point_positions, point_colors, point_sizes);
    DvzVisualDataUpdate point_updates[] = {
        {.attr_name = "position", .data = point_positions, .item_count = POINT_COUNT},
        {.attr_name = "color", .data = point_colors, .item_count = POINT_COUNT},
        {.attr_name = "diameter", .data = point_sizes, .item_count = POINT_COUNT},
    };
    int rc = dvz_visual_set_data_many(points, point_updates, 3);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_data_many() failed for points");
    rc = dvz_panel_add_visual(panel_2d, points, NULL);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual() failed for points");

    DvzVisual* spheres = dvz_sphere(scene, 0);
    EXAMPLE_CHECK(spheres != NULL, "dvz_sphere() failed");
    rc = dvz_sphere_mode(spheres, DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR);
    EXAMPLE_CHECK(rc == 0, "dvz_sphere_mode() failed");
    vec3 sphere_positions[SPHERE_COUNT] = {0};
    DvzColor sphere_colors[SPHERE_COUNT] = {0};
    float sphere_radii[SPHERE_COUNT] = {0};
    _fill_spheres(sphere_positions, sphere_colors, sphere_radii);
    DvzVisualDataUpdate sphere_updates[] = {
        {.attr_name = "position", .data = sphere_positions, .item_count = SPHERE_COUNT},
        {.attr_name = "color", .data = sphere_colors, .item_count = SPHERE_COUNT},
        {.attr_name = "radius", .data = sphere_radii, .item_count = SPHERE_COUNT},
    };
    rc = dvz_visual_set_data_many(spheres, sphere_updates, 3);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_data_many() failed for spheres");
    rc = dvz_panel_add_visual(panel_3d, spheres, NULL);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual() failed for spheres");

    BoundsOverlayState state = {
        .panel_2d = panel_2d,
        .panel_3d = panel_3d,
        .show_bounds = true,
    };
    bool ok = _apply_bounds_visibility(&state);
    EXAMPLE_CHECK(ok, "dvz_panel_set_bounds_visible() failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win = dvz_view_glfw(app, figure, 1200, 720, "bounds_overlay");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    DvzPanzoom* panzoom = dvz_view_panzoom(win, panel_2d, NULL);
    EXAMPLE_CHECK(panzoom != NULL, "failed to create or bind panzoom controller");
    DvzArcball* arcball = dvz_view_arcball(win, panel_3d, NULL);
    EXAMPLE_CHECK(arcball != NULL, "failed to create or bind arcball controller");

    DvzGuiConfig gui_config = dvz_gui_config();
    DvzGui* gui = dvz_view_gui(win, &gui_config);
    EXAMPLE_CHECK(gui != NULL, "dvz_view_gui() failed");
    dvz_view_set_gui_callback(win, _bounds_overlay_gui, &state);

    dvz_app_run(app, example_frame_count(argc, argv));
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
