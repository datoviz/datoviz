/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* controller_orbit_camera - orbit-camera controller attached to a small 3D mesh.
 *
 * Scenario: feature.controller_orbit_camera
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/controller_orbit_camera
 * Run:    ./build/examples/c/features/controller_orbit_camera --live
 * Smoke:  ./build/examples/c/features/controller_orbit_camera --png
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

typedef struct ControllerOrbitCameraState
{
    DvzGeometry* geometry;
} ControllerOrbitCameraState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Add one lit cube mesh to make orbit-camera movement visible.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param out_geometry geometry handle for cleanup on failure before upload completes
 * @return true on success
 */
static bool _add_orbit_camera_mesh(DvzScene* scene, DvzPanel* panel, DvzGeometry** out_geometry)
{
    const ExampleStyleColorRole face_roles[6] = {
        EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY, EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
        EXAMPLE_STYLE_COLOR_WARNING,        EXAMPLE_STYLE_COLOR_ERROR,
        EXAMPLE_STYLE_COLOR_TEXT,           EXAMPLE_STYLE_COLOR_MINOR_TICK,
    };
    DvzVisual* visual = example_graphite_cyan_cube_mesh(scene, 1.18, face_roles, out_geometry);
    if (visual == NULL)
        return false;

    DvzMaterialDesc material = dvz_phong_material_desc();
    material.light_direction[0] = -0.20f;
    material.light_direction[1] = -0.35f;
    material.light_direction[2] = +0.72f;
    material.phong.ambient = 0.26f;
    material.phong.diffuse = 0.80f;
    material.phong.specular = 0.18f;
    material.phong.shininess = 26.0f;
    if (dvz_visual_set_material(visual, &material) != 0)
        return false;

    return dvz_panel_add_visual(panel, visual, NULL) == 0;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the orbit-camera-controller feature scenario.
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

    ControllerOrbitCameraState* state = (ControllerOrbitCameraState*)calloc(1, sizeof(*state));
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
    camera.eye[1] = -3.2f;
    camera.eye[2] = 1.25f;
    camera.up[1] = 0.0f;
    camera.up[2] = 1.0f;
    camera.fov_y = 0.66f;
    camera.near = 0.05f;
    camera.far = 100.0f;
    if (!dvz_panel_set_camera(panel, &camera))
        return false;
    if (!example_add_xz_reference_grid(panel, -0.60f, 4.25f))
        return false;
    if (!_add_orbit_camera_mesh(ctx->scene, panel, &state->geometry))
        return false;

    DvzOrbitCameraDesc desc = dvz_orbit_camera_desc();
    desc.width = (float)ctx->width;
    desc.height = (float)ctx->height;
    desc.pivot[0] = 0.0f;
    desc.pivot[1] = 0.0f;
    desc.pivot[2] = 0.0f;
    DvzController* controller = dvz_orbit_camera(ctx->scene, &desc);
    if (controller == NULL)
        return false;
    DvzOrbitCamera* orbit = dvz_controller_orbit_camera(controller);
    if (orbit == NULL)
        return false;
    dvz_orbit_camera_pivot(orbit, (vec3){0.0f, 0.0f, 0.0f});
    if (dvz_scenario_bind_controller(ctx, panel, controller, DVZ_DIM_MASK_XYZ) != 0)
        return false;
    return true;
}



/**
 * Destroy the orbit-camera-controller feature scenario state.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    ControllerOrbitCameraState* state = (ControllerOrbitCameraState*)user;
    if (state == NULL)
        return;
    if (state->geometry != NULL)
        dvz_geometry_destroy(state->geometry);
    free(state);
}



/**
 * Return the orbit-camera-controller scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _controller_orbit_camera_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_controller_orbit_camera",
        .title = "controller_orbit_camera",
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
 * Run the orbit-camera-controller feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _controller_orbit_camera_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
