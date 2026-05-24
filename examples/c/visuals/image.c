/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* image — live sampled-field partial-update smoke example.
 *
 * Opens a GLFW window showing a scalar sampled field through an image visual and colormap. A bright
 * patch moves around the field by calling dvz_sampled_field_update_region() from the app frame
 * callback, exercising retained texture subregion uploads through the live scene -> DRP2 -> app
 * path.
 *
 * Build:  just example-c visuals/image
 * Run:    ./build/examples/c/visuals/image
 * Smoke:  ./build/examples/c/visuals/image 300
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "datoviz/app.h"
#include "datoviz/scene.h"
#include "example_common.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH           800
#define HEIGHT          600
#define FIELD_SIZE      48
#define PATCH_SIZE      8
#define UPDATE_INTERVAL 18
#define PATCH_COUNT     8



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct TextureUpdateState TextureUpdateState;

struct TextureUpdateState
{
    DvzSampledField* field;
    uint32_t frame_index;
    uint32_t patch_index;
    uint32_t patch_x;
    uint32_t patch_y;
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Fill a scalar field with a subdued background gradient.
 *
 * @param values output scalar field values
 */
static void _fill_field(float values[FIELD_SIZE * FIELD_SIZE])
{
    for (uint32_t y = 0; y < FIELD_SIZE; y++)
    {
        for (uint32_t x = 0; x < FIELD_SIZE; x++)
        {
            double fx = (double)x / (double)(FIELD_SIZE - 1);
            double fy = (double)y / (double)(FIELD_SIZE - 1);
            values[y * FIELD_SIZE + x] = (float)(0.10 + 0.15 * fx + 0.10 * fy);
        }
    }
}



/**
 * Fill one square patch directly in a full scalar field buffer.
 *
 * @param values scalar field values
 * @param x patch x coordinate in field samples
 * @param y patch y coordinate in field samples
 * @param value scalar value to write
 */
static void
_fill_field_patch(float values[FIELD_SIZE * FIELD_SIZE], uint32_t x, uint32_t y, float value)
{
    if (x + PATCH_SIZE > FIELD_SIZE || y + PATCH_SIZE > FIELD_SIZE)
        return;

    for (uint32_t row = 0; row < PATCH_SIZE; row++)
    {
        for (uint32_t col = 0; col < PATCH_SIZE; col++)
            values[(y + row) * FIELD_SIZE + x + col] = value;
    }
}



/**
 * Fill a patch payload with one scalar value.
 *
 * @param patch output patch values
 * @param value scalar value to write
 */
static void _fill_patch(float patch[PATCH_SIZE * PATCH_SIZE], float value)
{
    for (uint32_t i = 0; i < PATCH_SIZE * PATCH_SIZE; i++)
        patch[i] = value;
}



/**
 * Return one patch location from the bounded motion path.
 *
 * @param index patch path index
 * @param out_x output x coordinate in field samples
 * @param out_y output y coordinate in field samples
 */
static void _patch_position(uint32_t index, uint32_t* out_x, uint32_t* out_y)
{
    static const uint32_t path[PATCH_COUNT][2] = {
        {4, 4},
        {20, 4},
        {36, 4},
        {36, 20},
        {36, 36},
        {20, 36},
        {4, 36},
        {4, 20},
    };
    if (out_x == NULL || out_y == NULL)
        return;

    index %= PATCH_COUNT;
    *out_x = path[index][0];
    *out_y = path[index][1];
}



/**
 * Update one square patch in the sampled field.
 *
 * @param field sampled field to update
 * @param x patch x coordinate in field samples
 * @param y patch y coordinate in field samples
 * @param value scalar value to write
 * @return true on success, false on error
 */
static bool _update_patch(DvzSampledField* field, uint32_t x, uint32_t y, float value)
{
    if (field == NULL)
        return false;

    float patch[PATCH_SIZE * PATCH_SIZE] = {0};
    _fill_patch(patch, value);
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
        &(DvzFieldDataView){
            .data = patch,
            .bytes_per_row = PATCH_SIZE * sizeof(float),
            .rows_per_image = PATCH_SIZE,
        });
}



