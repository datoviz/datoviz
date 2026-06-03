/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* material_mesh - retained mesh visuals with explicit material parameters.
 *
 * Scenario: feature.material_mesh
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/material_mesh
 * Run:    ./build/examples/c/features/material_mesh
 * Smoke:  ./build/examples/c/features/material_mesh 1
 * PNG:    DVZ_CAPTURE=png ./build/examples/c/features/material_mesh 1
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
 * Set a visual-local translation transform.
 *
 * @param visual visual to transform
 * @param x translation on X
 * @param z translation on Z
 * @return true when the transform was accepted
 */
static bool _translate_visual(DvzVisual* visual, float x, float z)
{
    mat4 transform = {
        {1.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},
        {x, 0.0f, z, 1.0f},
    };
    return dvz_visual_set_transform(visual, transform) == 0;
}



/**
 * Add one cube mesh with a specific material.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param x visual-local X translation
 * @param z visual-local Z translation
 * @param material material descriptor
 * @param out_geometry geometry handle for cleanup on failure before upload completes
 * @return true on success
 */
static bool _add_material_cube(
    DvzScene* scene, DvzPanel* panel, float x, float z, const DvzMaterialDesc* material,
    DvzGeometry** out_geometry)
{
    const ExampleStyleColorRole face_roles[6] = {
        EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,
        EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
        EXAMPLE_STYLE_COLOR_WARNING,
        EXAMPLE_STYLE_COLOR_TEXT,
        EXAMPLE_STYLE_COLOR_GRID,
        EXAMPLE_STYLE_COLOR_MINOR_TICK,
    };
    DvzVisual* visual = example_graphite_cyan_cube_mesh(scene, 0.72, face_roles, out_geometry);
    if (visual == NULL)
        return false;

    if (dvz_visual_set_material(visual, material) != 0)
        return false;
    if (!_translate_visual(visual, x, z))
        return false;
    return dvz_panel_add_visual(panel, visual, NULL) == 0;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the material-mesh feature example.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    const uint32_t frame_count = example_frame_count_any(argc, argv);
    DvzAppCaptureConfig capture = dvz_app_capture_config_from_env("feature_material_mesh");

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

    DvzCameraDesc camera = dvz_camera_desc();
    camera.eye[0] = 0.0f;
    camera.eye[1] = -3.40f;
    camera.eye[2] = 1.30f;
    camera.up[1] = 0.0f;
    camera.up[2] = 1.0f;
    camera.fov_y = 0.64f;
    camera.near = 0.05f;
    camera.far = 100.0f;
    EXAMPLE_CHECK(dvz_panel_set_camera(panel, &camera), "dvz_panel_set_camera() failed");

    DvzMaterialDesc matte = dvz_phong_material_desc();
    matte.phong.ambient = 0.34f;
    matte.phong.diffuse = 0.84f;
    matte.phong.specular = 0.02f;
    matte.phong.shininess = 8.0f;

    DvzMaterialDesc glossy = dvz_phong_material_desc();
    glossy.phong.ambient = 0.18f;
    glossy.phong.diffuse = 0.70f;
    glossy.phong.specular = 0.48f;
    glossy.phong.shininess = 58.0f;

    DvzMaterialDesc rim = dvz_standard_material_desc();
    rim.standard.roughness = 0.42f;
    rim.standard.specular = 0.46f;
    rim.standard.rim_strength = 0.30f;

    EXAMPLE_CHECK(
        _add_material_cube(scene, panel, -0.88f, -0.10f, &matte, &geometry),
        "matte mesh setup failed");
    EXAMPLE_CHECK(
        _add_material_cube(scene, panel, +0.00f, +0.08f, &glossy, &geometry),
        "glossy mesh setup failed");
    EXAMPLE_CHECK(
        _add_material_cube(scene, panel, +0.88f, -0.10f, &rim, &geometry),
        "standard mesh setup failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "material_mesh");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    DvzController* controller = dvz_arcball(scene, NULL);
    EXAMPLE_CHECK(controller != NULL, "dvz_arcball() failed");
    DvzArcball* arcball = dvz_controller_arcball(controller);
    EXAMPLE_CHECK(arcball != NULL, "dvz_controller_arcball() failed");
    EXAMPLE_CHECK(
        dvz_view_bind_controller(win, panel, controller, DVZ_DIM_MASK_XYZ) == 0,
        "dvz_view_bind_controller() failed");
    dvz_arcball_set(arcball, (vec3){+0.58f, -0.14f, +0.26f});

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
