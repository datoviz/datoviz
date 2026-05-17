/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* visual_marker_stress_glfw - live GLFW marker visual parameter stress tool.
 *
 * Build:  register externally or locally as a C example target, then build
 * Run:    ./build/examples/c/visual_marker_stress_glfw [frames]
 */

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "_alloc.h"
#include "_compat.h"
#include "datoviz/app.h"
#include "datoviz/gui.h"
#include "datoviz/imgui.h"
#include "datoviz/scene.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define MARKER_STRESS_MAX_COUNT 32768u
#define MARKER_STRESS_WIDTH     1100u
#define MARKER_STRESS_HEIGHT    760u
#define MARKER_STRESS_PI        3.14159265358979323846f



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct MarkerStressState
{
    DvzVisual* visual;

    float(*positions)[3];
    DvzColor* colors;
    float* diameters;
    float* angles;
    uint32_t* shapes;

    uint32_t max_count;
    uint32_t active_count;
    float active_count_value;
    float diameter;
    float base_angle;
    float phase;
    float fill_alpha;
    float edge_color[4];
    float stroke_width;
    bool animate;
    bool mixed_shapes;
    bool filled;
    bool stroke;
    bool outline;
    int shape_index;
} MarkerStressState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return the marker shape represented by a GUI combo index.
 *
 * @param index GUI combo index
 * @return marker shape enum value
 */
static DvzMarkerShape _shape_from_index(int index)
{
    switch (index)
    {
    case 1:
        return DVZ_MARKER_SHAPE_SQUARE;
    case 2:
        return DVZ_MARKER_SHAPE_TRIANGLE;
    case 3:
        return DVZ_MARKER_SHAPE_DIAMOND;
    case 4:
        return DVZ_MARKER_SHAPE_CROSS;
    case 5:
        return DVZ_MARKER_SHAPE_RING;
    default:
        return DVZ_MARKER_SHAPE_DISC;
    }
}



/**
 * Clamp a floating-point active count to the supported marker range.
 *
 * @param value requested count value
 * @param max_count maximum supported count
 * @return clamped integer count
 */
static uint32_t _clamp_count(float value, uint32_t max_count)
{
    if (!isfinite(value) || value < 1.0f)
        return 1;
    if (value > (float)max_count)
        return max_count;
    return (uint32_t)(value + 0.5f);
}



/**
 * Convert a normalized color component to an RGBA8 channel.
 *
 * @param value normalized color component
 * @return clamped 8-bit channel
 */
static uint8_t _u8_from_float(float value)
{
    if (!isfinite(value) || value <= 0.0f)
        return 0;
    if (value >= 1.0f)
        return 255;
    return (uint8_t)(255.0f * value + 0.5f);
}



/**
 * Fill the marker position and base color arrays with deterministic synthetic data.
 *
 * @param state marker stress state
 */
static void _make_marker_data(MarkerStressState* state)
{
    const uint32_t columns = 256u;
    const float inv_columns = 1.0f / (float)(columns - 1u);
    const float rows = (float)((state->max_count + columns - 1u) / columns);
    const float inv_rows = rows > 1.0f ? 1.0f / (rows - 1.0f) : 1.0f;

    for (uint32_t i = 0; i < state->max_count; i++)
    {
        uint32_t col = i % columns;
        uint32_t row = i / columns;
        float u = (float)col * inv_columns;
        float v = (float)row * inv_rows;
        float wave = sinf(18.0f * u + 7.0f * v);

        state->positions[i][0] = -0.96f + 1.92f * u;
        state->positions[i][1] = -0.86f + 1.72f * v;
        state->positions[i][2] = 0.035f * wave;

        state->colors[i][0] = (uint8_t)(40.0f + 205.0f * u);
        state->colors[i][1] = (uint8_t)(220.0f - 140.0f * v);
        state->colors[i][2] = (uint8_t)(90.0f + 120.0f * (0.5f + 0.5f * wave));
        state->colors[i][3] = 230;

        state->diameters[i] = state->diameter;
        state->angles[i] = 0.0f;
        state->shapes[i] = (uint32_t)(i % 6u);
    }
}



/**
 * Upload the current fill alpha to the active marker color prefix.
 *
 * @param state marker stress state
 */
