/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* scalebar_units - retained scale bar with a custom time-unit string.
 *
 * Scenario: scalebar_units
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/scalebar_units
 * Run:    ./build/examples/c/features/scalebar_units
 * Smoke:  ./build/examples/c/features/scalebar_units 1
 * PNG:    DVZ_CAPTURE=png ./build/examples/c/features/scalebar_units 1
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "_assertions.h"
#include "datoviz/app.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH        1600u
#define HEIGHT       1200u
#define SAMPLE_COUNT 96u

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Copy one Datoviz color into an RGBA8 descriptor field.
 *
 * @param out output RGBA8 array
 * @param color source color
 * @param alpha alpha channel override
 */
static void _copy_color(uint8_t out[4], DvzColor color, uint8_t alpha)
{
    ANN(out);
    out[0] = color.r;
    out[1] = color.g;
    out[2] = color.b;
    out[3] = alpha;
}



/**
 * Add a deterministic time-series trace.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @return true when the visual was added
 */
static bool _add_signal(DvzScene* scene, DvzPanel* panel)
{
    ANN(scene);
    ANN(panel);

    vec3 data_positions[SAMPLE_COUNT] = {{0}};
    vec3 visual_positions[SAMPLE_COUNT] = {{0}};
    DvzColor colors[SAMPLE_COUNT] = {{0}};
    float widths[SAMPLE_COUNT] = {0};

    DvzColor accent = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY);
    for (uint32_t i = 0; i < SAMPLE_COUNT; i++)
    {
        const float t = SAMPLE_COUNT > 1u ? (float)i / (float)(SAMPLE_COUNT - 1u) : 0.0f;
        data_positions[i][0] = 250.0f * t;
        data_positions[i][1] =
            0.35f * sinf(TAU * 2.0f * t) + 0.16f * cosf(TAU * 5.0f * t + 0.4f);
        data_positions[i][2] = 0.0f;
        colors[i] = accent;
        colors[i].a = 230u;
        widths[i] = 3.0f;
    }

    int rc = dvz_panel_data_to_visual_positions(
        panel, (const float*)data_positions, (float*)visual_positions, SAMPLE_COUNT);
    if (rc != 0)
        return false;

    DvzVisual* path = dvz_path(scene, 0);
    if (path == NULL)
        return false;
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = visual_positions, .item_count = SAMPLE_COUNT},
        {.attr_name = "color", .data = colors, .item_count = SAMPLE_COUNT},
        {.attr_name = "stroke_width", .data = widths, .item_count = SAMPLE_COUNT},
    };
    if (dvz_visual_set_data_many(path, updates, 3) != 0)
        return false;
    if (dvz_visual_set_depth_test(path, false) != 0)
        return false;
    return dvz_panel_add_visual(panel, path, NULL) == 0;
}



/**
 * Add one scale bar whose panel X data units are milliseconds.
 *
 * @param scene scene owning unit objects
 * @param panel panel receiving the annotation
 * @return true when the scale bar was added
 */
static bool _add_time_scalebar(DvzScene* scene, DvzPanel* panel)
{
    ANN(scene);
    ANN(panel);

    DvzUnits* duration_units = dvz_units_builtin(scene, DVZ_UNIT_LADDER_DURATION, 0.001);
    if (duration_units == NULL)
        return false;

    DvzColor color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY);
    DvzScaleBarDesc desc = {
        DVZ_STRUCT_INIT_FIELDS(DvzScaleBarDesc),
        .dimension = DVZ_DIM_X,
        .anchor = DVZ_SCENE_ANCHOR_BOTTOM_LEFT,
        .label_position = DVZ_SCALEBAR_LABEL_ABOVE,
        .target_length_px = 240.0f,
        .min_length_px = 170.0f,
        .max_length_px = 310.0f,
        .offset_px = {72.0f, 82.0f},
        .tick_length_px = 18.0f,
        .line_width_px = 4.0f,
        .label_style = {
        DVZ_STRUCT_INIT_FIELDS(DvzTextStyle),
            .size_px = 17.0f,
            .renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS,
        },
    };
    _copy_color(desc.line_color, color, 255u);
    _copy_color(desc.label_style.color, color, 255u);

    DvzAnnotation* scalebar = dvz_annotation_scalebar(panel, &desc);
    return scalebar != NULL && dvz_scalebar_set_units((DvzScaleBar*)scalebar, duration_units) == 0;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the retained scale-bar custom-unit proof.
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

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel_full(figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel_full() failed");
    example_graphite_cyan_set_panel_background(panel);

    bool ok = dvz_panel_set_layout_reserve(
        panel, &(DvzPanelLayoutReserve){.left = 0.08f, .right = 0.08f, .bottom = 0.12f,
                                        .top = 0.08f});
    EXAMPLE_CHECK(ok, "dvz_panel_set_layout_reserve() failed");
    int rc = dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, 250.0);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_set_domain(x) failed");
    rc = dvz_panel_set_domain(panel, DVZ_DIM_Y, -1.0, 1.0);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_set_domain(y) failed");

    ok = _add_signal(scene, panel);
    EXAMPLE_CHECK(ok, "_add_signal() failed");
    ok = _add_time_scalebar(scene, panel);
    EXAMPLE_CHECK(ok, "_add_time_scalebar() failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "scalebar_units");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    DvzPanzoom* panzoom = dvz_view_panzoom(win, panel, NULL);
    EXAMPLE_CHECK(panzoom != NULL, "failed to bind panzoom controller");

    int rc_capture = dvz_view_capture_from_env(win, "scalebar_units");
    EXAMPLE_CHECK(rc_capture == 0, "dvz_view_capture_from_env() failed");

    dvz_app_run(app, example_frame_count(argc, argv));

    rc_capture = dvz_view_capture_stop(win);
    EXAMPLE_CHECK(rc_capture == 0, "dvz_view_capture_stop() failed");
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
