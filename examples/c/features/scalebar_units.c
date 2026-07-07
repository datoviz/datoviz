/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* scalebar_units uses duration units for a scale bar on a time-series panel.
 *
 * What to look for: the path visual uploads 96 time samples whose X positions span 0 to 250 and
 * whose Y positions form a waveform. The scale bar uses the built-in duration unit ladder, and
 * the panzoom controller keeps Y fixed so horizontal navigation changes the visible time span.
 * This is useful for signal, electrophysiology, or video-aligned plots where the reader needs a
 * time interval cue rather than a distance cue.
 *
 * Scenario: scalebar_units
 * Style: features, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c features/scalebar_units
 * Run:    ./build/examples/c/features/scalebar_units --live
 * Smoke:  ./build/examples/c/features/scalebar_units --png
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



DvzScenarioSpec dvz_example_scalebar_units_scenario(void);



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT
#define SAMPLE_COUNT 96u

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct ScaleBarUnitsState
{
    DvzPanzoom* panzoom;
} ScaleBarUnitsState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Add a deterministic time-series trace.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @return true when the visual was added
 */
static bool _add_signal(DvzScene* scene, DvzPanel* panel)
{
    ANN(scene);
    ANN(panel);

    vec3 data_positions[SAMPLE_COUNT] = {{0}};
    DvzColor colors[SAMPLE_COUNT] = {{0}};
    float widths[SAMPLE_COUNT] = {0};

    DvzColor accent = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY);
    for (uint32_t i = 0; i < SAMPLE_COUNT; i++)
    {
        const float t = SAMPLE_COUNT > 1u ? (float)i / (float)(SAMPLE_COUNT - 1u) : 0.0f;
        data_positions[i][0] = 250.0f * t;
        data_positions[i][1] =
            0.35f * sinf(TAU * 2.0f * t) + 0.16f * cosf(TAU * 5.0f * t + 0.4f);
        data_positions[i][2] = 0.0f;
        colors[i] = accent;
        colors[i].a = 230u;
        widths[i] = 3.0f;
    }

    DvzVisual* path = dvz_path(scene, 0);
    if (path == NULL)
        return false;
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = data_positions, .item_count = SAMPLE_COUNT},
        {.attr_name = "color", .data = colors, .item_count = SAMPLE_COUNT},
        {.attr_name = "stroke_width_px", .data = widths, .item_count = SAMPLE_COUNT},
    };
    if (dvz_visual_set_data_many(path, updates, 3) != 0)
        return false;
    if (dvz_visual_set_depth_test(path, false) != 0)
        return false;
    return dvz_panel_add_visual(panel, path, NULL) == 0;
}



/**
 * Add one scale bar whose panel X data units are milliseconds.
 *
 * @param scene scene owning unit objects
 * @param panel panel receiving the annotation
 * @return true when the scale bar was added
 */
static bool _add_time_scalebar(DvzScene* scene, DvzPanel* panel)
{
    ANN(scene);
    ANN(panel);

    DvzUnits* duration_units = dvz_units_builtin(scene, DVZ_UNIT_LADDER_DURATION, 0.001);
    if (duration_units == NULL)
        return false;

    DvzColor color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY);
    DvzScaleBarDesc desc = {
        DVZ_STRUCT_INIT_FIELDS(DvzScaleBarDesc),
        .dimension = DVZ_DIM_X,
        .anchor = DVZ_SCENE_ANCHOR_BOTTOM_LEFT,
        .label_position = DVZ_SCALEBAR_LABEL_ABOVE,
        .target_length_px = 240.0f,
        .min_length_px = 170.0f,
        .max_length_px = 310.0f,
        .offset_px = {72.0f, 82.0f},
        .tick_length_px = 18.0f,
        .line_width_px = 4.0f,
        .line_color = {color.r, color.g, color.b, 255u},
    };
    DvzTextStyle label_style = {
        DVZ_STRUCT_INIT_FIELDS(DvzTextStyle),
        .size_px = 17.0f,
        .renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS,
        .color = {color.r, color.g, color.b, 255u},
    };

    DvzScaleBar* scalebar = dvz_scale_bar(panel, &desc);
    return scalebar != NULL && dvz_scale_bar_set_label_style(scalebar, &label_style) == 0 &&
           dvz_scale_bar_set_units(scalebar, duration_units) == 0;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the retained scale-bar custom-unit scenario.
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

    if (dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, 250.0) != 0)
        return false;
    if (dvz_panel_set_domain(panel, DVZ_DIM_Y, -1.0, 1.0) != 0)
        return false;

    if (!_add_signal(ctx->scene, panel))
        return false;
    if (!_add_time_scalebar(ctx->scene, panel))
        return false;

    DvzPanzoomDesc panzoom_desc = dvz_panzoom_desc();
    panzoom_desc.controller_flags = DVZ_PANZOOM_FLAGS_FIXED_Y;
    DvzPanzoom* panzoom = dvz_scenario_panzoom(ctx, panel, &panzoom_desc, DVZ_DIM_MASK_XY);
    if (panzoom == NULL)
        return false;

    ScaleBarUnitsState* state = (ScaleBarUnitsState*)dvz_calloc(1, sizeof(*state));
    if (state == NULL)
        return false;
    state->panzoom = panzoom;
    if (out_user != NULL)
        *out_user = state;
    return true;
}



/**
 * Animate the visible duration span for generated gallery media.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_frame(DvzScenarioContext* ctx, void* user)
{
    ScaleBarUnitsState* state = (ScaleBarUnitsState*)user;
    if (ctx == NULL || !ctx->preview_mode || state == NULL || state->panzoom == NULL)
        return;

    const uint64_t count = ctx->preview_frame_count > 0 ? ctx->preview_frame_count : 1;
    const float phase = (float)(ctx->preview_frame_index % count) / (float)count;
    const float zoom_x = 1.48f + 0.48f * sinf(TAU * phase);
    (void)dvz_panzoom_pan(state->panzoom, (vec2){0.0f, 0.0f});
    (void)dvz_panzoom_zoom(state->panzoom, (vec2){zoom_x, 1.0f});
}



/**
 * Destroy the scale-bar units scenario state.
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
 * Return the scale-bar custom-unit scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_example_scalebar_units_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_scalebar_units",
        .title = "Scale Bar Units",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_TEXT_VISUAL | DVZ_SCENARIO_REQ_CONTROLLER |
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
 * Run the retained scale-bar custom-unit proof through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_example_scalebar_units_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