static void _upload_colors(MarkerStressState* state)
{
    uint8_t alpha = _u8_from_float(state->fill_alpha);
    for (uint32_t i = 0; i < state->active_count; i++)
        state->colors[i][3] = alpha;

    (void)dvz_visual_set_data(state->visual, "color", state->colors, state->active_count);
}



/**
 * Upload the current marker diameter to the active marker prefix.
 *
 * @param state marker stress state
 */
static void _upload_diameters(MarkerStressState* state)
{
    for (uint32_t i = 0; i < state->active_count; i++)
        state->diameters[i] = state->diameter;

    (void)dvz_visual_set_data(state->visual, "diameter", state->diameters, state->active_count);
}



/**
 * Upload the current marker angle field to the active marker prefix.
 *
 * @param state marker stress state
 */
static void _upload_angles(MarkerStressState* state)
{
    float phase = state->base_angle + state->phase;
    for (uint32_t i = 0; i < state->active_count; i++)
    {
        float offset = state->mixed_shapes ? 0.19f * (float)(i % 17u) : 0.0f;
        state->angles[i] = phase + offset;
    }

    (void)dvz_visual_set_data(state->visual, "angle", state->angles, state->active_count);
}



/**
 * Upload the current marker shape field to the active marker prefix.
 *
 * @param state marker stress state
 */
static void _upload_shapes(MarkerStressState* state)
{
    DvzMarkerShape shape = _shape_from_index(state->shape_index);
    for (uint32_t i = 0; i < state->active_count; i++)
        state->shapes[i] = state->mixed_shapes ? (uint32_t)(i % 6u) : (uint32_t)shape;

    (void)dvz_visual_set_data(state->visual, "shape", state->shapes, state->active_count);
}



/**
 * Upload all dense marker attributes for the active marker prefix.
 *
 * @param state marker stress state
 */
static void _upload_marker_attributes(MarkerStressState* state)
{
    (void)dvz_visual_set_data(state->visual, "position", state->positions, state->active_count);
    _upload_colors(state);
    _upload_diameters(state);
    _upload_angles(state);
    _upload_shapes(state);
}



/**
 * Apply the current fill, stroke, outline, and edge color style to the marker visual.
 *
 * @param state marker stress state
 */
static void _apply_marker_style(MarkerStressState* state)
{
    DvzMarkerStyle style = dvz_marker_style();
    style.edge_color[0] = _u8_from_float(state->edge_color[0]);
    style.edge_color[1] = _u8_from_float(state->edge_color[1]);
    style.edge_color[2] = _u8_from_float(state->edge_color[2]);
    style.edge_color[3] = _u8_from_float(state->edge_color[3]);
    style.stroke_width = state->stroke_width;
    style.filled = state->filled;
    style.stroke = state->stroke;
    style.outline = state->outline;
    (void)dvz_marker_set_style(state->visual, &style);
}



/**
 * Parse an optional bounded frame count from the command line.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return requested frame count, or 0 for the interactive loop
 */
static uint32_t _frame_count(int argc, char** argv)
{
    if (argc < 2 || argv == NULL || argv[1] == NULL)
        return 0;

    char* end = NULL;
    unsigned long value = strtoul(argv[1], &end, 10);
    if (end == argv[1] || (end != NULL && *end != '\0'))
        return 0;
    if (value > UINT32_MAX)
        return UINT32_MAX;
    return (uint32_t)value;
}



/**
 * Release heap-backed marker arrays owned by the stress state.
 *
 * @param state marker stress state
 */
static void _marker_stress_free(MarkerStressState* state)
{
    if (state == NULL)
        return;

    dvz_free(state->shapes);
    state->shapes = NULL;
    dvz_free(state->angles);
    state->angles = NULL;
    dvz_free(state->diameters);
    state->diameters = NULL;
    dvz_free(state->colors);
    state->colors = NULL;
    dvz_free(state->positions);
    state->positions = NULL;
}



/**
 * Allocate heap-backed marker arrays owned by the stress state.
 *
 * @param state marker stress state
 * @return 0 on success, -1 on allocation failure
 */
