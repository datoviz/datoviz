/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* panel_linked links the X panzoom extent of two stacked signal panels.
 *
 * What to look for: both panels plot deterministic path data over the same X domain but use
 * different Y domains and independent Y panzoom controllers. In the live preview, pan or zoom
 * horizontally in either panel and the other follows; vertical zoom remains separate. This is
 * useful for comparing related time series or spectra that share one coordinate axis but require
 * different amplitude scales.
 *
 * Scenario: feature.panel_linked
 * Style: features, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c features/panel_linked
 * Run:    ./build/examples/c/features/panel_linked --live
 * Smoke:  ./build/examples/c/features/panel_linked --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_example_panel_linked_scenario(void);



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT
#define PATH_COUNT 128u

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct PanelLinkedState
{
    DvzPanzoom* top_x;
    DvzPanzoom* bottom_x;
} PanelLinkedState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Set the same X data domain and a panel-specific Y domain.
 *
 * @param panel target panel
 * @param ymin minimum Y domain value
 * @param ymax maximum Y domain value
 * @return true when both dimensions were set
 */
static bool _set_panel_domain(DvzPanel* panel, double ymin, double ymax)
{
    ANN(panel);

    int rc = dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, 10.0);
    if (rc != 0)
        return false;
    rc = dvz_panel_set_domain(panel, DVZ_DIM_Y, ymin, ymax);
    return rc == 0;
}



/**
 * Add one deterministic line panel.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param phase signal phase offset
 * @param color_base base green channel for the line colors
 * @return true when the visual was added
 */
static bool _add_line_panel(DvzScene* scene, DvzPanel* panel, float phase, uint8_t color_base)
{
    ANN(scene);
    ANN(panel);

    vec3 data_positions[PATH_COUNT] = {{0}};
    DvzColor colors[PATH_COUNT] = {{0}};
    float widths[PATH_COUNT] = {0};

    for (uint32_t i = 0; i < PATH_COUNT; i++)
    {
        const float t = PATH_COUNT > 1u ? (float)i / (float)(PATH_COUNT - 1u) : 0.0f;
        data_positions[i][0] = 10.0f * t;
        data_positions[i][1] =
            0.62f * sinf(TAU * (1.05f * t + phase)) +
            0.22f * cosf(TAU * (2.30f * t + 0.17f + phase));
        data_positions[i][2] = 0.0f;
        colors[i] = dvz_color_rgba(70, (uint8_t)(color_base + 32.0f * t), 232, 242);
        widths[i] = 3.0f;
    }

    DvzVisual* visual = dvz_path(scene, 0);
    if (visual == NULL)
        return false;
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = data_positions, .item_count = PATH_COUNT},
        {.attr_name = "color", .data = colors, .item_count = PATH_COUNT},
        {.attr_name = "stroke_width_px", .data = widths, .item_count = PATH_COUNT},
    };
    if (dvz_visual_set_data_many(visual, updates, 3) != 0)
        return false;
    if (dvz_path_set_caps(visual, DVZ_SEGMENT_CAP_ROUND, DVZ_SEGMENT_CAP_ROUND) != 0)
        return false;
    if (dvz_path_set_join(visual, DVZ_PATH_JOIN_ROUND, 4.0f) != 0)
        return false;
    if (dvz_visual_set_depth_test(visual, false) != 0)
        return false;
    return dvz_panel_add_visual(panel, visual, NULL) == 0;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the linked-panel feature scenario.
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

    DvzGrid* grid = dvz_figure_grid(ctx->figure, 2, 1);
    if (grid == NULL)
        return false;
    if (!example_configure_compact_grid(grid, 0.0f, 34.0f))
        return false;

    DvzPanel* top = dvz_grid_panel(grid, 0, 0);
    DvzPanel* bottom = dvz_grid_panel(grid, 1, 0);
    if (top == NULL || bottom == NULL)
        return false;

    DvzPanel* panels[2] = {top, bottom};
    for (uint32_t i = 0; i < 2u; i++)
    {
        example_graphite_cyan_set_panel_background(panels[i]);
        DvzPanelBorderDesc border = dvz_panel_border_desc();
        border.color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_GRID);
        border.width_px = 1.5f;
        if (dvz_panel_set_border(panels[i], &border) != DVZ_OK)
            return false;
    }

    if (!_set_panel_domain(top, -1.1, 1.1))
        return false;
    if (!_set_panel_domain(bottom, -1.8, 1.8))
        return false;
    if (!_add_line_panel(ctx->scene, top, 0.03f, 188u))
        return false;
    if (!_add_line_panel(ctx->scene, bottom, 0.24f, 164u))
        return false;

    DvzController* top_x = dvz_panzoom(ctx->scene, NULL);
    DvzController* bottom_x = dvz_panzoom(ctx->scene, NULL);
    DvzController* top_y = dvz_panzoom(ctx->scene, NULL);
    DvzController* bottom_y = dvz_panzoom(ctx->scene, NULL);
    if (top_x == NULL || bottom_x == NULL || top_y == NULL || bottom_y == NULL)
        return false;

    DvzPanzoom* top_x_panzoom = dvz_controller_panzoom(top_x);
    DvzPanzoom* bottom_x_panzoom = dvz_controller_panzoom(bottom_x);
    DvzPanzoom* top_y_panzoom = dvz_controller_panzoom(top_y);
    DvzPanzoom* bottom_y_panzoom = dvz_controller_panzoom(bottom_y);
    if (
        top_x_panzoom == NULL || bottom_x_panzoom == NULL || top_y_panzoom == NULL ||
        bottom_y_panzoom == NULL)
        return false;

    PanelLinkedState* state = (PanelLinkedState*)dvz_calloc(1, sizeof(*state));
    if (state == NULL)
        return false;
    state->top_x = top_x_panzoom;
    state->bottom_x = bottom_x_panzoom;
    if (out_user != NULL)
        *out_user = state;

    dvz_panzoom_zoom(top_x_panzoom, (vec2){1.80f, 1.0f});
    dvz_panzoom_pan(top_x_panzoom, (vec2){+0.22f, 0.0f});
    dvz_panzoom_zoom(top_y_panzoom, (vec2){1.0f, 1.15f});
    dvz_panzoom_zoom(bottom_y_panzoom, (vec2){1.0f, 1.45f});

    if (dvz_scenario_bind_controller(ctx, top, top_x, DVZ_DIM_MASK_X) != 0)
        return false;
    if (dvz_scenario_bind_controller(ctx, top, top_y, DVZ_DIM_MASK_Y) != 0)
        return false;
    if (dvz_scenario_bind_controller(ctx, bottom, bottom_x, DVZ_DIM_MASK_X) != 0)
        return false;
    if (dvz_scenario_bind_controller(ctx, bottom, bottom_y, DVZ_DIM_MASK_Y) != 0)
        return false;
    return dvz_controller_link(
               ctx->scene, top_x, bottom_x, DVZ_CONTROLLER_LINK_EXTENT_X,
               DVZ_CONTROLLER_LINK_TWO_WAY) != NULL;
}



