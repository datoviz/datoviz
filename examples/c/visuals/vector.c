/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* vector - retained straight and curved vector visual example.
 *
 * Scenario: visual.vector
 * Style: visuals, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c visuals/vector
 * Run:    ./build/examples/c/visuals/vector
 * Smoke:  ./build/examples/c/visuals/vector 1
 * PNG:    DVZ_CAPTURE=png ./build/examples/c/visuals/vector 1
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

#define WIDTH              1600u
#define HEIGHT             1200u
#define FIELD_COLS         19u
#define FIELD_ROWS         11u
#define FIELD_COUNT        (FIELD_COLS * FIELD_ROWS)
#define CURVE_COUNT        5u
#define CURVE_POINT_COUNT  40u
#define CURVE_SAMPLE_COUNT (CURVE_COUNT * CURVE_POINT_COUNT)

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return a deterministic vector palette color.
 *
 * @param index color index
 * @param alpha output alpha
 * @return palette color
 */
static DvzColor _vector_color(uint32_t index, uint8_t alpha)
{
    DvzColor color = example_graphite_cyan_color(
        index % 3u == 0u   ? EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY
        : index % 3u == 1u ? EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY
                           : EXAMPLE_STYLE_COLOR_WARNING);
    color.a = alpha;
    return color;
}



/**
 * Fill a deterministic straight vector field.
 *
 * @param positions output vector anchor positions
 * @param vectors output vector displacements
 * @param colors output per-vector colors
 * @param widths output per-vector stroke widths
 */
static void _fill_straight_vectors(
    vec3 positions[FIELD_COUNT], vec3 vectors[FIELD_COUNT], DvzColor colors[FIELD_COUNT],
    float widths[FIELD_COUNT])
{
    ANN(positions);
    ANN(vectors);
    ANN(colors);
    ANN(widths);

    uint32_t idx = 0;
    for (uint32_t row = 0; row < FIELD_ROWS; row++)
    {
        const float v = FIELD_ROWS > 1u ? (float)row / (float)(FIELD_ROWS - 1u) : 0.0f;
        for (uint32_t col = 0; col < FIELD_COLS; col++)
        {
            const float u = FIELD_COLS > 1u ? (float)col / (float)(FIELD_COLS - 1u) : 0.0f;
            const float x = -0.86f + 1.72f * u;
            const float y = -0.66f + 1.32f * v;
            const float swirl = atan2f(y, x) + 0.34f * sinf(TAU * (u - 0.5f * v));
            const float wave = 0.5f + 0.5f * sinf(TAU * (0.7f * u + 0.45f * v));
            const float length = 0.052f + 0.050f * wave;

            positions[idx][0] = x;
            positions[idx][1] = y;
            positions[idx][2] = 0.0f;
            vectors[idx][0] = length * cosf(swirl);
            vectors[idx][1] = length * sinf(swirl);
            vectors[idx][2] = 0.0f;
            colors[idx] = _vector_color(row + col, 230);
            widths[idx] = 2.2f + 1.7f * wave;
            idx++;
        }
    }
}



/**
 * Fill curved vector paths.
 *
 * @param positions output path positions
 * @param colors output path colors
 * @param widths output path stroke widths
 * @param subpaths output subpath lengths
 */
static void _fill_curved_vectors(
    vec3 positions[CURVE_SAMPLE_COUNT], DvzColor colors[CURVE_SAMPLE_COUNT],
    float widths[CURVE_SAMPLE_COUNT], uint32_t subpaths[CURVE_COUNT])
{
    ANN(positions);
    ANN(colors);
    ANN(widths);
    ANN(subpaths);

    for (uint32_t curve = 0; curve < CURVE_COUNT; curve++)
    {
        subpaths[curve] = CURVE_POINT_COUNT;
        const float lane = -0.62f + 0.31f * (float)curve;
        const float phase = 0.13f + 0.11f * (float)curve;
        const float amp = 0.065f + 0.014f * (float)curve;
        const DvzColor color = _vector_color(curve, 210);

        for (uint32_t point = 0; point < CURVE_POINT_COUNT; point++)
        {
            const uint32_t dst = curve * CURVE_POINT_COUNT + point;
            const float t = CURVE_POINT_COUNT > 1u
                                ? (float)point / (float)(CURVE_POINT_COUNT - 1u)
                                : 0.0f;
            const float x = -0.90f + 1.80f * t;
            const float envelope = sinf(0.5f * TAU * t);
            const float y = lane + amp * envelope * sinf(TAU * (1.18f * t + phase));

            positions[dst][0] = x;
            positions[dst][1] = y;
            positions[dst][2] = 0.0f;
            colors[dst] = color;
            widths[dst] = 3.4f + 1.3f * envelope;
        }
    }
}



/**
 * Add a straight vector field visual.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param positions vector anchor positions
 * @param vectors vector displacements
 * @param colors per-vector colors
 * @param widths per-vector stroke widths
 * @return true when the visual was added
 */
