/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* legend_categorical shows how one categorical scale drives both marker colors and a legend.
 *
 * What to look for: five category IDs are assigned labels, colors, and marker shapes, then the
 * marker visual uploads matching position, color, diameter_px, angle, and shape arrays. Compare
 * the marker cloud with the right-side legend entries: the legend is useful because it explains
 * categorical groups without requiring readers to decode colors or shapes from the data alone.
 *
 * Scenario: feature.legend_categorical
 * Style: features, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c features/legend_categorical
 * Run:    ./build/examples/c/features/legend_categorical --live
 * Smoke:  ./build/examples/c/features/legend_categorical --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/scene.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



DvzScenarioSpec dvz_example_legend_categorical_scenario(void);



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT
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
    return dvz_scale_set_categories(scale, categories, CATEGORY_COUNT) == DVZ_OK ? scale : NULL;
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
    style.stroke_width_px = 1.5f;
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
        {.attr_name = "diameter_px", .data = diameters, .item_count = MARKER_COUNT},
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

    return dvz_legend(panel, scale, &desc) != NULL;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the retained categorical legend scenario.
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

    DvzScale* scale = _add_category_scale(ctx->scene);
    if (scale == NULL)
        return false;
    if (!_add_categorized_markers(ctx->scene, panel))
        return false;
    return _add_legend(panel, scale);
}



/**
 * Return the categorical legend scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_example_legend_categorical_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_legend_categorical",
        .title = "Categorical Legend",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_MARKER_VISUAL | DVZ_SCENARIO_REQ_TEXT_VISUAL,
        .init = _scenario_init,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the tentative categorical legend feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_example_legend_categorical_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
