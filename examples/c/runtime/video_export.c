/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* video_export - This example records a deterministic offscreen point animation to video.
 *
 * What to look for: a row of tick points stays fixed while one larger point moves through a
 * sinusoidal path; each frame updates the retained position, color, and diameter arrays before
 * rendering. The default path writes video_export.mp4 through the app capture API, while --png
 * renders a single smoke frame for quick validation.
 *
 * This runtime workflow is the reproducible offscreen video-export path: the scene data is updated
 * in a deterministic loop, frames are captured from an offscreen view, and the output location can
 * be controlled with DVZ_CAPTURE_DIR and DVZ_CAPTURE_BASENAME. Live window recording uses the same
 * capture API on a window-backed view, but it is documented separately because window size,
 * controller input, and wall-clock timing can affect the result.
 *
 * Scenario: runtime_video_export
 * Style: runtime, graphite_cyan, 1920x1080 output target
 *
 * Build:  just example-c runtime/video_export
 * Run:    ./build/examples/c/runtime/video_export
 * Smoke:  ./build/examples/c/runtime/video_export --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "datoviz/app.h"
#include "datoviz/canvas/enums.h"
#include "datoviz/scene.h"
#include "datoviz/video.h"
#include "example_common.h"
#include "example_style.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH       EXAMPLE_OUTPUT_WIDTH
#define HEIGHT      EXAMPLE_OUTPUT_HEIGHT
#define FPS         60.0
#define FRAME_COUNT 120u
#define TICK_COUNT  9u
#define POINT_COUNT (TICK_COUNT + 1u)
#define PATH_SIZE   512u
#define PROGRESS_WIDTH 32u

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct VideoExportState
{
    DvzVisual* point;
    vec3 positions[POINT_COUNT];
    DvzColor colors[POINT_COUNT];
    float diameters[POINT_COUNT];
} VideoExportState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return the frame count requested by the command line.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @param fallback default frame count
 * @return bounded frame count
 */
static uint32_t _frame_count(int argc, char** argv, uint32_t fallback)
{
    const char* value = NULL;
    uint32_t out = 0;
    if (example_arg_value(argc, argv, "--frames", &value) && example_parse_u32(value, &out) &&
        out != 0)
        return out;

    out = example_frame_count_any(argc, argv);
    return out != 0 ? out : fallback;
}



/**
 * Print command-line usage.
 */
static void _print_usage(void)
{
    dvz_fprintf(stdout, "usage: video_export [--frames N] [--png]\n");
    dvz_fprintf(stdout, "  default      record video_export.mp4 with the app capture API\n");
    dvz_fprintf(stdout, "  --frames N   record N frames instead of the default 120\n");
    dvz_fprintf(stdout, "  --png        render one offscreen PNG smoke image\n");
}



/**
 * Fill the deterministic point scene at one time.
 *
 * @param state video-export state
 * @param t time in seconds
 */
static void _fill_points(VideoExportState* state, double t)
{
    if (state == NULL)
        return;

    for (uint32_t i = 0; i < TICK_COUNT; i++)
    {
        const float u = TICK_COUNT > 1 ? (float)i / (float)(TICK_COUNT - 1u) : 0.0f;
        state->positions[i][0] = -0.80f + 1.60f * u;
        state->positions[i][1] = -0.38f;
        state->positions[i][2] = 0.0f;
        state->diameters[i] = i == TICK_COUNT / 2u ? 18.0f : 12.0f;
        state->colors[i] = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_GRID);
    }

    const float phase = 0.12f * TAU * (float)t;
    const uint32_t cursor = POINT_COUNT - 1u;
    state->positions[cursor][0] = 0.78f * sinf(phase);
    state->positions[cursor][1] = 0.18f * cosf(0.65f * phase);
    state->positions[cursor][2] = 0.0f;
    state->diameters[cursor] = 58.0f + 10.0f * sinf(1.7f * phase);
    state->colors[cursor] = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY);
}



/**
 * Upload all retained point attributes.
 *
 * @param state video-export state
 * @return true on success
 */
static bool _upload_points(VideoExportState* state)
{
    if (state == NULL || state->point == NULL)
        return false;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = state->positions, .item_count = POINT_COUNT},
        {.attr_name = "color", .data = state->colors, .item_count = POINT_COUNT},
        {.attr_name = "diameter_px", .data = state->diameters, .item_count = POINT_COUNT},
    };
    return dvz_visual_set_data_many(state->point, updates, 3) == 0;
}



/**
 * Fill the final capture path for display.
 *
 * @param capture capture configuration
 * @param extension output file extension
 * @param out output path buffer
 * @param size output path buffer size
 */
static void _capture_path(
    const DvzAppCaptureConfig* capture, const char* extension, char* out, size_t size)
{
    if (capture == NULL || extension == NULL || out == NULL || size == 0)
        return;

    const char* directory =
        capture->directory != NULL && capture->directory[0] != '\0' ? capture->directory : ".";
    const char* basename =
        capture->basename != NULL && capture->basename[0] != '\0' ? capture->basename
                                                                  : "video_export";
    const size_t dir_len = strlen(directory);
    if (strcmp(directory, ".") == 0)
        dvz_snprintf(out, size, "./%s%s", basename, extension);
    else if (dir_len > 0 && directory[dir_len - 1] == '/')
        dvz_snprintf(out, size, "%s%s%s", directory, basename, extension);
    else
        dvz_snprintf(out, size, "%s/%s%s", directory, basename, extension);
}



/**
 * Print bounded video export progress.
 *
 * @param completed number of rendered frames
 * @param total total frame count
 */
