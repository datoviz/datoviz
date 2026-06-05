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
 * Run:    ./build/examples/c/features/controller_fly --live
 * Smoke:  ./build/examples/c/features/controller_fly --png
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

typedef struct ControllerFlyState
{
    DvzGeometry* geometry;
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
    DvzVisual* visual = example_graphite_cyan_cube_mesh(scene, 1.10, face_roles, out_geometry);
    if (visual == NULL)
        return false;

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

    ControllerFlyState* state = (ControllerFlyState*)calloc(1, sizeof(*state));
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
    if (!_add_fly_cube(ctx->scene, panel, &state->geometry))
        return false;

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

    DvzController* controller = dvz_fly(ctx->scene, &desc);
    if (controller == NULL)
        return false;
    DvzFly* fly = dvz_controller_fly(controller);
    if (fly == NULL)
        return false;
    if (dvz_scenario_bind_controller(ctx, panel, controller, DVZ_DIM_MASK_XYZ) != 0)
        return false;
    dvz_fly_move_forward(fly, +0.34f);
    dvz_fly_move_right(fly, +0.18f);
    dvz_fly_move_up(fly, +0.08f);
    return true;
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
    free(state);
}



/**
 * Return the fly-controller scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _controller_fly_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_controller_fly",
        .title = "controller_fly",
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
 * Run the fly-controller feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _controller_fly_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
