/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* controller_fly - This example shows fly-style camera navigation through a 3D scene.
 *
 * Scenario: feature.controller_fly
 * Style: features, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c features/controller_fly
 * Run:    ./build/examples/c/features/controller_fly --live
 * Smoke:  ./build/examples/c/features/controller_fly --png
 *
 * What to look for: the same colored cube and reference grid used by the controller examples are
 * shown with a fly controller instead of an orbit controller. In the live preview, compare the
 * camera motion with arcball or turntable behavior: fly navigation translates the viewpoint
 * through the scene, which is useful for volume interiors, large 3D datasets, and walkthrough-style
 * exploration.
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

DvzScenarioSpec dvz_example_controller_fly_scenario(void);



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct ControllerFlyState
{
    DvzGeometry* geometry;
    DvzFly* fly;
    DvzCamera* camera;
} ControllerFlyState;



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
 * Initialize the fly-controller feature scenario.
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

    ControllerFlyState* state = (ControllerFlyState*)dvz_calloc(1, sizeof(*state));
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
    if (!_add_fly_cube(ctx->scene, panel, &state->geometry))
        return false;

    DvzFlyDesc desc = dvz_fly_desc();
    desc.mode = DVZ_FLY_MODE_PLANE;
    desc.initial_view = camera.view;
    desc.speed = 0.70f;
    desc.look_speed = 0.45f;

    DvzController* controller = dvz_fly(ctx->scene, &desc);
    if (controller == NULL)
        return false;
    DvzFly* fly = dvz_controller_fly(controller);
    if (fly == NULL)
        return false;
    state->fly = fly;
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

    ControllerFlyState* state = (ControllerFlyState*)user;
    ExamplePreviewFlyDesc desc = {
        .forward_amplitude = +0.22f,
        .right_amplitude = +0.16f,
        .up_amplitude = +0.05f,
        .yaw_amplitude = +0.08f,
        .pitch_amplitude = +0.045f,
    };
    (void)dvz_fly_set_camera(state->fly, state->camera);
    example_preview_fly(state->fly, ctx->preview_frame_index, ctx->preview_frame_count, &desc);
}



/**
 * Destroy the fly-controller feature scenario state.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    ControllerFlyState* state = (ControllerFlyState*)user;
    if (state == NULL)
        return;
    if (state->geometry != NULL)
        dvz_geometry_destroy(state->geometry);
    dvz_free(state);
}



/**
 * Return the fly-controller scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_example_controller_fly_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_controller_fly",
        .title = "Fly Controller",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_MESH_VISUAL | DVZ_SCENARIO_REQ_CONTROLLER,
        .init = _scenario_init,
        .frame = _scenario_frame,
        .destroy = _scenario_destroy,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the fly-controller feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_example_controller_fly_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
