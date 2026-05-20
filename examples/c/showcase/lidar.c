/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* lidar - local full-resolution LIDAR point-cloud EDL demo.
 *
 * Prepare: python tools/data/prepare_lidar.py --force
 * Build:   just example-c showcase/lidar
 * Run:     ./build/examples/c/showcase/lidar
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <float.h>
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
#define LIDAR_DATA_DIR "data/examples/lidar/prepared"
#define LIDAR_DEFAULT_POINT_SIZE 2.0f
#define LIDAR_DEFAULT_STRIDE 2u
#define CUE_DISTANCE_MIN 0.0f
#define CUE_DISTANCE_MAX 12.0f
#define CUE_DISTANCE_EPS 1e-4f
#define LIDAR_FLY_BACK_RATIO   0.09f
#define LIDAR_FLY_HEIGHT_RATIO 0.01f
#define LIDAR_FLY_SIDE_RATIO   0.25f
#define LIDAR_FLY_LOOK_RATIO   0.008f
#define LIDAR_FLY_TARGET_SIDE  0.21f
#define LIDAR_FLY_TARGET_BACK  0.006f



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



typedef struct LidarViewParams
{
    vec3 eye;
    vec3 target;
    vec3 up;
    float far;
    float speed;
} LidarViewParams;



typedef struct LidarExampleState
{
    DvzPanel* panel;
    DvzFly* fly;
    DvzVisual* visual;
    LidarDataset* dataset;
    bool edl_enabled;
    bool depth_cue_enabled;
    bool fps_mode;
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
    return LIDAR_DATA_DIR;
}



/**
 * Return the requested local-data sampling stride.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return sampling stride, clamped to at least one
 */
static uint32_t _data_stride(int argc, char** argv)
{
    for (int i = 1; i < argc; i++)
    {
        if (argv[i] == NULL)
            continue;
        const char* value = NULL;
        if (strncmp(argv[i], "--stride=", 9) == 0)
            value = argv[i] + 9;
        else if (strcmp(argv[i], "--stride") == 0 && i + 1 < argc && argv[i + 1] != NULL)
            value = argv[i + 1];
        if (value == NULL)
            continue;

        char* end = NULL;
        unsigned long stride = strtoul(value, &end, 10);
        if (end == value || (end != NULL && *end != '\0') || stride == 0)
            return LIDAR_DEFAULT_STRIDE;
        if (stride > UINT32_MAX)
            return UINT32_MAX;
        return (uint32_t)stride;
    }
    return LIDAR_DEFAULT_STRIDE;
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
        "prepare it with: python tools/data/prepare_lidar.py --force\n",
        data_dir != NULL ? data_dir : LIDAR_DATA_DIR);
}



/**
 * Load the local LIDAR arrays and allocate the per-point size buffer.
 *
 * @param data_dir directory containing lidar_pos.npy and lidar_color.npy
 * @param stride sampling stride
 * @param dataset output dataset
 * @return whether loading and validation succeeded
 */