static int _marker_stress_alloc(MarkerStressState* state)
{
    state->positions = (float(*)[3])dvz_calloc(state->max_count, sizeof(*state->positions));
    state->colors = (DvzColor*)dvz_calloc(state->max_count, sizeof(DvzColor));
    state->diameters = (float*)dvz_calloc(state->max_count, sizeof(float));
    state->angles = (float*)dvz_calloc(state->max_count, sizeof(float));
    state->shapes = (uint32_t*)dvz_calloc(state->max_count, sizeof(uint32_t));

    if (state->positions == NULL || state->colors == NULL || state->diameters == NULL ||
        state->angles == NULL || state->shapes == NULL)
    {
        _marker_stress_free(state);
        return -1;
    }
    return 0;
}



/*************************************************************************************************/
/*  GUI and frame callbacks                                                                      */
/*************************************************************************************************/

/**
 * Build the live marker stress control window.
 *
 * @param gui GUI overlay
 * @param win app window
 * @param user_data marker stress state
 */
static void _marker_stress_gui(DvzGui* gui, DvzAppWindow* win, void* user_data)
{
    (void)win;
    MarkerStressState* state = (MarkerStressState*)user_data;
    bool upload_all = false;
    bool upload_color = false;
    bool upload_diameter = false;
    bool upload_angle = false;
    bool upload_shape = false;
    bool style_changed = false;

    static const char* const shape_items[] = {
        "disc",
        "square",
        "triangle",
        "diamond",
        "cross",
        "ring",
    };

    if (dvz_gui_begin(gui, "Marker stress", NULL, 0))
    {
        upload_all |= dvz_gui_slider_float_format(
            gui, "Active count", &state->active_count_value, 1.0f, (float)state->max_count, "%.0f");
        if (igButton("1k", (ImVec2){0.0f, 0.0f}))
        {
            state->active_count_value = 1024.0f;
            upload_all = true;
        }
        igSameLine(0.0f, 8.0f);
        if (igButton("8k", (ImVec2){0.0f, 0.0f}))
        {
            state->active_count_value = 8192.0f;
            upload_all = true;
        }
        igSameLine(0.0f, 8.0f);
        if (igButton("max", (ImVec2){0.0f, 0.0f}))
        {
            state->active_count_value = (float)state->max_count;
            upload_all = true;
        }

        upload_shape |=
            dvz_gui_combo(gui, "Shape", &state->shape_index, shape_items, 6);
        upload_shape |= dvz_gui_checkbox(gui, "Mixed shapes", &state->mixed_shapes);

        upload_diameter |=
            dvz_gui_slider_float(gui, "Diameter", &state->diameter, 2.0f, 54.0f);
        upload_angle |= dvz_gui_slider_float_format(
            gui, "Angle", &state->base_angle, -MARKER_STRESS_PI, MARKER_STRESS_PI, "%.2f rad");
        (void)dvz_gui_checkbox(gui, "Animate rotation", &state->animate);

        style_changed |= dvz_gui_checkbox(gui, "Fill", &state->filled);
        style_changed |= dvz_gui_checkbox(gui, "Stroke", &state->stroke);
        style_changed |= dvz_gui_checkbox(gui, "Outline", &state->outline);
        style_changed |=
            dvz_gui_slider_float(gui, "Stroke width", &state->stroke_width, 0.0f, 10.0f);

        upload_color |= dvz_gui_slider_float(gui, "Fill alpha", &state->fill_alpha, 0.02f, 1.0f);
        style_changed |= dvz_gui_slider_float(gui, "Edge red", &state->edge_color[0], 0.0f, 1.0f);
        style_changed |=
            dvz_gui_slider_float(gui, "Edge green", &state->edge_color[1], 0.0f, 1.0f);
        style_changed |=
            dvz_gui_slider_float(gui, "Edge blue", &state->edge_color[2], 0.0f, 1.0f);
        style_changed |=
            dvz_gui_slider_float(gui, "Edge alpha", &state->edge_color[3], 0.0f, 1.0f);
    }
    dvz_gui_end(gui);

    if (upload_all)
    {
        state->active_count = _clamp_count(state->active_count_value, state->max_count);
        state->active_count_value = (float)state->active_count;
        _upload_marker_attributes(state);
    }
    else
    {
        if (upload_color)
            _upload_colors(state);
        if (upload_diameter)
            _upload_diameters(state);
        if (upload_angle)
            _upload_angles(state);
        if (upload_shape)
            _upload_shapes(state);
    }

    if (style_changed)
        _apply_marker_style(state);
}



