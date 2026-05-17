/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* marker - live GLFW marker visual parameter stress tool.
 *
 * Build:  register externally or locally as a C example target, then build
 * Run:    ./build/examples/c/visuals/marker [frames]
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
#define MARKER_STRESS_PANEL_X   0.03f
#define MARKER_STRESS_PANEL_Y   0.04f
#define MARKER_STRESS_PANEL_W   0.94f
#define MARKER_STRESS_PANEL_H   0.90f
#define MARKER_STRESS_X0        -0.96f
#define MARKER_STRESS_X1        +0.96f
#define MARKER_STRESS_Y0        -0.86f
#define MARKER_STRESS_Y1        +0.86f
#define MARKER_STRESS_ROTATION_SPEED_RAD_PER_SEC 1.35f



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef enum MarkerStressStyleMode
{
    MARKER_STRESS_STYLE_FILL = 0,
    MARKER_STRESS_STYLE_OUTLINE = 1,
    MARKER_STRESS_STYLE_BOTH = 2,
} MarkerStressStyleMode;



typedef struct MarkerStressState
{
    DvzAnimation* rotation_animation;
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
    float rotation_speed_rad_per_sec;
    float fill_alpha;
    float edge_color[4];
    float stroke_width;
    int style_mode;
    bool animate;
    bool mixed_shapes;
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
 * Wrap an angle to the slider-friendly [-pi, pi] interval.
 *
 * @param angle input angle in radians
 * @return wrapped angle in radians
 */
static float _wrap_angle(float angle)
{
    while (angle > MARKER_STRESS_PI)
        angle -= 2.0f * MARKER_STRESS_PI;
    while (angle < -MARKER_STRESS_PI)
        angle += 2.0f * MARKER_STRESS_PI;
    return angle;
}



/**
 * Apply the current fill alpha to the active marker color prefix.
 *
 * @param state marker stress state
 */
static void _apply_fill_alpha(MarkerStressState* state)
{
    uint8_t alpha = _u8_from_float(state->fill_alpha);
    for (uint32_t i = 0; i < state->active_count; i++)
        state->colors[i][3] = alpha;
}



/**
 * Apply the current marker diameter to the active marker prefix.
 *
 * @param state marker stress state
 */
static void _apply_diameters(MarkerStressState* state)
{
    for (uint32_t i = 0; i < state->active_count; i++)
        state->diameters[i] = state->diameter;
}



/**
 * Apply the current marker angle field to the active marker prefix.
 *
 * @param state marker stress state
 */
static void _apply_angles(MarkerStressState* state)
{
    float phase = state->base_angle + state->phase;
    for (uint32_t i = 0; i < state->active_count; i++)
    {
        float offset = state->mixed_shapes ? 0.19f * (float)(i % 17u) : 0.0f;
        state->angles[i] = phase + offset;
    }
}



/**
 * Apply the current marker shape field to the active marker prefix.
 *
 * @param state marker stress state
 */
static void _apply_shapes(MarkerStressState* state)
{
    DvzMarkerShape shape = _shape_from_index(state->shape_index);
    for (uint32_t i = 0; i < state->active_count; i++)
        state->shapes[i] = state->mixed_shapes ? (uint32_t)(i % 6u) : (uint32_t)shape;
}



/**
 * Fill active marker positions and base colors with deterministic synthetic data.
 *
 * @param state marker stress state
 */
static void _make_active_marker_data(MarkerStressState* state)
{
    if (state == NULL || state->active_count == 0 || state->active_count > state->max_count)
        return;

    const float panel_aspect =
        ((float)MARKER_STRESS_WIDTH * MARKER_STRESS_PANEL_W) /
        ((float)MARKER_STRESS_HEIGHT * MARKER_STRESS_PANEL_H);
    const float data_aspect =
        (MARKER_STRESS_X1 - MARKER_STRESS_X0) / (MARKER_STRESS_Y1 - MARKER_STRESS_Y0);
    const float target_aspect = panel_aspect / data_aspect;

    uint32_t columns = (uint32_t)ceilf(sqrtf((float)state->active_count * target_aspect));
    columns = columns == 0 ? 1 : columns;
    uint32_t rows = (state->active_count + columns - 1u) / columns;
    rows = rows == 0 ? 1 : rows;

    const uint32_t base_row_count = state->active_count / rows;
    const uint32_t extra_rows = state->active_count % rows;

    uint32_t i = 0;
    for (uint32_t row = 0; row < rows; row++)
    {
        const uint32_t row_count = base_row_count + (row < extra_rows ? 1u : 0u);
        const float v = rows > 1 ? (float)row / (float)(rows - 1u) : 0.5f;

        for (uint32_t col = 0; col < row_count; col++)
        {
            const float u = row_count > 1 ? (float)col / (float)(row_count - 1u) : 0.5f;
            const float wave = sinf(18.0f * u + 7.0f * v);

            state->positions[i][0] = MARKER_STRESS_X0 + (MARKER_STRESS_X1 - MARKER_STRESS_X0) * u;
            state->positions[i][1] = MARKER_STRESS_Y0 + (MARKER_STRESS_Y1 - MARKER_STRESS_Y0) * v;
            state->positions[i][2] = 0.035f * wave;

            state->colors[i][0] = (uint8_t)(40.0f + 205.0f * u);
            state->colors[i][1] = (uint8_t)(220.0f - 140.0f * v);
            state->colors[i][2] = (uint8_t)(90.0f + 120.0f * (0.5f + 0.5f * wave));
            i++;
        }
    }
    _apply_fill_alpha(state);
    _apply_diameters(state);
    _apply_angles(state);
    _apply_shapes(state);
}



/**
 * Upload the current fill alpha to the active marker color prefix.
 *
 * @param state marker stress state
 */
static void _upload_colors(MarkerStressState* state)
{
    _apply_fill_alpha(state);
    (void)dvz_visual_set_data(state->visual, "color", state->colors, state->active_count);
}



/**
 * Upload the current marker diameter to the active marker prefix.
 *
 * @param state marker stress state
 */
static void _upload_diameters(MarkerStressState* state)
{
    _apply_diameters(state);
    (void)dvz_visual_set_data(state->visual, "diameter", state->diameters, state->active_count);
}



/**
 * Upload the current marker angle field to the active marker prefix.
 *
 * @param state marker stress state
 */
static void _upload_angles(MarkerStressState* state)
{
    _apply_angles(state);
    (void)dvz_visual_set_data(state->visual, "angle", state->angles, state->active_count);
}



/**
 * Upload the current marker shape field to the active marker prefix.
 *
 * @param state marker stress state
 */
static void _upload_shapes(MarkerStressState* state)
{
    _apply_shapes(state);
    (void)dvz_visual_set_data(state->visual, "shape", state->shapes, state->active_count);
}



/**
 * Upload all dense marker attributes for the active marker prefix.
 *
 * @param state marker stress state
 */
static void _upload_marker_attributes(MarkerStressState* state)
{
    _make_active_marker_data(state);
    const DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = state->positions, .item_count = state->active_count},
        {.attr_name = "color", .data = state->colors, .item_count = state->active_count},
        {.attr_name = "diameter", .data = state->diameters, .item_count = state->active_count},
        {.attr_name = "angle", .data = state->angles, .item_count = state->active_count},
        {.attr_name = "shape", .data = state->shapes, .item_count = state->active_count},
    };
    if (dvz_visual_set_data_many(state->visual, updates, 5) != 0)
        dvz_fprintf(stderr, "marker visual data upload failed\n");
}



