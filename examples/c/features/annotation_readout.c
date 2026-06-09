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
 * Run:    ./build/examples/c/features/annotation_readout --live
 * Smoke:  ./build/examples/c/features/annotation_readout --png
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
#include "datoviz/scene.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



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
    style.size_px = 24.0f;
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
    placement.offset[0] = 10.0f;
    placement.offset[1] = -10.0f;
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
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the deterministic retained annotation readout scenario.
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

    vec3 data_positions[POINT_COUNT] = {{0}};
    DvzColor colors[POINT_COUNT] = {{0}};
    float diameters[POINT_COUNT] = {0};

    _fill_points(data_positions, colors, diameters);

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        return false;

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    if (panel == NULL)
        return false;
    example_graphite_cyan_set_panel_background(panel);

    if (!dvz_panel_set_layout_reserve(
        panel, &(DvzPanelLayoutReserve){.left = 0.09f, .right = 0.06f, .bottom = 0.10f,
                                        .top = 0.06f}))
        return false;
    if (dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, 10.0) != 0)
        return false;
    if (dvz_panel_set_domain(panel, DVZ_DIM_Y, -1.0, 1.0) != 0)
        return false;

    if (!_add_points(ctx->scene, panel, data_positions, colors, diameters))
        return false;

    DvzAnnotation* readout = _add_readout(panel, data_positions[READOUT_INDEX]);
    return readout != NULL;
}



/**
 * Return the annotation readout scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _annotation_readout_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_annotation_readout",
        .title = "annotation_readout",
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
 * Run the deterministic retained annotation readout feature proof through the native scenario
 * runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _annotation_readout_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
