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
 * Run:    ./build/examples/c/visuals/labels
 * Smoke:  ./build/examples/c/visuals/labels 1
 * PNG:    DVZ_CAPTURE=png ./build/examples/c/visuals/labels 1
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "_alloc.h"
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
#define FIELD_WIDTH  128u
#define FIELD_HEIGHT 96u
#define LABEL_COUNT  6u

static const float TAU = 6.28318530718f;

static const DvzCategoryId LABEL_IDS[LABEL_COUNT] = {3, 8, 13, 21, 34, 55};



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

    DvzScaleCategory categories[LABEL_COUNT] = {
        {
            .category_id = 3,
            .order = 0,
            .label = "region 3",
            .color = {76, 201, 240, 210},
        },
        {
            .category_id = 8,
            .order = 1,
            .label = "region 8",
            .color = {128, 255, 219, 210},
        },
        {
            .category_id = 13,
            .order = 2,
            .label = "region 13",
            .color = {255, 183, 3, 210},
        },
        {
            .category_id = 21,
            .order = 3,
            .label = "region 21",
            .color = {239, 71, 111, 210},
        },
        {
            .category_id = 34,
            .order = 4,
            .label = "region 34",
            .color = {201, 209, 217, 210},
        },
        {
            .category_id = 55,
            .order = 5,
            .label = "region 55",
            .color = {47, 141, 196, 210},
        },
    };
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
    return dvz_panel_add_visual(panel, visual, NULL) == 0;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the retained labels visual example.
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
    DvzView* win = NULL;
    bool capture_started = false;
    int32_t* labels = NULL;
    const uint32_t frame_count = example_frame_count_any(argc, argv);
    DvzAppCaptureConfig capture = dvz_app_capture_config_from_env("visual_labels");

    labels = (int32_t*)dvz_calloc(FIELD_WIDTH * FIELD_HEIGHT, sizeof(*labels));
    EXAMPLE_CHECK(labels != NULL, "labels allocation failed");
    _fill_labels(labels);

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel_full(figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel_full() failed");
    example_graphite_cyan_set_panel_background(panel);

    DvzScale* labels_scale = _add_labels_scale(scene);
    EXAMPLE_CHECK(labels_scale != NULL, "labels scale setup failed");
    EXAMPLE_CHECK(_add_labels(scene, panel, labels_scale, labels), "labels visual setup failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "visual_labels");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    DvzPanzoom* panzoom = dvz_view_panzoom(win, panel, NULL);
    EXAMPLE_CHECK(panzoom != NULL, "failed to create or bind panzoom controller");

    int rc = dvz_view_capture_start(win, &capture);
    EXAMPLE_CHECK(rc == 0, "dvz_view_capture_start() failed");
    capture_started = true;

    dvz_app_run(app, frame_count);

    rc = dvz_view_capture_stop(win);
    EXAMPLE_CHECK(rc == 0, "dvz_view_capture_stop() failed");
    capture_started = false;
    ret = 0;

cleanup:
    if (capture_started && win != NULL)
        (void)dvz_view_capture_stop(win);
    if (app != NULL)
        dvz_app_destroy(app);
    dvz_free(labels);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