static bool _load_lidar_dataset(const char* data_dir, uint32_t stride, LidarDataset* dataset)
{
    ANN(data_dir);
    ANN(dataset);
    if (stride == 0)
        stride = 1;

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
    float(*positions)[3] = dvz_read_npy(pos_path, &pos_size);
    DvzColor* colors = dvz_read_npy(color_path, &color_size);
    if (positions == NULL || colors == NULL)
    {
        dvz_free(colors);
        dvz_free(positions);
        _print_prepare_hint(data_dir);
        return false;
    }

    if (pos_size == 0 || color_size == 0 || pos_size % (3 * sizeof(float)) != 0 ||
        color_size % sizeof(DvzColor) != 0)
    {
        dvz_fprintf(stderr, "invalid LIDAR .npy payload sizes\n");
        dvz_free(colors);
        dvz_free(positions);
        return false;
    }
    if (sizeof(DvzColor) != 4)
    {
        dvz_fprintf(stderr, "unexpected DvzColor size\n");
        dvz_free(colors);
        dvz_free(positions);
        return false;
    }

    DvzSize pos_count = pos_size / (3 * sizeof(float));
    DvzSize color_count = color_size / sizeof(DvzColor);
    if (pos_count != color_count || pos_count > UINT32_MAX)
    {
        dvz_fprintf(stderr, "LIDAR position/color counts do not match or exceed uint32 range\n");
        dvz_free(colors);
        dvz_free(positions);
        return false;
    }

    DvzSize sampled_count = (pos_count + stride - 1) / stride;
    if (sampled_count > UINT32_MAX)
    {
        dvz_fprintf(stderr, "sampled LIDAR point count exceeds uint32 range\n");
        dvz_free(colors);
        dvz_free(positions);
        return false;
    }
    if (stride > 1)
    {
        for (DvzSize dst = 0, src = 0; src < pos_count; dst++, src += stride)
        {
            positions[dst][0] = positions[src][0];
            positions[dst][1] = positions[src][1];
            positions[dst][2] = positions[src][2];
            colors[dst][0] = colors[src][0];
            colors[dst][1] = colors[src][1];
            colors[dst][2] = colors[src][2];
            colors[dst][3] = colors[src][3];
        }
    }

    float* sizes = (float*)dvz_calloc((size_t)sampled_count, sizeof(float));
    if (sizes == NULL)
    {
        dvz_fprintf(stderr, "unable to allocate LIDAR point sizes\n");
        dvz_free(colors);
        dvz_free(positions);
        return false;
    }

    for (DvzSize i = 0; i < sampled_count; i++)
    {
        positions[i][0] *= LIDAR_POSITION_SCALE;
        positions[i][1] *= LIDAR_POSITION_SCALE;
        positions[i][2] *= LIDAR_POSITION_SCALE;
        sizes[i] = LIDAR_DEFAULT_POINT_SIZE;
    }

    dataset->positions = positions;
    dataset->colors = colors;
    dataset->sizes = sizes;
    dataset->point_count = (uint32_t)sampled_count;

    dvz_fprintf(
        stderr, "loaded LIDAR data with %u points (stride=%u)\n", dataset->point_count, stride);
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
 * Return the axis that best matches the implicit ground-zero floor normal.
 *
 * @param min_pos point-cloud minimum bounds
 * @param max_pos point-cloud maximum bounds
 * @return detected up axis index
 */
static uint32_t _detect_floor_up_axis(const vec3 min_pos, const vec3 max_pos)
{
    uint32_t axis = 1;
    float best_extent = FLT_MAX;
    for (uint32_t i = 0; i < 3; i++)
    {
        float extent = max_pos[i] - min_pos[i];
        if (min_pos[i] <= 0.0f && max_pos[i] >= 0.0f && extent < best_extent)
        {
            axis = i;
            best_extent = extent;
        }
    }
    if (best_extent < FLT_MAX)
        return axis;

    for (uint32_t i = 0; i < 3; i++)
    {
        float extent = max_pos[i] - min_pos[i];
        if (extent < best_extent)
        {
            axis = i;
            best_extent = extent;
        }
    }
    return axis;
}



/**
 * Compute a fly-camera pose from the point-cloud bounds and the ground-zero floor plane.
 *
 * @param dataset loaded LIDAR dataset
 * @param out output fly view parameters
 * @return whether the pose could be computed
 */
static bool _compute_lidar_view_params(const LidarDataset* dataset, LidarViewParams* out)
{
    ANN(dataset);
    ANN(out);
    if (dataset->positions == NULL || dataset->point_count == 0)
        return false;

    vec3 min_pos = {FLT_MAX, FLT_MAX, FLT_MAX};
    vec3 max_pos = {-FLT_MAX, -FLT_MAX, -FLT_MAX};
    for (uint32_t i = 0; i < dataset->point_count; i++)
    {
        for (uint32_t j = 0; j < 3; j++)
        {
            float value = dataset->positions[i][j];
            if (value < min_pos[j])
                min_pos[j] = value;
            if (value > max_pos[j])
                max_pos[j] = value;
        }
    }

    vec3 center = {
        0.5f * (min_pos[0] + max_pos[0]),
        0.5f * (min_pos[1] + max_pos[1]),
        0.5f * (min_pos[2] + max_pos[2]),
    };
    vec3 extent = {
        max_pos[0] - min_pos[0],
        max_pos[1] - min_pos[1],
        max_pos[2] - min_pos[2],
    };

    uint32_t up_axis = _detect_floor_up_axis(min_pos, max_pos);
    uint32_t side_axis = up_axis == 0 ? 1 : 0;
    uint32_t forward_axis = up_axis == 2 ? 1 : 2;
    if (extent[side_axis] > extent[forward_axis])
    {
        uint32_t tmp = side_axis;
        side_axis = forward_axis;
        forward_axis = tmp;
    }

    float forward_extent = extent[forward_axis] > 0.0f ? extent[forward_axis] : 1.0f;
    float side_extent = extent[side_axis] > 0.0f ? extent[side_axis] : forward_extent;
    float ground =
        min_pos[up_axis] <= 0.0f && max_pos[up_axis] >= 0.0f ? 0.0f : min_pos[up_axis];

    vec3 eye = {center[0], center[1], center[2]};
    vec3 target = {center[0], center[1], center[2]};
    vec3 up = {0.0f, 0.0f, 0.0f};
    eye[up_axis] = ground + LIDAR_FLY_HEIGHT_RATIO * forward_extent;
    eye[side_axis] = center[side_axis] + LIDAR_FLY_SIDE_RATIO * side_extent;
    eye[forward_axis] = min_pos[forward_axis] - LIDAR_FLY_BACK_RATIO * forward_extent;
    target[up_axis] = ground + LIDAR_FLY_LOOK_RATIO * forward_extent;
    target[side_axis] = center[side_axis] + LIDAR_FLY_TARGET_SIDE * side_extent;
    target[forward_axis] = min_pos[forward_axis] - LIDAR_FLY_TARGET_BACK * forward_extent;
    up[up_axis] = 1.0f;

    dvz_memcpy(out->eye, sizeof(out->eye), eye, sizeof(eye));
    dvz_memcpy(out->target, sizeof(out->target), target, sizeof(target));
    dvz_memcpy(out->up, sizeof(out->up), up, sizeof(up));
    out->far = 10.0f * forward_extent;
    out->speed = 0.32f * forward_extent;

    dvz_fprintf(
        stderr,
        "LIDAR floor plane: axis=%u value=%.3f; fly eye=(%.3f %.3f %.3f), "
        "target=(%.3f %.3f %.3f)\n",
        up_axis, ground, out->eye[0], out->eye[1], out->eye[2], out->target[0],
        out->target[1], out->target[2]);
    return true;
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
            state->visual, "pixel_size", state->dataset->sizes, state->dataset->point_count) != 0)
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

    if (state->depth_cue_near < CUE_DISTANCE_MIN)
        state->depth_cue_near = CUE_DISTANCE_MIN;
    if (state->depth_cue_near > CUE_DISTANCE_MAX - CUE_DISTANCE_EPS)
        state->depth_cue_near = CUE_DISTANCE_MAX - CUE_DISTANCE_EPS;
    if (state->depth_cue_far > CUE_DISTANCE_MAX)
        state->depth_cue_far = CUE_DISTANCE_MAX;
    if (state->depth_cue_far <= state->depth_cue_near + CUE_DISTANCE_EPS)
        state->depth_cue_far = state->depth_cue_near + CUE_DISTANCE_EPS;

    DvzDepthCueDesc desc = {
        .mode = state->depth_cue_mode,
        .metric = DVZ_DEPTH_CUE_METRIC_EYE_DISTANCE,
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
 * Apply the retained navigation-mode state to the fly controller.
 *
 * @param state example state
 */
static void _apply_fly_mode(LidarExampleState* state)
{
    ANN(state);
    if (state->fly == NULL)
        return;
    dvz_fly_set_mode(state->fly, state->fps_mode ? DVZ_FLY_MODE_PLANE : DVZ_FLY_MODE_FREE);
}



/**
 * Reset the example controls to useful LIDAR defaults.
 *
 * @param state example state
 */
static void _reset_lidar_controls(LidarExampleState* state)
{
    ANN(state);
    state->edl_enabled = false;
    state->depth_cue_enabled = false;
    state->fps_mode = true;
    state->depth_cue_mode = DVZ_DEPTH_CUE_FADE_TO_BACKGROUND;
    state->radius = 2.0f;
    state->strength = 70.0f;
    state->depth_scale = 1.0f;
    state->depth_cue_near = 3.5f;
    state->depth_cue_far = 10.2f;
    state->depth_cue_strength = 0.30f;
    state->depth_cue_background[0] = 0.030f;
    state->depth_cue_background[1] = 0.036f;
    state->depth_cue_background[2] = 0.042f;
    state->depth_cue_background[3] = 1.0f;
    state->point_size = LIDAR_DEFAULT_POINT_SIZE;
    _apply_edl(state);
    _apply_depth_cue(state);
    _apply_fly_mode(state);
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
    bool nav_changed = false;
    bool point_changed = false;
    if (dvz_gui_begin(gui, "LIDAR EDL", NULL, 0))
    {
        nav_changed |= dvz_gui_checkbox(gui, "FPS mode", &state->fps_mode);
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
            dvz_gui_slider_float(
                gui, "Cue near", &state->depth_cue_near, CUE_DISTANCE_MIN, CUE_DISTANCE_MAX);
        cue_changed |=
            dvz_gui_slider_float(
                gui, "Cue far", &state->depth_cue_far, CUE_DISTANCE_MIN, CUE_DISTANCE_MAX);
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

    if (nav_changed)
        _apply_fly_mode(state);
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
    uint32_t stride = _data_stride(argc, argv);
    if (!_load_lidar_dataset(data_dir, stride, &dataset))
        return 1;

    LidarViewParams view = {0};
    if (!_compute_lidar_view_params(&dataset, &view))
    {
        dvz_fprintf(stderr, "failed to compute the LIDAR fly-camera pose\n");
        _destroy_lidar_dataset(&dataset);
        return 1;
    }

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
    dvz_memcpy(camera_desc.eye, sizeof(camera_desc.eye), view.eye, sizeof(view.eye));
    dvz_memcpy(camera_desc.target, sizeof(camera_desc.target), view.target, sizeof(view.target));
    dvz_memcpy(camera_desc.up, sizeof(camera_desc.up), view.up, sizeof(view.up));
    camera_desc.near = 0.1f;
    camera_desc.far = view.far;
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
        dvz_visual_set_data(visual, "pixel_size", dataset.sizes, dataset.point_count) != 0 ||
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
        .fps_mode = true,
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

    DvzAppWindow* win = dvz_app_window_glfw(app, figure, WIDTH, HEIGHT, "lidar");
    if (win == NULL)
    {
        dvz_fprintf(stderr, "dvz_app_window_glfw() failed (GLFW unavailable?)\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        _destroy_lidar_dataset(&dataset);
        return 1;
    }

    DvzFlyDesc fly_desc = dvz_fly_desc();
    fly_desc.mode = DVZ_FLY_MODE_PLANE;
    dvz_memcpy(fly_desc.position, sizeof(fly_desc.position), view.eye, sizeof(view.eye));
    dvz_memcpy(fly_desc.target, sizeof(fly_desc.target), view.target, sizeof(view.target));
    dvz_memcpy(fly_desc.up, sizeof(fly_desc.up), view.up, sizeof(view.up));
    fly_desc.speed = view.speed;
    DvzController* fly_controller = dvz_scene_fly(scene, &fly_desc);
    DvzFly* fly = dvz_controller_fly(fly_controller);
    if (fly == NULL || dvz_panel_bind_controller(panel, fly_controller, DVZ_DIM_MASK_XYZ) != 0)
    {
        dvz_fprintf(stderr, "failed to create or bind the LIDAR fly controller\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        _destroy_lidar_dataset(&dataset);
        return 1;
    }
    dvz_fly_connect(fly, dvz_app_window_input(win));
    gui_state.fly = fly;

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
