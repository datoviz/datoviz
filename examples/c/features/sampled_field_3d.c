/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* sampled_field_3d - scene-owned 3D sampled field bound to a volume visual.
 *
 * Scenario: feature.sampled_field_3d
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/sampled_field_3d
 * Run:    ./build/examples/c/features/sampled_field_3d
 * Smoke:  ./build/examples/c/features/sampled_field_3d 1
 * PNG:    DVZ_CAPTURE=png ./build/examples/c/features/sampled_field_3d 1
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

#define WIDTH      1600u
#define HEIGHT     1200u
#define FIELD_SIZE 40u



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Convert a normalized scalar to an 8-bit sample.
 *
 * @param value normalized value
 * @return 8-bit value
 */
static uint8_t _u8(float value)
{
    if (value < 0.0f)
        value = 0.0f;
    if (value > 1.0f)
        value = 1.0f;
    return (uint8_t)(255.0f * value + 0.5f);
}



/**
 * Fill a compact scalar volume.
 *
 * @param data output scalar volume
 */
static void _fill_volume(uint8_t data[FIELD_SIZE * FIELD_SIZE * FIELD_SIZE])
{
    for (uint32_t z = 0; z < FIELD_SIZE; z++)
    {
        const float nz = 2.0f * (float)z / (float)(FIELD_SIZE - 1u) - 1.0f;
        for (uint32_t y = 0; y < FIELD_SIZE; y++)
        {
            const float ny = 2.0f * (float)y / (float)(FIELD_SIZE - 1u) - 1.0f;
            for (uint32_t x = 0; x < FIELD_SIZE; x++)
            {
                const float nx = 2.0f * (float)x / (float)(FIELD_SIZE - 1u) - 1.0f;
                const float shell = expf(-10.0f * fabsf(nx * nx + ny * ny + nz * nz - 0.34f));
                const float core = expf(-9.0f * (nx * nx + 1.7f * ny * ny + 0.7f * nz * nz));
                data[(z * FIELD_SIZE + y) * FIELD_SIZE + x] = _u8(0.55f * shell + 0.45f * core);
            }
        }
    }
}



/**
 * Attach a small transfer function to the volume.
 *
 * @param scene scene owning scale resources
 * @param volume volume visual
 * @return true on success
 */
static bool _attach_transfer(DvzScene* scene, DvzVisual* volume)
{
    DvzScale* scale = dvz_scale(
        scene, &(DvzScaleDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleDesc),
                   .kind = DVZ_SCALE_CONTINUOUS});
    if (scale == NULL)
        return false;
    dvz_scale_set_domain(scale, 0.0, 1.0);

    DvzColormap* colormap = dvz_colormap(scene, NULL);
    if (colormap == NULL)
        return false;

    DvzColormapStop stops[5] = {
        {.position = 0.00, .rgba = {12, 16, 24, 255}},
        {.position = 0.24, .rgba = {19, 62, 103, 255}},
        {.position = 0.50, .rgba = {76, 201, 240, 255}},
        {.position = 0.78, .rgba = {128, 255, 219, 255}},
        {.position = 1.00, .rgba = {255, 183, 3, 255}},
    };
    dvz_colormap_set_stops(colormap, stops, 5);
    dvz_scale_set_colormap(scale, colormap);

    DvzVolumeAlphaStop alpha[5] = {
        {.position = 0.00, .alpha = 0.00f},
        {.position = 0.18, .alpha = 0.00f},
        {.position = 0.38, .alpha = 0.26f},
        {.position = 0.70, .alpha = 0.70f},
        {.position = 1.00, .alpha = 0.94f},
    };
    if (dvz_volume_set_alpha_stops(volume, alpha, 5) != 0)
        return false;
    return dvz_visual_set_scale(volume, "color", scale) == 0;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the 3D sampled-field feature example.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    const uint32_t frame_count = example_frame_count_any(argc, argv);
    DvzAppCaptureConfig capture = dvz_app_capture_config_from_env("feature_sampled_field_3d");

    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;
    DvzView* win = NULL;
    uint8_t data[FIELD_SIZE * FIELD_SIZE * FIELD_SIZE] = {0};

    _fill_volume(data);

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel_full(figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel_full() failed");
    example_graphite_cyan_set_panel_background(panel);

    DvzCameraDesc camera = dvz_camera_desc();
    camera.eye[0] = 1.65f;
    camera.eye[1] = -2.85f;
    camera.eye[2] = 1.35f;
    camera.fov_y = 0.68f;
    EXAMPLE_CHECK(dvz_panel_set_camera(panel, &camera), "dvz_panel_set_camera() failed");

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc),
                   .dim = DVZ_FIELD_DIM_3D,
                   .format = DVZ_FIELD_FORMAT_R8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = FIELD_SIZE,
                   .height = FIELD_SIZE,
                   .depth = FIELD_SIZE});
    EXAMPLE_CHECK(field != NULL, "dvz_sampled_field() failed");
    EXAMPLE_CHECK(
        dvz_sampled_field_set_data(
            field, &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView),
                       .data = data,
                       .bytes_per_row = FIELD_SIZE,
                       .rows_per_image = FIELD_SIZE}),
        "dvz_sampled_field_set_data() failed");

    DvzVisual* volume = dvz_volume(scene, 0);
    EXAMPLE_CHECK(volume != NULL, "dvz_volume() failed");
    EXAMPLE_CHECK(dvz_visual_set_field(volume, "field", field), "dvz_visual_set_field() failed");
    EXAMPLE_CHECK(_attach_transfer(scene, volume), "volume transfer setup failed");
    EXAMPLE_CHECK(dvz_volume_set_opacity(volume, 0.82f) == 0, "dvz_volume_set_opacity() failed");
    EXAMPLE_CHECK(dvz_volume_set_step_count(volume, 96u) == 0, "dvz_volume_set_step_count() failed");
    EXAMPLE_CHECK(dvz_panel_add_visual(panel, volume, NULL) == 0, "dvz_panel_add_visual() failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "sampled_field_3d");
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
