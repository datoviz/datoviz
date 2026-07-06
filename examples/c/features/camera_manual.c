/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* camera_manual - This example shows explicit perspective-camera setup for a 3D scene.
 *
 * Scenario: feature.camera_manual
 * Style: features, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c features/camera_manual
 * Run:    ./build/examples/c/features/camera_manual --live
 * Smoke:  ./build/examples/c/features/camera_manual --png
 *
 * What to look for: the camera descriptor sets eye, target, up, field of view, and near/far clip
 * planes before the panel renders a lit cube on a reference grid. Compare the cube's perspective
 * with the grid orientation and how much of the scene is visible. Manual camera setup is useful
 * when screenshots, reproducible views, or analysis layouts need a known viewpoint instead of an
 * interaction-derived one.
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_example_camera_manual_scenario(void);



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Add one lit cube mesh so the manual camera framing has a visible 3D reference.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @return true on success
 */
static bool _add_cube(DvzScene* scene, DvzPanel* panel)
{
    const ExampleStyleColorRole face_roles[6] = {
        EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,
        EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
        EXAMPLE_STYLE_COLOR_WARNING,
        EXAMPLE_STYLE_COLOR_ERROR,
        EXAMPLE_STYLE_COLOR_TEXT,
        EXAMPLE_STYLE_COLOR_MINOR_TICK,
    };
    DvzVisual* visual = example_graphite_cyan_cube_mesh(scene, 1.25, face_roles, NULL);
    if (visual == NULL)
        return false;

    DvzMaterialDesc material = example_default_phong_material_desc();
    if (dvz_visual_set_material(visual, &material) != 0)
        return false;
    return dvz_panel_add_visual(panel, visual, NULL) == 0;
}


/**
 * Configure the panel camera with explicit view and projection fields.
 *
 * @param panel target panel
 * @return true on success
 */
static bool _set_manual_camera(DvzPanel* panel)
{
    DvzCameraDesc camera = dvz_camera_desc();
    camera.view.eye[0] = 1.55f;
    camera.view.eye[1] = -2.00f;
    camera.view.eye[2] = 1.45f;
    camera.view.target[0] = 0.0f;
    camera.view.target[1] = 0.0f;
    camera.view.target[2] = 0.0f;
    camera.view.up[0] = 0.0f;
    camera.view.up[1] = 0.0f;
    camera.view.up[2] = 1.0f;
    camera.projection.fov_y = 0.66f;
    camera.projection.near_clip = 0.05f;
    camera.projection.far_clip = 100.0f;
    return dvz_panel_set_camera_desc(panel, &camera) == 0;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the manual-camera feature scenario.
 *
 * @param ctx scenario context
 * @param out_user unused scenario state output
 * @return true on success
 */
static bool _scenario_init(DvzScenarioContext* ctx, void** out_user)
{
    if (ctx == NULL)
        return false;
    if (out_user != NULL)
        *out_user = NULL;

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        return false;

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    if (panel == NULL)
        return false;
    example_graphite_cyan_set_panel_background(panel);

    if (!_set_manual_camera(panel))
        return false;
    if (!_add_cube(ctx->scene, panel))
        return false;

    DvzController* controller = dvz_arcball(ctx->scene, NULL);
    if (controller == NULL)
        return false;
    return dvz_scenario_bind_controller(ctx, panel, controller, DVZ_DIM_MASK_XYZ) == 0;
}


/**
 * Return the manual-camera scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_example_camera_manual_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_camera_manual",
        .title = "Manual Camera",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements =
            DVZ_SCENARIO_REQ_MESH_VISUAL | DVZ_SCENARIO_REQ_CONTROLLER | DVZ_SCENARIO_REQ_ARCBALL,
        .init = _scenario_init,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the manual-camera feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_example_camera_manual_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