static void _print_progress(uint32_t completed, uint32_t total)
{
    if (total == 0)
        return;

    const uint32_t filled = (uint32_t)(((uint64_t)completed * PROGRESS_WIDTH) / total);
    dvz_fprintf(stdout, "\rvideo_export: [");
    for (uint32_t i = 0; i < PROGRESS_WIDTH; i++)
        fputc(i < filled ? '#' : '.', stdout);
    dvz_fprintf(stdout, "] %u/%u frames", completed, total);
    fflush(stdout);
}



/**
 * Create the retained scene used by the video export.
 *
 * @param scene scene
 * @param state state to initialize
 * @return figure, or NULL on error
 */
static DvzFigure* _create_scene(DvzScene* scene, VideoExportState* state)
{
    if (scene == NULL || state == NULL)
        return NULL;

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    DvzPanel* panel = figure != NULL ? dvz_panel_full(figure) : NULL;
    state->point = dvz_point(scene, 0);
    if (figure == NULL || panel == NULL || state->point == NULL)
        return NULL;
    example_graphite_cyan_set_panel_background(panel);

    _fill_points(state, 0.0);
    if (!_upload_points(state))
        return NULL;

    DvzPointStyleDesc style = dvz_point_style_desc();
    style.aspect = DVZ_SHAPE_ASPECT_FILLED;
    style.stroke_width_px = 0.0f;
    if (dvz_point_set_style(state->point, &style) != 0)
        return NULL;
    if (dvz_visual_set_depth_test(state->point, false) != 0)
        return NULL;
    if (dvz_panel_add_visual(panel, state->point, NULL) != 0)
        return NULL;
    return figure;
}



/**
 * Render one PNG smoke frame.
 *
 * @param view offscreen view
 * @param state video-export state
 * @param path PNG output path
 * @return true on success
 */
static bool _write_png(DvzView* view, VideoExportState* state, const char* path)
{
    if (view == NULL || state == NULL || path == NULL)
        return false;

    _fill_points(state, 0.75);
    if (!_upload_points(state))
        return false;
    if (dvz_view_render_once(view) != DVZ_CANVAS_FRAME_READY)
        return false;
    return dvz_view_capture_png(view, path) == 0;
}



/**
 * Record the deterministic animation as a video.
 *
 * @param view offscreen view
 * @param state video-export state
 * @param capture capture configuration
 * @param frame_count number of frames to record
 * @return true on success
 */
static bool _write_video(
    DvzView* view, VideoExportState* state, const DvzAppCaptureConfig* capture,
    uint32_t frame_count)
{
    if (view == NULL || state == NULL || capture == NULL || frame_count == 0)
        return false;

    if (dvz_view_capture_start(view, capture) != 0)
        return false;

    bool ok = true;
    _print_progress(0, frame_count);
    for (uint32_t frame = 0; frame < frame_count; frame++)
    {
        const double t = (double)frame / FPS;
        _fill_points(state, t);
        if (!_upload_points(state) || dvz_view_render_once(view) != DVZ_CANVAS_FRAME_READY)
        {
            ok = false;
            break;
        }
        _print_progress(frame + 1u, frame_count);
    }
    dvz_fprintf(stdout, "\n");
    return dvz_view_capture_stop(view) == 0 && ok;
}



/*************************************************************************************************/
/*  Main                                                                                         */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    if (example_arg_has(argc, argv, "--help") || example_arg_has(argc, argv, "-h"))
    {
        _print_usage();
        return 0;
    }

    const bool png_mode = example_arg_has(argc, argv, "--png");
    const uint32_t frame_count = _frame_count(argc, argv, FRAME_COUNT);

    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;
    VideoExportState state = {0};

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");
    dvz_scene_set_clock_mode(scene, DVZ_SCENE_CLOCK_FIXED_STEP);
    dvz_scene_set_fps(scene, FPS);

    DvzFigure* figure = _create_scene(scene, &state);
    EXAMPLE_CHECK(figure != NULL, "failed to create video export scene");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU?)");

    DvzView* view = dvz_view_offscreen(app, figure, WIDTH, HEIGHT);
    EXAMPLE_CHECK(view != NULL, "dvz_view_offscreen() failed");

    if (png_mode)
    {
        char png_path[PATH_SIZE] = {0};
        example_outpath(argv[0], "video_export.png", png_path, sizeof(png_path));
        EXAMPLE_CHECK(_write_png(view, &state, png_path), "PNG smoke capture failed");
        dvz_fprintf(stdout, "video_export: wrote %s (%ux%u exact pixels)\n", png_path, WIDTH, HEIGHT);
    }
    else
    {
        DvzAppCaptureConfig capture = dvz_app_capture_config();
        capture.flags = DVZ_APP_CAPTURE_VIDEO;
        const char* capture_dir = getenv("DVZ_CAPTURE_DIR");
        const char* capture_basename = getenv("DVZ_CAPTURE_BASENAME");
        capture.directory =
            capture_dir != NULL && capture_dir[0] != '\0' ? capture_dir : ".";
        capture.basename = capture_basename != NULL && capture_basename[0] != '\0'
                               ? capture_basename
                               : "video_export";
        capture.fps = FPS;
        capture.video_capture_mode = DVZ_VIDEO_CAPTURE_CPU_READBACK;

        char video_path[PATH_SIZE] = {0};
        _capture_path(&capture, ".mp4", video_path, sizeof(video_path));
        EXAMPLE_CHECK(
            _write_video(view, &state, &capture, frame_count), "video capture failed");
        dvz_fprintf(
            stdout, "video_export: wrote %s (%u frames at %.0f FPS)\n", video_path, frame_count,
            FPS);
    }
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