/**
 * Apply the current fill, stroke, outline, and edge color style to the marker visual.
 *
 * @param state marker stress state
 */
static void _apply_marker_style(MarkerStressState* state)
{
    DvzMarkerStyle style = dvz_marker_style();
    switch ((MarkerStressStyleMode)state->style_mode)
    {
    case MARKER_STRESS_STYLE_OUTLINE:
        style.filled = false;
        style.stroke = true;
        style.outline = true;
        break;
    case MARKER_STRESS_STYLE_BOTH:
        style.filled = true;
        style.stroke = true;
        style.outline = false;
        break;
    case MARKER_STRESS_STYLE_FILL:
    default:
        style.filled = true;
        style.stroke = false;
        style.outline = false;
        break;
    }
    style.edge_color[0] = _u8_from_float(state->edge_color[0]);
    style.edge_color[1] = _u8_from_float(state->edge_color[1]);
    style.edge_color[2] = _u8_from_float(state->edge_color[2]);
    style.edge_color[3] = _u8_from_float(state->edge_color[3]);
    style.stroke_width = state->stroke_width;
    (void)dvz_marker_set_style(state->visual, &style);
}



/**
 * Apply the retained animation toggle to the scene timer.
 *
 * @param state marker stress state
 * @param animate whether marker rotation should animate
 */
