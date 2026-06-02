/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* depth_cue - interactive point depth-cue example.
 *
 * Build:  just example-c techniques/depth_cue
 * Run:    ./build/examples/c/techniques/depth_cue
 */

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
#include "example_common.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define POINT_COUNT 8192u
#define WIDTH       1000u
#define HEIGHT      760u
#define ROTATION_SPEED_RAD_PER_SEC 0.22f
#define CLIP_CUE_MIN  0.0f
#define CLIP_CUE_MAX  1.0f
#define EYE_CUE_MIN   0.0f
#define EYE_CUE_MAX   6.0f
#define WORLD_CUE_MIN 0.0f
#define WORLD_CUE_MAX 2.0f
#define CUE_EPS       1e-4f

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct DepthCueExampleState
{
    DvzVisual* visual;
    DvzExampleVisualSpin spin;
    float* sizes;
    uint32_t point_count;
    bool depth_cue_enabled;
    bool spin_enabled;
    DvzDepthCueMode depth_cue_mode;
    DvzDepthCueMetric depth_cue_metric;
    DvzDepthCueFalloff depth_cue_falloff;
    float depth_cue_near;
    float depth_cue_far;
    float depth_cue_strength;
    float depth_cue_density;
    float depth_cue_background[4];
    float point_size;
} DepthCueExampleState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Build a deterministic point volume with visible front, middle, and rear layers.
 *
 * @param positions point positions
 * @param colors point colors
 * @param sizes point sizes
 * @param count number of points to fill
 */
static void _build_points(
    vec3* positions, DvzColor* colors, float* sizes, uint32_t count)
{
    ANN(positions);
    ANN(colors);
    ANN(sizes);

    for (uint32_t i = 0; i < count; i++)
    {
        uint32_t layer = i % 4u;
        uint32_t j = i / 4u;
        float u = (float)(j % 64u) / 63.0f;
        float v = (float)((j / 64u) % 32u) / 31.0f;
        float theta = TAU * u + 0.35f * (float)layer;
        float ring = 0.18f + 0.72f * v;
        float wobble = 0.08f * sinf(9.0f * u + 2.5f * (float)layer);

        positions[i][0] = ring * cosf(theta) + wobble;
        positions[i][1] = ring * sinf(theta) * 0.72f;
        positions[i][2] = -0.72f + 0.48f * (float)layer + 0.08f * cosf(5.0f * theta);

        colors[i] = dvz_color_rgb(
            (uint8_t)(210u - 28u * layer), (uint8_t)(70u + 42u * layer),
            (uint8_t)(90u + 34u * layer));
        sizes[i] = 6.0f;
    }
}



/**
 * Apply the retained point-size control to the visual.
 *
 * @param state example state
 */
static void _apply_point_size(DepthCueExampleState* state)
{
    ANN(state);
    ANN(state->visual);
    ANN(state->sizes);

    for (uint32_t i = 0; i < state->point_count; i++)
        state->sizes[i] = state->point_size;
    if (dvz_visual_set_data(state->visual, "diameter", state->sizes, state->point_count) != 0)
        dvz_fprintf(stderr, "failed to update point size\n");
}



/**
 * Apply the retained spin control to the scene animation.
 *
 * @param state example state
 */
static void _apply_spin(DepthCueExampleState* state)
{
    ANN(state);
    if (state->spin.animation == NULL)
        return;
    if (state->spin_enabled)
        example_visual_spin_start(&state->spin, 0.0);
    else
        example_visual_spin_stop(&state->spin);
}



/**
 * Return a short label for a depth-cue mode.
 *
 * @param mode depth-cue mode
 * @return display label
 */
static const char* _depth_cue_mode_label(DvzDepthCueMode mode)
{
    switch (mode)
    {
    case DVZ_DEPTH_CUE_FADE_TO_BACKGROUND:
        return "Fade";
    case DVZ_DEPTH_CUE_DESATURATE:
        return "Desaturate";
    case DVZ_DEPTH_CUE_DARKEN:
        return "Darken";
    case DVZ_DEPTH_CUE_NONE:
    default:
        return "None";
    }
}



/**
 * Cycle to the next supported depth-cue mode.
 *
 * @param mode current depth-cue mode
 * @return next depth-cue mode
 */
