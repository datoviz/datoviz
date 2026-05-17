/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* visual_point_stress_glfw - live point-visual parameter stress example.
 *
 * Opens a GLFW window with a deterministic high-count point cloud. A GUI overlay exercises point
 * count rebinding, diameter and alpha uploads, circular fill/edge/both styling, edge-color
 * material updates, depth testing, MSAA alpha-to-coverage, depth cueing, arcball animation, and
 * optional per-frame full reuploads.
 *
 * Build:  just example-c visual_point_stress_glfw
 * Run:    ./build/examples/c/visual_point_stress_glfw
 * Smoke:  ./build/examples/c/visual_point_stress_glfw 300
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
#include "_assertions.h"
#include "_compat.h"
#include "datoviz/app.h"
#include "datoviz/gui.h"
#include "datoviz/scene.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  1100u
#define HEIGHT 760u

#define MAX_POINTS     262144u
#define PRESET_COUNT   4u
#define DEFAULT_PRESET 2

#define ROTATION_SPEED_RAD_PER_SEC 0.32f
#define CUE_DISTANCE_MIN           0.0f
#define CUE_DISTANCE_MAX           8.0f
#define CUE_DISTANCE_EPS           1e-4f

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct PointStressState PointStressState;

typedef enum PointStressStyleMode
{
    POINT_STRESS_STYLE_FILL = 0,
    POINT_STRESS_STYLE_EDGE = 1,
    POINT_STRESS_STYLE_BOTH = 2,
} PointStressStyleMode;



