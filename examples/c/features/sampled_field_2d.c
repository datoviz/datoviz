/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* sampled_field_2d - scene-owned 2D sampled field bound to an image visual.
 *
 * Scenario: feature.sampled_field_2d
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/sampled_field_2d
 * Run:    ./build/examples/c/features/sampled_field_2d
 * Smoke:  ./build/examples/c/features/sampled_field_2d 1
 * PNG:    DVZ_CAPTURE=png ./build/examples/c/features/sampled_field_2d 1
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "datoviz/app.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH        1600u
#define HEIGHT       1200u
#define FIELD_WIDTH  96u
#define FIELD_HEIGHT 72u

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Fill a deterministic scalar field in [0, 1].
 *
 * @param values output scalar field
 */
static void _fill_field(float values[FIELD_WIDTH * FIELD_HEIGHT])
{
    for (uint32_t y = 0; y < FIELD_HEIGHT; y++)
    {
        const float v = (float)y / (float)(FIELD_HEIGHT - 1u);
        for (uint32_t x = 0; x < FIELD_WIDTH; x++)
        {
            const float u = (float)x / (float)(FIELD_WIDTH - 1u);
            const float wave = 0.5f + 0.5f * sinf(TAU * (2.0f * u + 1.4f * v));
            const float basin = expf(-7.0f * ((u - 0.64f) * (u - 0.64f) + (v - 0.42f) * (v - 0.42f)));
            values[y * FIELD_WIDTH + x] = 0.62f * wave + 0.38f * basin;
        }
    }
}



/**
 * Create the continuous scale used by the image visual.
 *
 * @param scene scene owning the scale
 * @return scale object, or NULL on error
 */
static DvzScale* _add_scale(DvzScene* scene)
{
    DvzScale* scale = dvz_scale(
        scene, &(DvzScaleDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleDesc),
                   .kind = DVZ_SCALE_CONTINUOUS});
    if (scale == NULL)
        return NULL;
    dvz_scale_set_domain(scale, 0.0, 1.0);

    DvzColormap* colormap = dvz_colormap(scene, NULL);
    if (colormap == NULL)
        return NULL;

    DvzColormapStop stops[5] = {
        {.position = 0.00, .rgba = {16, 23, 34, 255}},
        {.position = 0.26, .rgba = {36, 92, 128, 255}},
        {.position = 0.52, .rgba = {76, 201, 240, 255}},
        {.position = 0.78, .rgba = {128, 255, 219, 255}},
        {.position = 1.00, .rgba = {255, 183, 3, 255}},
    };
    dvz_colormap_set_stops(colormap, stops, 5);
    dvz_scale_set_colormap(scale, colormap);
    return scale;
}



/**
 * Add one image visual backed by a sampled field.
 *
 * @param scene scene owning objects
 * @param panel panel receiving the image
 * @param scale scalar color scale
 * @param values field values
 * @return true on success
 */
static bool _add_field_image(
    DvzScene* scene, DvzPanel* panel, DvzScale* scale, const float* values)
{
    const vec3 positions[4] = {
        {-0.82f, -0.62f, 0.0f},
        {-0.82f, +0.62f, 0.0f},
        {+0.82f, -0.62f, 0.0f},
        {+0.82f, +0.62f, 0.0f},
    };
    const vec2 texcoords[4] = {
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
        scene, &(DvzSampledFieldDesc){DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc),
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_R32_FLOAT,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = FIELD_WIDTH,
                   .height = FIELD_HEIGHT,
                   .depth = 1});
    if (field == NULL)
        return false;
    if (!dvz_sampled_field_set_data(
            field, &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView),
                       .data = values,
                       .bytes_per_row = FIELD_WIDTH * sizeof(float),
                       .rows_per_image = FIELD_HEIGHT}))
        return false;
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
 * Run the 2D sampled-field feature example.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    const uint32_t frame_count = example_frame_count_any(argc, argv);
    DvzAppCaptureConfig capture = dvz_app_capture_config_from_env("feature_sampled_field_2d");

    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;
    DvzView* win = NULL;
    float values[FIELD_WIDTH * FIELD_HEIGHT] = {0};

    _fill_field(values);

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel_full(figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel_full() failed");
    example_graphite_cyan_set_panel_background(panel);

    DvzScale* scale = _add_scale(scene);
    EXAMPLE_CHECK(scale != NULL, "scale setup failed");
    EXAMPLE_CHECK(_add_field_image(scene, panel, scale, values), "sampled-field image setup failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "sampled_field_2d");
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
