/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* reference_grid - plane-oriented reference grid in an arcball 3D scene.
 *
 * Scenario: feature.reference_grid
 * Style: features, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c features/reference_grid
 * Run:    ./build/examples/c/features/reference_grid --live
 * Smoke:  ./build/examples/c/features/reference_grid --png
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
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the reference-grid feature scenario.
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

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        return false;

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    if (panel == NULL)
        return false;
    example_graphite_cyan_set_panel_background(panel);

    if (example_set_default_3d_camera(panel, 1.3f) == NULL)
        return false;

    DvzReferenceGridDesc grid = dvz_reference_grid_desc();
    grid.plane = DVZ_REFERENCE_GRID_XZ;
    grid.origin[1] = -0.50f;
    grid.size[0] = 8.0f;
    grid.size[1] = 8.0f;
    grid.spacing = 0.25f;
    grid.major_every = 4;
    if (dvz_reference_grid(panel, &grid) == NULL)
        return false;

    DvzController* controller = dvz_turntable(ctx->scene, NULL);
    if (controller == NULL)
        return false;
    if (dvz_scenario_bind_controller(ctx, panel, controller, DVZ_DIM_MASK_XYZ) != 0)
        return false;
    return true;
}


/**
 * Return the reference-grid scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _reference_grid_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_reference_grid",
        .title = "Reference Grid",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .init = _scenario_init,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the reference-grid feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _reference_grid_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
