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
#include "example_common.h"



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
    MARKER_STRESS_STYLE_STROKE = 1,
    MARKER_STRESS_STYLE_OUTLINE = 2,
} MarkerStressStyleMode;



typedef struct MarkerStressState
{
    DvzAnimation* rotation_animation;
    DvzVisual* visual;

    vec3* positions;
    DvzColor* colors;
    float* diameters;
    float* angles;
    uint32_t* shapes;

    uint32_t max_count;
    uint32_t active_count;
    float active_count_value;
    float diameter_px;
    float base_angle;
    float phase;
    float rotation_speed_rad_per_sec;
    float fill_alpha;
    float edge_color[4];
    float stroke_width_px;
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
        state->colors[i].a = alpha;
}



/**
 * Apply the current marker diameter_px to the active marker prefix.
 *
 * @param state marker stress state
 */
static void _apply_diameters(MarkerStressState* state)
{
    for (uint32_t i = 0; i < state->active_count; i++)
        state->diameters[i] = state->diameter_px;
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

            state->colors[i].r = (uint8_t)(40.0f + 205.0f * u);
            state->colors[i].g = (uint8_t)(220.0f - 140.0f * v);
            state->colors[i].b = (uint8_t)(90.0f + 120.0f * (0.5f + 0.5f * wave));
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
 * Upload the current marker diameter_px to the active marker prefix.
 *
 * @param state marker stress state
 */
static void _upload_diameters(MarkerStressState* state)
{
    _apply_diameters(state);
    (void)dvz_visual_set_data(state->visual, "diameter_px", state->diameters, state->active_count);
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
        {.attr_name = "diameter_px", .data = state->diameters, .item_count = state->active_count},
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
    case MARKER_STRESS_STYLE_STROKE:
        style.aspect = DVZ_SHAPE_ASPECT_STROKE;
        break;
    case MARKER_STRESS_STYLE_OUTLINE:
        style.aspect = DVZ_SHAPE_ASPECT_OUTLINE;
        break;
    case MARKER_STRESS_STYLE_FILL:
    default:
        style.aspect = DVZ_SHAPE_ASPECT_FILLED;
        break;
    }
    style.edge_color = dvz_color_from_unit(
        state->edge_color[0], state->edge_color[1], state->edge_color[2], state->edge_color[3]);
    style.stroke_width_px = state->stroke_width_px;
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
        if (state->rotation_animation != NULL)
            dvz_anim_phase_set_value(state->rotation_animation, 0.0f);
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
    state->positions = (vec3*)dvz_calloc(state->max_count, sizeof(*state->positions));
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
 * @param win view
 * @param user_data marker stress state
 */
static void _marker_stress_gui(DvzGui* gui, DvzView* win, void* user_data)
{
    (void)win;
    MarkerStressState* state = (MarkerStressState*)user_data;
    bool upload_all = false;
    bool upload_color = false;
    bool upload_diameter = false;
    bool upload_angle = false;
    bool upload_shape = false;
    bool style_changed = false;
    bool speed_changed = false;
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
            dvz_gui_slider_float(gui, "Diameter", &state->diameter_px, 2.0f, 256.0f);
        upload_angle |= dvz_gui_slider_float_format(
            gui, "Base angle", &state->base_angle, -MARKER_STRESS_PI, MARKER_STRESS_PI,
            "%.2f rad");
        if (dvz_gui_checkbox(gui, "Animate rotation", &animate))
            _set_animation_enabled(state, animate);
        speed_changed |= dvz_gui_slider_float_format(
            gui, "Rotation speed", &state->rotation_speed_rad_per_sec, 0.0f, 8.0f,
            "%.2f rad/s");

        static const char* const style_labels[] = {
            "fill",
            "stroke",
            "outline",
        };
        style_changed |= dvz_gui_combo(gui, "Style", &state->style_mode, style_labels, 3);
        style_changed |=
            dvz_gui_slider_float(gui, "Stroke width", &state->stroke_width_px, 0.0f, 10.0f);

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

    if (speed_changed && state->rotation_animation != NULL)
        dvz_anim_set_speed(state->rotation_animation, state->rotation_speed_rad_per_sec);
}



/**
 * Apply the wrapped marker rotation phase from the scene clock.
 *
 * @param animation phase animation
 * @param value wrapped rotation phase in radians
 * @param delta unwrapped phase delta in radians
 * @param user_data marker stress state
 */
static void _marker_rotation_phase(
    DvzAnimation* animation, float value, float delta, void* user_data)
{
    (void)animation;
    (void)delta;
    MarkerStressState* state = (MarkerStressState*)user_data;
    if (state == NULL || !state->animate)
        return;

    state->phase = value;
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
    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;
    MarkerStressState state = {0};

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, MARKER_STRESS_WIDTH, MARKER_STRESS_HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel =
        dvz_panel(figure, &(DvzPanelDesc){
                              MARKER_STRESS_PANEL_X,
                              MARKER_STRESS_PANEL_Y,
                              MARKER_STRESS_PANEL_W,
                              MARKER_STRESS_PANEL_H,
                          });
    EXAMPLE_CHECK(panel != NULL, "dvz_panel() failed");
    dvz_panel_set_background_color(panel, dvz_color_from_unit(0.065f, 0.075f, 0.095f, 1.0f));

    DvzVisual* visual = dvz_marker(scene, 0);
    EXAMPLE_CHECK(visual != NULL, "dvz_marker() failed");

    state.visual = visual;
    state.max_count = MARKER_STRESS_MAX_COUNT;
    state.active_count = 8192u;
    state.active_count_value = 8192.0f;
    state.diameter_px = 22.0f;
    state.rotation_speed_rad_per_sec = MARKER_STRESS_ROTATION_SPEED_RAD_PER_SEC;
    state.fill_alpha = 0.88f;
    state.edge_color[0] = 0.02f;
    state.edge_color[1] = 0.025f;
    state.edge_color[2] = 0.035f;
    state.edge_color[3] = 1.0f;
    state.stroke_width_px = 2.0f;
    state.style_mode = MARKER_STRESS_STYLE_OUTLINE;
    state.animate = false;
    state.mixed_shapes = true;

    int rc = _marker_stress_alloc(&state);
    EXAMPLE_CHECK(rc == 0, "marker stress allocation failed");

    _upload_marker_attributes(&state);
    _apply_marker_style(&state);
    state.rotation_animation = dvz_anim_phase(
        scene, &(DvzAnimPhaseDesc){
                   DVZ_STRUCT_INIT_FIELDS(DvzAnimPhaseDesc),
                   .initial = state.phase,
                   .speed = state.rotation_speed_rad_per_sec,
                   .wrap_min = -MARKER_STRESS_PI,
                   .wrap_max = +MARKER_STRESS_PI,
                   .callback = _marker_rotation_phase,
                   .user_data = &state,
               });
    EXAMPLE_CHECK(state.rotation_animation != NULL, "dvz_anim_phase() failed");
    rc = dvz_panel_add_visual(panel, visual, NULL);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual() failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win =
        dvz_view_glfw(app, figure, MARKER_STRESS_WIDTH, MARKER_STRESS_HEIGHT,
                            "marker");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    DvzPanzoom* panzoom = dvz_view_panzoom(win, panel, NULL);
    EXAMPLE_CHECK(panzoom != NULL, "failed to create or bind panzoom controller");

    DvzGui* gui = dvz_view_gui(win, NULL);
    EXAMPLE_CHECK(gui != NULL, "dvz_view_gui() failed");

    dvz_view_set_gui_callback(win, _marker_stress_gui, &state);
    dvz_scene_set_clock_mode(scene, DVZ_SCENE_CLOCK_REALTIME);
    dvz_scene_set_fps(scene, 60.0);
    dvz_app_run(app, example_frame_count(argc, argv));
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    _marker_stress_free(&state);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
