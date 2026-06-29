/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* splat - retained Gaussian splat visual with deterministic screen-space ellipses.
 *
 * Scenario: visual.splat
 * Style: visuals, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c visuals/splat
 * Run:    ./build/examples/c/visuals/splat --live
 * Smoke:  ./build/examples/c/visuals/splat --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "_assertions.h"
#include "datoviz/scene.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT
#define SPLAT_COUNT 84u

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Fill a small deterministic Gaussian splat cloud.
 *
 * @param positions output splat centers
 * @param colors output splat colors
 * @param sigma output screen-space ellipse radii in pixels
 * @param angles output ellipse angles in radians
 */
static void _fill_splats(
    vec3 positions[SPLAT_COUNT], DvzColor colors[SPLAT_COUNT], vec2 sigma[SPLAT_COUNT],
    float angles[SPLAT_COUNT])
{
    ANN(positions);
    ANN(colors);
    ANN(sigma);
    ANN(angles);

    const ExampleStyleColorRole roles[] = {
        EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,
        EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
        EXAMPLE_STYLE_COLOR_WARNING,
        EXAMPLE_STYLE_COLOR_ERROR,
    };

    for (uint32_t i = 0; i < SPLAT_COUNT; i++)
    {
        const float t = SPLAT_COUNT > 1u ? (float)i / (float)(SPLAT_COUNT - 1u) : 0.0f;
        const float arm = (float)(i % 7u);
        const float local = (float)(i / 7u) / (float)(SPLAT_COUNT / 7u);
        const float theta = TAU * (1.55f * local + arm / 7.0f);
        const float radius = 0.10f + 0.78f * sqrtf(local);

        positions[i][0] = radius * cosf(theta);
        positions[i][1] = 0.76f * radius * sinf(theta);
        positions[i][2] = 0.01f * sinf(TAU * t);

        colors[i] = example_graphite_cyan_color(roles[i % DVZ_ARRAY_COUNT(roles)]);
        colors[i].a = 118u + (uint8_t)(64u * (i % 3u));

        sigma[i][0] = 10.0f + 7.5f * (0.5f + 0.5f * sinf(TAU * (t + 0.11f * arm)));
        sigma[i][1] = 4.0f + 4.5f * (0.5f + 0.5f * cosf(TAU * (0.7f * t + 0.05f * arm)));
        angles[i] = theta + 0.45f * sinf(TAU * t);
    }
}



/**
 * Add one retained Gaussian splat visual to the panel.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @return true when the visual was added
 */
static bool _add_splats(DvzScene* scene, DvzPanel* panel)
{
    ANN(scene);
    ANN(panel);

    vec3 positions[SPLAT_COUNT] = {{0}};
    DvzColor colors[SPLAT_COUNT] = {{0}};
    vec2 sigma[SPLAT_COUNT] = {{0}};
    float angles[SPLAT_COUNT] = {0};
    _fill_splats(positions, colors, sigma, angles);

    DvzVisual* visual = dvz_splat(scene, 0);
    if (visual == NULL)
        return false;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = SPLAT_COUNT},
        {.attr_name = "color", .data = colors, .item_count = SPLAT_COUNT},
        {.attr_name = "sigma", .data = sigma, .item_count = SPLAT_COUNT},
        {.attr_name = "angle", .data = angles, .item_count = SPLAT_COUNT},
    };
    if (dvz_visual_set_data_many(visual, updates, 4) != 0)
        return false;
    return dvz_panel_add_visual(panel, visual, NULL) == 0;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the retained Gaussian splat visual scenario.
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

    if (!_add_splats(ctx->scene, panel))
        return false;

    DvzPanzoom* panzoom = dvz_scenario_panzoom(ctx, panel, NULL, DVZ_DIM_MASK_XY);
    return panzoom != NULL;
}



/**
 * Return the splat visual scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _splat_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "visual_splat",
        .title = "visual_splat",
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
 * Run the retained Gaussian splat visual example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _splat_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
