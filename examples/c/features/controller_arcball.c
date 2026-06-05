/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* controller_arcball - arcball controller attached to a small 3D mesh.
 *
 * Scenario: feature.controller_arcball
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/controller_arcball
 * Run:    ./build/examples/c/features/controller_arcball --live
 * Smoke:  ./build/examples/c/features/controller_arcball --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "datoviz/geom.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  1600u
#define HEIGHT 1200u



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct ControllerArcballState
{
    DvzGeometry* geometry;
} ControllerArcballState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Add one lit cube mesh to make arcball rotation visible.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param out_geometry geometry handle for cleanup on failure before upload completes
 * @return true on success
 */
static bool _add_arcball_mesh(DvzScene* scene, DvzPanel* panel, DvzGeometry** out_geometry)
{
    const ExampleStyleColorRole face_roles[6] = {
        EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,
        EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
        EXAMPLE_STYLE_COLOR_WARNING,
        EXAMPLE_STYLE_COLOR_ERROR,
        EXAMPLE_STYLE_COLOR_TEXT,
        EXAMPLE_STYLE_COLOR_MINOR_TICK,
    };
    DvzVisual* visual = example_graphite_cyan_cube_mesh(scene, 1.18, face_roles, out_geometry);
    if (visual == NULL)
        return false;

    DvzMaterialDesc material = dvz_phong_material_desc();
    material.light_direction[0] = -0.18f;
    material.light_direction[1] = -0.36f;
    material.light_direction[2] = +0.74f;
    material.phong.ambient = 0.28f;
    material.phong.diffuse = 0.78f;
    material.phong.specular = 0.16f;
    material.phong.shininess = 22.0f;
    if (dvz_visual_set_material(visual, &material) != 0)
        return false;

    return dvz_panel_add_visual(panel, visual, NULL) == 0;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the arcball-controller feature scenario.
 *
 * @param ctx scenario context
 * @param out_user scenario state output
 * @return true on success
 */
static bool _scenario_init(DvzScenarioContext* ctx, void** out_user)
{
    if (ctx == NULL)
        return false;
    if (out_user != NULL)
        *out_user = NULL;

    ControllerArcballState* state = (ControllerArcballState*)calloc(1, sizeof(*state));
    if (state == NULL)
        return false;
    if (out_user != NULL)
        *out_user = state;

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        return false;

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    if (panel == NULL)
        return false;
    example_graphite_cyan_set_panel_background(panel);

    DvzCameraDesc camera = dvz_camera_desc();
    camera.eye[0] = 0.0f;
    camera.eye[1] = -3.0f;
    camera.eye[2] = 1.30f;
    camera.up[1] = 0.0f;
    camera.up[2] = 1.0f;
    camera.fov_y = 0.68f;
    camera.near = 0.05f;
    camera.far = 100.0f;
    if (!dvz_panel_set_camera(panel, &camera))
        return false;
    if (!_add_arcball_mesh(ctx->scene, panel, &state->geometry))
        return false;

    DvzController* controller = dvz_arcball(ctx->scene, NULL);
    if (controller == NULL)
        return false;
    DvzArcball* arcball = dvz_controller_arcball(controller);
    if (arcball == NULL)
        return false;
    if (dvz_scenario_bind_controller(ctx, panel, controller, DVZ_DIM_MASK_XYZ) != 0)
        return false;
    dvz_arcball_set(arcball, (vec3){+0.58f, -0.16f, +0.32f});
    return true;
}



/**
 * Destroy the arcball-controller feature scenario state.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    ControllerArcballState* state = (ControllerArcballState*)user;
    if (state == NULL)
        return;
    if (state->geometry != NULL)
        dvz_geometry_destroy(state->geometry);
    free(state);
}



/**
 * Return the arcball-controller scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _controller_arcball_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_controller_arcball",
        .title = "controller_arcball",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .init = _scenario_init,
        .destroy = _scenario_destroy,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the arcball-controller feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _controller_arcball_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