/**
 * Advance animated marker rotation after each completed app frame.
 *
 * @param win app window
 * @param user_data marker stress state
 */
static void _marker_stress_frame(DvzAppWindow* win, void* user_data)
{
    MarkerStressState* state = (MarkerStressState*)user_data;
    if (!state->animate)
        return;

    state->phase += 0.035f;
    if (state->phase > 2.0f * MARKER_STRESS_PI)
        state->phase -= 2.0f * MARKER_STRESS_PI;

    _upload_angles(state);
    dvz_app_window_request_frame(win);
}



/*************************************************************************************************/
/*  Main                                                                                         */
/*************************************************************************************************/

/**
 * Run the live GLFW marker stress example.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScene* scene = dvz_scene();
    if (scene == NULL)
    {
        dvz_fprintf(stderr, "dvz_scene() failed\n");
        return 1;
    }

    DvzFigure* figure = dvz_figure(scene, MARKER_STRESS_WIDTH, MARKER_STRESS_HEIGHT, 0);
    if (figure == NULL)
    {
        dvz_fprintf(stderr, "dvz_figure() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.03f, 0.04f, 0.94f, 0.90f});
    if (panel == NULL)
    {
        dvz_fprintf(stderr, "dvz_panel() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_panel_set_background_color(panel, 0.065f, 0.075f, 0.095f, 1.0f);

    DvzVisual* visual = dvz_marker(scene, 0);
    if (visual == NULL)
    {
        dvz_fprintf(stderr, "dvz_marker() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    MarkerStressState state = {
        .visual = visual,
        .max_count = MARKER_STRESS_MAX_COUNT,
        .active_count = 8192u,
        .active_count_value = 8192.0f,
        .diameter = 11.0f,
        .fill_alpha = 0.88f,
        .edge_color = {0.02f, 0.025f, 0.035f, 1.0f},
        .stroke_width = 1.5f,
        .animate = true,
        .mixed_shapes = true,
        .filled = true,
        .stroke = true,
        .outline = false,
    };

    if (_marker_stress_alloc(&state) != 0)
    {
        dvz_fprintf(stderr, "marker stress allocation failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    _make_marker_data(&state);
    _upload_marker_attributes(&state);
    _apply_marker_style(&state);
    if (dvz_panel_add_visual(panel, visual, NULL) != 0)
    {
        dvz_fprintf(stderr, "dvz_panel_add_visual() failed\n");
        _marker_stress_free(&state);
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        dvz_fprintf(stderr, "dvz_app() failed (no GPU or display?)\n");
        _marker_stress_free(&state);
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzAppWindow* win =
        dvz_app_window_glfw(app, figure, MARKER_STRESS_WIDTH, MARKER_STRESS_HEIGHT,
                            "visual_marker_stress_glfw");
    if (win == NULL)
    {
        dvz_fprintf(stderr, "dvz_app_window_glfw() failed (GLFW unavailable?)\n");
        dvz_app_destroy(app);
        _marker_stress_free(&state);
        dvz_scene_destroy(scene);
        return 1;
    }

    dvz_panel_set_panzoom(panel, dvz_app_window_input(win), 0);

    DvzGuiConfig gui_config = dvz_gui_config();
    DvzGui* gui = dvz_app_window_gui(win, &gui_config);
    if (gui == NULL)
    {
        dvz_fprintf(stderr, "dvz_app_window_gui() failed\n");
        dvz_app_destroy(app);
        _marker_stress_free(&state);
        dvz_scene_destroy(scene);
        return 1;
    }

    dvz_app_window_set_gui_callback(win, _marker_stress_gui, &state);
    dvz_app_window_set_frame_callback(win, _marker_stress_frame, &state);
    dvz_app_run(app, _frame_count(argc, argv));

    dvz_app_destroy(app);
    _marker_stress_free(&state);
    dvz_scene_destroy(scene);
    return 0;
}