/*************************************************************************************************/
/*  Callbacks                                                                                    */
/*************************************************************************************************/

/**
 * Move the highlighted texture patch every few frames.
 *
 * @param win view whose frame just completed
 * @param user_data texture-update example state
 */
static void _texture_update_frame(DvzView* win, void* user_data)
{
    (void)win;
    TextureUpdateState* state = (TextureUpdateState*)user_data;
    if (state == NULL || state->field == NULL)
        return;

    state->frame_index++;
    if (state->frame_index % UPDATE_INTERVAL != 0)
        return;

    if (!_update_patch(state->field, state->patch_x, state->patch_y, 0.18f))
    {
        fprintf(stderr, "clearing previous patch failed\n");
        return;
    }

    state->patch_index = (state->patch_index + 1) % PATCH_COUNT;
    _patch_position(state->patch_index, &state->patch_x, &state->patch_y);
    if (!_update_patch(state->field, state->patch_x, state->patch_y, 1.0f))
    {
        fprintf(stderr, "updating patch failed\n");
        return;
    }

    printf(
        "partial texture update frame=%u region=(%u,%u %ux%u)\n",
        state->frame_index, state->patch_x, state->patch_y, PATCH_SIZE, PATCH_SIZE);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel_full(figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel() failed");

    DvzScale* scale = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CONTINUOUS});
    EXAMPLE_CHECK(scale != NULL, "dvz_scale() failed");
    dvz_scale_set_domain(scale, 0.0, 1.0);

    DvzColormap* colormap = dvz_colormap(scene, NULL);
    EXAMPLE_CHECK(colormap != NULL, "dvz_colormap() failed");
    DvzColormapStop stops[4] = {
        {.position = 0.00, .rgba = {22, 31, 42, 255}},
        {.position = 0.30, .rgba = {38, 110, 143, 255}},
        {.position = 0.65, .rgba = {232, 176, 57, 255}},
        {.position = 1.00, .rgba = {245, 246, 238, 255}},
    };
    dvz_colormap_set_stops(colormap, stops, 4);
    dvz_scale_set_colormap(scale, colormap);

    DvzVisual* image = dvz_image(scene, 0);
    EXAMPLE_CHECK(image != NULL, "dvz_image() failed");

    float positions[4][3] = {
        {-0.90f, -0.90f, 0.0f},
        {-0.90f, 0.90f, 0.0f},
        {0.90f, -0.90f, 0.0f},
        {0.90f, 0.90f, 0.0f},
    };
    float texcoords[4][2] = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
    };
    int rc = dvz_visual_set_data(image, "position", positions, 4);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_data(position) failed");

    rc = dvz_visual_set_data(image, "texcoords", texcoords, 4);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_data(texcoords) failed");

    rc = dvz_visual_set_scale(image, "colormap", scale);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_scale(colormap) failed");

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_R32_FLOAT,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = FIELD_SIZE,
                   .height = FIELD_SIZE,
                   .depth = 1,
               });
    EXAMPLE_CHECK(field != NULL, "dvz_sampled_field() failed");

    TextureUpdateState state = {.field = field};
    _patch_position(0, &state.patch_x, &state.patch_y);

    float values[FIELD_SIZE * FIELD_SIZE] = {0};
    _fill_field(values);
    _fill_field_patch(values, state.patch_x, state.patch_y, 1.0f);
    bool ok = dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){
                   .data = values,
                   .bytes_per_row = FIELD_SIZE * sizeof(float),
                   .rows_per_image = FIELD_SIZE,
               });
    EXAMPLE_CHECK(ok, "dvz_sampled_field_set_data() failed");

    ok = dvz_visual_set_field(image, "field", field);
    EXAMPLE_CHECK(ok, "dvz_visual_set_field() failed");

    rc = dvz_panel_add_visual(panel, image, NULL);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual() failed");

    dvz_panel_set_background_color(panel, 0.04f, 0.05f, 0.06f, 1.0f);

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win =
        dvz_view_glfw(app, figure, WIDTH, HEIGHT, "image");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");
    dvz_view_set_frame_callback(win, _texture_update_frame, &state);

    dvz_app_run(app, example_frame_count(argc, argv));
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
