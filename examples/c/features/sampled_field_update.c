/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* This example reuses one sampled scalar field in two image visuals while updating patches.
 *
 * What to look for: both panels sample the same R32_FLOAT field and the same moving highlighted
 * patch, but each panel applies a different continuous color scale. Every few frames the previous
 * 12x12 patch is restored and a new region is uploaded with dvz_sampled_field_update_region().
 * Compare the two panels during live playback; shared fields are useful when several views need
 * different color treatments of the same changing image data.
 *
 * Scenario: features_sampled_field_update
 * Style: features, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c features/sampled_field_update
 * Run:    ./build/examples/c/features/sampled_field_update --live
 * Smoke:  ./build/examples/c/features/sampled_field_update --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "_alloc.h"
#include "datoviz/scene.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_example_sampled_field_update_scenario(void);



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT
#define FIELD_WIDTH   96u
#define FIELD_HEIGHT  72u
#define PATCH_SIZE    12u
#define PATCH_COUNT   8u
#define UPDATE_STRIDE 10u

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct SampledFieldUpdateState
{
    DvzSampledField* field;
    float values[FIELD_WIDTH * FIELD_HEIGHT];
    uint32_t patch_index;
    uint32_t patch_x;
    uint32_t patch_y;
} SampledFieldUpdateState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Clamp a scalar to [0, 1].
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
 * Return the deterministic background value for one field sample.
 *
 * @param x sample x coordinate
 * @param y sample y coordinate
 * @return normalized scalar value
 */
static float _background_value(uint32_t x, uint32_t y)
{
    const float u = (float)x / (float)(FIELD_WIDTH - 1u);
    const float v = (float)y / (float)(FIELD_HEIGHT - 1u);
    float value = 0.16f + 0.20f * u + 0.10f * v;
    value += 0.12f * sinf(TAU * (1.8f * u + 0.35f * v));
    value += 0.08f * cosf(TAU * (0.45f * u - 2.4f * v));
    value += 0.22f * expf(-10.0f * ((u - 0.66f) * (u - 0.66f) + (v - 0.35f) * (v - 0.35f)));
    return _clamp01(value);
}



/**
 * Return one patch location from the bounded motion path.
 *
 * @param index patch path index
 * @param out_x output sample-space x coordinate
 * @param out_y output sample-space y coordinate
 */
static void _patch_position(uint32_t index, uint32_t* out_x, uint32_t* out_y)
{
    static const uint32_t path[PATCH_COUNT][2] = {
        {10u, 8u},
        {42u, 8u},
        {74u, 10u},
        {72u, 30u},
        {58u, 52u},
        {30u, 50u},
        {12u, 36u},
        {24u, 20u},
    };
    index %= PATCH_COUNT;
    if (out_x != NULL)
        *out_x = path[index][0];
    if (out_y != NULL)
        *out_y = path[index][1];
}



/**
 * Fill one patch payload with background values.
 *
 * @param patch output patch payload
 * @param x0 region x origin
 * @param y0 region y origin
 */
static void _fill_background_patch(float patch[PATCH_SIZE * PATCH_SIZE], uint32_t x0, uint32_t y0)
{
    for (uint32_t y = 0; y < PATCH_SIZE; y++)
    {
        for (uint32_t x = 0; x < PATCH_SIZE; x++)
            patch[y * PATCH_SIZE + x] = _background_value(x0 + x, y0 + y);
    }
}



/**
 * Fill one highlighted patch payload.
 *
 * @param patch output patch payload
 * @param phase animation phase
 */
static void _fill_highlight_patch(float patch[PATCH_SIZE * PATCH_SIZE], float phase)
{
    const float pulse = 0.82f + 0.12f * sinf(phase);
    for (uint32_t y = 0; y < PATCH_SIZE; y++)
    {
        const float vy = 2.0f * (float)y / (float)(PATCH_SIZE - 1u) - 1.0f;
        for (uint32_t x = 0; x < PATCH_SIZE; x++)
        {
            const float ux = 2.0f * (float)x / (float)(PATCH_SIZE - 1u) - 1.0f;
            const float falloff = expf(-2.4f * (ux * ux + vy * vy));
            patch[y * PATCH_SIZE + x] = _clamp01(0.42f + pulse * falloff);
        }
    }
}



