/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* guide_spans - retained horizontal and vertical guide spans in panel data coordinates.
 *
 * Scenario: feature.guide_spans
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/guide_spans
 * Run:    ./build/examples/c/features/guide_spans --live
 * Smoke:  ./build/examples/c/features/guide_spans --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "datoviz/scene.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH       1600u
#define HEIGHT      1200u
#define POINT_COUNT 7u



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct GuideSpansState
{
    DvzGuideSpan* hspan;
    DvzGuideSpan* vspan;
    bool updated;
} GuideSpansState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Add a compact point series in data coordinates.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @return true when the points were added
 */
static bool _add_points(DvzScene* scene, DvzPanel* panel)
{
    ANN(scene);
    ANN(panel);

    const vec3 data_positions[POINT_COUNT] = {
        {0.5f, 0.35f, 0.0f}, {1.5f, 0.82f, 0.0f}, {2.5f, 1.10f, 0.0f}, {3.5f, 0.62f, 0.0f},
        {4.5f, 1.35f, 0.0f}, {5.5f, 1.58f, 0.0f}, {6.5f, 1.05f, 0.0f},
    };
    vec3 visual_positions[POINT_COUNT] = {{0}};
    DvzColor colors[POINT_COUNT] = {{0}};
    float diameters[POINT_COUNT] = {0};

    for (uint32_t i = 0; i < POINT_COUNT; i++)
    {
        colors[i] = i % 2u == 0u
                        ? example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY)
                        : example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY);
        diameters[i] = i == 4u ? 38.0f : 25.0f;
    }

    if (dvz_panel_data_to_visual_positions(
            panel, (const float*)data_positions, (float*)visual_positions, POINT_COUNT) != 0)
        return false;

    DvzVisual* point = dvz_point(scene, 0);
    if (point == NULL)
        return false;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = visual_positions, .item_count = POINT_COUNT},
        {.attr_name = "color", .data = colors, .item_count = POINT_COUNT},
        {.attr_name = "diameter", .data = diameters, .item_count = POINT_COUNT},
    };
    if (dvz_visual_set_data_many(point, updates, 3) != 0)
        return false;
    if (dvz_visual_set_depth_test(point, false) != 0)
        return false;
    return dvz_panel_add_visual(panel, point, NULL) == 0;
}



/**
 * Configure axes for the guide-span example.
 *
 * @param panel target panel
 * @return true when axes were configured
 */
static bool _add_axes(DvzPanel* panel)
{
    ANN(panel);

    DvzAxis* x_axis = dvz_panel_axis(panel, DVZ_DIM_X);
    DvzAxis* y_axis = dvz_panel_axis(panel, DVZ_DIM_Y);
    if (x_axis == NULL || y_axis == NULL)
        return false;
    if (!example_graphite_cyan_apply_axis_style(x_axis, false, NULL))
        return false;
    if (!example_graphite_cyan_apply_axis_style(y_axis, true, NULL))
        return false;
    if (!dvz_axis_set_grid(x_axis, true) || !dvz_axis_set_grid(y_axis, true))
        return false;
    return dvz_axis_set_label(x_axis, "time") && dvz_axis_set_label(y_axis, "amplitude");
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the guide-span feature example.
 *
 * @param ctx scenario context
 * @param out_user scenario state output
 * @return true on success
 */
static bool _scenario_init(DvzScenarioContext* ctx, void** out_user)
{
    if (ctx == NULL || out_user == NULL)
        return false;

    GuideSpansState* state = (GuideSpansState*)dvz_calloc(1, sizeof(*state));
    if (state == NULL)
        return false;

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        goto error;

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    if (panel == NULL)
        goto error;
    example_graphite_cyan_set_panel_background(panel);

    if (!dvz_panel_set_layout_reserve(
            panel, &(DvzPanelLayoutReserve){
                       .left = 0.13f, .right = 0.06f, .bottom = 0.14f, .top = 0.06f}))
        goto error;
    if (dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, 7.0) != 0)
        goto error;
    if (dvz_panel_set_domain(panel, DVZ_DIM_Y, 0.0, 2.0) != 0)
        goto error;

    DvzGuideSpanDesc vdesc = dvz_guide_span_desc();
    vdesc.fill_color = dvz_color_rgba(76, 201, 240, 42);
    vdesc.outline_color = dvz_color_rgba(76, 201, 240, 170);
    vdesc.outline_width_px = 2.0f;
    vdesc.label = "window";
    state->vspan = dvz_vspan(panel, 1.2, 2.8, &vdesc);
    if (state->vspan == NULL)
        goto error;

    DvzGuideSpanDesc hdesc = dvz_guide_span_desc();
    hdesc.fill_color = dvz_color_rgba(255, 183, 3, 36);
    hdesc.outline_color = dvz_color_rgba(255, 183, 3, 170);
    hdesc.outline_width_px = 2.0f;
    hdesc.label = "target band";
    state->hspan = dvz_hspan(panel, 0.72, 1.28, &hdesc);
    if (state->hspan == NULL)
        goto error;

    if (!_add_points(ctx->scene, panel) || !_add_axes(panel))
        goto error;
    if (dvz_scenario_panzoom(ctx, panel, NULL, DVZ_DIM_MASK_XY) == NULL)
        goto error;

    *out_user = state;
    return true;

error:
    dvz_free(state);
    return false;
}



/**
 * Move both guide spans once so range updates are exercised.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_frame(DvzScenarioContext* ctx, void* user)
{
    if (ctx == NULL || user == NULL)
        return;

    GuideSpansState* state = (GuideSpansState*)user;
    if (!state->updated && ctx->time >= 1.0)
    {
        state->updated = dvz_guide_span_set_range(state->vspan, 4.0, 5.8) == 0 &&
                         dvz_guide_span_set_range(state->hspan, 1.15, 1.70) == 0;
    }
}



/**
 * Destroy the guide-span example state.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    dvz_free(user);
}



/**
 * Return the guide-span scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _guide_spans_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_guide_spans",
        .title = "guide_spans",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .init = _scenario_init,
        .frame = _scenario_frame,
        .destroy = _scenario_destroy,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the guide-span feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _guide_spans_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