static DvzDepthCueMode _depth_cue_mode_next(DvzDepthCueMode mode)
{
    switch (mode)
    {
    case DVZ_DEPTH_CUE_FADE_TO_BACKGROUND:
        return DVZ_DEPTH_CUE_DESATURATE;
    case DVZ_DEPTH_CUE_DESATURATE:
        return DVZ_DEPTH_CUE_DARKEN;
    case DVZ_DEPTH_CUE_DARKEN:
    case DVZ_DEPTH_CUE_NONE:
    default:
        return DVZ_DEPTH_CUE_FADE_TO_BACKGROUND;
    }
}



/**
 * Return a short label for a depth-cue metric.
 *
 * @param metric depth-cue metric
 * @return display label
 */
static const char* _depth_cue_metric_label(DvzDepthCueMetric metric)
{
    switch (metric)
    {
    case DVZ_DEPTH_CUE_METRIC_EYE_DISTANCE:
        return "Eye distance";
    case DVZ_DEPTH_CUE_METRIC_WORLD_DISTANCE:
        return "World distance";
    case DVZ_DEPTH_CUE_METRIC_CLIP_DEPTH:
    default:
        return "Clip depth";
    }
}



/**
 * Cycle to the next supported depth-cue metric.
 *
 * @param metric current depth-cue metric
 * @return next depth-cue metric
 */
static DvzDepthCueMetric _depth_cue_metric_next(DvzDepthCueMetric metric)
{
    switch (metric)
    {
    case DVZ_DEPTH_CUE_METRIC_CLIP_DEPTH:
        return DVZ_DEPTH_CUE_METRIC_EYE_DISTANCE;
    case DVZ_DEPTH_CUE_METRIC_EYE_DISTANCE:
        return DVZ_DEPTH_CUE_METRIC_WORLD_DISTANCE;
    case DVZ_DEPTH_CUE_METRIC_WORLD_DISTANCE:
    default:
        return DVZ_DEPTH_CUE_METRIC_CLIP_DEPTH;
    }
}


/**
 * Return the slider bounds for one depth-cue metric.
 *
 * @param metric depth-cue metric
 * @param min output lower bound
 * @param max output upper bound
 */
static void _depth_cue_metric_range(DvzDepthCueMetric metric, float* min, float* max)
{
    ANN(min);
    ANN(max);

    switch (metric)
    {
    case DVZ_DEPTH_CUE_METRIC_EYE_DISTANCE:
        *min = EYE_CUE_MIN;
        *max = EYE_CUE_MAX;
        break;
    case DVZ_DEPTH_CUE_METRIC_WORLD_DISTANCE:
        *min = WORLD_CUE_MIN;
        *max = WORLD_CUE_MAX;
        break;
    case DVZ_DEPTH_CUE_METRIC_CLIP_DEPTH:
    default:
        *min = CLIP_CUE_MIN;
        *max = CLIP_CUE_MAX;
        break;
    }
}



/**
 * Reset the cue thresholds to useful defaults for the active metric.
 *
 * @param state example state
 */
static void _reset_cue_thresholds(DepthCueExampleState* state)
{
    ANN(state);

    switch (state->depth_cue_metric)
    {
    case DVZ_DEPTH_CUE_METRIC_EYE_DISTANCE:
        state->depth_cue_near = 2.3f;
        state->depth_cue_far = 3.9f;
        break;
    case DVZ_DEPTH_CUE_METRIC_WORLD_DISTANCE:
        state->depth_cue_near = 0.4f;
        state->depth_cue_far = 1.2f;
        break;
    case DVZ_DEPTH_CUE_METRIC_CLIP_DEPTH:
    default:
        state->depth_cue_near = 0.98f;
        state->depth_cue_far = 1.0f;
        break;
    }
}



/**
 * Return a short label for a depth-cue falloff.
 *
 * @param falloff depth-cue falloff
 * @return display label
 */
static const char* _depth_cue_falloff_label(DvzDepthCueFalloff falloff)
{
    switch (falloff)
    {
    case DVZ_DEPTH_CUE_FALLOFF_EXPONENTIAL:
        return "Exponential";
    case DVZ_DEPTH_CUE_FALLOFF_LINEAR:
    default:
        return "Linear";
    }
}



/**
 * Cycle to the next supported depth-cue falloff.
 *
 * @param falloff current depth-cue falloff
 * @return next depth-cue falloff
 */
static DvzDepthCueFalloff _depth_cue_falloff_next(DvzDepthCueFalloff falloff)
{
    return falloff == DVZ_DEPTH_CUE_FALLOFF_LINEAR ? DVZ_DEPTH_CUE_FALLOFF_EXPONENTIAL :
                                                     DVZ_DEPTH_CUE_FALLOFF_LINEAR;
}



