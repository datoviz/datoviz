/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* controller_turntable - This example shows world-up orbiting with a turntable controller.
 *
 * Scenario: feature.controller_turntable
 * Style: features, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c features/controller_turntable
 * Run:    ./build/examples/c/features/controller_turntable --live
 * Smoke:  ./build/examples/c/features/controller_turntable --png
 *
 * What to look for: the lit cube and XZ grid provide a stable reference frame while the controller
 * orbits around a fixed pivot without rolling the camera. In the live preview, drag around the
 * object and compare the upright grid with an arcball view; the turntable pattern is useful for
 * inspecting surfaces, meshes, and laboratory-coordinate 3D data where "up" should stay up.
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "_alloc.h"
#include "datoviz/geom.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_controller_preview.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_example_controller_turntable_scenario(void);



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct ControllerTurntableState
{
    DvzGeometry* geometry;
    DvzTurntable* turntable;
    DvzCamera* camera;
} ControllerTurntableState;



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the turntable-controller feature scenario.
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

    ControllerTurntableState* state = (ControllerTurntableState*)dvz_calloc(1, sizeof(*state));
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
    DvzCameraDesc camera = example_controller_camera_desc();
    if (dvz_panel_set_camera_desc(panel, &camera) != 0)
        return false;
    state->camera = dvz_panel_camera(panel);
    if (state->camera == NULL)
        return false;
    if (!example_add_graphite_cyan_xz_reference_grid(
            panel, EXAMPLE_CONTROLLER_GRID_ORIGIN_Y, true))
        return false;
    if (!example_add_graphite_cyan_cube_mesh(
            ctx->scene, panel, EXAMPLE_CONTROLLER_CUBE_SIZE, NULL, &state->geometry))
        return false;

    DvzTurntableDesc desc = dvz_turntable_desc();
    desc.controller_flags = DVZ_TURNTABLE_FLAGS_CLAMP_DISTANCE;
    desc.initial_view = camera.view;
    desc.min_pitch = -0.72f;
    desc.max_pitch = +0.72f;
    desc.min_distance = 2.40f;
    desc.max_distance = 6.20f;

    DvzController* controller = dvz_turntable(ctx->scene, &desc);
    if (controller == NULL)
        return false;
    DvzTurntable* turntable = dvz_controller_turntable(controller);
    if (turntable == NULL)
        return false;
    state->turntable = turntable;
    if (dvz_scenario_bind_controller(ctx, panel, controller, DVZ_DIM_MASK_XYZ) != 0)
        return false;
    return true;
}


/**
 * Apply deterministic preview motion for generated gallery media.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_frame(DvzScenarioContext* ctx, void* user)
{
    if (ctx == NULL || !ctx->preview_mode || user == NULL)
        return;

    ControllerTurntableState* state = (ControllerTurntableState*)user;
    ExamplePreviewTurntableDesc desc = {
        .yaw_amplitude = +0.14f,
        .pitch_amplitude = +0.055f,
        .distance_delta = 0.0f,
    };
    (void)dvz_turntable_set_camera(state->turntable, state->camera);
    example_preview_turntable(
        state->turntable, ctx->preview_frame_index, ctx->preview_frame_count, &desc);
}



/**
 * Destroy the turntable-controller feature scenario state.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    ControllerTurntableState* state = (ControllerTurntableState*)user;
    if (state == NULL)
        return;
    if (state->geometry != NULL)
        dvz_geometry_destroy(state->geometry);
    dvz_free(state);
}



/**
 * Return the turntable-controller scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_example_controller_turntable_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_controller_turntable",
        .title = "Turntable Controller",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_MESH_VISUAL | DVZ_SCENARIO_REQ_CONTROLLER |
                        DVZ_SCENARIO_REQ_FRAME_CALLBACKS,
        .init = _scenario_init,
        .frame = _scenario_frame,
        .destroy = _scenario_destroy,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the turntable-controller feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_example_controller_turntable_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
