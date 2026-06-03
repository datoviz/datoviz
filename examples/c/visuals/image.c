/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* image - deterministic scalar sampled field rendered with the retained image visual.
 *
 * Scenario: visual.image
 * Style: visuals, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c visuals/image
 * Run:    ./build/examples/c/visuals/image
 * Smoke:  ./build/examples/c/visuals/image 1
 * PNG:    DVZ_CAPTURE=png ./build/examples/c/visuals/image 1
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

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
#define FIELD_WIDTH  320u
#define FIELD_HEIGHT 240u

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Clamp a scalar to the unit interval.
 *
 * @param value input value
 * @return clamped value
 */
static float _clamp01(float value)
{
    if (value < 0.0f)
        return 0.0f;
    if (value > 1.0f)
        return 1.0f;
    return value;
}



/**
 * Return a deterministic sampled-field value.
 *
 * @param x normalized X coordinate
 * @param y normalized Y coordinate
 * @return normalized scalar sample
 */
static float _sample_field(float x, float y)
{
    float value = 0.12f;
    value += 0.15f * x + 0.08f * y;
    value += 0.08f * sinf(TAU * (2.4f * x + 0.35f * y));
    value += 0.06f * cosf(TAU * (0.70f * x - 3.2f * y));

    const float ridge = sinf(TAU * (1.15f * x + 0.18f * sinf(TAU * y)));
    value += 0.21f * expf(-20.0f * (ridge - 0.12f) * (ridge - 0.12f));

    const float centers[7][3] = {
        {0.16f, 0.23f, 0.060f}, {0.28f, 0.70f, 0.047f}, {0.44f, 0.42f, 0.036f},
        {0.60f, 0.78f, 0.052f}, {0.72f, 0.52f, 0.043f}, {0.82f, 0.25f, 0.055f},
        {0.88f, 0.68f, 0.038f},
    };
    for (uint32_t i = 0; i < 7u; i++)
    {
        const float dx = x - centers[i][0];
        const float dy = y - centers[i][1];
        const float sigma = centers[i][2];
        const float d2 = (dx * dx + 1.25f * dy * dy) / (2.0f * sigma * sigma);
        value += (0.18f + 0.05f * (float)(i % 3u)) * expf(-d2);
    }

    const float marker_x = x - 0.67f;
    const float marker_y = y - 0.46f;
    value += 0.30f * expf(-(marker_x * marker_x + marker_y * marker_y) / (2.0f * 0.075f * 0.075f));

    return _clamp01(value);
}



/**
 * Fill the scalar sampled field.
 *
 * @param values output normalized scalar field
 */
static void _fill_field(float* values)
{
    ANN(values);

    for (uint32_t y = 0; y < FIELD_HEIGHT; y++)
    {
        for (uint32_t x = 0; x < FIELD_WIDTH; x++)
        {
            const float u = FIELD_WIDTH > 1u ? (float)x / (float)(FIELD_WIDTH - 1u) : 0.0f;
            const float v = FIELD_HEIGHT > 1u ? (float)y / (float)(FIELD_HEIGHT - 1u) : 0.0f;
            values[y * FIELD_WIDTH + x] = _sample_field(u, v);
        }
    }
}



/**
 * Create the graphite/cyan/amber scalar image scale.
 *
 * @param scene scene owning the scale
 * @return created scale, or NULL on failure
 */
static DvzScale* _add_image_scale(DvzScene* scene)
{
    ANN(scene);

    DvzScale* scale = dvz_scale(
        scene,
        &(DvzScaleDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleDesc), .kind = DVZ_SCALE_CONTINUOUS});
    if (scale == NULL)
        return NULL;
    dvz_scale_set_domain(scale, 0.0, 1.0);

    DvzColormap* colormap = dvz_colormap(scene, NULL);
    if (colormap == NULL)
        return NULL;

    DvzColormapStop stops[6] = {
        {.position = 0.00, .rgba = {14, 17, 23, 255}},
        {.position = 0.18, .rgba = {22, 42, 62, 255}},
        {.position = 0.40, .rgba = {35, 124, 165, 255}},
        {.position = 0.64, .rgba = {76, 201, 240, 255}},
        {.position = 0.84, .rgba = {128, 255, 219, 255}},
        {.position = 1.00, .rgba = {255, 183, 3, 255}},
    };
    dvz_colormap_set_stops(colormap, stops, 6);
    dvz_scale_set_colormap(scale, colormap);
    return scale;
}



/**
 * Add the sampled-field image visual to the panel.
 *
 * @param scene scene owning the visual and field
 * @param panel panel receiving the image
 * @param scale scalar color scale
 * @param values scalar field values
 * @return true when the image was added
 */
static bool _add_image(DvzScene* scene, DvzPanel* panel, DvzScale* scale, float* values)
{
    ANN(scene);
    ANN(panel);
    ANN(scale);
    ANN(values);

    vec3 positions[4] = {
        {-0.88f, -0.66f, 0.0f},
        {-0.88f, +0.66f, 0.0f},
        {+0.88f, -0.66f, 0.0f},
        {+0.88f, +0.66f, 0.0f},
    };
    vec2 texcoords[4] = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
    };

    DvzVisual* image = dvz_image(scene, 0);
    if (image == NULL)
        return false;
    if (dvz_visual_set_data(image, "position", positions, 4) != 0)
        return false;
    if (dvz_visual_set_data(image, "texcoords", texcoords, 4) != 0)
        return false;
    if (dvz_visual_set_scale(image, "color", scale) != 0)
        return false;

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc),
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_R32_FLOAT,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = FIELD_WIDTH,
                   .height = FIELD_HEIGHT,
                   .depth = 1,
               });
    if (field == NULL)
        return false;
    if (!dvz_sampled_field_set_data(
            field, &(DvzFieldDataView){
                       DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView),
                       .data = values,
                       .bytes_per_row = FIELD_WIDTH * sizeof(float),
                       .rows_per_image = FIELD_HEIGHT,
                   }))
    {
        return false;
    }
    if (!dvz_visual_set_field(image, "field", field))
        return false;
    if (dvz_visual_set_depth_test(image, false) != 0)
        return false;
    return dvz_panel_add_visual(panel, image, NULL) == 0;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the retained image visual example.
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
    float* values = NULL;
    const uint32_t frame_count = example_frame_count_any(argc, argv);
    DvzAppCaptureConfig capture = dvz_app_capture_config_from_env("visual_image");

    values = (float*)dvz_malloc(FIELD_WIDTH * FIELD_HEIGHT * sizeof(*values));
    EXAMPLE_CHECK(values != NULL, "field allocation failed");
    _fill_field(values);

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel_full(figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel_full() failed");
    example_graphite_cyan_set_panel_background(panel);

    DvzScale* scale = _add_image_scale(scene);
    EXAMPLE_CHECK(scale != NULL, "adding image scale failed");

    bool ok = _add_image(scene, panel, scale, values);
    EXAMPLE_CHECK(ok, "adding image visual failed");
    dvz_free(values);
    values = NULL;

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "visual_image");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

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
    dvz_free(values);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