/**
 * Apply the retained depth-cue state to the point visual.
 *
 * @param state example state
 */
static void _apply_depth_cue(DepthCueExampleState* state)
{
    ANN(state);
    ANN(state->visual);

    if (!state->depth_cue_enabled)
    {
        if (dvz_visual_set_depth_cue(state->visual, NULL) != 0)
            dvz_fprintf(stderr, "dvz_visual_set_depth_cue(NULL) failed\n");
        return;
    }

    float range_min = 0.0f;
    float range_max = 1.0f;
    _depth_cue_metric_range(state->depth_cue_metric, &range_min, &range_max);
    if (state->depth_cue_near < range_min)
        state->depth_cue_near = range_min;
    if (state->depth_cue_near > range_max - CUE_EPS)
        state->depth_cue_near = range_max - CUE_EPS;
    if (state->depth_cue_far > range_max)
        state->depth_cue_far = range_max;
    if (state->depth_cue_far <= state->depth_cue_near + CUE_EPS)
        state->depth_cue_far = state->depth_cue_near + CUE_EPS;

    DvzDepthCueDesc desc = {DVZ_STRUCT_INIT_FIELDS(DvzDepthCueDesc),
        .mode = state->depth_cue_mode,
        .metric = state->depth_cue_metric,
        .falloff = state->depth_cue_falloff,
        .near_depth = state->depth_cue_near,
        .far_depth = state->depth_cue_far,
        .strength = state->depth_cue_strength,
        .density = state->depth_cue_density,
        .background_color = {
            state->depth_cue_background[0],
            state->depth_cue_background[1],
            state->depth_cue_background[2],
            state->depth_cue_background[3],
        },
    };
    if (dvz_visual_set_depth_cue(state->visual, &desc) != 0)
        dvz_fprintf(stderr, "dvz_visual_set_depth_cue() failed\n");
}



/**
 * Reset the example controls to useful depth-cue defaults.
 *
 * @param state example state
 */
static void _reset_controls(DepthCueExampleState* state)
{
    ANN(state);
    state->depth_cue_enabled = true;
    state->depth_cue_mode = DVZ_DEPTH_CUE_FADE_TO_BACKGROUND;
    state->depth_cue_metric = DVZ_DEPTH_CUE_METRIC_EYE_DISTANCE;
    state->depth_cue_falloff = DVZ_DEPTH_CUE_FALLOFF_LINEAR;
    _reset_cue_thresholds(state);
    state->depth_cue_strength = 0.62f;
    state->depth_cue_density = 3.0f;
    state->depth_cue_background[0] = 0.035f;
    state->depth_cue_background[1] = 0.045f;
    state->depth_cue_background[2] = 0.055f;
    state->depth_cue_background[3] = 1.0f;
    state->point_size = 6.0f;
    _apply_depth_cue(state);
    _apply_point_size(state);
}



/**
 * Build the live depth-cue controls.
 *
 * @param gui GUI overlay
 * @param win view
 * @param user_data example state
 */