/**
 * Upload one patch into the scene-owned sampled field.
 *
 * @param field sampled field to update
 * @param x sample-space x coordinate
 * @param y sample-space y coordinate
 * @param patch patch payload
 * @return true on success
 */
static bool _update_patch(
    DvzSampledField* field, uint32_t x, uint32_t y, const float patch[PATCH_SIZE * PATCH_SIZE])
{
    if (field == NULL || patch == NULL)
        return false;

    return dvz_sampled_field_update_region(
               field,
               (DvzFieldRegion){
                   .x = x,
                   .y = y,
                   .z = 0,
                   .width = PATCH_SIZE,
                   .height = PATCH_SIZE,
                   .depth = 1,
               },
               &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView),
                   .data = patch,
                   .bytes_per_row = PATCH_SIZE * sizeof(float),
                   .rows_per_image = PATCH_SIZE,
               }) == DVZ_OK;
}



/**
 * Fill the initial full field payload with a highlighted patch.
 *
 * @param state sampled-field example state
 */
static void _fill_initial_field(SampledFieldUpdateState* state)
{
    for (uint32_t y = 0; y < FIELD_HEIGHT; y++)
    {
        for (uint32_t x = 0; x < FIELD_WIDTH; x++)
            state->values[y * FIELD_WIDTH + x] = _background_value(x, y);
    }

    _patch_position(0, &state->patch_x, &state->patch_y);
    float patch[PATCH_SIZE * PATCH_SIZE] = {0};
    _fill_highlight_patch(patch, 0.0f);
    for (uint32_t y = 0; y < PATCH_SIZE; y++)
    {
        for (uint32_t x = 0; x < PATCH_SIZE; x++)
            state->values[(state->patch_y + y) * FIELD_WIDTH + state->patch_x + x] =
                patch[y * PATCH_SIZE + x];
    }
}



/**
 * Create one continuous scalar color scale.
 *
 * @param scene scene owning the scale resources
 * @param name colormap name
 * @param colors colormap colors
 * @param color_count number of colors
 * @return created scale, or NULL on error
 */
static DvzScale* _add_scale(
    DvzScene* scene, const char* name, const DvzColor* colors, uint32_t color_count)
{
    DvzColormap* colormap = dvz_colormap_custom(scene, name, colors, color_count);
    if (colormap == NULL)
        return NULL;

    DvzScale* scale = dvz_scale(
        scene, &(DvzScaleDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleDesc),
                   .kind = DVZ_SCALE_CONTINUOUS});
    if (scale == NULL)
        return NULL;

    dvz_scale_set_domain(scale, 0.0, 1.0);
    dvz_scale_set_colormap(scale, colormap);
    return scale;
}



/**
 * Add one image visual bound to an existing sampled field.
 *
 * @param scene scene owning visuals
 * @param panel target panel
 * @param field sampled field reused by the visual
 * @param scale color scale used by this visual
 * @return true on success
 */
