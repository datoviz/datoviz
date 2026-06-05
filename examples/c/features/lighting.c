/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* lighting - lit sphere cluster with explicit light direction.
 *
 * Scenario: feature.lighting
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/lighting
 * Run:    ./build/examples/c/features/lighting --live
 * Smoke:  ./build/examples/c/features/lighting --png
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

#define WIDTH        1600u
#define HEIGHT       1200u
#define SPHERE_COUNT 9u



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Add one lit sphere visual with deterministic positions and one material light direction.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @return true on success
 */
static bool _add_lit_spheres(DvzScene* scene, DvzPanel* panel)
{
    vec3 positions[SPHERE_COUNT] = {
        {-0.72f, -0.28f, -0.20f},
        {-0.36f, -0.28f, +0.05f},
        {+0.00f, -0.28f, +0.18f},
        {+0.36f, -0.28f, +0.05f},
        {+0.72f, -0.28f, -0.20f},
        {-0.48f, +0.18f, -0.06f},
        {-0.12f, +0.18f, +0.16f},
        {+0.24f, +0.18f, +0.12f},
        {+0.60f, +0.18f, -0.08f},
    };
    const float radii[SPHERE_COUNT] = {
        0.150f, 0.170f, 0.190f, 0.170f, 0.150f, 0.165f, 0.205f, 0.185f, 0.155f};
    DvzColor colors[SPHERE_COUNT] = {
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_GRID),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_GRID),
    };

    DvzVisual* visual = dvz_sphere(scene, DVZ_SPHERE_FLAGS_LIGHTING);
    if (visual == NULL)
        return false;
    if (dvz_sphere_mode(visual, DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR) != 0)
        return false;

    DvzMaterialDesc material = dvz_standard_material_desc();
    material.light_direction[0] = -0.34f;
    material.light_direction[1] = -0.46f;
    material.light_direction[2] = +0.82f;
    material.standard.roughness = 0.52f;
    material.standard.specular = 0.42f;
    material.standard.rim_strength = 0.18f;
    if (dvz_visual_set_material(visual, &material) != 0)
        return false;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = SPHERE_COUNT},
        {.attr_name = "radius", .data = radii, .item_count = SPHERE_COUNT},
        {.attr_name = "color", .data = colors, .item_count = SPHERE_COUNT},
    };
    if (dvz_visual_set_data_many(visual, updates, 3) != 0)
        return false;
    return dvz_panel_add_visual(panel, visual, NULL) == 0;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the lighting feature scenario.
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

    DvzCameraDesc camera = dvz_camera_desc();
    camera.eye[0] = 0.0f;
    camera.eye[1] = -2.80f;
    camera.eye[2] = 1.05f;
    camera.up[1] = 0.0f;
    camera.up[2] = 1.0f;
    camera.fov_y = 0.58f;
    camera.near = 0.05f;
    camera.far = 100.0f;
    if (!dvz_panel_set_camera(panel, &camera))
        return false;

    if (!_add_lit_spheres(ctx->scene, panel))
        return false;

    DvzController* controller = dvz_arcball(ctx->scene, NULL);
    if (controller == NULL)
        return false;
    DvzArcball* arcball = dvz_controller_arcball(controller);
    if (arcball == NULL)
        return false;
    if (dvz_scenario_bind_controller(ctx, panel, controller, DVZ_DIM_MASK_XYZ) != 0)
        return false;
    dvz_arcball_set(arcball, (vec3){+0.46f, -0.12f, +0.18f});
    return true;
}



/**
 * Return the lighting scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _lighting_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_lighting",
        .title = "lighting",
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
 * Run the lighting feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _lighting_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
