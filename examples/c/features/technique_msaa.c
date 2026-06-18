/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* msaa - side-by-side multisample antialiasing on slanted 3D cube silhouettes.
 *
 * Scenario: feature.msaa
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/technique_msaa
 * Run:    ./build/examples/c/features/technique_msaa --live
 * Smoke:  ./build/examples/c/features/technique_msaa --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH      1600u
#define HEIGHT     1200u
#define CUBE_COUNT 4u



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return one visual-local translation transform.
 *
 * @param x translation x
 * @param y translation y
 * @param z translation z
 * @param out output matrix
 */
static void _translation(float x, float y, float z, mat4 out)
{
    if (out == NULL)
        return;
    memset(out, 0, sizeof(mat4));
    out[0][0] = 1.0f;
    out[1][1] = 1.0f;
    out[2][2] = 1.0f;
    out[3][3] = 1.0f;
    out[3][0] = x;
    out[3][1] = y;
    out[3][2] = z;
}



/**
 * Add one slanted cube cluster whose edges make MSAA differences visible.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @return true on success
 */
static bool _add_cube_cluster(DvzScene* scene, DvzPanel* panel)
{
    static const vec3 positions[CUBE_COUNT] = {
        {-0.54f, -0.30f, -0.05f},
        {+0.04f, -0.08f, +0.12f},
        {+0.52f, +0.16f, +0.00f},
        {-0.10f, +0.45f, +0.18f},
    };
    const double sizes[CUBE_COUNT] = {0.50, 0.62, 0.44, 0.34};

    const ExampleStyleColorRole roles[6] = {
        EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY, EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
        EXAMPLE_STYLE_COLOR_WARNING,        EXAMPLE_STYLE_COLOR_TEXT,
        EXAMPLE_STYLE_COLOR_GRID,           EXAMPLE_STYLE_COLOR_MINOR_TICK,
    };

    for (uint32_t i = 0; i < CUBE_COUNT; i++)
    {
        DvzVisual* cube = example_graphite_cyan_cube_mesh(scene, sizes[i], roles, NULL);
        if (cube == NULL)
            return false;

        DvzMaterialDesc material = dvz_standard_material_desc();
        material.standard.roughness = 0.46f;
        material.standard.specular = 0.34f;
        material.standard.rim_strength = 0.18f;
        if (dvz_visual_set_material(cube, &material) != 0)
            return false;

        mat4 transform = {{0}};
        _translation(positions[i][0], positions[i][1], positions[i][2], transform);
        if (dvz_visual_set_transform(cube, transform) != 0)
            return false;
        if (dvz_panel_add_visual(panel, cube, NULL) != 0)
            return false;
    }
    return true;
}



/**
 * Set the shared 3D camera used by both panels.
 *
 * @param panel target panel
 * @return true on success
 */
static bool _set_camera(DvzPanel* panel)
{
    DvzCameraDesc camera = dvz_camera_desc();
    camera.eye[0] = +0.10f;
    camera.eye[1] = +1.20f;
    camera.eye[2] = +3.10f;
    camera.fov_y = 0.54f;
    camera.near_clip = 0.05f;
    camera.far_clip = 100.0f;
    return dvz_panel_set_camera(panel, &camera) != NULL;
}



/**
 * Set the shared arcball orientation used by both panels.
 *
 * @param ctx scenario context
 * @param panel target panel
 * @return true on success
 */
static DvzController* _bind_arcball(DvzScenarioContext* ctx, DvzPanel* panel)
{
    DvzController* controller = dvz_arcball(ctx->scene, NULL);
    if (controller == NULL)
        return NULL;
    DvzArcball* arcball = dvz_controller_arcball(controller);
    if (arcball == NULL)
        return NULL;
    if (dvz_scenario_bind_controller(ctx, panel, controller, DVZ_DIM_MASK_XYZ) != 0)
        return NULL;
    dvz_arcball_set(arcball, (vec3){+0.58f, -0.24f, +0.18f});
    return controller;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the MSAA feature scenario.
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

    DvzGrid* grid = dvz_figure_grid(ctx->figure, 1, 2);
    if (grid == NULL)
        return false;
    if (!dvz_grid_set_margins(
            grid, &(DvzPanelReserve){
                      .left_px = 42.0f, .right_px = 42.0f, .top_px = 38.0f, .bottom_px = 38.0f}))
        return false;
    if (!dvz_grid_set_gutter(grid, 30.0f, 0.0f))
        return false;

    DvzPanel* single = dvz_grid_panel(grid, 0, 0);
    DvzPanel* multisample = dvz_grid_panel(grid, 0, 1);
    if (single == NULL || multisample == NULL)
        return false;
    example_graphite_cyan_set_panel_background(single);
    example_graphite_cyan_set_panel_background(multisample);
    if (!example_add_panel_label(single, "single sample", 18.0f, 18.0f) ||
        !example_add_panel_label(multisample, "8x MSAA", 18.0f, 18.0f))
        return false;

    if (!_set_camera(single) || !_set_camera(multisample))
        return false;
    DvzController* single_controller = _bind_arcball(ctx, single);
    DvzController* multisample_controller = _bind_arcball(ctx, multisample);
    if (single_controller == NULL || multisample_controller == NULL)
        return false;
    if (!example_link_controllers_bidirectional(
            ctx->scene, single_controller, multisample_controller,
            DVZ_CONTROLLER_LINK_ROTATION | DVZ_CONTROLLER_LINK_PAN | DVZ_CONTROLLER_LINK_ZOOM))
        return false;
    if (!_add_cube_cluster(ctx->scene, single) || !_add_cube_cluster(ctx->scene, multisample))
        return false;

    DvzMsaaDesc msaa = dvz_msaa_desc();
    msaa.enabled = true;
    msaa.sample_count = 8u;
    msaa.alpha_to_coverage = false;
    return dvz_panel_set_msaa(multisample, &msaa);
}



/**
 * Return the MSAA scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _msaa_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "technique_msaa",
        .title = "msaa",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_MESH_VISUAL | DVZ_SCENARIO_REQ_CONTROLLER |
                        DVZ_SCENARIO_REQ_ARCBALL,
        .init = _scenario_init,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the MSAA feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _msaa_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