struct PointStressState
{
    DvzPanel* panel;
    DvzVisual* visual;
    DvzAnimation* spin;
    float (*base_positions)[3];
    float (*positions)[3];
    DvzColor* colors;
    float* diameters;
    uint32_t max_count;
    uint32_t active_count;
    uint32_t frame_index;
    int preset_index;
    float diameter;
    float alpha;
    float stroke_width;
    float edge_rgb[3];
    int style_mode;
    int alpha_mode;
    bool depth_test_enabled;
    bool msaa_enabled;
    bool msaa_alpha_to_coverage;
    float msaa_sample_count;
    bool depth_cue_enabled;
    float depth_cue_near;
    float depth_cue_far;
    float depth_cue_strength;
    bool spin_enabled;
    bool mutate_each_frame;
    bool full_reupload_each_frame;
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

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
 * Return a deterministic active-count preset.
 *
 * @param preset preset index
 * @return point count for the preset
 */
static uint32_t _preset_count(int preset)
{
    static const uint32_t counts[PRESET_COUNT] = {16384u, 65536u, 131072u, MAX_POINTS};

    if (preset < 0)
        return counts[0];
    if ((uint32_t)preset >= PRESET_COUNT)
        return counts[PRESET_COUNT - 1];
    return counts[preset];
}



/**
 * Convert a normalized float channel to an 8-bit color channel.
 *
 * @param value normalized channel value
 * @return clamped 8-bit channel value
 */
static uint8_t _u8_from_unit(float value)
{
    if (value < 0.0f)
        value = 0.0f;
    if (value > 1.0f)
        value = 1.0f;
    return (uint8_t)(255.0f * value + 0.5f);
}



/**
 * Fill deterministic point positions, colors, and diameters.
 *
 * @param state point stress state
 */
static void _build_points(PointStressState* state)
{
    ANN(state);
    ANN(state->base_positions);
    ANN(state->positions);
    ANN(state->colors);
    ANN(state->diameters);

    const float n = (float)state->max_count;
    for (uint32_t i = 0; i < state->max_count; i++)
    {
        const float t = (float)i / fmaxf(1.0f, n - 1.0f);
        const float layer = floorf(20.0f * t);
        const float local = 20.0f * t - layer;
        const float theta = TAU * (0.61803398875f * (float)i + 0.037f * layer);
        const float radius =
            0.14f + 1.18f * sqrtf(local) + 0.07f * sinf(13.0f * local + 0.31f * layer);
        const float z = -1.05f + 2.10f * ((layer + local) / 20.0f);
        const float twist = theta + 1.15f * z;

        state->base_positions[i][0] = radius * cosf(twist);
        state->base_positions[i][1] = radius * sinf(twist);
        state->base_positions[i][2] = z + 0.10f * sinf(17.0f * t);
        state->positions[i][0] = state->base_positions[i][0];
        state->positions[i][1] = state->base_positions[i][1];
        state->positions[i][2] = state->base_positions[i][2];

        const float warm = 0.5f + 0.5f * sinf(7.0f * t + 0.2f * layer);
        const float cool = 0.5f + 0.5f * cosf(11.0f * t - 0.3f * layer);
        state->colors[i][0] = (uint8_t)(45.0f + 185.0f * warm);
        state->colors[i][1] = (uint8_t)(70.0f + 150.0f * (1.0f - local));
        state->colors[i][2] = (uint8_t)(80.0f + 165.0f * cool);
        state->colors[i][3] = 230;
        state->diameters[i] = 4.5f + 2.5f * local;
    }
}



/**
 * Apply the active alpha value to the retained color array.
 *
 * @param state point stress state
 */
static void _apply_alpha_to_colors(PointStressState* state)
{
    ANN(state);
    ANN(state->colors);

    const uint8_t alpha = _u8_from_unit(state->alpha);
    for (uint32_t i = 0; i < state->max_count; i++)
        state->colors[i][3] = alpha;
}



/**
 * Apply the active diameter value to the retained diameter array.
 *
 * @param state point stress state
 */
static void _apply_diameter_to_points(PointStressState* state)
{
    ANN(state);
    ANN(state->diameters);

    for (uint32_t i = 0; i < state->max_count; i++)
    {
        const float ripple = 0.84f + 0.16f * sinf(0.011f * (float)i);
        state->diameters[i] = state->diameter * ripple;
    }
}



/**
 * Upload the active point array prefix to the visual.
 *
 * @param state point stress state
 * @return true when all uploads succeeded
 */
static bool _upload_points(PointStressState* state)
{
    ANN(state);
    ANN(state->visual);

    if (state->active_count == 0 || state->active_count > state->max_count)
        return false;

    if (dvz_visual_set_data(state->visual, "position", state->positions, state->active_count) !=
            0 ||
        dvz_visual_set_data(state->visual, "color", state->colors, state->active_count) != 0 ||
        dvz_visual_set_data(state->visual, "diameter", state->diameters, state->active_count) != 0)
    {
        dvz_fprintf(stderr, "point visual data upload failed\n");
        return false;
    }
    return true;
}



/**
 * Apply point fill, edge, or fill-and-edge styling.
 *
 * @param state point stress state
 */
static void _apply_style(PointStressState* state)
{
    ANN(state);
    ANN(state->visual);

    DvzPointStyleDesc style = dvz_point_style_desc();
    switch ((PointStressStyleMode)state->style_mode)
    {
    case POINT_STRESS_STYLE_EDGE:
        style.filled = false;
        style.stroke = true;
        style.outline = true;
        break;
    case POINT_STRESS_STYLE_BOTH:
        style.filled = true;
        style.stroke = true;
        style.outline = false;
        break;
    case POINT_STRESS_STYLE_FILL:
    default:
        style.filled = true;
        style.stroke = false;
        style.outline = false;
        break;
    }
    style.stroke_width = state->stroke_width;
    style.edge_color[0] = _u8_from_unit(state->edge_rgb[0]);
    style.edge_color[1] = _u8_from_unit(state->edge_rgb[1]);
    style.edge_color[2] = _u8_from_unit(state->edge_rgb[2]);
    style.edge_color[3] = 255;

    if (dvz_point_set_style(state->visual, &style) != 0)
        dvz_fprintf(stderr, "dvz_point_set_style() failed\n");
}



/**
 * Apply the depth, alpha, and MSAA controls that affect point occlusion and edge coverage.
 *
 * @param state point stress state
 */
static void _apply_depth_and_msaa(PointStressState* state)
{
    ANN(state);
    ANN(state->panel);
    ANN(state->visual);

    if (dvz_visual_set_depth_test(state->visual, state->depth_test_enabled) != 0)
        dvz_fprintf(stderr, "dvz_visual_set_depth_test() failed\n");

    DvzAlphaMode alpha_mode =
        state->alpha_mode == 1 ? DVZ_ALPHA_BLENDED : DVZ_ALPHA_OPAQUE;
    if (dvz_visual_set_alpha_mode(state->visual, alpha_mode) != 0)
        dvz_fprintf(stderr, "dvz_visual_set_alpha_mode() failed\n");

    if (!state->msaa_enabled)
    {
        if (!dvz_panel_set_msaa(state->panel, NULL))
            dvz_fprintf(stderr, "dvz_panel_set_msaa(NULL) failed\n");
        return;
    }

    uint32_t sample_count = (uint32_t)(state->msaa_sample_count + 0.5f);
    if (sample_count < 2)
        sample_count = 2;
    if (sample_count > 8)
        sample_count = 8;
    state->msaa_sample_count = (float)sample_count;

    DvzMsaaDesc msaa = {
        .enabled = true,
        .sample_count = sample_count,
        .alpha_to_coverage = state->msaa_alpha_to_coverage,
    };
    if (!dvz_panel_set_msaa(state->panel, &msaa))
        dvz_fprintf(stderr, "dvz_panel_set_msaa() failed\n");
}



/**
 * Apply the retained depth-cue descriptor to the point visual.
 *
 * @param state point stress state
 */
static void _apply_depth_cue(PointStressState* state)
{
    ANN(state);
    ANN(state->visual);

    if (!state->depth_cue_enabled)
    {
        if (dvz_visual_set_depth_cue(state->visual, NULL) != 0)
            dvz_fprintf(stderr, "dvz_visual_set_depth_cue(NULL) failed\n");
        return;
    }

    if (state->depth_cue_near < CUE_DISTANCE_MIN)
        state->depth_cue_near = CUE_DISTANCE_MIN;
    if (state->depth_cue_near > CUE_DISTANCE_MAX - CUE_DISTANCE_EPS)
        state->depth_cue_near = CUE_DISTANCE_MAX - CUE_DISTANCE_EPS;
    if (state->depth_cue_far > CUE_DISTANCE_MAX)
        state->depth_cue_far = CUE_DISTANCE_MAX;
    if (state->depth_cue_far <= state->depth_cue_near + CUE_DISTANCE_EPS)
        state->depth_cue_far = state->depth_cue_near + CUE_DISTANCE_EPS;

    DvzDepthCueDesc desc = {
        .mode = DVZ_DEPTH_CUE_FADE_TO_BACKGROUND,
        .metric = DVZ_DEPTH_CUE_METRIC_EYE_DISTANCE,
        .falloff = DVZ_DEPTH_CUE_FALLOFF_LINEAR,
        .near_depth = state->depth_cue_near,
        .far_depth = state->depth_cue_far,
        .strength = state->depth_cue_strength,
        .density = 1.0f,
        .background_color = {0.035f, 0.040f, 0.050f, 1.0f},
    };
    if (dvz_visual_set_depth_cue(state->visual, &desc) != 0)
        dvz_fprintf(stderr, "dvz_visual_set_depth_cue() failed\n");
}



/**
 * Apply the retained spin toggle to the scene animation.
 *
 * @param state point stress state
 */
static void _apply_spin(PointStressState* state)
{
    ANN(state);

    if (state->spin == NULL)
        return;
    if (state->spin_enabled)
        dvz_anim_start(state->spin, 0.0);
    else
        dvz_anim_stop(state->spin);
}



/**
 * Reset the live controls to a useful stress baseline.
 *
 * @param state point stress state
 */
static void _reset_controls(PointStressState* state)
{
    ANN(state);
    ANN(state->base_positions);
    ANN(state->positions);

    const size_t position_bytes = (size_t)state->max_count * sizeof(*state->positions);
    (void)dvz_memcpy(state->positions, position_bytes, state->base_positions, position_bytes);

    state->preset_index = DEFAULT_PRESET;
    state->active_count = _preset_count(state->preset_index);
    state->diameter = 5.0f;
    state->alpha = 0.78f;
    state->stroke_width = 1.25f;
    state->edge_rgb[0] = 0.03f;
    state->edge_rgb[1] = 0.04f;
    state->edge_rgb[2] = 0.05f;
    state->style_mode = POINT_STRESS_STYLE_BOTH;
    state->alpha_mode = 0;
    state->depth_test_enabled = true;
    state->msaa_enabled = true;
    state->msaa_alpha_to_coverage = true;
    state->msaa_sample_count = 4.0f;
    state->depth_cue_enabled = true;
    state->depth_cue_near = 2.0f;
    state->depth_cue_far = 4.8f;
    state->depth_cue_strength = 0.40f;
    state->spin_enabled = true;
    state->mutate_each_frame = false;
    state->full_reupload_each_frame = false;

    _apply_alpha_to_colors(state);
    _apply_diameter_to_points(state);
    _apply_style(state);
    _apply_depth_and_msaa(state);
    _apply_depth_cue(state);
    _apply_spin(state);
    (void)_upload_points(state);
}



/**
 * Mutate the active point positions and diameters for the next frame.
 *
 * @param state point stress state
 */
static void _mutate_points(PointStressState* state)
{
    ANN(state);
    ANN(state->base_positions);
    ANN(state->positions);
    ANN(state->diameters);

    const float time = 0.035f * (float)state->frame_index;
    for (uint32_t i = 0; i < state->active_count; i++)
    {
        const float phase = time + 0.00091f * (float)i;
        const float wobble = 0.030f * sinf(phase);
        const float c = cosf(0.12f * time);
        const float s = sinf(0.12f * time);
        const float x = state->base_positions[i][0];
        const float y = state->base_positions[i][1];

        state->positions[i][0] = c * x - s * y + wobble;
        state->positions[i][1] = s * x + c * y + 0.025f * cosf(1.7f * phase);
        state->positions[i][2] = state->base_positions[i][2] + 0.045f * sinf(0.7f * phase);
        state->diameters[i] = state->diameter * (0.78f + 0.22f * sinf(phase + 0.4f));
    }
}



/**
 * Build the live point stress controls.
 *
 * @param gui GUI overlay
 * @param win app window
 * @param user_data point stress state
 */
static void _point_stress_gui(DvzGui* gui, DvzAppWindow* win, void* user_data)
{
    (void)win;
    PointStressState* state = (PointStressState*)user_data;
    if (state == NULL)
        return;

    bool count_changed = false;
    bool data_changed = false;
    bool style_changed = false;
    bool depth_changed = false;
    bool cue_changed = false;
    bool spin_changed = false;
    if (dvz_gui_begin(gui, "Point stress", NULL, 0))
    {
        static const char* const count_labels[PRESET_COUNT] = {
            "16k points",
            "65k points",
            "131k points",
            "262k points",
        };
        count_changed |= dvz_gui_combo(
            gui, "Active count", &state->preset_index, count_labels, (int)PRESET_COUNT);

        data_changed |= dvz_gui_slider_float(gui, "Diameter", &state->diameter, 1.0f, 32.0f);
        data_changed |= dvz_gui_slider_float(gui, "Alpha", &state->alpha, 0.02f, 1.0f);
        static const char* const style_labels[] = {
            "fill",
            "edge",
            "both",
        };
        style_changed |= dvz_gui_combo(gui, "Style", &state->style_mode, style_labels, 3);
        style_changed |=
            dvz_gui_slider_float(gui, "Stroke width", &state->stroke_width, 0.0f, 8.0f);
        style_changed |= dvz_gui_slider_float(gui, "Edge red", &state->edge_rgb[0], 0.0f, 1.0f);
        style_changed |=
            dvz_gui_slider_float(gui, "Edge green", &state->edge_rgb[1], 0.0f, 1.0f);
        style_changed |= dvz_gui_slider_float(gui, "Edge blue", &state->edge_rgb[2], 0.0f, 1.0f);
        static const char* const alpha_labels[] = {
            "opaque",
            "blended",
        };
        depth_changed |= dvz_gui_combo(gui, "Alpha mode", &state->alpha_mode, alpha_labels, 2);
        depth_changed |= dvz_gui_checkbox(gui, "Depth test", &state->depth_test_enabled);
        depth_changed |= dvz_gui_checkbox(gui, "MSAA", &state->msaa_enabled);
        depth_changed |=
            dvz_gui_slider_float(gui, "MSAA samples", &state->msaa_sample_count, 2.0f, 8.0f);
        depth_changed |=
            dvz_gui_checkbox(gui, "Alpha-to-coverage", &state->msaa_alpha_to_coverage);
        cue_changed |= dvz_gui_checkbox(gui, "Depth cue", &state->depth_cue_enabled);
        cue_changed |= dvz_gui_slider_float(
            gui, "Cue near", &state->depth_cue_near, CUE_DISTANCE_MIN, CUE_DISTANCE_MAX);
        cue_changed |= dvz_gui_slider_float(
            gui, "Cue far", &state->depth_cue_far, CUE_DISTANCE_MIN, CUE_DISTANCE_MAX);
        cue_changed |=
            dvz_gui_slider_float(gui, "Cue strength", &state->depth_cue_strength, 0.0f, 1.0f);
        spin_changed |= dvz_gui_checkbox(gui, "Arcball spin", &state->spin_enabled);
        dvz_gui_checkbox(gui, "Mutate every frame", &state->mutate_each_frame);
        dvz_gui_checkbox(gui, "Full reupload every frame", &state->full_reupload_each_frame);
        if (dvz_gui_button(gui, "Reset"))
        {
            _reset_controls(state);
            count_changed = false;
            data_changed = false;
            style_changed = false;
            depth_changed = false;
            cue_changed = false;
            spin_changed = false;
        }
    }
    dvz_gui_end(gui);

    if (count_changed)
    {
        state->active_count = _preset_count(state->preset_index);
        data_changed = true;
    }
    if (data_changed)
    {
        _apply_alpha_to_colors(state);
        _apply_diameter_to_points(state);
        (void)_upload_points(state);
    }
    if (style_changed)
        _apply_style(state);
    if (depth_changed)
        _apply_depth_and_msaa(state);
    if (cue_changed)
        _apply_depth_cue(state);
    if (spin_changed)
        _apply_spin(state);
}



/**
 * Mutate or reupload point data from the frame callback.
 *
 * @param win app window
 * @param user_data point stress state
 */
static void _point_stress_frame(DvzAppWindow* win, void* user_data)
{
    PointStressState* state = (PointStressState*)user_data;
    if (state == NULL)
        return;

    state->frame_index++;
    if (state->mutate_each_frame)
        _mutate_points(state);
    if (state->mutate_each_frame || state->full_reupload_each_frame)
    {
        (void)_upload_points(state);
        dvz_app_window_request_frame(win);
    }
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    if (MAX_POINTS > SIZE_MAX / sizeof(float[3]) || MAX_POINTS > SIZE_MAX / sizeof(DvzColor) ||
        MAX_POINTS > SIZE_MAX / sizeof(float))
    {
        dvz_fprintf(stderr, "point allocation size overflow\n");
        return 1;
    }

    DvzScene* scene = dvz_scene();
    if (scene == NULL)
    {
        dvz_fprintf(stderr, "dvz_scene() failed\n");
        return 1;
    }

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    if (figure == NULL)
    {
        dvz_fprintf(stderr, "dvz_figure() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzPanel* panel =
        dvz_panel(figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    if (panel == NULL)
    {
        dvz_fprintf(stderr, "dvz_panel() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.eye[2] = 4.2f;
    camera_desc.near = 0.1f;
    camera_desc.far = 100.0f;
    if (!dvz_panel_set_camera(panel, &camera_desc))
    {
        dvz_fprintf(stderr, "dvz_panel_set_camera() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_panel_set_background_color(panel, 0.035f, 0.040f, 0.050f, 1.0f);

    DvzVisual* visual = dvz_point(scene, 0);
    if (visual == NULL)
    {
        dvz_fprintf(stderr, "dvz_point() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    PointStressState state = {
        .panel = panel,
        .visual = visual,
        .max_count = MAX_POINTS,
        .preset_index = DEFAULT_PRESET,
        .active_count = _preset_count(DEFAULT_PRESET),
    };
    state.base_positions = (float(*)[3])dvz_calloc(MAX_POINTS, sizeof(*state.base_positions));
    state.positions = (float(*)[3])dvz_calloc(MAX_POINTS, sizeof(*state.positions));
    state.colors = (DvzColor*)dvz_calloc(MAX_POINTS, sizeof(DvzColor));
    state.diameters = (float*)dvz_calloc(MAX_POINTS, sizeof(float));
    if (state.base_positions == NULL || state.positions == NULL || state.colors == NULL ||
        state.diameters == NULL)
    {
        dvz_fprintf(stderr, "point allocation failed\n");
        dvz_free(state.diameters);
        dvz_free(state.colors);
        dvz_free(state.positions);
        dvz_free(state.base_positions);
        dvz_scene_destroy(scene);
        return 1;
    }

    _build_points(&state);
    state.diameter = 5.0f;
    state.alpha = 0.78f;
    _apply_alpha_to_colors(&state);
    _apply_diameter_to_points(&state);
    if (!_upload_points(&state) || dvz_panel_add_visual(panel, visual, NULL) != 0)
    {
        dvz_fprintf(stderr, "point visual setup failed\n");
        dvz_free(state.diameters);
        dvz_free(state.colors);
        dvz_free(state.positions);
        dvz_free(state.base_positions);
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        dvz_fprintf(stderr, "dvz_app() failed (no GPU or display?)\n");
        dvz_free(state.diameters);
        dvz_free(state.colors);
        dvz_free(state.positions);
        dvz_free(state.base_positions);
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzAppWindow* win =
        dvz_app_window_glfw(app, figure, WIDTH, HEIGHT, "visual_point_stress_glfw");
    if (win == NULL)
    {
        dvz_fprintf(stderr, "dvz_app_window_glfw() failed (GLFW unavailable?)\n");
        dvz_app_destroy(app);
        dvz_free(state.diameters);
        dvz_free(state.colors);
        dvz_free(state.positions);
        dvz_free(state.base_positions);
        dvz_scene_destroy(scene);
        return 1;
    }

    dvz_panel_set_arcball(panel, dvz_app_window_input(win), 0);
    DvzArcball* arcball = dvz_panel_arcball(panel);
    if (arcball == NULL)
    {
        dvz_fprintf(stderr, "dvz_panel_set_arcball() failed\n");
        dvz_app_destroy(app);
        dvz_free(state.diameters);
        dvz_free(state.colors);
        dvz_free(state.positions);
        dvz_free(state.base_positions);
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_arcball_set(arcball, (vec3){+0.42f, -0.10f, +0.18f});

    state.spin = dvz_anim_arcball_spin(
        scene, arcball, (vec3){0.0f, 1.0f, 0.0f}, ROTATION_SPEED_RAD_PER_SEC,
        DVZ_ARCBALL_SPIN_FLAGS_PAUSE_ON_INTERACTION);
    if (state.spin == NULL)
    {
        dvz_fprintf(stderr, "dvz_anim_arcball_spin() failed\n");
        dvz_app_destroy(app);
        dvz_free(state.diameters);
        dvz_free(state.colors);
        dvz_free(state.positions);
        dvz_free(state.base_positions);
        dvz_scene_destroy(scene);
        return 1;
    }
    _reset_controls(&state);

    DvzGuiConfig gui_config = dvz_gui_config();
    DvzGui* gui = dvz_app_window_gui(win, &gui_config);
    if (gui == NULL)
    {
        dvz_fprintf(stderr, "dvz_app_window_gui() failed\n");
        dvz_app_destroy(app);
        dvz_free(state.diameters);
        dvz_free(state.colors);
        dvz_free(state.positions);
        dvz_free(state.base_positions);
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_app_window_set_gui_callback(win, _point_stress_gui, &state);
    dvz_app_window_set_frame_callback(win, _point_stress_frame, &state);

    dvz_scene_set_clock_mode(scene, DVZ_CLOCK_REALTIME);
    dvz_scene_set_fps(scene, 60.0);
    dvz_app_run(app, _frame_count(argc, argv));

    dvz_app_destroy(app);
    dvz_free(state.diameters);
    dvz_free(state.colors);
    dvz_free(state.positions);
    dvz_free(state.base_positions);
    dvz_scene_destroy(scene);
    return 0;
}
