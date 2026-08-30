/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* This example writes a retained scene to a compact JSON diagnostic file.
 *
 * What to look for: the live view is intentionally small, with three point items whose position,
 * color, and diameter_px arrays are uploaded before the scene is serialized. The useful output is
 * also the generated json_export.json file: it should contain the figure/panel/visual structure
 * rather than just pixels. This helps beginners see that Datoviz scenes keep inspectable state that
 * can be exported for debugging or tooling.
 *
 * Scenario: features_json_export
 * Style: features, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c features/json_export
 * Run:    ./build/examples/c/features/json_export --live
 * Smoke:  ./build/examples/c/features/json_export --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "_assertions.h"
#include "datoviz/fileio.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT
#define POINT_COUNT 3u
#define JSON_PATH   "json_export.json"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Add a tiny retained point visual.
 *
 * @param scene scene owning the visual
 * @param panel target panel
 * @return true when the visual was added
 */
static bool _add_points(DvzScene* scene, DvzPanel* panel)
{
    ANN(scene);
    ANN(panel);

    const vec3 positions[POINT_COUNT] = {
        {-0.45f, -0.18f, 0.0f},
        {+0.00f, +0.28f, 0.0f},
        {+0.45f, -0.18f, 0.0f},
    };
    DvzColor colors[POINT_COUNT] = {
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY),
    };
    const float diameters[POINT_COUNT] = {32.0f, 44.0f, 32.0f};

    DvzVisual* point = dvz_point(scene, 0);
    if (point == NULL)
        return false;
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = POINT_COUNT},
        {.attr_name = "color", .data = colors, .item_count = POINT_COUNT},
        {.attr_name = "diameter_px", .data = diameters, .item_count = POINT_COUNT},
    };
    if (dvz_visual_set_data_many(point, updates, 3) != 0)
        return false;
    return dvz_panel_add_visual(panel, point, NULL) == 0;
}



/**
 * Serialize the scene once and save a compact diagnostic artifact.
 *
 * @param scene scene to serialize
 * @return true when serialization returned a plausible JSON document
 */
static bool _serialize_scene(DvzScene* scene)
{
    ANN(scene);

    char* json = dvz_scene_json(scene);
    if (json == NULL)
        return false;

    const size_t length = strlen(json);
    const bool ok = length > 2u && json[0] == '{' && strstr(json, "\"figures\"") != NULL;
    if (ok)
    {
        char path[1024] = {0};
        example_outpath(NULL, JSON_PATH, path, sizeof(path));
        const int rc = dvz_write_bytes(path, "wb", (DvzSize)length, (const uint8_t*)json);
        if (rc == 0)
            fprintf(stdout, "json_export: wrote %s (%zu bytes)\n", path, length);
        else
            fprintf(stderr, "json_export: failed to write %s\n", path);
        dvz_scene_json_destroy(json);
        return rc == 0;
    }

    dvz_scene_json_destroy(json);
    return false;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the scene-JSON feature example.
 *
 * @param ctx scenario context
 * @param out_user scenario state output
 * @return true on success
 */
static bool _scenario_init(DvzScenarioContext* ctx, void** out_user)
{
    if (ctx == NULL)
        return false;
    if (out_user != NULL)
        *out_user = NULL;

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        return false;

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    if (panel == NULL)
        return false;
    example_graphite_cyan_set_panel_background(panel);

    if (!_add_points(ctx->scene, panel))
        return false;
    return _serialize_scene(ctx->scene);
}



/**
 * Return the scene-JSON scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _json_export_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "features_json_export",
        .title = "JSON Export",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .init = _scenario_init,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the scene-JSON feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _json_export_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
