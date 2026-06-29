/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* controller_turntable - constrained turntable controller around a small 3D mesh.
 *
 * Scenario: feature.controller_turntable
 * Style: features, graphite_cyan, 1600x1200 capture target
 * Intent: demonstrate stable world-up orbit around a fixed pivot, with pitch/distance clamps and
 * no roll. The opening view is shared with other controller examples, but the interaction model is
 * the upright object-inspection pattern.
 *
 * Build:  just example-c features/controller_turntable
 * Run:    ./build/examples/c/features/controller_turntable --live
 * Smoke:  ./build/examples/c/features/controller_turntable --png
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
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_example_controller_turntable_scenario(void);



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  1600u
#define HEIGHT 1200u



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct ControllerTurntableState
{
    DvzGeometry* geometry;
} ControllerTurntableState;



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
        EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,
        EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
        EXAMPLE_STYLE_COLOR_WARNING,
        EXAMPLE_STYLE_COLOR_ERROR,
        EXAMPLE_STYLE_COLOR_TEXT,
        EXAMPLE_STYLE_COLOR_MINOR_TICK,
    };
    DvzVisual* visual = example_graphite_cyan_cube_mesh(
        scene, EXAMPLE_CONTROLLER_CUBE_SIZE, face_roles, out_geometry);
    if (visual == NULL)
        return false;

    DvzMaterialDesc material = example_default_phong_material_desc();
    if (dvz_visual_set_material(visual, &material) != 0)
        return false;

    return dvz_panel_add_visual(panel, visual, NULL) == 0;
}



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

    ControllerTurntableState* state = (ControllerTurntableState*)calloc(1, sizeof(*state));
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
    if (example_set_controller_camera(panel) == NULL)
        return false;
    DvzReferenceGridDesc grid = dvz_reference_grid_desc();
    grid.plane = DVZ_REFERENCE_GRID_XZ;
    grid.origin[1] = EXAMPLE_CONTROLLER_GRID_ORIGIN_Y;
    grid.size[0] = EXAMPLE_XZ_REFERENCE_GRID_SIZE;
    grid.size[1] = EXAMPLE_XZ_REFERENCE_GRID_SIZE;
    grid.spacing = EXAMPLE_XZ_REFERENCE_GRID_SPACING;
    grid.major_every = EXAMPLE_XZ_REFERENCE_GRID_MAJOR_EVERY;
    grid.minor_color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_GRID);
    grid.minor_color.a = EXAMPLE_XZ_REFERENCE_GRID_MINOR_ALPHA;
    grid.major_color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_MINOR_TICK);
    grid.major_color.a = EXAMPLE_XZ_REFERENCE_GRID_MAJOR_ALPHA;
    grid.axis_color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT);
    grid.axis_color.a = EXAMPLE_XZ_REFERENCE_GRID_AXIS_ALPHA;
    grid.minor_width_px = EXAMPLE_XZ_REFERENCE_GRID_MINOR_WIDTH;
    grid.major_width_px = EXAMPLE_XZ_REFERENCE_GRID_MAJOR_WIDTH;
    grid.axis_width_px = EXAMPLE_XZ_REFERENCE_GRID_AXIS_WIDTH;
    grid.depth_test = true;
    if (dvz_reference_grid(panel, &grid) == NULL)
        return false;
    if (!_add_turntable_cube(ctx->scene, panel, &state->geometry))
        return false;

    DvzTurntableDesc desc = dvz_turntable_desc();
    DvzCameraDesc camera = example_controller_camera_desc();
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
    if (dvz_scenario_bind_controller(ctx, panel, controller, DVZ_DIM_MASK_XYZ) != 0)
        return false;
    return true;
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
    free(state);
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
        .title = "controller_turntable",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_MESH_VISUAL | DVZ_SCENARIO_REQ_CONTROLLER,
        .init = _scenario_init,
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
