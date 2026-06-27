/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* guide_lines - retained horizontal and vertical guide lines in panel data coordinates.
 *
 * Scenario: feature.guide_lines
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/guide_lines
 * Run:    ./build/examples/c/features/guide_lines --live
 * Smoke:  ./build/examples/c/features/guide_lines --png
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
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_example_guide_lines_scenario(void);



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH      1600u
#define HEIGHT     1200u
#define PATH_COUNT 192u

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct GuideLinesState
{
    DvzPanel* panel;
    DvzGuideLine* hline;
    DvzGuideLine* vline;
} GuideLinesState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Add a deterministic path so the guide coordinates have visible context.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @return true when the path was added
 */
static bool _add_curve(DvzScene* scene, DvzPanel* panel)
{
    ANN(scene);
    ANN(panel);

    vec3 data_positions[PATH_COUNT] = {{0}};
    DvzColor colors[PATH_COUNT] = {{0}};
    float widths[PATH_COUNT] = {0};

    for (uint32_t i = 0; i < PATH_COUNT; i++)
    {
        const float t = (float)i / (float)(PATH_COUNT - 1u);
        const float x = 10.0f * t;
        data_positions[i][0] = x;
        data_positions[i][1] = 0.72f * sinf(TAU * t) + 0.18f * sinf(3.0f * TAU * t);
        data_positions[i][2] = 0.0f;
        colors[i] = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY);
        widths[i] = 4.0f;
    }

    DvzVisual* path = dvz_path(scene, 0);
    if (path == NULL)
        return false;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = data_positions, .item_count = PATH_COUNT},
        {.attr_name = "color", .data = colors, .item_count = PATH_COUNT},
        {.attr_name = "stroke_width_px", .data = widths, .item_count = PATH_COUNT},
    };
    if (dvz_visual_set_data_many(path, updates, 3) != 0)
        return false;
    if (dvz_path_set_caps(path, DVZ_SEGMENT_CAP_ROUND, DVZ_SEGMENT_CAP_ROUND) != 0)
        return false;
    if (dvz_path_set_join(path, DVZ_PATH_JOIN_ROUND, 4.0f) != 0)
        return false;
    return dvz_panel_add_visual(panel, path, NULL) == 0;
}



/**
 * Configure axes for guide-line context.
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
    return dvz_axis_set_label(x_axis, "x") && dvz_axis_set_label(y_axis, "signal");
}


/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the guide-line feature example.
 *
 * @param ctx scenario context
 * @param out_user scenario state output
 * @return true on success
 */
static bool _scenario_init(DvzScenarioContext* ctx, void** out_user)
{
    if (ctx == NULL || out_user == NULL)
        return false;

    GuideLinesState* state = (GuideLinesState*)dvz_calloc(1, sizeof(*state));
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

    if (dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, 10.0) != 0)
        goto error;
    if (dvz_panel_set_domain(panel, DVZ_DIM_Y, -1.25, 1.25) != 0)
        goto error;

    DvzGuideLineDesc hdesc = dvz_guide_line_desc();
    hdesc.color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING);
    hdesc.stroke_width_px = 3.0f;
    hdesc.label = "threshold";
    state->hline = dvz_hline(panel, 0.45, &hdesc);
    if (state->hline == NULL)
        goto error;

    DvzGuideLineDesc vdesc = dvz_guide_line_desc();
    vdesc.color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY);
    vdesc.stroke_width_px = 2.5f;
    vdesc.label = "event";
    state->vline = dvz_vline(panel, 3.6, &vdesc);
    if (state->vline == NULL)
        goto error;

    if (!_add_curve(ctx->scene, panel) || !_add_axes(panel))
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
 * Move both guide lines to the current pointer position.
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

    GuideLinesState* state = (GuideLinesState*)user;
    double data[2] = {0};
    if (!dvz_panel_position_to_data(
            state->panel, DVZ_PANEL_COORD_FIGURE_PX,
            (const double[2]){pointer->x, pointer->y}, data))
    {
        return;
    }

    (void)dvz_guide_line_set_value(state->vline, data[0]);
    (void)dvz_guide_line_set_value(state->hline, data[1]);
}



/**
 * Destroy the guide-line example state.
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
 * Return the guide-line scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_example_guide_lines_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_guide_lines",
        .title = "guide_lines",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_CONTROLLER | DVZ_SCENARIO_REQ_PANZOOM,
        .init = _scenario_init,
        .event = _scenario_event,
        .destroy = _scenario_destroy,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the guide-line feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_example_guide_lines_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