static bool _add_field_image(
    DvzScene* scene, DvzPanel* panel, DvzSampledField* field, DvzScale* scale)
{
    const vec3 positions[4] = {
        {-0.88f, -0.66f, 0.0f},
        {-0.88f, +0.66f, 0.0f},
        {+0.88f, -0.66f, 0.0f},
        {+0.88f, +0.66f, 0.0f},
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
    if (dvz_visual_set_field(image, "field", field) != DVZ_OK)
        return false;
    if (dvz_visual_set_depth_test(image, false) != 0)
        return false;
    return dvz_panel_add_visual(panel, image, NULL) == 0;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the sampled-field update/reuse scenario.
 *
 * @param ctx scenario context
 * @param out_user scenario state output
 * @return true on success
 */
static bool _scenario_init(DvzScenarioContext* ctx, void** out_user)
{
    if (ctx == NULL || out_user == NULL)
        return false;

    SampledFieldUpdateState* state =
        (SampledFieldUpdateState*)dvz_calloc(1, sizeof(SampledFieldUpdateState));
    if (state == NULL)
        return false;

    _fill_initial_field(state);

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        goto error;

    DvzGrid* grid = dvz_figure_grid(ctx->figure, 1, 2);
    if (grid == NULL)
        goto error;
    if (dvz_grid_set_margins(
            grid,
            &(DvzPanelReserve){
                .left_px = 64.0f, .right_px = 64.0f, .top_px = 64.0f, .bottom_px = 64.0f}) != DVZ_OK)
        goto error;
    if (dvz_grid_set_gutter(grid, 34.0f, 0.0f) != DVZ_OK)
        goto error;

    DvzPanel* panels[2] = {dvz_grid_panel(grid, 0, 0), dvz_grid_panel(grid, 0, 1)};
    for (uint32_t i = 0; i < 2; i++)
    {
        if (panels[i] == NULL)
            goto error;
        example_graphite_cyan_set_panel_background(panels[i]);

        DvzPanelBorderDesc border = dvz_panel_border_desc();
        border.color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_GRID);
        border.width_px = 1.25f;
        if (dvz_panel_set_border(panels[i], &border) != DVZ_OK)
            goto error;
    }

    state->field = dvz_sampled_field(
        ctx->scene, &(DvzSampledFieldDesc){DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc),
                        .dim = DVZ_FIELD_DIM_2D,
                        .format = DVZ_FIELD_FORMAT_R32_FLOAT,
                        .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                        .width = FIELD_WIDTH,
                        .height = FIELD_HEIGHT,
                        .depth = 1});
    if (state->field == NULL)
        goto error;
    if (dvz_sampled_field_set_data(
            state->field, &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView),
                              .data = state->values,
                              .bytes_per_row = FIELD_WIDTH * sizeof(float),
                              .rows_per_image = FIELD_HEIGHT}) != DVZ_OK)
        goto error;

    DvzColor left_colors[] = {
        dvz_color_rgb(14, 17, 23),
        dvz_color_rgb(25, 79, 118),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING),
    };
    DvzColor right_colors[] = {
        dvz_color_rgb(14, 17, 23),
        dvz_color_rgb(20, 58, 64),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING),
    };
    DvzScale* left_scale =
        _add_scale(ctx->scene, "sampled_field_update_cyan", left_colors, 5);
    DvzScale* right_scale =
        _add_scale(ctx->scene, "sampled_field_update_warm", right_colors, 5);
    if (left_scale == NULL || right_scale == NULL)
        goto error;

    if (!_add_field_image(ctx->scene, panels[0], state->field, left_scale))
        goto error;
    if (!_add_field_image(ctx->scene, panels[1], state->field, right_scale))
        goto error;

    *out_user = state;
    return true;

error:
    dvz_free(state);
    return false;
}



/**
 * Advance the sampled-field subregion update.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_frame(DvzScenarioContext* ctx, void* user)
{
    SampledFieldUpdateState* state = (SampledFieldUpdateState*)user;
    if (ctx == NULL || state == NULL || state->field == NULL)
        return;

    const uint32_t frame_index =
        ctx->preview_mode ? (uint32_t)ctx->preview_frame_index + 1u
                          : (uint32_t)ctx->frame_index + 1u;
    uint32_t update_stride = UPDATE_STRIDE;
    if (ctx->preview_mode)
    {
        const double frames_per_live_update =
            dvz_scenario_preview_fps(ctx) * (double)UPDATE_STRIDE / 60.0;
        update_stride = (uint32_t)fmax(1.0, round(frames_per_live_update));
    }
    if (frame_index % update_stride != 0)
        return;

    float patch[PATCH_SIZE * PATCH_SIZE] = {0};
    _fill_background_patch(patch, state->patch_x, state->patch_y);
    if (!_update_patch(state->field, state->patch_x, state->patch_y, patch))
        return;

    state->patch_index = (state->patch_index + 1u) % PATCH_COUNT;
    _patch_position(state->patch_index, &state->patch_x, &state->patch_y);
    const float phase = ctx->preview_mode ? (float)(60.0 * 0.12 * dvz_scenario_preview_time(ctx))
                                          : (float)frame_index * 0.12f;
    _fill_highlight_patch(patch, phase);
    (void)_update_patch(state->field, state->patch_x, state->patch_y, patch);
}



/**
 * Destroy the sampled-field update scenario state.
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
 * Return the sampled-field update scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_example_sampled_field_update_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "features_sampled_field_update",
        .title = "Sampled Field Update",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_IMAGE_VISUAL,
        .init = _scenario_init,
        .frame = _scenario_frame,
        .destroy = _scenario_destroy,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the sampled-field update feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_example_sampled_field_update_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
