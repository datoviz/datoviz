/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* controller_arcball - This example shows an arcball controller rotating a 3D view.
 *
 * Scenario: feature.controller_arcball
 * Style: features, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c features/controller_arcball
 * Run:    ./build/examples/c/features/controller_arcball --live
 * Smoke:  ./build/examples/c/features/controller_arcball --png
 *
 * What to look for: a colored cube mesh and XZ reference grid make rotation easy to see, and the
 * controller is bound to all three dimensions of the panel. In the live preview, drag the mouse
 * and compare how the cube can roll freely with the view. Arcball interaction is useful for
 * unconstrained inspection of 3D data, where seeing an object from arbitrary orientations matters
 * more than preserving a fixed world-up direction.
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
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct ControllerArcballState
{
    DvzGeometry* geometry;
    DvzArcball* arcball;
} ControllerArcballState;



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

    ControllerArcballState* state = (ControllerArcballState*)dvz_calloc(1, sizeof(*state));
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
    if (!example_add_graphite_cyan_xz_reference_grid(
            panel, EXAMPLE_CONTROLLER_GRID_ORIGIN_Y, true))
        return false;
    if (!example_add_graphite_cyan_cube_mesh(
            ctx->scene, panel, EXAMPLE_CONTROLLER_CUBE_SIZE, NULL, &state->geometry))
        return false;

    DvzController* controller = dvz_arcball(ctx->scene, NULL);
    if (controller == NULL)
        return false;
    DvzArcball* arcball = dvz_controller_arcball(controller);
    if (arcball == NULL)
        return false;
    state->arcball = arcball;
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

    ControllerArcballState* state = (ControllerArcballState*)user;
    ExamplePreviewArcballDesc desc = {
        .base_angles = {+0.38f, -0.22f, +0.12f},
        .amplitude = {+0.42f, +0.28f, +0.32f},
        .zoom = 1.0f,
    };
    example_preview_arcball(
        state->arcball, ctx->preview_frame_index, ctx->preview_frame_count, &desc);
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
    dvz_free(state);
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
        .title = "Arcball Controller",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_CONTROLLER | DVZ_SCENARIO_REQ_ARCBALL,
        .init = _scenario_init,
        .frame = _scenario_frame,
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
