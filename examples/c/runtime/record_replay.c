/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* record_replay - This example records the DRP2 frame stream from an offscreen view and replays it
 * from a DVZR recording.
 *
 * What to look for: the recorded scene is a five-point visual with explicit position, color, and
 * diameter arrays. The app records the backend-neutral rendering commands produced for the frame,
 * not a video or editable scene file. It writes the DVZR recording plus two PNGs, one captured from
 * the original offscreen view and one captured from the replay view; compare those images to verify
 * that replaying the command stream reproduces the same rendered frame.
 *
 * This experimental workflow is useful when you need visual regression artifacts or a compact trace
 * for renderer debugging without keeping the original app process alive.
 *
 * Scenario: runtime_record_replay
 * Style: runtime, native app
 *
 * Build:  just example-c runtime/record_replay
 * Run:    ./build/examples/c/runtime/record_replay
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "datoviz/app.h"
#include "datoviz/canvas/enums.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_OUTPUT_WIDTH
#define HEIGHT EXAMPLE_OUTPUT_HEIGHT
#define POINT_COUNT 5u



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Build the retained scene that will be recorded.
 *
 * @param scene target scene
 * @return figure containing the scene, or NULL
 */
static DvzFigure* _record_replay_figure(DvzScene* scene)
{
    if (scene == NULL)
        return NULL;

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    DvzPanel* panel = figure != NULL ? dvz_panel_full(figure) : NULL;
    DvzVisual* point = dvz_point(scene, 0);
    if (figure == NULL || panel == NULL || point == NULL)
        return NULL;
    example_graphite_cyan_set_panel_background(panel);

    vec3 positions[POINT_COUNT] = {
        {-0.70f, -0.45f, 0.0f}, {-0.35f, +0.20f, 0.0f}, {+0.00f, -0.05f, 0.0f},
        {+0.35f, +0.42f, 0.0f}, {+0.70f, -0.25f, 0.0f},
    };
    DvzColor colors[POINT_COUNT] = {
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_MINOR_TICK),
    };
    float diameters[POINT_COUNT] = {34.0f, 46.0f, 58.0f, 46.0f, 34.0f};
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = POINT_COUNT},
        {.attr_name = "color", .data = colors, .item_count = POINT_COUNT},
        {.attr_name = "diameter_px", .data = diameters, .item_count = POINT_COUNT},
    };
    if (dvz_visual_set_data_many(point, updates, 3) != 0)
        return NULL;
    if (dvz_panel_add_visual(panel, point, NULL) != 0)
        return NULL;
    return figure;
}



/*************************************************************************************************/
/*  Main                                                                                         */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    (void)argc;

    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;

    char recording_path[512] = {0};
    char original_png[512] = {0};
    char replay_png[512] = {0};
    example_outpath(argv[0], "record_replay.dvzr", recording_path, sizeof(recording_path));
    example_outpath(argv[0], "record_replay_original.png", original_png, sizeof(original_png));
    example_outpath(argv[0], "record_replay_replay.png", replay_png, sizeof(replay_png));

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");
    DvzFigure* figure = _record_replay_figure(scene);
    EXAMPLE_CHECK(figure != NULL, "failed to build recorded figure");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU?)");

    DvzView* record_view = dvz_view_offscreen(app, figure, WIDTH, HEIGHT);
    EXAMPLE_CHECK(record_view != NULL, "dvz_view_offscreen(record) failed");
    EXAMPLE_CHECK(
        dvz_view_record_start(record_view, recording_path) == 0, "dvz_view_record_start() failed");
    EXAMPLE_CHECK(
        dvz_view_render_once(record_view) == DVZ_CANVAS_FRAME_READY, "record render failed");
    EXAMPLE_CHECK(dvz_view_record_stop(record_view) == 0, "dvz_view_record_stop() failed");
    EXAMPLE_CHECK(
        dvz_view_capture_png(record_view, original_png) == 0, "failed to capture recorded frame");

    DvzFigure* replay_figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(replay_figure != NULL, "dvz_figure(replay) failed");
    DvzView* replay_view = dvz_view_offscreen(app, replay_figure, WIDTH, HEIGHT);
    EXAMPLE_CHECK(replay_view != NULL, "dvz_view_offscreen(replay) failed");
    EXAMPLE_CHECK(
        dvz_view_replay_start(replay_view, recording_path) == 0, "dvz_view_replay_start() failed");
    dvz_view_replay_set_paced(replay_view, false);
    EXAMPLE_CHECK(dvz_view_replay_frame_count(replay_view) > 0, "replay has no frames");
    EXAMPLE_CHECK(
        dvz_view_render_once(replay_view) == DVZ_CANVAS_FRAME_READY, "replay render failed");
    EXAMPLE_CHECK(dvz_view_replay_stop(replay_view) == 0, "dvz_view_replay_stop() failed");
    EXAMPLE_CHECK(
        dvz_view_capture_png(replay_view, replay_png) == 0, "failed to capture replay frame");

    dvz_fprintf(stdout, "record_replay: wrote %s\n", recording_path);
    dvz_fprintf(stdout, "record_replay: wrote %s\n", original_png);
    dvz_fprintf(stdout, "record_replay: wrote %s\n", replay_png);
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