static void _set_animation_enabled(MarkerStressState* state, bool animate)
{
    if (state == NULL || state->animate == animate)
        return;

    if (!animate)
    {
        state->base_angle = _wrap_angle(state->base_angle + state->phase);
        state->phase = 0.0f;
        _upload_angles(state);
    }

    state->animate = animate;
    if (state->rotation_animation == NULL)
        return;
    if (state->animate)
        dvz_anim_start(state->rotation_animation, 0.0);
    else
        dvz_anim_stop(state->rotation_animation);
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
    bool animate = state->animate;

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
            dvz_gui_slider_float(gui, "Diameter", &state->diameter, 2.0f, 256.0f);
        upload_angle |= dvz_gui_slider_float_format(
            gui, "Base angle", &state->base_angle, -MARKER_STRESS_PI, MARKER_STRESS_PI,
            "%.2f rad");
        if (dvz_gui_checkbox(gui, "Animate rotation", &animate))
            _set_animation_enabled(state, animate);
        (void)dvz_gui_slider_float_format(
            gui, "Rotation speed", &state->rotation_speed_rad_per_sec, 0.0f, 8.0f,
            "%.2f rad/s");

        static const char* const style_labels[] = {
            "fill",
            "outline",
            "both",
        };
        style_changed |= dvz_gui_combo(gui, "Style", &state->style_mode, style_labels, 3);
        style_changed |=
            dvz_gui_slider_float(gui, "Stroke width", &state->stroke_width, 0.0f, 10.0f);

        upload_color |= dvz_gui_slider_float(gui, "Fill alpha", &state->fill_alpha, 0.02f, 1.0f);
        style_changed |= dvz_gui_color_edit4(gui, "Edge color", state->edge_color, 0);
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
 * Advance animated marker rotation from the scene clock.
 *
 * @param animation timer animation
 * @param t scene-clock time in seconds
 * @param dt scene-clock delta in seconds
 * @param user_data marker stress state
 */
static void _marker_rotation_timer(
    DvzAnimation* animation, double t, double dt, void* user_data)
{
    (void)animation;
    (void)t;
    MarkerStressState* state = (MarkerStressState*)user_data;
    if (state == NULL || !state->animate || dt <= 0.0)
        return;

    state->phase += state->rotation_speed_rad_per_sec * (float)dt;
    state->phase = _wrap_angle(state->phase);

    _upload_angles(state);
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

    DvzPanel* panel =
        dvz_panel(figure, (DvzPanelDesc){
                              MARKER_STRESS_PANEL_X,
                              MARKER_STRESS_PANEL_Y,
                              MARKER_STRESS_PANEL_W,
                              MARKER_STRESS_PANEL_H,
                          });
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
        .diameter = 22.0f,
        .rotation_speed_rad_per_sec = MARKER_STRESS_ROTATION_SPEED_RAD_PER_SEC,
        .fill_alpha = 0.88f,
        .edge_color = {0.02f, 0.025f, 0.035f, 1.0f},
        .stroke_width = 2.0f,
        .style_mode = MARKER_STRESS_STYLE_BOTH,
        .animate = false,
        .mixed_shapes = true,
    };

    if (_marker_stress_alloc(&state) != 0)
    {
        dvz_fprintf(stderr, "marker stress allocation failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    _upload_marker_attributes(&state);
    _apply_marker_style(&state);
    state.rotation_animation = dvz_anim_timer(scene, 0.0, _marker_rotation_timer, &state);
    if (state.rotation_animation == NULL)
    {
        dvz_fprintf(stderr, "dvz_anim_timer() failed\n");
        _marker_stress_free(&state);
        dvz_scene_destroy(scene);
        return 1;
    }
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
                            "marker");
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
    dvz_scene_set_clock_mode(scene, DVZ_CLOCK_REALTIME);
    dvz_scene_set_fps(scene, 60.0);
    dvz_app_run(app, _frame_count(argc, argv));

    dvz_app_destroy(app);
    _marker_stress_free(&state);
    dvz_scene_destroy(scene);
    return 0;
}
