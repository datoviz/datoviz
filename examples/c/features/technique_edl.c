/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* edl - Eye-Dome Lighting applied to a regular 3D sphere lattice.
 *
 * Scenario: feature.edl
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/technique_edl
 * Run:    ./build/examples/c/features/technique_edl --live
 * Smoke:  ./build/examples/c/features/technique_edl --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/scene.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH       1600u
#define HEIGHT      1200u
#define LATTICE_SIDE  3u
#define SPHERE_COUNT  (LATTICE_SIDE * LATTICE_SIDE * LATTICE_SIDE)



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Add one sphere-lattice visual to a panel.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @return true on success
 */
static bool _add_sphere_lattice(DvzScene* scene, DvzPanel* panel)
{
    vec3 positions[SPHERE_COUNT] = {{0}};
    DvzColor colors[SPHERE_COUNT] = {{0}};
    float radii[SPHERE_COUNT] = {0};

    for (uint32_t z = 0; z < LATTICE_SIDE; z++)
    {
        for (uint32_t y = 0; y < LATTICE_SIDE; y++)
        {
            for (uint32_t x = 0; x < LATTICE_SIDE; x++)
            {
                const uint32_t i = z * LATTICE_SIDE * LATTICE_SIDE + y * LATTICE_SIDE + x;
                positions[i][0] = -0.58f + 0.58f * (float)x;
                positions[i][1] = -0.44f + 0.44f * (float)y;
                positions[i][2] = -0.72f + 0.72f * (float)z;
                colors[i] = example_graphite_cyan_color(
                    z == 0u   ? EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY
                    : z == 1u ? EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY
                              : EXAMPLE_STYLE_COLOR_WARNING);
                radii[i] = 0.115f;
            }
        }
    }

    DvzVisual* sphere = dvz_sphere(scene, DVZ_SPHERE_FLAGS_LIGHTING);
    if (sphere == NULL)
        return false;
    if (dvz_sphere_mode(sphere, DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR) != 0)
        return false;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = SPHERE_COUNT},
        {.attr_name = "color", .data = colors, .item_count = SPHERE_COUNT},
        {.attr_name = "radius", .data = radii, .item_count = SPHERE_COUNT},
    };
    if (dvz_visual_set_data_many(sphere, updates, 3) != 0)
        return false;

    DvzMaterialDesc material = dvz_standard_material_desc();
    material.light_direction[0] = -0.32f;
    material.light_direction[1] = -0.55f;
    material.light_direction[2] = +0.76f;
    material.standard.roughness = 0.46f;
    material.standard.specular = 0.44f;
    material.standard.rim_strength = 0.18f;
    if (dvz_visual_set_material(sphere, &material) != 0)
        return false;

    return dvz_panel_add_visual(panel, sphere, NULL) == 0;
}



/**
 * Set the shared 3D camera.
 *
 * @param panel target panel
 * @return true on success
 */
static bool _set_camera(DvzPanel* panel)
{
    DvzCameraDesc camera = dvz_camera_desc();
    camera.eye[0] = 0.0f;
    camera.eye[1] = -3.45f;
    camera.eye[2] = 1.05f;
    camera.up[1] = 0.0f;
    camera.up[2] = 1.0f;
    camera.fov_y = 0.62f;
    camera.near = 0.05f;
    camera.far = 100.0f;
    return dvz_panel_set_camera(panel, &camera) != NULL;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the EDL feature scenario.
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

    DvzPanel* plain = dvz_grid_panel(grid, 0, 0);
    DvzPanel* lit = dvz_grid_panel(grid, 0, 1);
    if (plain == NULL || lit == NULL)
        return false;
    example_graphite_cyan_set_panel_background(plain);
    example_graphite_cyan_set_panel_background(lit);

    if (!_set_camera(plain) || !_set_camera(lit))
        return false;
    if (!_add_sphere_lattice(ctx->scene, plain) || !_add_sphere_lattice(ctx->scene, lit))
        return false;

    DvzEdlDesc edl = dvz_edl_desc();
    edl.radius = 2.0f;
    edl.strength = 58.0f;
    edl.depth_scale = 1.0f;
    return dvz_panel_set_edl(lit, &edl);
}



/**
 * Return the EDL scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _edl_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "technique_edl",
        .title = "edl",
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
 * Run the Eye-Dome Lighting feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _edl_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