/**
 * Apply deterministic linked-X panning for generated gallery media.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_frame(DvzScenarioContext* ctx, void* user)
{
    PanelLinkedState* state = (PanelLinkedState*)user;
    if (ctx == NULL || !ctx->preview_mode || state == NULL)
        return;

    const uint64_t count = ctx->preview_frame_count > 0 ? ctx->preview_frame_count : 1;
    const float phase = (float)(ctx->preview_frame_index % count) / (float)count;
    const float theta = TAU * phase;
    vec2 pan = {+0.22f + 0.22f * sinf(theta), 0.0f};
    vec2 zoom = {1.80f, 1.0f};
    if (state->top_x != NULL)
    {
        dvz_panzoom_zoom(state->top_x, zoom);
        dvz_panzoom_pan(state->top_x, pan);
    }
    if (state->bottom_x != NULL)
    {
        dvz_panzoom_zoom(state->bottom_x, zoom);
        dvz_panzoom_pan(state->bottom_x, pan);
    }
}



/**
 * Destroy the linked-panel scenario state.
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
 * Return the linked-panel scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_example_panel_linked_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_panel_linked",
        .title = "Linked Panels",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_POINT_VISUAL | DVZ_SCENARIO_REQ_CONTROLLER |
                        DVZ_SCENARIO_REQ_PANZOOM | DVZ_SCENARIO_REQ_FRAME_CALLBACKS,
        .init = _scenario_init,
        .frame = _scenario_frame,
        .destroy = _scenario_destroy,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the linked-panel feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_example_panel_linked_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
