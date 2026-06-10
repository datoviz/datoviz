/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* labels - retained labels visual with a small deterministic integer sampled field.
 *
 * Scenario: visual.labels
 * Style: visuals, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c visuals/labels
 * Run:    ./build/examples/c/visuals/labels --live
 * Smoke:  ./build/examples/c/visuals/labels --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "datoviz/scene.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH        1600u
#define HEIGHT       1200u
#define FIELD_WIDTH  512u
#define FIELD_HEIGHT 384u
#define LABEL_COUNT  6u

static const float TAU = 6.28318530718f;

static const DvzCategoryId LABEL_IDS[LABEL_COUNT] = {3, 8, 13, 21, 34, 55};



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_visual_labels_scenario(void);



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct LabelsVisualState
{
    int32_t* labels;
} LabelsVisualState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return a generated integer label for one normalized sample coordinate.
 *
 * @param x normalized X coordinate
 * @param y normalized Y coordinate
 * @return integer label ID, or 0 for background
 */
static int32_t _sample_label(float x, float y)
{
    const float dx = x - 0.50f;
    const float dy = y - 0.50f;
    const float tissue = dx * dx / 0.42f + dy * dy / 0.30f;
    if (tissue > 1.0f)
        return 0;

    const float wave = 0.5f + 0.5f * sinf(TAU * (2.1f * x + 1.4f * y));
    const float band = floorf(3.0f * x + 2.0f * y + 0.8f * wave);
    uint32_t index = (uint32_t)band % LABEL_COUNT;

    const float island_dx = x - 0.68f;
    const float island_dy = y - 0.38f;
    if (island_dx * island_dx + 1.8f * island_dy * island_dy < 0.018f)
        index = 4u;

    return (int32_t)LABEL_IDS[index];
}



/**
 * Fill the deterministic label field.
 *
 * @param labels output signed label field
 */
static void _fill_labels(int32_t labels[FIELD_WIDTH * FIELD_HEIGHT])
{
    ANN(labels);

    for (uint32_t y = 0; y < FIELD_HEIGHT; y++)
    {
        for (uint32_t x = 0; x < FIELD_WIDTH; x++)
        {
            const float u = FIELD_WIDTH > 1u ? (float)x / (float)(FIELD_WIDTH - 1u) : 0.0f;
            const float v = FIELD_HEIGHT > 1u ? (float)y / (float)(FIELD_HEIGHT - 1u) : 0.0f;
            labels[y * FIELD_WIDTH + x] = _sample_label(u, v);
        }
    }
}



/**
 * Create the categorical labels scale.
 *
 * @param scene scene owning the scale
 * @return created scale, or NULL on failure
 */
static DvzScale* _add_labels_scale(DvzScene* scene)
{
    ANN(scene);

    DvzScale* scale = dvz_scale(
        scene, &(DvzScaleDesc){
                   DVZ_STRUCT_INIT_FIELDS(DvzScaleDesc),
                   .kind = DVZ_SCALE_CATEGORICAL,
                   .label = "labels",
               });
    if (scale == NULL)
        return NULL;

    const ExampleStyleColorRole roles[LABEL_COUNT] = {
        EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,
        EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
        EXAMPLE_STYLE_COLOR_WARNING,
        EXAMPLE_STYLE_COLOR_ERROR,
        EXAMPLE_STYLE_COLOR_TEXT,
        EXAMPLE_STYLE_COLOR_MINOR_TICK,
    };
    DvzScaleCategory categories[LABEL_COUNT] = {0};
    for (uint32_t i = 0; i < LABEL_COUNT; i++)
    {
        categories[i].category_id = LABEL_IDS[i];
        categories[i].order = i;
        categories[i].label = "region";
        categories[i].color = example_graphite_cyan_color(roles[i]);
        categories[i].color.a = 220;
    }
    return dvz_scale_set_categories(scale, categories, LABEL_COUNT) ? scale : NULL;
}



