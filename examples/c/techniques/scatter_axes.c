/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* scatter_axes — interactive 2D scatter plot with WIP axes.
 *
 * Opens a GLFW window showing random discs with per-point color and diameter.
 * Left-drag to pan, right-drag or scroll to zoom, double-click to reset.
 * Axis tick labels and axis labels render through the scene text visual path.
 *
 * Build:  just example-c scatter_axes
 * Run:    ./build/examples/c/techniques/scatter_axes
 */

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "datoviz/app.h"
#include "datoviz/gui.h"
#include "datoviz/scene.h"
#include "example_common.h"

#define N 2000



typedef struct ScatterAxesState
{
    DvzPanel* panel;
    DvzAxis* x_axis;
    DvzAxis* y_axis;
    DvzAxisStyle x_style;
    DvzAxisStyle y_style;
    DvzAxisTickPolicy x_policy;
    DvzAxisTickPolicy y_policy;
    int text_renderer;
    bool x_visible;
    bool y_visible;
    bool x_grid;
    bool y_grid;
    bool show_bounds;
} ScatterAxesState;



/**
 * Return a reproducible random float in [0, 1].
 *
 * @return random float
 */
static float _randf(void)
{
    return (float)rand() / (float)RAND_MAX;
}



/**
 * Fill scatter arrays with synthetic clustered points.
 *
 * @param positions output data-space positions
 * @param colors output RGBA colors
 * @param sizes output diameters in pixels
 */
static void _make_scatter(vec3 positions[N], DvzColor colors[N], float sizes[N])
{
    for (uint32_t i = 0; i < N; i++)
    {
        float t = (float)i / (float)(N - 1);
        float angle = 8.0f * 3.14159265358979323846f * t;
        float radius = 0.35f + 2.35f * t;
        float x = radius * cosf(angle) + 0.35f * (_randf() - 0.5f);
        float y = radius * sinf(angle) + 0.35f * (_randf() - 0.5f);

        positions[i][0] = x;
        positions[i][1] = y;
        positions[i][2] = 0.0f;

        colors[i][0] = (uint8_t)(40.0f + 215.0f * t);
        colors[i][1] = (uint8_t)(180.0f + 60.0f * _randf());
        colors[i][2] = (uint8_t)(255.0f - 180.0f * t);
        colors[i][3] = 230;

        sizes[i] = 4.0f + 14.0f * _randf();
    }
}



/**
 * Return the axis text renderer matching one GUI combo index.
 *
 * @param index GUI combo index
 * @return text renderer
 */
static DvzTextRenderer _axis_renderer_from_index(int index)
{
    if (index == 1)
        return DVZ_TEXT_RENDERER_BITMAP_ATLAS;
    if (index == 2)
        return DVZ_TEXT_RENDERER_SMALL_BITMAP_ATLAS;
    return DVZ_TEXT_RENDERER_MSDF_ATLAS;
}



/**
 * Apply the live axis controls to the retained axes.
 *
 * @param state example state
 */
static void _apply_axis_controls(ScatterAxesState* state)
{
    if (state == NULL)
        return;
    state->x_style.text_renderer = _axis_renderer_from_index(state->text_renderer);
    state->y_style.text_renderer = state->x_style.text_renderer;
    (void)dvz_axis_set_style(state->x_axis, &state->x_style);
    (void)dvz_axis_set_style(state->y_axis, &state->y_style);
    (void)dvz_axis_set_tick_policy(state->x_axis, &state->x_policy);
    (void)dvz_axis_set_tick_policy(state->y_axis, &state->y_policy);
    (void)dvz_axis_set_visible(state->x_axis, state->x_visible);
    (void)dvz_axis_set_visible(state->y_axis, state->y_visible);
    (void)dvz_axis_set_grid(state->x_axis, state->x_grid);
    (void)dvz_axis_set_grid(state->y_axis, state->y_grid);
    (void)dvz_panel_set_bounds_visible(state->panel, state->show_bounds);
}



/**
 * Build the live axis control panel.
 *
 * @param gui GUI overlay
 * @param win view
 * @param user_data example state
 */
