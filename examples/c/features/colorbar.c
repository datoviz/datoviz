/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* colorbar - deterministic scalar image with one retained continuous colorbar.
 *
 * Scenario: colorbar
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/colorbar
 * Run:    ./build/examples/c/features/colorbar
 * Smoke:  ./build/examples/c/features/colorbar 1
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

#define WIDTH        1600u
#define HEIGHT       1200u
#define FIELD_WIDTH  192u
#define FIELD_HEIGHT 144u

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Clamp a float to the unit interval.
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
 * Return one deterministic scalar sample.
 *
 * @param x normalized X coordinate
 * @param y normalized Y coordinate
 * @return normalized scalar value
 */
static float _sample_scalar(float x, float y)
{
    float value = 0.18f + 0.35f * x + 0.28f * y;
    value += 0.09f * sinf(TAU * (1.7f * x + 0.25f * y));
    value += 0.07f * cosf(TAU * (0.4f * x - 2.2f * y));

    const float dx = x - 0.68f;
    const float dy = y - 0.54f;
    value += 0.34f * expf(-(dx * dx + 1.8f * dy * dy) / (2.0f * 0.055f * 0.055f));

    return _clamp01(value);
}



/**
 * Fill the scalar field backing the image.
 *
 * @param values output normalized scalar samples
 */
static void _fill_field(float values[FIELD_WIDTH * FIELD_HEIGHT])
{
    ANN(values);

    for (uint32_t y = 0; y < FIELD_HEIGHT; y++)
    {
        for (uint32_t x = 0; x < FIELD_WIDTH; x++)
        {
            const float u = FIELD_WIDTH > 1u ? (float)x / (float)(FIELD_WIDTH - 1u) : 0.0f;
            const float v = FIELD_HEIGHT > 1u ? (float)y / (float)(FIELD_HEIGHT - 1u) : 0.0f;
            values[y * FIELD_WIDTH + x] = _sample_scalar(u, v);
        }
    }
}



/**
 * Create the continuous scale shared by the image and colorbar.
 *
 * @param scene scene owning scale resources
 * @return created scale, or NULL on failure
 */
static DvzScale* _add_scale(DvzScene* scene)
{
    ANN(scene);

    DvzScale* scale = dvz_scale(
        scene, &(DvzScaleDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleDesc),
                   .kind = DVZ_SCALE_CONTINUOUS,
                   .label = "intensity",
                   .format = {DVZ_STRUCT_INIT_FIELDS(DvzFormatDesc),
                              .precision = 2,
                              .trim_trailing_zeros = true},
               });
    if (scale == NULL)
        return NULL;

    dvz_scale_set_domain(scale, 0.0, 1.0);
    dvz_scale_set_view_range(scale, 0.0, 1.0);

    DvzColormap* colormap = example_graphite_cyan_colormap(scene);
    if (colormap == NULL)
        return NULL;
    dvz_scale_set_colormap(scale, colormap);
    return scale;
}



/**
 * Add one scalar image used as the colorbar's visual referent.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param scale color scale bound to the image
 * @param values scalar field values
 * @return true when the image was added
 */
static bool
_add_scalar_image(DvzScene* scene, DvzPanel* panel, DvzScale* scale, float* values)
{
    ANN(scene);
    ANN(panel);
    ANN(scale);
    ANN(values);

    vec3 data_positions[4] = {
        {0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
    };
    vec3 visual_positions[4] = {{0}};
    vec2 texcoords[4] = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
    };

    int rc = dvz_panel_data_to_visual_positions(
        panel, (const float*)data_positions, (float*)visual_positions, 4);
    if (rc != 0)
        return false;

    DvzVisual* image = dvz_image(scene, 0);
    if (image == NULL)
        return false;
    if (dvz_visual_set_data(image, "position", visual_positions, 4) != 0)
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
                   .depth = 1,
               });
    if (field == NULL)
        return false;
    if (!dvz_sampled_field_set_data(
            field, &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView),
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
 * Run the deterministic retained colorbar feature proof.
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
    float values[FIELD_WIDTH * FIELD_HEIGHT] = {0};

    _fill_field(values);

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel_full(figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel_full() failed");
    example_graphite_cyan_set_panel_background(panel);

    bool ok = dvz_panel_set_layout_reserve(
        panel, &(DvzPanelLayoutReserve){.left = 0.045f, .right = 0.040f, .bottom = 0.055f,
                                        .top = 0.045f});
    EXAMPLE_CHECK(ok, "dvz_panel_set_layout_reserve() failed");
    int rc = dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, 1.0);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_set_domain(x) failed");
    rc = dvz_panel_set_domain(panel, DVZ_DIM_Y, 0.0, 1.0);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_set_domain(y) failed");

    DvzScale* scale = _add_scale(scene);
    EXAMPLE_CHECK(scale != NULL, "adding colorbar scale failed");

    ok = _add_scalar_image(scene, panel, scale, values);
    EXAMPLE_CHECK(ok, "adding scalar image failed");

    DvzColorbar* colorbar = dvz_colorbar(
        panel, scale,
        &(DvzColorbarDesc){DVZ_STRUCT_INIT_FIELDS(DvzColorbarDesc),
            .orientation = DVZ_COLORBAR_ORIENTATION_VERTICAL,
            .anchor = DVZ_SCENE_ANCHOR_PANEL_RIGHT,
            .title = "intensity",
            .reserve_px = 110.0f,
            .ramp_width_px = 28.0f,
            .plot_gap_px = 14.0f,
            .tick_length_px = 6.0f,
            .label_gap_px = 7.0f,
        });
    EXAMPLE_CHECK(colorbar != NULL, "dvz_colorbar() failed");
    dvz_colorbar_set_format(
        colorbar, &(DvzFormatDesc){DVZ_STRUCT_INIT_FIELDS(DvzFormatDesc),
                      .precision = 2,
                      .trim_trailing_zeros = true});

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "colorbar");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    dvz_app_run(app, example_frame_count_any(argc, argv));
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
