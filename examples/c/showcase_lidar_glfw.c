/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* showcase_lidar_glfw - local full-resolution LIDAR point-cloud EDL demo.
 *
 * Prepare: python examples/c/prepare_lidar_npy.py
 * Build:   just example-c showcase_lidar_glfw
 * Run:     ./build/examples/c/showcase_lidar_glfw --data-dir build/local_data/lidar
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "datoviz/app.h"
#include "datoviz/fileio.h"
#include "datoviz/gui.h"
#include "datoviz/scene.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  800u
#define HEIGHT 600u
#define LIDAR_POSITION_SCALE 5.0f
#define LIDAR_DEFAULT_POINT_SIZE 2.0f

static const vec3 LIDAR_FLY_EYE = {+2.0f, +2.0f, -6.0f};
static const vec3 LIDAR_FLY_TARGET = {+1.71428573f, +1.57142854f, -5.14285707f};
static const vec3 LIDAR_FLY_UP = {-0.13552618f, +0.90350789f, +0.40657854f};



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct LidarDataset
{
    float (*positions)[3];
    DvzColor* colors;
    float* sizes;
    uint32_t point_count;
} LidarDataset;



typedef struct LidarExampleState
{
    DvzPanel* panel;
    DvzVisual* visual;
    LidarDataset* dataset;
    bool edl_enabled;
    bool depth_cue_enabled;
    DvzDepthCueMode depth_cue_mode;
    float radius;
    float strength;
    float depth_scale;
    float depth_cue_near;
    float depth_cue_far;
    float depth_cue_strength;
    float depth_cue_background[4];
    float point_size;
} LidarExampleState;



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
 * Return the configured local data directory.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return data directory path
 */
static const char* _data_dir(int argc, char** argv)
{
    for (int i = 1; i < argc; i++)
    {
        if (argv[i] == NULL)
            continue;
        if (strncmp(argv[i], "--data-dir=", 11) == 0)
            return argv[i] + 11;
        if (strcmp(argv[i], "--data-dir") == 0 && i + 1 < argc && argv[i + 1] != NULL)
            return argv[i + 1];
    }
    return "build/local_data/lidar";
}



/**
 * Join a directory and basename into a fixed-size path buffer.
 *
 * @param dir directory path
 * @param basename filename
 * @param out output path buffer
 * @param out_size output path buffer size
 * @return whether the path fit in the output buffer
 */
static bool _join_path(const char* dir, const char* basename, char* out, size_t out_size)
{
    ANN(dir);
    ANN(basename);
    ANN(out);

    int written = dvz_snprintf(out, out_size, "%s/%s", dir, basename);
    return written > 0 && (size_t)written < out_size;
}



/**
 * Print the local-data preparation instructions.
 *
 * @param data_dir expected data directory
 */
static void _print_prepare_hint(const char* data_dir)
{
    dvz_fprintf(
        stderr,
        "missing local LIDAR .npy data in %s\n"
        "prepare it with: python examples/c/prepare_lidar_npy.py\n",
        data_dir != NULL ? data_dir : "build/local_data/lidar");
}



/**
 * Load the local LIDAR arrays and allocate the per-point size buffer.
 *
 * @param data_dir directory containing lidar_pos.npy and lidar_color.npy
 * @param dataset output dataset
 * @return whether loading and validation succeeded
 */
static bool _load_lidar_dataset(const char* data_dir, LidarDataset* dataset)
{
    ANN(data_dir);
    ANN(dataset);

    char pos_path[1024] = {0};
    char color_path[1024] = {0};
    if (!_join_path(data_dir, "lidar_pos.npy", pos_path, sizeof(pos_path)) ||
        !_join_path(data_dir, "lidar_color.npy", color_path, sizeof(color_path)))
    {
        dvz_fprintf(stderr, "LIDAR data path is too long\n");
        return false;
    }

    DvzSize pos_size = 0;
    DvzSize color_size = 0;
    char* pos = dvz_read_npy(pos_path, &pos_size);
    char* color = dvz_read_npy(color_path, &color_size);
    if (pos == NULL || color == NULL)
    {
        dvz_free(color);
        dvz_free(pos);
        _print_prepare_hint(data_dir);
        return false;
    }

    if (pos_size == 0 || color_size == 0 || pos_size % (3 * sizeof(float)) != 0 ||
        color_size % sizeof(DvzColor) != 0)
    {
        dvz_fprintf(stderr, "invalid LIDAR .npy payload sizes\n");
        dvz_free(color);
        dvz_free(pos);
        return false;
    }
    if (sizeof(DvzColor) != 4)
    {
        dvz_fprintf(stderr, "unexpected DvzColor size\n");
        dvz_free(color);
        dvz_free(pos);
        return false;
    }

    DvzSize pos_count = pos_size / (3 * sizeof(float));
    DvzSize color_count = color_size / sizeof(DvzColor);
    if (pos_count != color_count || pos_count > UINT32_MAX)
    {
        dvz_fprintf(stderr, "LIDAR position/color counts do not match or exceed uint32 range\n");
        dvz_free(color);
        dvz_free(pos);
        return false;
    }

    float* sizes = (float*)dvz_calloc((size_t)pos_count, sizeof(float));
    if (sizes == NULL)
    {
        dvz_fprintf(stderr, "unable to allocate LIDAR point sizes\n");
        dvz_free(color);
        dvz_free(pos);
        return false;
    }

    float(*positions)[3] = (float(*)[3])pos;
    for (DvzSize i = 0; i < pos_count; i++)
    {
        positions[i][0] *= LIDAR_POSITION_SCALE;
        positions[i][1] *= LIDAR_POSITION_SCALE;
        positions[i][2] *= LIDAR_POSITION_SCALE;
        sizes[i] = LIDAR_DEFAULT_POINT_SIZE;
    }

    dataset->positions = positions;
    dataset->colors = (DvzColor*)color;
    dataset->sizes = sizes;
    dataset->point_count = (uint32_t)pos_count;

    dvz_fprintf(stderr, "loaded LIDAR data with %u points\n", dataset->point_count);
    return true;
}



