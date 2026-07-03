/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* bounds_overlay - diagnostic retained visual bounds in 2D and 3D panels.
 *
 * Scenario: feature_bounds_overlay
 * Style: features, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c features/bounds_overlay
 * Run:    ./build/examples/c/features/bounds_overlay --live
 * Smoke:  ./build/examples/c/features/bounds_overlay --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

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
#define POINT_COUNT  256u
#define SPHERE_COUNT 32u



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Fill a deterministic 2D point cloud with varying sizes.
 *
 * @param positions output positions
 * @param colors output colors
 * @param sizes output point diameters
 */
static void _fill_points(
    vec3 positions[POINT_COUNT], DvzColor colors[POINT_COUNT], float sizes[POINT_COUNT])
{
    ANN(positions);
    ANN(colors);
    ANN(sizes);

    for (uint32_t i = 0; i < POINT_COUNT; i++)
    {
        const float t = (float)i / (float)(POINT_COUNT - 1u);
        const float angle = 28.274333882308138f * t;
        const float radius = 0.08f + 0.82f * t;
        positions[i][0] = radius * cosf(angle);
        positions[i][1] = radius * sinf(angle);
        positions[i][2] = 0.0f;

        colors[i] = dvz_color_rgba(
            (uint8_t)(42.0f + 180.0f * t), (uint8_t)(210.0f - 72.0f * t),
            (uint8_t)(255.0f - 165.0f * t), 235);
        sizes[i] = 8.0f + 22.0f * t;
    }
}



/**
 * Fill deterministic 3D sphere centers and radii.
 *
 * @param positions output centers
 * @param colors output colors
 * @param radii output radii
 */
static void _fill_spheres(
    vec3 positions[SPHERE_COUNT], DvzColor colors[SPHERE_COUNT], float radii[SPHERE_COUNT])
{
    ANN(positions);
    ANN(colors);
    ANN(radii);

    for (uint32_t i = 0; i < SPHERE_COUNT; i++)
    {
        const uint32_t ix = i % 4u;
        const uint32_t iy = (i / 4u) % 4u;
        const uint32_t iz = i / 16u;
        const float jx = 0.035f * sinf(1.7f * (float)i);
        const float jy = 0.035f * cosf(2.1f * (float)i);
        const float jz = 0.055f * sinf(0.9f * (float)i);
        positions[i][0] = -0.54f + 0.36f * (float)ix + jx;
        positions[i][1] = -0.54f + 0.36f * (float)iy + jy;
        positions[i][2] = -0.36f + 0.72f * (float)iz + jz;

        const float t = (float)i / (float)(SPHERE_COUNT - 1u);
        colors[i] = dvz_color_rgb(
            (uint8_t)(230.0f - 120.0f * t), (uint8_t)(80.0f + 120.0f * t),
            (uint8_t)(120.0f + 100.0f * t));
        const float radius_classes[3] = {0.070f, 0.105f, 0.165f};
        const uint32_t radius_class = (i * 7u + iz) % 3u;
        radii[i] = radius_classes[radius_class] + 0.006f * sinf(0.8f * (float)i);
    }
}



/**
 * Add the 2D point visual whose aggregate bounds are shown by the overlay.
 *
 * @param scene scene owning visuals
 * @param panel target panel
 * @return true on success
 */
static bool _add_points(DvzScene* scene, DvzPanel* panel)
{
    ANN(scene);
    ANN(panel);

    DvzVisual* points = dvz_point(scene, 0);
    if (points == NULL)
        return false;

    vec3 point_positions[POINT_COUNT] = {0};
    DvzColor point_colors[POINT_COUNT] = {0};
    float point_sizes[POINT_COUNT] = {0};
    _fill_points(point_positions, point_colors, point_sizes);

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = point_positions, .item_count = POINT_COUNT},
        {.attr_name = "color", .data = point_colors, .item_count = POINT_COUNT},
        {.attr_name = "diameter_px", .data = point_sizes, .item_count = POINT_COUNT},
    };
    if (dvz_visual_set_data_many(points, updates, 3) != 0)
        return false;
    return dvz_panel_add_visual(panel, points, NULL) == 0;
}



/**
 * Add the 3D sphere visual whose aggregate bounds are shown by the overlay.
 *
 * @param scene scene owning visuals
 * @param panel target panel
 * @return true on success
 */
static bool _add_spheres(DvzScene* scene, DvzPanel* panel)
{
    ANN(scene);
    ANN(panel);

    DvzVisual* spheres = dvz_sphere(scene, 0);
    if (spheres == NULL)
        return false;
    if (dvz_sphere_set_mode(spheres, DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR) != 0)
        return false;

    vec3 sphere_positions[SPHERE_COUNT] = {0};
    DvzColor sphere_colors[SPHERE_COUNT] = {0};
    float sphere_radii[SPHERE_COUNT] = {0};
    _fill_spheres(sphere_positions, sphere_colors, sphere_radii);

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = sphere_positions, .item_count = SPHERE_COUNT},
        {.attr_name = "color", .data = sphere_colors, .item_count = SPHERE_COUNT},
        {.attr_name = "radius", .data = sphere_radii, .item_count = SPHERE_COUNT},
    };
    if (dvz_visual_set_data_many(spheres, updates, 3) != 0)
        return false;
    return dvz_panel_add_visual(panel, spheres, NULL) == 0;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the bounds-overlay feature scenario.
 *
 * @param ctx scenario context
 * @param out_user unused scenario state output
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

    DvzPanel* panel_2d = dvz_panel(ctx->figure, &(DvzPanelDesc){0.04f, 0.08f, 0.44f, 0.84f});
    DvzPanel* panel_3d = dvz_panel(ctx->figure, &(DvzPanelDesc){0.52f, 0.08f, 0.44f, 0.84f});
    if (panel_2d == NULL || panel_3d == NULL)
        return false;
    example_graphite_cyan_set_panel_background(panel_2d);
    example_graphite_cyan_set_panel_background(panel_3d);

    if (example_set_default_3d_camera(panel_3d, 1.0f) == NULL)
        return false;

    if (!_add_points(ctx->scene, panel_2d))
        return false;
    if (!_add_spheres(ctx->scene, panel_3d))
        return false;
    if (dvz_panel_set_bounds_visible(panel_2d, true) != 0)
        return false;
    if (dvz_panel_set_bounds_visible(panel_3d, true) != 0)
        return false;

    DvzController* panzoom = dvz_panzoom(ctx->scene, NULL);
    if (panzoom == NULL)
        return false;
    if (dvz_scenario_bind_controller(ctx, panel_2d, panzoom, DVZ_DIM_MASK_XY) != 0)
        return false;

    DvzController* arcball = dvz_arcball(ctx->scene, NULL);
    if (arcball == NULL)
        return false;
    if (dvz_scenario_bind_controller(ctx, panel_3d, arcball, DVZ_DIM_MASK_XYZ) != 0)
        return false;
    return true;
}



/**
 * Return the bounds-overlay feature scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _bounds_overlay_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_bounds_overlay",
        .title = "bounds_overlay",
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
 * Run the bounds-overlay feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _bounds_overlay_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
