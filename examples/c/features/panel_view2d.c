/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* This example compares a regular 2D panel with one that keeps equal data aspect while their
 * relative widths oscillate.
 *
 * What to look for: both panels draw the same unit circle path and styled X/Y axes. The left panel
 * uses a direct [-1, 1] domain, while the right panel configures an equal-aspect view and verifies
 * matching data units per pixel in X and Y. Compare the circle shape and grid spacing; equal
 * aspect is essential when distance, angle, or shape should not be visually distorted. During
 * live playback, the left circle stretches with its panel while the right circle remains circular
 * and its visible data extent changes.
 *
 * Scenario: features_panel_view2d
 * Style: features, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c features/panel_view2d
 * Run:    ./build/examples/c/features/panel_view2d --live
 * Smoke:  ./build/examples/c/features/panel_view2d --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#include "_alloc.h"
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
#define CIRCLE_COUNT 97u

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct PanelView2DState
{
    DvzPanel* free_panel;
    DvzPanel* fit_panel;
} PanelView2DState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Add a unit circle in data coordinates.
 *
 * @param scene scene owning visuals
 * @param panel target panel
 * @param color rectangle color
 * @return true when visuals were added
 */
static bool _add_domain_shape(DvzScene* scene, DvzPanel* panel, DvzColor color)
{
    ANN(scene);
    ANN(panel);

    vec3 circle[CIRCLE_COUNT] = {0};
    DvzColor circle_colors[CIRCLE_COUNT] = {{0}};
    float circle_widths[CIRCLE_COUNT] = {0};
    for (uint32_t i = 0; i < CIRCLE_COUNT; i++)
    {
        const float t = (float)i / (float)(CIRCLE_COUNT - 1u);
        const float a = 6.283185307179586f * t;
        circle[i][0] = cosf(a);
        circle[i][1] = sinf(a);
        circle[i][2] = 0.0f;
        circle_colors[i] = color;
        circle_widths[i] = 4.0f;
    }

    DvzVisual* path = dvz_path(scene, 0);
    if (path == NULL)
        return false;
    DvzVisualDataUpdate path_updates[] = {
        {.attr_name = "position", .data = circle, .item_count = CIRCLE_COUNT},
        {.attr_name = "color", .data = circle_colors, .item_count = CIRCLE_COUNT},
        {.attr_name = "stroke_width_px", .data = circle_widths, .item_count = CIRCLE_COUNT},
    };
    if (dvz_visual_set_data_many(path, path_updates, 3) != 0)
        return false;
    if (dvz_path_set_caps(path, DVZ_SEGMENT_CAP_ROUND, DVZ_SEGMENT_CAP_ROUND) != 0)
        return false;
    if (dvz_path_set_join(path, DVZ_PATH_JOIN_ROUND, 4.0f) != 0)
        return false;
    DvzVisualAttachDesc path_attach = {
        DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc), .coord_space = DVZ_VISUAL_COORD_DATA};
    if (dvz_panel_add_visual(panel, path, &path_attach) != 0)
        return false;

    return true;
}



/**
 * Configure axes for the panel-view fit example.
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
    DvzAxisTickPolicy ticks = dvz_axis_tick_policy();
    ticks.target_count = 5;
    ticks.min_pixel_spacing = 130.0f;
    ticks.minor_per_interval = 0;
    if (dvz_axis_set_tick_policy(x_axis, &ticks) != DVZ_OK || dvz_axis_set_tick_policy(y_axis, &ticks) != DVZ_OK)
        return false;
    if (dvz_axis_set_grid(x_axis, true) != DVZ_OK || dvz_axis_set_grid(y_axis, true) != DVZ_OK)
        return false;
    return dvz_axis_set_label(x_axis, "domain x") == DVZ_OK &&
           dvz_axis_set_label(y_axis, "domain y") == DVZ_OK;
}



/**
 * Update the two panel rectangles for one animation phase.
 *
 * @param state scenario state
 * @param phase normalized animation phase in [0, 1)
 * @return true when both panel rectangles were updated
 */
