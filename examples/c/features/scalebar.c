/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* scalebar - minimal retained scale bar attached to one 2D panel.
 *
 * Scenario: scale_bar
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/scalebar
 * Run:    ./build/examples/c/features/scalebar
 * Smoke:  ./build/examples/c/features/scalebar 1
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

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

#define WIDTH       1600u
#define HEIGHT      1200u
#define POINT_COUNT 5u



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
 * Add a small ruler-like point visual in data coordinates.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @return true when the visual was added
 */
static bool _add_points(DvzScene* scene, DvzPanel* panel)
{
    ANN(scene);
    ANN(panel);

    vec3 data_positions[POINT_COUNT] = {
        {0.0f, 0.0f, 0.0f},
        {2.0f, 0.0f, 0.0f},
        {4.0f, 0.0f, 0.0f},
        {6.0f, 0.0f, 0.0f},
        {8.0f, 0.0f, 0.0f},
    };
    vec3 visual_positions[POINT_COUNT] = {{0}};
    DvzColor colors[POINT_COUNT] = {{0}};
    float diameters[POINT_COUNT] = {0};

    DvzColor muted = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_GRID);
    DvzColor accent = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY);
    for (uint32_t i = 0; i < POINT_COUNT; i++)
    {
        colors[i] = i == 0 || i == POINT_COUNT - 1u ? accent : muted;
        colors[i].a = 230u;
        diameters[i] = i == 0 || i == POINT_COUNT - 1u ? 16.0f : 10.0f;
    }

    int rc = dvz_panel_data_to_visual_positions(
        panel, (const float*)data_positions, (float*)visual_positions, POINT_COUNT);
    if (rc != 0)
        return false;

    DvzVisual* points = dvz_point(scene, 0);
    if (points == NULL)
        return false;
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = visual_positions, .item_count = POINT_COUNT},
        {.attr_name = "color", .data = colors, .item_count = POINT_COUNT},
        {.attr_name = "diameter", .data = diameters, .item_count = POINT_COUNT},
    };
    if (dvz_visual_set_data_many(points, updates, 3) != 0)
        return false;
    if (dvz_visual_set_depth_test(points, false) != 0)
        return false;
    return dvz_panel_add_visual(panel, points, NULL) == 0;
}



/**
 * Add one retained 2D scale bar in panel data coordinates.
 *
 * @param panel panel receiving the annotation
 * @return true when the scale bar was added
 */
static bool _add_scalebar(DvzPanel* panel)
{
    ANN(panel);

    DvzColor color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT);
    DvzScaleBarDesc desc = {
        DVZ_STRUCT_INIT_FIELDS(DvzScaleBarDesc),
        .dimension = DVZ_DIM_X,
        .anchor = DVZ_SCENE_ANCHOR_BOTTOM_LEFT,
        .label_position = DVZ_SCALEBAR_LABEL_ABOVE,
        .target_length_px = 180.0f,
        .min_length_px = 120.0f,
        .max_length_px = 240.0f,
        .offset_px = {34.0f, 34.0f},
        .tick_length_px = 9.0f,
        .line_width_px = 2.0f,
        .unit = "m",
        .data_to_unit = 0.001,
        .label_style = {
        DVZ_STRUCT_INIT_FIELDS(DvzTextStyle),
            .size_px = 17.0f,
            .renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS,
        },
    };
    _copy_color(desc.line_color, color, 255u);
    _copy_color(desc.label_style.color, color, 255u);

    DvzAnnotation* scalebar = dvz_annotation_scalebar(panel, &desc);
    return scalebar != NULL;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the minimal retained scale-bar feature proof.
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
    int rc = dvz_panel_set_domain(panel, DVZ_DIM_X, -1.0, 9.0);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_set_domain(x) failed");
    rc = dvz_panel_set_domain(panel, DVZ_DIM_Y, -1.0, 1.0);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_set_domain(y) failed");

    ok = _add_points(scene, panel);
    EXAMPLE_CHECK(ok, "_add_points() failed");
    ok = _add_scalebar(panel);
    EXAMPLE_CHECK(ok, "_add_scalebar() failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "scalebar");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    DvzPanzoom* panzoom = dvz_view_panzoom(win, panel, NULL);
    EXAMPLE_CHECK(panzoom != NULL, "failed to bind panzoom controller");

    dvz_app_run(app, example_frame_count(argc, argv));
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
