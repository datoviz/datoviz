/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* controller_turntable - constrained turntable controller around a small 3D mesh.
 *
 * Scenario: feature.controller_turntable
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/controller_turntable
 * Run:    ./build/examples/c/features/controller_turntable
 * Smoke:  ./build/examples/c/features/controller_turntable 1
 * PNG:    DVZ_CAPTURE=png ./build/examples/c/features/controller_turntable 1
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/app.h"
#include "datoviz/geom.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  1600u
#define HEIGHT 1200u



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Add one colored cube with asymmetric lighting for constrained turntable motion.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param out_geometry geometry handle for cleanup on failure before upload completes
 * @return true on success
 */
static bool _add_turntable_cube(DvzScene* scene, DvzPanel* panel, DvzGeometry** out_geometry)
{
    const ExampleStyleColorRole face_roles[6] = {
        EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
        EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,
        EXAMPLE_STYLE_COLOR_WARNING,
        EXAMPLE_STYLE_COLOR_ERROR,
        EXAMPLE_STYLE_COLOR_TEXT,
        EXAMPLE_STYLE_COLOR_MINOR_TICK,
    };
    DvzVisual* visual = example_graphite_cyan_cube_mesh(scene, 1.16, face_roles, out_geometry);
    if (visual == NULL)
        return false;

    DvzMaterialDesc material = dvz_phong_material_desc();
    material.light_direction[0] = -0.28f;
    material.light_direction[1] = -0.22f;
    material.light_direction[2] = +0.78f;
    material.phong.ambient = 0.26f;
    material.phong.diffuse = 0.80f;
    material.phong.specular = 0.18f;
    material.phong.shininess = 24.0f;
    if (dvz_visual_set_material(visual, &material) != 0)
        return false;

    return dvz_panel_add_visual(panel, visual, NULL) == 0;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the turntable-controller feature example.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    const uint32_t frame_count = example_frame_count_any(argc, argv);
    DvzAppCaptureConfig capture = dvz_app_capture_config_from_env("feature_controller_turntable");

    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;
    DvzView* win = NULL;
    DvzGeometry* geometry = NULL;

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel_full(figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel_full() failed");
    example_graphite_cyan_set_panel_background(panel);
    EXAMPLE_CHECK(_add_turntable_cube(scene, panel, &geometry), "turntable cube setup failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "controller_turntable");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    DvzTurntableDesc desc = dvz_turntable_desc();
    desc.controller_flags = DVZ_TURNTABLE_FLAGS_CLAMP_DISTANCE;
    desc.up[0] = 0.0f;
    desc.up[1] = 0.0f;
    desc.up[2] = 1.0f;
    desc.distance = 3.65f;
    desc.yaw = -0.50f;
    desc.pitch = +0.28f;
    desc.min_pitch = -0.72f;
    desc.max_pitch = +0.72f;
    desc.min_distance = 2.40f;
    desc.max_distance = 6.20f;

    DvzTurntable* turntable = dvz_view_turntable(win, panel, &desc);
    EXAMPLE_CHECK(turntable != NULL, "failed to create or bind turntable controller");
    dvz_turntable_orbit(turntable, +0.42f, +0.24f);
    dvz_turntable_dolly(turntable, -0.30f);

    EXAMPLE_CHECK(
        example_run_with_capture(app, win, frame_count, &capture),
        "example_run_with_capture() failed");
    ret = 0;

cleanup:
    if (geometry != NULL)
        dvz_geometry_destroy(geometry);
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
