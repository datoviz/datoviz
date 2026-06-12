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
 * Run:    ./build/examples/c/features/sampled_field_3d --live
 * Smoke:  ./build/examples/c/features/sampled_field_3d --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "_alloc.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH      1600u
#define HEIGHT     1200u
#define FIELD_SIZE 40u



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct SampledField3DState
{
    uint8_t data[FIELD_SIZE * FIELD_SIZE * FIELD_SIZE];
} SampledField3DState;



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
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the 3D sampled-field scenario.
 *
 * @param ctx scenario context
 * @param out_user scenario state output
 * @return true on success
 */
static bool _scenario_init(DvzScenarioContext* ctx, void** out_user)
{
    if (ctx == NULL || out_user == NULL)
        return false;

    SampledField3DState* state =
        (SampledField3DState*)dvz_calloc(1, sizeof(SampledField3DState));
    if (state == NULL)
        return false;

    _fill_volume(state->data);

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        goto error;

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    if (panel == NULL)
        goto error;
    example_graphite_cyan_set_panel_background(panel);

    if (example_set_default_3d_camera(panel, 1.0f) == NULL)
        goto error;

    DvzSampledField* field = dvz_sampled_field(
        ctx->scene, &(DvzSampledFieldDesc){DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc),
                        .dim = DVZ_FIELD_DIM_3D,
                        .format = DVZ_FIELD_FORMAT_R8_UNORM,
                        .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                        .width = FIELD_SIZE,
                        .height = FIELD_SIZE,
                        .depth = FIELD_SIZE});
    if (field == NULL)
        goto error;
    if (!dvz_sampled_field_set_data(
            field, &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView),
                       .data = state->data,
                       .bytes_per_row = FIELD_SIZE,
                       .rows_per_image = FIELD_SIZE}))
        goto error;

    DvzVisual* volume = dvz_volume(ctx->scene, 0);
    if (volume == NULL)
        goto error;
    if (!dvz_visual_set_field(volume, "field", field))
        goto error;
    if (!_attach_transfer(ctx->scene, volume))
        goto error;
    if (dvz_volume_set_opacity(volume, 0.82f) != 0)
        goto error;
    if (dvz_volume_set_step_count(volume, 96u) != 0)
        goto error;
    if (dvz_panel_add_visual(panel, volume, NULL) != 0)
        goto error;

    *out_user = state;
    return true;

error:
    dvz_free(state);
    return false;
}



/**
 * Destroy the 3D sampled-field scenario.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    dvz_free(user);
}



/**
 * Return the sampled-field 3D scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _sampled_field_3d_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_sampled_field_3d",
        .title = "sampled_field_3d",
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
 * Run the 3D sampled-field feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _sampled_field_3d_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
