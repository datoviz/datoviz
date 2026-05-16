/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* hello_points_depth_cue_glfw - interactive point depth-cue example.
 *
 * Build:  just example-c hello_points_depth_cue_glfw
 * Run:    ./build/examples/c/hello_points_depth_cue_glfw
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



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define POINT_COUNT 8192u
#define WIDTH       1000u
#define HEIGHT      760u
#define ROTATION_SPEED_RAD_PER_SEC 0.22f

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct DepthCueExampleState
{
    DvzVisual* visual;
    DvzAnimation* spin;
    float* sizes;
    uint32_t point_count;
    bool depth_cue_enabled;
    bool spin_enabled;
    DvzDepthCueMode depth_cue_mode;
    float depth_cue_near;
    float depth_cue_far;
    float depth_cue_strength;
    float depth_cue_background[4];
    float point_size;
} DepthCueExampleState;



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
    if (argc < 2 || argv == NULL)
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
 * Build a deterministic point volume with visible front, middle, and rear layers.
 *
 * @param positions point positions
 * @param colors point colors
 * @param sizes point sizes
 * @param count number of points to fill
 */
static void _build_points(
    float (*positions)[3], DvzColor* colors, float* sizes, uint32_t count)
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

        colors[i][0] = (uint8_t)(210u - 28u * layer);
        colors[i][1] = (uint8_t)(70u + 42u * layer);
        colors[i][2] = (uint8_t)(90u + 34u * layer);
        colors[i][3] = 255;
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
    if (dvz_visual_set_data(state->visual, "size", state->sizes, state->point_count) != 0)
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
    if (state->spin == NULL)
        return;
    if (state->spin_enabled)
        dvz_anim_start(state->spin, 0.0);
    else
        dvz_anim_stop(state->spin);
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

    if (state->depth_cue_far <= state->depth_cue_near + 1e-4f)
        state->depth_cue_far = state->depth_cue_near + 1e-4f;

    DvzDepthCueDesc desc = {
        .mode = state->depth_cue_mode,
        .near_depth = state->depth_cue_near,
        .far_depth = state->depth_cue_far,
        .strength = state->depth_cue_strength,
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
    state->depth_cue_near = 0.42f;
    state->depth_cue_far = 0.98f;
    state->depth_cue_strength = 0.62f;
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
 * @param win app window
 * @param user_data example state
 */
static void _depth_cue_gui(DvzGui* gui, DvzAppWindow* win, void* user_data)
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
        cue_changed |=
            dvz_gui_slider_float(gui, "Cue near", &state->depth_cue_near, 0.0f, 1.0f);
        cue_changed |=
            dvz_gui_slider_float(gui, "Cue far", &state->depth_cue_far, 0.0f, 1.0f);
        cue_changed |=
            dvz_gui_slider_float(gui, "Cue strength", &state->depth_cue_strength, 0.0f, 1.0f);
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
    camera_desc.eye[2] = 3.0f;
    camera_desc.near = 0.1f;
    camera_desc.far = 100.0f;
    if (!dvz_panel_set_camera(panel, &camera_desc))
    {
        dvz_fprintf(stderr, "dvz_panel_set_camera() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzVisual* visual = dvz_point(scene, 0);
    if (visual == NULL)
    {
        dvz_fprintf(stderr, "dvz_point() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    float(*positions)[3] = (float(*)[3])dvz_calloc(POINT_COUNT, sizeof(*positions));
    DvzColor* colors = (DvzColor*)dvz_calloc(POINT_COUNT, sizeof(DvzColor));
    float* sizes = (float*)dvz_calloc(POINT_COUNT, sizeof(float));
    if (positions == NULL || colors == NULL || sizes == NULL)
    {
        dvz_fprintf(stderr, "point allocation failed\n");
        dvz_free(sizes);
        dvz_free(colors);
        dvz_free(positions);
        dvz_scene_destroy(scene);
        return 1;
    }
    _build_points(positions, colors, sizes, POINT_COUNT);

    if (dvz_visual_set_data(visual, "position", positions, POINT_COUNT) != 0 ||
        dvz_visual_set_data(visual, "color", colors, POINT_COUNT) != 0 ||
        dvz_visual_set_data(visual, "size", sizes, POINT_COUNT) != 0 ||
        dvz_panel_add_visual(panel, visual, NULL) != 0)
    {
        dvz_fprintf(stderr, "point visual setup failed\n");
        dvz_free(sizes);
        dvz_free(colors);
        dvz_free(positions);
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_panel_set_background_color(panel, 0.035f, 0.045f, 0.055f, 1.0f);

    DepthCueExampleState gui_state = {
        .visual = visual,
        .sizes = sizes,
        .point_count = POINT_COUNT,
    };
    _reset_controls(&gui_state);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        dvz_fprintf(stderr, "dvz_app() failed (no GPU or display?)\n");
        dvz_free(sizes);
        dvz_free(colors);
        dvz_free(positions);
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzAppWindow* win =
        dvz_app_window_glfw(app, figure, WIDTH, HEIGHT, "hello_points_depth_cue_glfw");
    if (win == NULL)
    {
        dvz_fprintf(stderr, "dvz_app_window_glfw() failed (GLFW unavailable?)\n");
        dvz_app_destroy(app);
        dvz_free(sizes);
        dvz_free(colors);
        dvz_free(positions);
        dvz_scene_destroy(scene);
        return 1;
    }

    dvz_panel_set_arcball(panel, dvz_app_window_input(win), 0);
    DvzArcball* arcball = dvz_panel_arcball(panel);
    if (arcball == NULL)
    {
        dvz_fprintf(stderr, "dvz_panel_set_arcball() failed\n");
        dvz_app_destroy(app);
        dvz_free(sizes);
        dvz_free(colors);
        dvz_free(positions);
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_arcball_set(arcball, (vec3){+0.38f, -0.18f, +0.30f});

    DvzAnimation* spin = dvz_anim_arcball_spin(
        scene, arcball, (vec3){0.0f, 1.0f, 0.0f}, ROTATION_SPEED_RAD_PER_SEC,
        DVZ_ARCBALL_SPIN_FLAGS_PAUSE_ON_INTERACTION);
    if (spin == NULL)
    {
        dvz_fprintf(stderr, "dvz_anim_arcball_spin() failed\n");
        dvz_app_destroy(app);
        dvz_free(sizes);
        dvz_free(colors);
        dvz_free(positions);
        dvz_scene_destroy(scene);
        return 1;
    }
    gui_state.spin = spin;
    gui_state.spin_enabled = true;
    _apply_spin(&gui_state);

    DvzGuiConfig gui_config = dvz_gui_config();
    DvzGui* gui = dvz_app_window_gui(win, &gui_config);
    if (gui == NULL)
    {
        dvz_fprintf(stderr, "dvz_app_window_gui() failed\n");
        dvz_app_destroy(app);
        dvz_free(sizes);
        dvz_free(colors);
        dvz_free(positions);
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_app_window_set_gui_callback(win, _depth_cue_gui, &gui_state);

    dvz_app_run(app, _frame_count(argc, argv));

    dvz_app_destroy(app);
    dvz_free(sizes);
    dvz_free(colors);
    dvz_free(positions);
    dvz_scene_destroy(scene);
    return 0;
}