/**
 * Release CPU-side LIDAR buffers.
 *
 * @param dataset dataset to release
 */
static void _destroy_lidar_dataset(LidarDataset* dataset)
{
    if (dataset == NULL)
        return;
    dvz_free(dataset->sizes);
    dvz_free(dataset->colors);
    dvz_free(dataset->positions);
    dataset->sizes = NULL;
    dataset->colors = NULL;
    dataset->positions = NULL;
    dataset->point_count = 0;
}



/**
 * Apply the retained point-size control to the visual.
 *
 * @param state example state
 */
static void _apply_point_size(LidarExampleState* state)
{
    ANN(state);
    ANN(state->visual);
    ANN(state->dataset);
    ANN(state->dataset->sizes);

    for (uint32_t i = 0; i < state->dataset->point_count; i++)
        state->dataset->sizes[i] = state->point_size;
    if (dvz_visual_set_data(
            state->visual, "size", state->dataset->sizes, state->dataset->point_count) != 0)
        dvz_fprintf(stderr, "failed to update point size\n");
}



/**
 * Apply the retained EDL state to the panel.
 *
 * @param state example state
 */
static void _apply_edl(LidarExampleState* state)
{
    ANN(state);
    ANN(state->panel);

    if (!state->edl_enabled)
    {
        (void)dvz_panel_set_edl(state->panel, NULL);
        return;
    }

    DvzEdlDesc desc = {
        .radius = state->radius,
        .strength = state->strength,
        .depth_scale = state->depth_scale,
    };
    if (!dvz_panel_set_edl(state->panel, &desc))
        dvz_fprintf(stderr, "dvz_panel_set_edl() failed\n");
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
 * Apply the retained depth-cue state to the pixel visual.
 *
 * @param state example state
 */
static void _apply_depth_cue(LidarExampleState* state)
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
 * Reset the example controls to useful LIDAR defaults.
 *
 * @param state example state
 */
static void _reset_lidar_controls(LidarExampleState* state)
{
    ANN(state);
    state->edl_enabled = true;
    state->depth_cue_enabled = true;
    state->depth_cue_mode = DVZ_DEPTH_CUE_FADE_TO_BACKGROUND;
    state->radius = 2.0f;
    state->strength = 70.0f;
    state->depth_scale = 1.0f;
    state->depth_cue_near = 0.35f;
    state->depth_cue_far = 0.98f;
    state->depth_cue_strength = 0.30f;
    state->depth_cue_background[0] = 0.030f;
    state->depth_cue_background[1] = 0.036f;
    state->depth_cue_background[2] = 0.042f;
    state->depth_cue_background[3] = 1.0f;
    state->point_size = LIDAR_DEFAULT_POINT_SIZE;
    _apply_edl(state);
    _apply_depth_cue(state);
    _apply_point_size(state);
}



/**
 * Build the live LIDAR EDL controls.
 *
 * @param gui GUI overlay
 * @param win app window
 * @param user_data example state
 */
static void _lidar_gui(DvzGui* gui, DvzAppWindow* win, void* user_data)
{
    (void)win;
    LidarExampleState* state = (LidarExampleState*)user_data;
    if (state == NULL)
        return;

    bool changed = false;
    bool cue_changed = false;
    bool point_changed = false;
    if (dvz_gui_begin(gui, "LIDAR EDL", NULL, 0))
    {
        point_changed |=
            dvz_gui_slider_float(gui, "Point size", &state->point_size, 1.0f, 10.0f);
        changed |= dvz_gui_checkbox(gui, "Enable EDL", &state->edl_enabled);
        changed |= dvz_gui_slider_float(gui, "Radius", &state->radius, 1.0f, 8.0f);
        changed |= dvz_gui_slider_float(gui, "Strength", &state->strength, 0.0f, 160.0f);
        changed |= dvz_gui_slider_float(gui, "Depth scale", &state->depth_scale, 0.1f, 8.0f);
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
            _reset_lidar_controls(state);
            changed = false;
            cue_changed = false;
            point_changed = false;
        }
    }
    dvz_gui_end(gui);

    if (changed)
        _apply_edl(state);
    if (cue_changed)
        _apply_depth_cue(state);
    if (point_changed)
        _apply_point_size(state);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    LidarDataset dataset = {0};
    const char* data_dir = _data_dir(argc, argv);
    if (!_load_lidar_dataset(data_dir, &dataset))
        return 1;

    DvzScene* scene = dvz_scene();
    if (scene == NULL)
    {
        dvz_fprintf(stderr, "dvz_scene() failed\n");
        _destroy_lidar_dataset(&dataset);
        return 1;
    }

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    if (figure == NULL)
    {
        dvz_fprintf(stderr, "dvz_figure() failed\n");
        dvz_scene_destroy(scene);
        _destroy_lidar_dataset(&dataset);
        return 1;
    }

    DvzPanel* panel =
        dvz_panel(figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    if (panel == NULL)
    {
        dvz_fprintf(stderr, "dvz_panel() failed\n");
        dvz_scene_destroy(scene);
        _destroy_lidar_dataset(&dataset);
        return 1;
    }

    DvzCameraDesc camera_desc = dvz_camera_desc();
    dvz_memcpy(camera_desc.eye, sizeof(camera_desc.eye), LIDAR_FLY_EYE, sizeof(LIDAR_FLY_EYE));
    dvz_memcpy(
        camera_desc.target, sizeof(camera_desc.target), LIDAR_FLY_TARGET,
        sizeof(LIDAR_FLY_TARGET));
    dvz_memcpy(camera_desc.up, sizeof(camera_desc.up), LIDAR_FLY_UP, sizeof(LIDAR_FLY_UP));
    camera_desc.near = 0.1f;
    camera_desc.far = 100.0f;
    if (!dvz_panel_set_camera(panel, &camera_desc))
    {
        dvz_fprintf(stderr, "dvz_panel_set_camera() failed\n");
        dvz_scene_destroy(scene);
        _destroy_lidar_dataset(&dataset);
        return 1;
    }

    DvzVisual* visual = dvz_pixel(scene, 0);
    if (visual == NULL)
    {
        dvz_fprintf(stderr, "dvz_pixel() failed\n");
        dvz_scene_destroy(scene);
        _destroy_lidar_dataset(&dataset);
        return 1;
    }

    if (dvz_visual_set_data(visual, "position", dataset.positions, dataset.point_count) != 0 ||
        dvz_visual_set_data(visual, "color", dataset.colors, dataset.point_count) != 0 ||
        dvz_visual_set_data(visual, "size", dataset.sizes, dataset.point_count) != 0 ||
        dvz_panel_add_visual(panel, visual, NULL) != 0)
    {
        dvz_fprintf(stderr, "LIDAR visual setup failed\n");
        dvz_scene_destroy(scene);
        _destroy_lidar_dataset(&dataset);
        return 1;
    }
    dvz_panel_set_background_color(panel, 0.030f, 0.036f, 0.042f, 1.0f);

    LidarExampleState gui_state = {
        .panel = panel,
        .visual = visual,
        .dataset = &dataset,
    };
    _reset_lidar_controls(&gui_state);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        dvz_fprintf(stderr, "dvz_app() failed (no GPU or display?)\n");
        dvz_scene_destroy(scene);
        _destroy_lidar_dataset(&dataset);
        return 1;
    }

    DvzAppWindow* win = dvz_app_window_glfw(app, figure, WIDTH, HEIGHT, "showcase_lidar_glfw");
    if (win == NULL)
    {
        dvz_fprintf(stderr, "dvz_app_window_glfw() failed (GLFW unavailable?)\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        _destroy_lidar_dataset(&dataset);
        return 1;
    }

    dvz_panel_set_arcball(panel, dvz_app_window_input(win), 0);
    DvzArcball* arcball = dvz_panel_arcball(panel);
    if (arcball == NULL)
    {
        dvz_fprintf(stderr, "dvz_panel_set_arcball() failed\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        _destroy_lidar_dataset(&dataset);
        return 1;
    }

    DvzGuiConfig gui_config = dvz_gui_config();
    DvzGui* gui = dvz_app_window_gui(win, &gui_config);
    if (gui == NULL)
    {
        dvz_fprintf(stderr, "dvz_app_window_gui() failed\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        _destroy_lidar_dataset(&dataset);
        return 1;
    }
    dvz_app_window_set_gui_callback(win, _lidar_gui, &gui_state);

    dvz_app_run(app, _frame_count(argc, argv));

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    _destroy_lidar_dataset(&dataset);
    return 0;
}
