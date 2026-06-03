/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* annotation_readout - retained label annotation anchored to data.
 *
 * Scenario: annotation_readout
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/annotation_readout
 * Run:    ./build/examples/c/features/annotation_readout
 * Smoke:  ./build/examples/c/features/annotation_readout 1
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "_assertions.h"
#include "_compat.h"
#include "datoviz/app.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH       1600u
#define HEIGHT      1200u
#define POINT_COUNT 96u

static const float TAU = 6.28318530718f;
static const uint32_t READOUT_INDEX = 61u;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Fill deterministic point positions in data coordinates.
 *
 * @param positions output data-space positions
 * @param colors output point colors
 * @param diameters output point diameters
 */
static void _fill_points(
    vec3 positions[POINT_COUNT], DvzColor colors[POINT_COUNT], float diameters[POINT_COUNT])
{
    ANN(positions);
    ANN(colors);
    ANN(diameters);

    const DvzColor muted = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_GRID);
    const DvzColor accent = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY);
    for (uint32_t i = 0; i < POINT_COUNT; i++)
    {
        const float t = POINT_COUNT > 1u ? (float)i / (float)(POINT_COUNT - 1u) : 0.0f;
        const float x = 10.0f * t;
        const float y = 0.45f * sinf(TAU * 1.7f * t) + 0.22f * cosf(TAU * 4.0f * t + 0.2f);

        positions[i][0] = x;
        positions[i][1] = y;
        positions[i][2] = 0.0f;

        colors[i] = muted;
        colors[i].a = 190u;
        diameters[i] = 8.0f;
    }

    colors[READOUT_INDEX] = accent;
    colors[READOUT_INDEX].a = 255u;
    diameters[READOUT_INDEX] = 16.0f;
}



/**
 * Add one point visual used as an anchor target for the annotation.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param data_positions data-space positions
 * @param colors point colors
 * @param diameters point diameters
 * @return true when the visual was added
 */
static bool _add_points(
    DvzScene* scene, DvzPanel* panel, vec3 data_positions[POINT_COUNT],
    const DvzColor colors[POINT_COUNT], const float diameters[POINT_COUNT])
{
    ANN(scene);
    ANN(panel);
    ANN(data_positions);
    ANN(colors);
    ANN(diameters);

    vec3 visual_positions[POINT_COUNT] = {{0}};
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

    DvzPointStyleDesc style = dvz_point_style_desc();
    style.aspect = DVZ_SHAPE_ASPECT_FILLED;
    style.stroke_width = 0.0f;
    if (dvz_point_set_style(points, &style) != 0)
        return false;

    return dvz_panel_add_visual(panel, points, NULL) == 0;
}



/**
 * Convert one data-space point to a panel-local pixel anchor.
 *
 * @param panel target panel
 * @param x data X coordinate
 * @param y data Y coordinate
 * @param out_px output panel-local pixel anchor
 * @return true when conversion succeeded
 */
static bool _data_to_panel_pixel(DvzPanel* panel, double x, double y, float out_px[2])
{
    ANN(panel);
    ANN(out_px);

    DvzRect plot = {0};
    if (!dvz_panel_plot_rect_px(panel, &plot) || plot.width <= 0.0f || plot.height <= 0.0f)
        return false;

    const double tx = x / 10.0;
    const double ty = (y + 1.0) / 2.0;
    out_px[0] = plot.x + (float)tx * plot.width;
    out_px[1] = plot.y + (1.0f - (float)ty) * plot.height;
    return true;
}



/**
 * Add one retained label annotation at the highlighted data point.
 *
 * @param panel panel receiving the annotation
 * @param position highlighted data-space position
 * @return created annotation, or NULL on failure
 */
static DvzAnnotation* _add_readout(DvzPanel* panel, const vec3 position)
{
    ANN(panel);

    DvzColor text = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT);
    DvzTextStyle style = dvz_text_style();
    style.size_px = 15.0f;
    style.renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS;
    style.color[0] = text.r;
    style.color[1] = text.g;
    style.color[2] = text.b;
    style.color[3] = 255u;

    float anchor_px[2] = {0};
    if (!_data_to_panel_pixel(panel, position[0], position[1], anchor_px))
        return NULL;

    DvzTextPlacement placement = dvz_text_placement();
    placement.mode = DVZ_TEXT_PLACEMENT_SCREEN;
    placement.anchor = DVZ_SCENE_ANCHOR_SCREEN;
    placement.position[0] = anchor_px[0];
    placement.position[1] = anchor_px[1];
    placement.position[2] = position[2];
    placement.offset[0] = 20.0f;
    placement.offset[1] = -20.0f;
    placement.text_anchor[0] = 0.0f;
    placement.text_anchor[1] = 0.5f;
    placement.has_text_anchor = true;
    placement.depth_test = false;

    char label[128] = {0};
    int n = dvz_snprintf(label, sizeof(label), "peak  x %.2f  y %.2f", position[0], position[1]);
    if (n <= 0 || (size_t)n >= sizeof(label))
        return NULL;

    return dvz_annotation_label(
        panel, &(DvzLabelDesc){DVZ_STRUCT_INIT_FIELDS(DvzLabelDesc),
                   .text = label,
                   .style = style,
                   .placement = placement,
               });
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the deterministic retained annotation readout feature proof.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    const uint32_t frame_count = example_frame_count_any(argc, argv);
    DvzAppCaptureConfig capture = dvz_app_capture_config_from_env("feature_annotation_readout");

    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;
    vec3 data_positions[POINT_COUNT] = {{0}};
    DvzColor colors[POINT_COUNT] = {{0}};
    float diameters[POINT_COUNT] = {0};

    _fill_points(data_positions, colors, diameters);

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel_full(figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel_full() failed");
    example_graphite_cyan_set_panel_background(panel);

    bool ok = dvz_panel_set_layout_reserve(
        panel, &(DvzPanelLayoutReserve){.left = 0.09f, .right = 0.06f, .bottom = 0.10f,
                                        .top = 0.06f});
    EXAMPLE_CHECK(ok, "dvz_panel_set_layout_reserve() failed");
    int rc = dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, 10.0);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_set_domain(x) failed");
    rc = dvz_panel_set_domain(panel, DVZ_DIM_Y, -1.0, 1.0);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_set_domain(y) failed");

    ok = _add_points(scene, panel, data_positions, colors, diameters);
    EXAMPLE_CHECK(ok, "adding readout points failed");

    DvzAnnotation* readout = _add_readout(panel, data_positions[READOUT_INDEX]);
    EXAMPLE_CHECK(readout != NULL, "dvz_annotation_label() failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "annotation_readout");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    EXAMPLE_CHECK(
        example_run_with_capture(app, win, frame_count, &capture),
        "example_run_with_capture() failed");
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