static bool _add_straight_vectors(
    DvzScene* scene, DvzPanel* panel, vec3 positions[FIELD_COUNT], vec3 vectors[FIELD_COUNT],
    DvzColor colors[FIELD_COUNT], float widths[FIELD_COUNT])
{
    ANN(scene);
    ANN(panel);
    ANN(positions);
    ANN(vectors);
    ANN(colors);
    ANN(widths);

    DvzVisual* vector = dvz_vector(scene, 0);
    if (vector == NULL)
        return false;

    DvzVectorStyle style = dvz_vector_style();
    style.anchor = DVZ_VECTOR_ANCHOR_CENTER;
    style.end_cap = DVZ_SEGMENT_CAP_TRIANGLE_OUT;
    if (dvz_vector_set_style(vector, &style) != 0)
        return false;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = FIELD_COUNT},
        {.attr_name = "vector", .data = vectors, .item_count = FIELD_COUNT},
        {.attr_name = "color", .data = colors, .item_count = FIELD_COUNT},
        {.attr_name = "stroke_width", .data = widths, .item_count = FIELD_COUNT},
    };
    if (dvz_visual_set_data_many(vector, updates, 4) != 0)
        return false;
    if (dvz_visual_set_depth_test(vector, false) != 0)
        return false;

    return dvz_panel_add_visual(panel, vector, NULL) == 0;
}



/**
 * Add curved vector paths.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param positions path positions
 * @param colors path colors
 * @param widths path stroke widths
 * @param subpaths path lengths
 * @return true when the visual was added
 */
static bool _add_curved_vectors(
    DvzScene* scene, DvzPanel* panel, vec3 positions[CURVE_SAMPLE_COUNT],
    DvzColor colors[CURVE_SAMPLE_COUNT], float widths[CURVE_SAMPLE_COUNT],
    uint32_t subpaths[CURVE_COUNT])
{
    ANN(scene);
    ANN(panel);
    ANN(positions);
    ANN(colors);
    ANN(widths);
    ANN(subpaths);

    DvzVisual* vector = dvz_vector(scene, 0);
    if (vector == NULL)
        return false;

    DvzVectorStyle style = dvz_vector_style();
    style.start_cap = DVZ_SEGMENT_CAP_ROUND;
    style.end_cap = DVZ_SEGMENT_CAP_TRIANGLE_OUT;
    if (dvz_vector_set_style(vector, &style) != 0)
        return false;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = CURVE_SAMPLE_COUNT},
        {.attr_name = "color", .data = colors, .item_count = CURVE_SAMPLE_COUNT},
        {.attr_name = "stroke_width", .data = widths, .item_count = CURVE_SAMPLE_COUNT},
    };
    if (dvz_visual_set_data_many(vector, updates, 3) != 0)
        return false;
    if (dvz_vector_set_subpaths(vector, CURVE_COUNT, subpaths) != 0)
        return false;
    if (dvz_visual_set_alpha_mode(vector, DVZ_ALPHA_BLENDED) != 0)
        return false;
    if (dvz_visual_set_depth_test(vector, false) != 0)
        return false;

    return dvz_panel_add_visual(
               panel, vector,
               &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc), .z_layer = 1}) ==
           0;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the retained vector visual example.
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
    const uint32_t frame_count = example_frame_count_any(argc, argv);
    DvzAppCaptureConfig capture = dvz_app_capture_config_from_env("visual_vector");

    vec3 straight_positions[FIELD_COUNT] = {{0}};
    vec3 straight_vectors[FIELD_COUNT] = {{0}};
    DvzColor straight_colors[FIELD_COUNT] = {{0}};
    float straight_widths[FIELD_COUNT] = {0};
    vec3 curve_positions[CURVE_SAMPLE_COUNT] = {{0}};
    DvzColor curve_colors[CURVE_SAMPLE_COUNT] = {{0}};
    float curve_widths[CURVE_SAMPLE_COUNT] = {0};
    uint32_t subpaths[CURVE_COUNT] = {0};

    _fill_straight_vectors(straight_positions, straight_vectors, straight_colors, straight_widths);
    _fill_curved_vectors(curve_positions, curve_colors, curve_widths, subpaths);

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel_full(figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel_full() failed");
    example_graphite_cyan_set_panel_background(panel);

    EXAMPLE_CHECK(
        _add_straight_vectors(
            scene, panel, straight_positions, straight_vectors, straight_colors, straight_widths),
        "adding straight vector visual failed");
    EXAMPLE_CHECK(
        _add_curved_vectors(scene, panel, curve_positions, curve_colors, curve_widths, subpaths),
        "adding curved vector visual failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "visual_vector");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    DvzPanzoom* panzoom = dvz_view_panzoom(win, panel, NULL);
    EXAMPLE_CHECK(panzoom != NULL, "failed to create or bind panzoom controller");

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
