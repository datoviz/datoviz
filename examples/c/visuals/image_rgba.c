/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* image_rgba - This example displays a generated RGBA image as a 2D sampled field.
 *
 * What to look for: the FIELD_WIDTH by FIELD_HEIGHT RGBA8 array is attached directly as a color
 * sampled field. The image contains deterministic red, green, blue, and alpha variation, so the
 * translucent band blends with the graphite panel background while the opaque regions keep their
 * source colors.
 *
 * Scenario: visuals_image_rgba
 * Style: visuals, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c visuals/image_rgba
 * Run:    ./build/examples/c/visuals/image_rgba --live
 * Smoke:  ./build/examples/c/visuals/image_rgba --png
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

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT
#define FIELD_WIDTH  384u
#define FIELD_HEIGHT 256u

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_visual_image_rgba_scenario(void);



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct RgbaImageVisualState
{
    uint8_t* pixels;
} RgbaImageVisualState;



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
 * Convert a normalized scalar to an 8-bit channel.
 *
 * @param value normalized input
 * @return 8-bit channel value
 */
static uint8_t _u8(float value)
{
    return (uint8_t)(255.0f * _clamp01(value) + 0.5f);
}



/**
 * Fill the RGBA sampled field.
 *
 * @param pixels output RGBA8 image
 */
static void _fill_rgba(uint8_t* pixels)
{
    ANN(pixels);

    for (uint32_t y = 0; y < FIELD_HEIGHT; y++)
    {
        for (uint32_t x = 0; x < FIELD_WIDTH; x++)
        {
            const float u = FIELD_WIDTH > 1u ? (float)x / (float)(FIELD_WIDTH - 1u) : 0.0f;
            const float v = FIELD_HEIGHT > 1u ? (float)y / (float)(FIELD_HEIGHT - 1u) : 0.0f;
            const float cx = u - 0.52f;
            const float cy = v - 0.50f;
            const float radius = sqrtf(cx * cx + cy * cy);
            const float ring = 0.5f + 0.5f * sinf(TAU * (9.0f * radius - 0.18f * u));
            const float weave = 0.5f + 0.5f * cosf(TAU * (2.4f * u + 1.7f * v));
            const float tile = ((x / 24u + y / 24u) % 2u) == 0u ? 0.08f : -0.03f;

            uint8_t* px = &pixels[4u * (y * FIELD_WIDTH + x)];
            px[0] = _u8(0.16f + 0.72f * u + 0.12f * ring + tile);
            px[1] = _u8(0.18f + 0.55f * (1.0f - v) + 0.22f * weave - tile);
            px[2] = _u8(0.34f + 0.40f * ring + 0.18f * sinf(TAU * (u - 0.3f * v)));

            const float diagonal = fabsf((u + 0.23f * sinf(TAU * v)) - (0.30f + 0.55f * v));
            float alpha = 0.98f;
            alpha -= 0.46f * expf(-diagonal * diagonal / (2.0f * 0.036f * 0.036f));
            alpha -= 0.28f * expf(-radius * radius / (2.0f * 0.18f * 0.18f));
            px[3] = _u8(alpha);
        }
    }
}



/**
 * Add the RGBA image visual to the panel.
 *
 * @param scene scene owning the visual and field
 * @param panel panel receiving the image
 * @param pixels RGBA8 field values
 * @return true when the image was added
 */
static bool _add_image(DvzScene* scene, DvzPanel* panel, uint8_t* pixels)
{
    ANN(scene);
    ANN(panel);
    ANN(pixels);

    vec3 positions[4] = {
        {-0.88f, -0.64f, 0.0f},
        {-0.88f, +0.64f, 0.0f},
        {+0.88f, -0.64f, 0.0f},
        {+0.88f, +0.64f, 0.0f},
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

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc),
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_RGBA8_UNORM,
                   .semantic = DVZ_FIELD_SEMANTIC_COLOR,
                   .width = FIELD_WIDTH,
                   .height = FIELD_HEIGHT,
                   .depth = 1,
               });
    if (field == NULL)
        return false;
    if (dvz_sampled_field_set_data(
            field, &(DvzFieldDataView){
                       DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView),
                       .data = pixels,
                       .bytes_per_row = FIELD_WIDTH * 4u,
                       .rows_per_image = FIELD_HEIGHT,
                   }) != DVZ_OK)
    {
        return false;
    }
    if (dvz_visual_set_field(image, "field", field) != DVZ_OK)
        return false;
    if (dvz_visual_set_depth_test(image, false) != 0)
        return false;
    if (dvz_visual_set_alpha_mode(image, DVZ_ALPHA_BLENDED) != 0)
        return false;
    return dvz_panel_add_visual(panel, image, NULL) == 0;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the retained RGBA image visual scenario.
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

    RgbaImageVisualState* state = (RgbaImageVisualState*)dvz_malloc(sizeof(*state));
    if (state == NULL)
        return false;
    state->pixels = NULL;

    state->pixels = (uint8_t*)dvz_malloc(FIELD_WIDTH * FIELD_HEIGHT * 4u);
    if (state->pixels == NULL)
    {
        dvz_free(state);
        return false;
    }
    _fill_rgba(state->pixels);

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        goto error;

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    if (panel == NULL)
        goto error;
    example_graphite_cyan_set_panel_background(panel);

    if (!_add_image(ctx->scene, panel, state->pixels))
        goto error;

    if (out_user != NULL)
        *out_user = state;
    return true;

error:
    dvz_free(state->pixels);
    dvz_free(state);
    return false;
}



/**
 * Destroy the retained RGBA image visual scenario state.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;

    RgbaImageVisualState* state = (RgbaImageVisualState*)user;
    if (state == NULL)
        return;
    dvz_free(state->pixels);
    dvz_free(state);
}



/**
 * Return the RGBA image visual scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_visual_image_rgba_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "visuals_image_rgba",
        .title = "RGBA Image",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_IMAGE_VISUAL,
        .init = _scenario_init,
        .destroy = _scenario_destroy,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the retained RGBA image visual example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_visual_image_rgba_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