static void _scatter_axes_gui(DvzGui* gui, DvzView* win, void* user_data)
{
    (void)win;
    ScatterAxesState* state = (ScatterAxesState*)user_data;
    if (state == NULL)
        return;

    bool changed = false;
    float target_ticks = (float)state->x_policy.target_count;
    float min_spacing = state->x_policy.min_pixel_spacing;
    float minor_ticks = (float)state->x_policy.minor_per_interval;
    if (dvz_gui_begin(gui, "Axes", NULL, 0))
    {
        changed |= dvz_gui_checkbox(gui, "X axis", &state->x_visible);
        changed |= dvz_gui_checkbox(gui, "Y axis", &state->y_visible);
        changed |= dvz_gui_checkbox(gui, "X grid", &state->x_grid);
        changed |= dvz_gui_checkbox(gui, "Y grid", &state->y_grid);
        changed |= dvz_gui_checkbox(gui, "Bounds", &state->show_bounds);

        dvz_gui_separator_text(gui, "Text");
        changed |=
            dvz_gui_slider_float(gui, "Tick size", &state->x_style.tick_size_px, 8.0f, 28.0f);
        state->y_style.tick_size_px = state->x_style.tick_size_px;
        changed |=
            dvz_gui_slider_float(gui, "Label size", &state->x_style.label_size_px, 10.0f, 34.0f);
        state->y_style.label_size_px = state->x_style.label_size_px;
        const char* renderer_items[] = {"MSDF", "Bitmap", "Small bitmap"};
        changed |= dvz_gui_combo(gui, "Renderer", &state->text_renderer, renderer_items, 3);
        changed |=
            dvz_gui_slider_float(gui, "Tick gap", &state->x_style.tick_gap_px, 2.0f, 24.0f);
        state->y_style.tick_gap_px = state->x_style.tick_gap_px;
        changed |=
            dvz_gui_slider_float(gui, "Label gap", &state->x_style.label_gap_px, 16.0f, 60.0f);
        state->y_style.label_gap_px = state->x_style.label_gap_px;

        dvz_gui_separator_text(gui, "Ticks");
        changed |= dvz_gui_slider_float(gui, "Target ticks", &target_ticks, 2.0f, 12.0f);
        changed |= dvz_gui_slider_float(gui, "Min spacing", &min_spacing, 40.0f, 180.0f);
        changed |= dvz_gui_slider_float(gui, "Minor ticks", &minor_ticks, 0.0f, 8.0f);
    }
    dvz_gui_end(gui);

    if (changed)
    {
        state->x_policy.target_count = (uint32_t)(target_ticks + 0.5f);
        state->y_policy.target_count = state->x_policy.target_count;
        state->x_policy.min_pixel_spacing = min_spacing;
        state->y_policy.min_pixel_spacing = min_spacing;
        state->x_policy.minor_per_interval = (uint32_t)(minor_ticks + 0.5f);
        state->y_policy.minor_per_interval = state->x_policy.minor_per_interval;
        _apply_axis_controls(state);
    }
}



int main(int argc, char** argv)
{
    srand(42);

    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, 1000, 700, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.08f, 0.06f, 0.86f, 0.86f});
    EXAMPLE_CHECK(panel != NULL, "dvz_panel() failed");
    dvz_panel_set_background_color(panel, 0.07f, 0.08f, 0.11f, 1.0f);
    bool ok = dvz_panel_set_layout_reserve(
        panel, &(DvzPanelLayoutReserve){.left = 0.14f, .right = 0.0f, .bottom = 0.18f,
                                        .top = 0.0f});
    EXAMPLE_CHECK(ok, "dvz_panel_set_layout_reserve() failed");

    DvzVisual* visual = dvz_point(scene, 0);
    EXAMPLE_CHECK(visual != NULL, "dvz_point() failed");

    vec3 data_positions[N];
    vec3 visual_positions[N];
    DvzColor colors[N];
    float sizes[N];
    _make_scatter(data_positions, colors, sizes);

    dvz_panel_set_domain(panel, DVZ_DIM_X, -5.0, +5.0);
    dvz_panel_set_domain(panel, DVZ_DIM_Y, -3.0, +3.0);
    int rc = dvz_panel_data_to_visual_positions(
        panel, (const float*)data_positions, (float*)visual_positions, N);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_data_to_visual_positions() failed");

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = visual_positions, .item_count = N},
        {.attr_name = "color", .data = colors, .item_count = N},
        {.attr_name = "diameter", .data = sizes, .item_count = N},
    };
    rc = dvz_visual_set_data_many(visual, updates, 3);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_data_many() failed");
    rc = dvz_panel_add_visual(panel, visual, NULL);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual() failed");

    DvzAxis* x_axis = dvz_panel_axis(panel, DVZ_DIM_X);
    DvzAxis* y_axis = dvz_panel_axis(panel, DVZ_DIM_Y);
    dvz_axis_set_grid(x_axis, true);
    dvz_axis_set_grid(y_axis, true);
    dvz_axis_set_label(x_axis, "x");
    dvz_axis_set_label(y_axis, "y");

    ScatterAxesState state = {
        .panel = panel,
        .x_axis = x_axis,
        .y_axis = y_axis,
        .x_style = dvz_axis_style(),
        .y_style = dvz_axis_style(),
        .x_policy = dvz_axis_tick_policy(),
        .y_policy = dvz_axis_tick_policy(),
        .text_renderer = 0,
        .x_visible = true,
        .y_visible = true,
        .x_grid = true,
        .y_grid = true,
    };

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win = dvz_view_glfw(app, figure, 1000, 700, "scatter_axes");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    DvzPanzoom* panzoom = dvz_view_panzoom(win, panel, NULL);
    EXAMPLE_CHECK(panzoom != NULL, "failed to create or bind panzoom controller");

    DvzGuiConfig gui_config = dvz_gui_config();
    DvzGui* gui = dvz_view_gui(win, &gui_config);
    EXAMPLE_CHECK(gui != NULL, "dvz_view_gui() failed");
    dvz_view_set_gui_callback(win, _scatter_axes_gui, &state);

    dvz_app_run(app, example_frame_count(argc, argv));
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
