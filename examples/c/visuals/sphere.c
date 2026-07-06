/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* sphere - This example displays a 3D cluster of raycast impostor spheres.
 *
 * What to look for: positions locate the sphere centers, radii control physical size, and colors
 * distinguish groups inside the cluster. Compare overlap, depth ordering, and highlights while
 * rotating the live view to see why impostor spheres are useful for atoms, particles, cells, and
 * other many-object 3D scientific data.
 *
 * Scenario: visual.sphere
 * Style: visuals, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c visuals/sphere
 * Run:    ./build/examples/c/visuals/sphere --live
 * Smoke:  ./build/examples/c/visuals/sphere --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "_assertions.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT
#define SPHERE_COUNT 42u

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_visual_sphere_scenario(void);



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Fill a deterministic compact sphere cluster.
 *
 * @param positions output sphere centers
 * @param radii output sphere radii
 * @param colors output sphere colors
 */
static void _fill_spheres(
    vec3 positions[SPHERE_COUNT], float radii[SPHERE_COUNT], DvzColor colors[SPHERE_COUNT])
{
    ANN(positions);
    ANN(radii);
    ANN(colors);

    const ExampleStyleColorRole palette[] = {
        EXAMPLE_STYLE_COLOR_WARNING,
        EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,
        EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
        EXAMPLE_STYLE_COLOR_ERROR,
    };

    for (uint32_t i = 0; i < SPHERE_COUNT; i++)
    {
        const float t = (float)i / (float)(SPHERE_COUNT - 1u);
        const float angle = TAU * (0.145f * (float)i + 0.07f * sinf(11.0f * t));
        const float layer = 2.0f * t - 1.0f;
        const float ring = 0.36f + 0.50f * sqrtf(1.0f - 0.58f * layer * layer);
        const float wobble = 0.5f + 0.5f * sinf(17.0f * t + 0.4f);

        positions[i][0] = ring * cosf(angle);
        positions[i][1] = 0.70f * layer + 0.10f * sinf(3.0f * angle);
        positions[i][2] = ring * sinf(angle) + 0.26f * layer;
        radii[i] = 0.075f + 0.070f * (1.0f - fabsf(layer)) + 0.028f * wobble;

        const ExampleStyleColorRole role = palette[i % DVZ_ARRAY_COUNT(palette)];
        colors[i] = example_graphite_cyan_color(role);
    }
}



/**
 * Add one retained sphere visual to the panel.
 *
 * @param scene scene owning visual
 * @param panel panel receiving visual
 * @return true when the visual was added
 */
static bool _add_spheres(DvzScene* scene, DvzPanel* panel)
{
    ANN(scene);
    ANN(panel);

    vec3 positions[SPHERE_COUNT] = {{0}};
    float radii[SPHERE_COUNT] = {0};
    DvzColor colors[SPHERE_COUNT] = {{0}};
    _fill_spheres(positions, radii, colors);

    DvzVisual* visual = dvz_sphere(scene, DVZ_SPHERE_FLAGS_LIGHTING);
    if (visual == NULL)
        return false;
    if (dvz_sphere_set_mode(visual, DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR) != 0)
        return false;

    DvzMaterialDesc material = example_default_standard_material_desc();
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



/**
 * Initialize the retained sphere visual scenario.
 *
 * @param ctx scenario context
 * @param out_user scenario state output
 * @return whether initialization succeeded
 */
static bool _scenario_init(DvzScenarioContext* ctx, void** out_user)
{
    if (ctx == NULL)
        return false;
    if (out_user != NULL)
        *out_user = NULL;

    bool ok = false;
    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    EXAMPLE_CHECK(ctx->figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel_full() failed");
    example_graphite_cyan_set_panel_background(panel);

    EXAMPLE_CHECK(example_set_default_3d_camera(panel, 1.0f), "dvz_panel_set_camera_desc() failed");

    EXAMPLE_CHECK(_add_spheres(ctx->scene, panel), "sphere visual setup failed");

#ifndef DVZ_EXAMPLE_NO_APP
    DvzMsaaDesc msaa_desc = dvz_msaa_desc();
    msaa_desc.sample_count = 8;
    msaa_desc.alpha_to_coverage = true;
    (void)dvz_panel_set_msaa(panel, &msaa_desc);
#endif

    DvzController* arcball_controller = dvz_arcball(ctx->scene, NULL);
    EXAMPLE_CHECK(arcball_controller != NULL, "dvz_arcball() failed");
    EXAMPLE_CHECK(
        dvz_scenario_bind_controller(ctx, panel, arcball_controller, DVZ_DIM_MASK_XYZ) == 0,
        "dvz_scenario_bind_controller() failed");

    ok = true;
cleanup:
    return ok;
}



/**
 * Return the retained sphere visual scenario.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_visual_sphere_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "sphere_impostor",
        .title = "Sphere",
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
 * Run the retained sphere visual through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_visual_sphere_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