/**
 * Add the retained labels visual to the panel.
 *
 * @param scene scene owning the visual and field
 * @param panel panel receiving the visual
 * @param scale categorical label scale
 * @param labels signed label field
 * @return true when the labels visual was added
 */
static bool _add_labels(
    DvzScene* scene, DvzPanel* panel, DvzScale* scale, int32_t labels[FIELD_WIDTH * FIELD_HEIGHT])
{
    ANN(scene);
    ANN(panel);
    ANN(scale);
    ANN(labels);

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc),
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_R32_SINT,
                   .semantic = DVZ_FIELD_SEMANTIC_LABEL,
                   .width = FIELD_WIDTH,
                   .height = FIELD_HEIGHT,
                   .depth = 1,
               });
    if (field == NULL)
        return false;
    if (!dvz_sampled_field_set_data(
            field, &(DvzFieldDataView){
                       DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView),
                       .data = labels,
                       .bytes_per_row = FIELD_WIDTH * sizeof(int32_t),
                       .rows_per_image = FIELD_HEIGHT,
                   }))
    {
        return false;
    }

    DvzVisual* visual = dvz_labels(scene, 0);
    if (visual == NULL)
        return false;

    vec3 positions[1] = {{0.0f, 0.0f, 0.0f}};
    vec2 extents[1] = {{1.72f, 1.28f}};
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = 1},
        {.attr_name = "extent", .data = extents, .item_count = 1},
    };
    if (dvz_visual_set_data_many(visual, updates, 2) != 0)
        return false;
    if (!dvz_visual_set_field(visual, "field", field))
        return false;
    if (dvz_visual_set_scale(visual, "labels", scale) != 0)
        return false;
    if (dvz_labels_set_opacity(visual, 0.92f) != 0)
        return false;
    if (dvz_labels_set_background(visual, 0) != 0)
        return false;
    if (dvz_labels_set_boundary(
            visual, true, 1.5f, example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_PANEL_BG)) != 0)
    {
        return false;
    }
    if (dvz_visual_set_depth_test(visual, false) != 0)
        return false;
    if (dvz_visual_set_alpha_mode(visual, DVZ_ALPHA_BLENDED) != 0)
        return false;
    return dvz_panel_add_visual(panel, visual, NULL) == 0;
}



/**
 * Free the retained label field.
 *
 * @param state labels visual state
 */
static void _free_state(LabelsVisualState* state)
{
    if (state == NULL)
        return;
    dvz_free(state->labels);
    dvz_free(state);
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the retained labels visual scenario.
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

    LabelsVisualState* state = (LabelsVisualState*)dvz_calloc(1, sizeof(*state));
    if (state == NULL)
        return false;

    state->labels = (int32_t*)dvz_calloc(FIELD_WIDTH * FIELD_HEIGHT, sizeof(*state->labels));
    if (state->labels == NULL)
        goto error;
    _fill_labels(state->labels);

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        goto error;

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    if (panel == NULL)
        goto error;
    example_graphite_cyan_set_panel_background(panel);

    DvzScale* labels_scale = _add_labels_scale(ctx->scene);
    if (labels_scale == NULL)
        goto error;
    if (!_add_labels(ctx->scene, panel, labels_scale, state->labels))
        goto error;

    DvzPanzoom* panzoom = dvz_scenario_panzoom(ctx, panel, NULL, DVZ_DIM_MASK_XY);
    if (panzoom == NULL)
        goto error;

    if (out_user != NULL)
        *out_user = state;
    return true;

error:
    _free_state(state);
    return false;
}



/**
 * Destroy the retained labels visual scenario state.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    _free_state((LabelsVisualState*)user);
}



/**
 * Return the labels visual scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_visual_labels_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "visual_labels",
        .title = "visual_labels",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .init = _scenario_init,
        .destroy = _scenario_destroy,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the retained labels visual example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_visual_labels_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
