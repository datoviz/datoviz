/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* controller_fly - fly controller with deterministic camera translation.
 *
 * Scenario: feature.controller_fly
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/controller_fly
 * Run:    ./build/examples/c/features/controller_fly
 * Smoke:  ./build/examples/c/features/controller_fly 1
 * PNG:    DVZ_CAPTURE=png ./build/examples/c/features/controller_fly 1
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
 * Add one colored cube so fly-camera translation has visible parallax.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param out_geometry geometry handle for cleanup on failure before upload completes
 * @return true on success
 */
static bool _add_fly_cube(DvzScene* scene, DvzPanel* panel, DvzGeometry** out_geometry)
{
    DvzVisual* visual = dvz_mesh(scene, 0);
    if (visual == NULL)
        return false;

    const DvzColor face_colors[DVZ_GEOM_CUBE_FACE_COUNT] = {
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ERROR),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_MINOR_TICK),
    };
    DvzGeometry* cube = dvz_geom_cube(&(DvzGeometryCubeDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzGeometryCubeDesc),
        .size = 1.10,
        .face_colors = face_colors,
        .face_color_count = DVZ_GEOM_CUBE_FACE_COUNT,
    });
    if (cube == NULL)
        return false;
    if (out_geometry != NULL)
        *out_geometry = cube;

    if (!example_mesh_geometry(visual, cube))
        return false;
    dvz_geometry_destroy(cube);
    if (out_geometry != NULL)
        *out_geometry = NULL;

    DvzMaterialDesc material = dvz_phong_material_desc();
    material.light_direction[0] = -0.20f;
    material.light_direction[1] = -0.42f;
    material.light_direction[2] = +0.72f;
    material.phong.ambient = 0.30f;
    material.phong.diffuse = 0.76f;
    material.phong.specular = 0.14f;
    material.phong.shininess = 20.0f;
    if (dvz_visual_set_material(visual, &material) != 0)
        return false;

    return dvz_panel_add_visual(panel, visual, NULL) == 0;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the fly-controller feature example.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    const uint32_t frame_count = example_frame_count_any(argc, argv);
    DvzAppCaptureConfig capture = dvz_app_capture_config_from_env("feature_controller_fly");

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
    EXAMPLE_CHECK(_add_fly_cube(scene, panel, &geometry), "fly cube setup failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "controller_fly");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    DvzFlyDesc desc = dvz_fly_desc();
    desc.mode = DVZ_FLY_MODE_PLANE;
    desc.position[0] = -0.36f;
    desc.position[1] = -3.80f;
    desc.position[2] = +1.34f;
    desc.target[0] = 0.00f;
    desc.target[1] = 0.00f;
    desc.target[2] = 0.22f;
    desc.up[0] = 0.0f;
    desc.up[1] = 0.0f;
    desc.up[2] = 1.0f;
    desc.speed = 0.70f;

    DvzFly* fly = dvz_view_fly(win, panel, &desc);
    EXAMPLE_CHECK(fly != NULL, "failed to create or bind fly controller");
    dvz_fly_move_forward(fly, +0.34f);
    dvz_fly_move_right(fly, +0.18f);
    dvz_fly_move_up(fly, +0.08f);

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
