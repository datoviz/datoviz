/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* orientation_gizmo adds a small orientation widget to a 3D panel.
 *
 * What to look for: the main scene is a lit cube controlled by an arcball, and the orientation
 * gizmo is placed in the panel's bottom-right corner with a fixed screen size. Rotate the live
 * cube and compare the cube faces with the gizmo axes; the widget helps users keep track of 3D
 * orientation when inspecting volumes, meshes, or spatial point clouds.
 *
 * Scenario: feature.orientation_gizmo
 * Style: features, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c features/orientation_gizmo
 * Run:    ./build/examples/c/features/orientation_gizmo --live
 * Smoke:  ./build/examples/c/features/orientation_gizmo --png
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

typedef struct OrientationGizmoState
{
    DvzGeometry* geometry;
    DvzArcball* arcball;
} OrientationGizmoState;



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the orientation-gizmo feature scenario.
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

    OrientationGizmoState* state = (OrientationGizmoState*)dvz_calloc(1, sizeof(*state));
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

    if (example_set_default_3d_camera(panel, 1.0f) == NULL)
        return false;
    if (!example_add_graphite_cyan_xz_reference_grid(panel, -0.59f, true))
        return false;
    if (!example_add_graphite_cyan_cube_mesh(ctx->scene, panel, 1.18, NULL, &state->geometry))
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

    DvzOrientationGizmoDesc gizmo = dvz_orientation_gizmo_desc();
    gizmo.placement = dvz_placement_panel_corner(
        DVZ_HORIZONTAL_ANCHOR_RIGHT, DVZ_VERTICAL_ANCHOR_BOTTOM, 150, 150, -18, -18);
    return dvz_orientation_gizmo(panel, &gizmo) != NULL;
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

    OrientationGizmoState* state = (OrientationGizmoState*)user;
    ExamplePreviewArcballDesc desc = example_preview_arcball_cube_desc();
    example_preview_arcball(
        state->arcball, ctx->preview_frame_index, ctx->preview_frame_count, &desc);
}



/**
 * Destroy the orientation-gizmo feature scenario state.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    OrientationGizmoState* state = (OrientationGizmoState*)user;
    if (state == NULL)
        return;
    if (state->geometry != NULL)
        dvz_geometry_destroy(state->geometry);
    dvz_free(state);
}



/**
 * Return the orientation-gizmo scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _orientation_gizmo_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_orientation_gizmo",
        .title = "Orientation Gizmo",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_MESH_VISUAL | DVZ_SCENARIO_REQ_CONTROLLER |
                        DVZ_SCENARIO_REQ_ARCBALL,
        .init = _scenario_init,
        .frame = _scenario_frame,
        .destroy = _scenario_destroy,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the orientation-gizmo feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _orientation_gizmo_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