static void _depth_cue_gui(DvzGui* gui, DvzView* win, void* user_data)
{
    (void)win;
    DepthCueExampleState* state = (DepthCueExampleState*)user_data;
    if (state == NULL)
        return;

    bool cue_changed = false;
    bool point_changed = false;
    bool spin_changed = false;
    if (dvz_gui_begin(gui, "Depth Cue", NULL, 0))
    {
        point_changed |=
            dvz_gui_slider_float(gui, "Point size", &state->point_size, 1.0f, 24.0f);
        spin_changed |= dvz_gui_checkbox(gui, "Auto rotate", &state->spin_enabled);
        cue_changed |= dvz_gui_checkbox(gui, "Depth cue", &state->depth_cue_enabled);
        char mode_label[64];
        dvz_snprintf(
            mode_label, sizeof(mode_label), "Mode: %s",
            _depth_cue_mode_label(state->depth_cue_mode));
        if (dvz_gui_button(gui, mode_label))
        {
            state->depth_cue_mode = _depth_cue_mode_next(state->depth_cue_mode);
            cue_changed = true;
        }
        char metric_label[64];
        dvz_snprintf(
            metric_label, sizeof(metric_label), "Metric: %s",
            _depth_cue_metric_label(state->depth_cue_metric));
        if (dvz_gui_button(gui, metric_label))
        {
            state->depth_cue_metric = _depth_cue_metric_next(state->depth_cue_metric);
            _reset_cue_thresholds(state);
            cue_changed = true;
        }
        char falloff_label[64];
        dvz_snprintf(
            falloff_label, sizeof(falloff_label), "Falloff: %s",
            _depth_cue_falloff_label(state->depth_cue_falloff));
        if (dvz_gui_button(gui, falloff_label))
        {
            state->depth_cue_falloff = _depth_cue_falloff_next(state->depth_cue_falloff);
            cue_changed = true;
        }
        float range_min = 0.0f;
        float range_max = 1.0f;
        _depth_cue_metric_range(state->depth_cue_metric, &range_min, &range_max);
        cue_changed |=
            dvz_gui_slider_float(gui, "Cue near", &state->depth_cue_near, range_min, range_max);
        cue_changed |=
            dvz_gui_slider_float(gui, "Cue far", &state->depth_cue_far, range_min, range_max);
        cue_changed |=
            dvz_gui_slider_float(gui, "Cue strength", &state->depth_cue_strength, 0.0f, 1.0f);
        cue_changed |=
            dvz_gui_slider_float(gui, "Fog density", &state->depth_cue_density, 0.1f, 8.0f);
        if (dvz_gui_button(gui, "Reset"))
        {
            _reset_controls(state);
            cue_changed = false;
            point_changed = false;
        }
    }
    dvz_gui_end(gui);

    if (cue_changed)
        _apply_depth_cue(state);
    if (point_changed)
        _apply_point_size(state);
    if (spin_changed)
        _apply_spin(state);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;
    vec3* positions = NULL;
    DvzColor* colors = NULL;
    float* sizes = NULL;

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel_full(figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel() failed");

    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.eye[2] = 3.0f;
    camera_desc.near = 0.1f;
    camera_desc.far = 100.0f;
    bool ok = dvz_panel_set_camera(panel, &camera_desc);
    EXAMPLE_CHECK(ok, "dvz_panel_set_camera() failed");

    DvzVisual* visual = dvz_point(scene, 0);
    EXAMPLE_CHECK(visual != NULL, "dvz_point() failed");

    positions = (vec3*)dvz_calloc(POINT_COUNT, sizeof(*positions));
    colors = (DvzColor*)dvz_calloc(POINT_COUNT, sizeof(DvzColor));
    sizes = (float*)dvz_calloc(POINT_COUNT, sizeof(float));
    EXAMPLE_CHECK(positions != NULL && colors != NULL && sizes != NULL, "point allocation failed");
    _build_points(positions, colors, sizes, POINT_COUNT);

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = POINT_COUNT},
        {.attr_name = "color", .data = colors, .item_count = POINT_COUNT},
        {.attr_name = "diameter", .data = sizes, .item_count = POINT_COUNT},
    };
    int rc = dvz_visual_set_data_many(visual, updates, 3);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_data_many() failed");

    rc = dvz_panel_add_visual(panel, visual, NULL);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual() failed");
    dvz_panel_set_background_color(panel, 0.035f, 0.045f, 0.055f, 1.0f);

    DepthCueExampleState gui_state = {
        .visual = visual,
        .sizes = sizes,
        .point_count = POINT_COUNT,
    };
    _reset_controls(&gui_state);

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win =
        dvz_view_glfw(app, figure, WIDTH, HEIGHT, "depth_cue");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    DvzArcball* arcball = dvz_view_arcball(win, panel, NULL);
    EXAMPLE_CHECK(arcball != NULL, "failed to create or bind arcball controller");
    dvz_arcball_set(arcball, (vec3){+0.38f, -0.18f, +0.30f});

    EXAMPLE_CHECK(
        example_visual_spin(
            scene, visual, (vec3){0.0f, 1.0f, 0.0f}, ROTATION_SPEED_RAD_PER_SEC, NULL,
            &gui_state.spin),
        "example_visual_spin() failed");
    gui_state.spin_enabled = true;
    _apply_spin(&gui_state);

    DvzGuiConfig gui_config = dvz_gui_config();
    DvzGui* gui = dvz_view_gui(win, &gui_config);
    EXAMPLE_CHECK(gui != NULL, "dvz_view_gui() failed");
    dvz_view_set_gui_callback(win, _depth_cue_gui, &gui_state);

    dvz_app_run(app, example_frame_count(argc, argv));
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    example_visual_spin_destroy(&gui_state.spin);
    dvz_free(sizes);
    dvz_free(colors);
    dvz_free(positions);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
