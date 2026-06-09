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
    DvzPanel* panel;
    DvzGuideSpan* hspan;
    DvzGuideSpan* vspan;
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


/**
 * Convert one panel-local pointer position to the panel data domain.
 *
 * @param panel target panel
 * @param panel_x panel-local X coordinate in logical pixels
 * @param panel_y panel-local Y coordinate in logical pixels
 * @param out_x output data X
 * @param out_y output data Y
 * @return whether the position is inside the plot rectangle
 */
static bool
_panel_pointer_to_data(DvzPanel* panel, double panel_x, double panel_y, double* out_x, double* out_y)
{
    if (panel == NULL || out_x == NULL || out_y == NULL)
        return false;

    DvzRect plot = {0};
    if (!dvz_panel_plot_rect_px(panel, &plot) || plot.width <= 0.0f || plot.height <= 0.0f)
        return false;

    double x0 = 0.0;
    double x1 = 0.0;
    double y0 = 0.0;
    double y1 = 0.0;
    if (!dvz_panel_visible_domain(panel, DVZ_DIM_X, &x0, &x1) ||
        !dvz_panel_visible_domain(panel, DVZ_DIM_Y, &y0, &y1))
    {
        return false;
    }

    const double tx = (panel_x - (double)plot.x) / (double)plot.width;
    const double ty = (panel_y - (double)plot.y) / (double)plot.height;
    if (tx < 0.0 || tx > 1.0 || ty < 0.0 || ty > 1.0)
        return false;

    *out_x = x0 + tx * (x1 - x0);
    *out_y = y1 - ty * (y1 - y0);
    return true;
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
    state->panel = panel;
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
 * Center both guide spans around the current pointer position.
 *
 * @param ctx scenario context
 * @param event scenario event
 * @param user scenario state
 */
static void _scenario_event(DvzScenarioContext* ctx, const DvzScenarioEvent* event, void* user)
{
    if (ctx == NULL || event == NULL || user == NULL || event->kind != DVZ_SCENARIO_EVENT_POINTER)
        return;

    const DvzScenarioPointerEvent* pointer = &event->content.pointer;
    if (
        pointer->type != DVZ_SCENARIO_POINTER_MOVE &&
        pointer->type != DVZ_SCENARIO_POINTER_DRAG)
        return;

    GuideSpansState* state = (GuideSpansState*)user;
    double panel_x = 0.0;
    double panel_y = 0.0;
    double data_x = 0.0;
    double data_y = 0.0;
    if (!dvz_scenario_panel_pointer_position(state->panel, pointer, &panel_x, &panel_y))
        return;
    if (!_panel_pointer_to_data(state->panel, panel_x, panel_y, &data_x, &data_y))
        return;

    const double half_x = 0.65;
    const double half_y = 0.22;
    (void)dvz_guide_span_set_range(state->vspan, data_x - half_x, data_x + half_x);
    (void)dvz_guide_span_set_range(state->hspan, data_y - half_y, data_y + half_y);
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
        .event = _scenario_event,
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