static bool _set_panel_split(PanelView2DState* state, float phase)
{
    if (state == NULL || state->free_panel == NULL || state->fit_panel == NULL)
        return false;

    const float margin = 0.04f;
    const float gutter = 0.025f;
    const float available = 1.0f - 2.0f * margin - gutter;
    const float split = 0.5f + 0.20f * sinf(TAU * phase);
    const float left_width = available * split;
    const float right_width = available - left_width;
    const DvzPanelDesc free_desc = {margin, 0.08f, left_width, 0.84f};
    const DvzPanelDesc fit_desc = {
        margin + left_width + gutter, 0.08f, right_width, 0.84f};
    return dvz_panel_set_desc(state->free_panel, &free_desc) == DVZ_OK &&
           dvz_panel_set_desc(state->fit_panel, &fit_desc) == DVZ_OK;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the panel-view fit feature example.
 *
 * @param ctx scenario context
 * @param out_user scenario state output
 * @return true on success
 */
static bool _scenario_init(DvzScenarioContext* ctx, void** out_user)
{
    if (ctx == NULL || out_user == NULL)
        return false;

    PanelView2DState* state = (PanelView2DState*)dvz_calloc(1, sizeof(PanelView2DState));
    if (state == NULL)
        return false;

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        goto error;

    DvzPanelDesc free_desc = {0.04f, 0.08f, 0.4475f, 0.84f};
    DvzPanelDesc fit_desc = {0.5125f, 0.08f, 0.4475f, 0.84f};
    DvzPanel* free_panel = dvz_panel(ctx->figure, &free_desc);
    DvzPanel* fit_panel = dvz_panel(ctx->figure, &fit_desc);
    if (free_panel == NULL || fit_panel == NULL)
        goto error;
    state->free_panel = free_panel;
    state->fit_panel = fit_panel;

    DvzPanel* panels[2] = {free_panel, fit_panel};
    for (uint32_t i = 0; i < 2u; i++)
    {
        example_graphite_cyan_set_panel_background(panels[i]);
        if (!_add_axes(panels[i]))
            goto error;
    }

    if (dvz_panel_set_domain(free_panel, DVZ_DIM_X, -1.0, +1.0) != 0)
        goto error;
    if (dvz_panel_set_domain(free_panel, DVZ_DIM_Y, -1.0, +1.0) != 0)
        goto error;

    if (!example_configure_equal_aspect_panel(
            fit_panel, (DvzDataDomain){.min = -1.0, .max = +1.0},
            (DvzDataDomain){.min = -1.0, .max = +1.0}, 0.18))
        goto error;

    double x_min = 0.0;
    double x_max = 0.0;
    double y_min = 0.0;
    double y_max = 0.0;
    if (!dvz_panel_visible_domain(fit_panel, DVZ_DIM_X, &x_min, &x_max))
        goto error;
    if (!dvz_panel_visible_domain(fit_panel, DVZ_DIM_Y, &y_min, &y_max))
        goto error;
    const double x_span = x_max - x_min;
    const double y_span = y_max - y_min;
    DvzRect plot = {0};
    if (!dvz_panel_plot_rect_px(fit_panel, &plot) || !(plot.width > 0.0f) ||
        !(plot.height > 0.0f))
        goto error;
    const double x_units_per_px = x_span / (double)plot.width;
    const double y_units_per_px = y_span / (double)plot.height;
    if (fabs(x_units_per_px - y_units_per_px) > 1e-4)
        goto error;

    if (!_add_domain_shape(
            ctx->scene, free_panel,
            example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY)))
        goto error;
    if (!_add_domain_shape(
            ctx->scene, fit_panel,
            example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY)))
        goto error;
    DvzController* free_controller = dvz_panzoom(ctx->scene, NULL);
    if (free_controller == NULL)
        goto error;
    DvzPanzoom* free_panzoom = dvz_controller_panzoom(free_controller);
    if (
        free_panzoom == NULL ||
        dvz_scenario_bind_controller(ctx, free_panel, free_controller, DVZ_DIM_MASK_XY) != 0)
    {
        goto error;
    }

    DvzPanzoomDesc fit_panzoom_desc = dvz_panzoom_desc();
    fit_panzoom_desc.controller_flags = DVZ_PANZOOM_FLAGS_KEEP_ASPECT;
    DvzController* fit_controller = dvz_panzoom(ctx->scene, &fit_panzoom_desc);
    if (fit_controller == NULL)
        goto error;
    DvzPanzoom* fit_panzoom = dvz_controller_panzoom(fit_controller);
    if (
        fit_panzoom == NULL ||
        dvz_scenario_bind_controller(ctx, fit_panel, fit_controller, DVZ_DIM_MASK_XY) != 0)
    {
        goto error;
    }

    *out_user = state;
    return true;

error:
    dvz_free(state);
    return false;
}



/**
 * Animate the relative panel widths for one frame.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_frame(DvzScenarioContext* ctx, void* user)
{
    if (ctx == NULL || user == NULL)
        return;

    const double time = ctx->preview_mode ? dvz_scenario_preview_time(ctx) : ctx->time;
    const float phase = (float)fmod(time / 4.0, 1.0);
    (void)_set_panel_split((PanelView2DState*)user, phase);
}



/**
 * Destroy the panel-view scenario state.
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
 * Return the panel 2D view scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _panel_view2d_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "features_panel_view2d",
        .title = "Panel View 2D",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_FRAME_CALLBACKS | DVZ_SCENARIO_REQ_CONTINUOUS_FRAMES,
        .continuous_frames = true,
        .init = _scenario_init,
        .frame = _scenario_frame,
        .destroy = _scenario_destroy,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the panel-view fit feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _panel_view2d_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
