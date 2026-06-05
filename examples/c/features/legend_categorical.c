/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* legend_categorical - tentative retained categorical legend proof.
 *
 * Scenario: feature.legend_categorical
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/legend_categorical
 * Run:    ./build/examples/c/features/legend_categorical
 * Smoke:  ./build/examples/c/features/legend_categorical 1
 * PNG:    DVZ_CAPTURE=png ./build/examples/c/features/legend_categorical 1
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/app.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH          1600u
#define HEIGHT         1200u
#define CATEGORY_COUNT 5u
#define MARKER_COUNT   18u

static const DvzCategoryId CATEGORY_IDS[CATEGORY_COUNT] = {10, 20, 30, 40, 50};
static const char* CATEGORY_LABELS[CATEGORY_COUNT] = {
    "sample A", "sample B", "control", "review", "outlier"};
static const uint32_t CATEGORY_SHAPES[CATEGORY_COUNT] = {
    DVZ_MARKER_SHAPE_DISC, DVZ_MARKER_SHAPE_SQUARE, DVZ_MARKER_SHAPE_TRIANGLE,
    DVZ_MARKER_SHAPE_DIAMOND, DVZ_MARKER_SHAPE_RING};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return the style color role for one category index.
 *
 * @param index category index
 * @return style color role
 */
static ExampleStyleColorRole _category_role(uint32_t index)
{
    static const ExampleStyleColorRole roles[CATEGORY_COUNT] = {
        EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,
        EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
        EXAMPLE_STYLE_COLOR_WARNING,
        EXAMPLE_STYLE_COLOR_TEXT,
        EXAMPLE_STYLE_COLOR_ERROR,
    };
    return roles[index % CATEGORY_COUNT];
}



/**
 * Create the categorical scale that drives the legend entries.
 *
 * @param scene scene owning the scale
 * @return created scale, or NULL on failure
 */
static DvzScale* _add_category_scale(DvzScene* scene)
{
    if (scene == NULL)
        return NULL;

    DvzScale* scale = dvz_scale(
        scene, &(DvzScaleDesc){
                   DVZ_STRUCT_INIT_FIELDS(DvzScaleDesc),
                   .kind = DVZ_SCALE_CATEGORICAL,
                   .label = "groups",
               });
    if (scale == NULL)
        return NULL;

    DvzScaleCategory categories[CATEGORY_COUNT] = {0};
    for (uint32_t i = 0; i < CATEGORY_COUNT; i++)
    {
        categories[i].category_id = CATEGORY_IDS[i];
        categories[i].order = i;
        categories[i].label = CATEGORY_LABELS[i];
        categories[i].color = example_graphite_cyan_color(_category_role(i));
        categories[i].color.a = 232u;
    }
    return dvz_scale_set_categories(scale, categories, CATEGORY_COUNT) ? scale : NULL;
}



/**
 * Add a deterministic categorized marker cloud using the same category colors as the legend.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @return true when the visual was added
 */
static bool _add_categorized_markers(DvzScene* scene, DvzPanel* panel)
{
    if (scene == NULL || panel == NULL)
        return false;

    DvzVisual* marker = dvz_marker(scene, 0);
    if (marker == NULL)
        return false;

    DvzMarkerStyle style = dvz_marker_style();
    style.aspect = DVZ_SHAPE_ASPECT_FILLED;
    style.stroke_width = 1.5f;
    DvzColor edge = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_PANEL_BG);
    style.edge_color = dvz_color_rgba(edge.r, edge.g, edge.b, 220);
    if (dvz_marker_set_style(marker, &style) != 0)
        return false;

    vec3 positions[MARKER_COUNT] = {0};
    DvzColor colors[MARKER_COUNT] = {0};
    float diameters[MARKER_COUNT] = {0};
    float angles[MARKER_COUNT] = {0};
    uint32_t shapes[MARKER_COUNT] = {0};

    for (uint32_t i = 0; i < MARKER_COUNT; i++)
    {
        const uint32_t category = i % CATEGORY_COUNT;
        const uint32_t row = i / 6u;
        const uint32_t col = i % 6u;
        positions[i][0] = -0.78f + 0.31f * (float)col + 0.035f * (float)(row % 2u);
        positions[i][1] = -0.50f + 0.48f * (float)row;
        positions[i][2] = 0.0f;
        colors[i] = example_graphite_cyan_color(_category_role(category));
        colors[i].a = 232u;
        diameters[i] = category == 3u ? 62.0f : 52.0f;
        angles[i] = 0.22f * (float)i;
        shapes[i] = CATEGORY_SHAPES[category];
    }

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = MARKER_COUNT},
        {.attr_name = "color", .data = colors, .item_count = MARKER_COUNT},
        {.attr_name = "diameter", .data = diameters, .item_count = MARKER_COUNT},
        {.attr_name = "angle", .data = angles, .item_count = MARKER_COUNT},
        {.attr_name = "shape", .data = shapes, .item_count = MARKER_COUNT},
    };
    if (dvz_visual_set_data_many(marker, updates, 5) != 0)
        return false;
    if (dvz_visual_set_depth_test(marker, false) != 0)
        return false;
    return dvz_panel_add_visual(panel, marker, NULL) == 0;
}



/**
 * Add a right-edge categorical legend.
 *
 * @param panel panel receiving the legend
 * @param scale categorical scale
 * @return true when the legend was added
 */
static bool _add_legend(DvzPanel* panel, DvzScale* scale)
{
    if (panel == NULL || scale == NULL)
        return false;

    DvzLegendDesc desc = dvz_legend_desc();
    desc.placement_mode = DVZ_LEGEND_PLACEMENT_ATTACHED;
    desc.anchor = DVZ_SCENE_ANCHOR_PANEL_RIGHT;
    desc.title = "groups";
    desc.reserve_px = 230.0f;
    desc.edge_offset_px = 26.0f;
    desc.plot_gap_px = 20.0f;
    desc.mark_size_px = 18.0f;
    desc.entry_gap_px = 13.0f;
    desc.mark_label_gap_px = 12.0f;

    DvzLegend* legend = dvz_legend(panel, scale, &desc);
    if (legend == NULL)
        return false;

    return dvz_legend_set_highlight(legend, CATEGORY_IDS[3]);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the tentative categorical legend feature example.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    const uint32_t frame_count = example_frame_count_any(argc, argv);
    DvzAppCaptureConfig capture = dvz_app_capture_config_from_env("feature_legend_categorical");

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
        panel, &(DvzPanelLayoutReserve){.left = 0.055f, .right = 0.020f, .bottom = 0.065f,
                                        .top = 0.045f});
    EXAMPLE_CHECK(ok, "dvz_panel_set_layout_reserve() failed");

    DvzScale* scale = _add_category_scale(scene);
    EXAMPLE_CHECK(scale != NULL, "categorical scale setup failed");
    EXAMPLE_CHECK(_add_categorized_markers(scene, panel), "categorized marker setup failed");
    EXAMPLE_CHECK(_add_legend(panel, scale), "categorical legend setup failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "legend_categorical");
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
