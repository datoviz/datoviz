/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* image_probe - live image-query smoke example.
 *
 * Opens a GLFW window showing a non-uniform image. Move the cursor over the panel to issue one
 * image query per frame through the scene -> DRP2 -> vklite live app path. Click to pin the next
 * resolved image-query value as a retained readout card. Resolved query values are printed when
 * the sampled RGBA changes.
 *
 * Build:  just example-c visuals/image_probe
 * Run:    ./build/examples/c/techniques/image_probe
 * Smoke:  ./build/examples/c/techniques/image_probe 300
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "datoviz/app.h"
#include "datoviz/input/router.h"
#include "datoviz/scene.h"
#include "example_common.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  800
#define HEIGHT 600
#define IMG    32



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct ImageProbeState ImageProbeState;

struct ImageProbeState
{
    DvzScene* scene;
    DvzPanel* panel;
    bool cursor_valid;
    double cursor_x;
    double cursor_y;
    double last_rgba[4];
    bool last_hit;
    bool has_last_result;
    bool pin_next_result;
    uint32_t pinned_count;
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Fill an RGBA image with quadrant colors and a local gradient.
 *
 * @param pixels output RGBA image data
 */
static void _fill_probe_image(uint8_t pixels[IMG * IMG * 4])
{
    for (uint32_t y = 0; y < IMG; y++)
    {
        for (uint32_t x = 0; x < IMG; x++)
        {
            uint32_t i = 4 * (y * IMG + x);
            uint8_t ramp_x = (uint8_t)((x * 95) / (IMG - 1));
            uint8_t ramp_y = (uint8_t)((y * 95) / (IMG - 1));

            if (x < IMG / 2 && y < IMG / 2)
            {
                pixels[i + 0] = (uint8_t)(160 + ramp_x);
                pixels[i + 1] = (uint8_t)(20 + ramp_y / 3);
                pixels[i + 2] = 40;
            }
            else if (x >= IMG / 2 && y < IMG / 2)
            {
                pixels[i + 0] = 25;
                pixels[i + 1] = (uint8_t)(155 + ramp_y);
                pixels[i + 2] = (uint8_t)(55 + ramp_x / 2);
            }
            else if (x < IMG / 2 && y >= IMG / 2)
            {
                pixels[i + 0] = (uint8_t)(35 + ramp_x / 2);
                pixels[i + 1] = 65;
                pixels[i + 2] = (uint8_t)(155 + ramp_y);
            }
            else
            {
                pixels[i + 0] = (uint8_t)(170 + ramp_x / 2);
                pixels[i + 1] = (uint8_t)(155 + ramp_y / 2);
                pixels[i + 2] = 35;
            }
            pixels[i + 3] = 255;
        }
    }
}



/**
 * Return whether a query result differs enough from the last printed value.
 *
 * @param state image query example state
 * @param query query result to compare
 * @return true when the result should be printed
 */
static bool _query_changed(const ImageProbeState* state, const DvzQueryResult* query)
{
    if (state == NULL || query == NULL)
        return false;
    if (!state->has_last_result || state->last_hit != query->hit)
        return true;
    if (!query->hit)
        return false;

    for (uint32_t i = 0; i < 4; i++)
    {
        double delta = query->vector[i] - state->last_rgba[i];
        if (delta < 0)
            delta = -delta;
        if (delta >= (1.0 / 255.0))
            return true;
    }
    return false;
}



/**
 * Remember the last printed query result.
 *
 * @param state image query example state
 * @param query query result to store
 */
static void _store_query_result(ImageProbeState* state, const DvzQueryResult* query)
{
    if (state == NULL || query == NULL)
        return;

    state->has_last_result = true;
    state->last_hit = query->hit;
    for (uint32_t i = 0; i < 4; i++)
        state->last_rgba[i] = query->vector[i];
}



/*************************************************************************************************/
/*  Callbacks                                                                                    */
/*************************************************************************************************/

/**
 * Record the latest cursor position in window coordinates.
 *
 * @param router input router emitting the event
 * @param event pointer event payload
 * @param user_data image probe example state
 */
static void
_image_probe_pointer(DvzInputRouter* router, const DvzPointerEvent* event, void* user_data)
{
    (void)router;
    ImageProbeState* state = (ImageProbeState*)user_data;
    if (state == NULL || event == NULL)
        return;
    if (event->type != DVZ_POINTER_EVENT_MOVE && event->type != DVZ_POINTER_EVENT_CLICK)
        return;

    state->cursor_valid = true;
    state->cursor_x = event->pos[0];
    state->cursor_y = event->pos[1];
    if (event->type == DVZ_POINTER_EVENT_CLICK)
        state->pin_next_result = true;
}



/**
 * Poll query results and queue the next cursor query.
 *
 * @param win view whose frame just completed
 * @param user_data image probe example state
 */
static void _image_probe_frame(DvzView* win, void* user_data)
{
    (void)win;
    ImageProbeState* state = (ImageProbeState*)user_data;
    if (state == NULL)
        return;

    DvzQueryResult query = {0};
    while (dvz_scene_poll_query(state->scene, &query))
    {
        if (state->pin_next_result)
        {
            if (query.status == DVZ_QUERY_STATUS_HIT && query.hit)
            {
                DvzPinnedReadout* readout = dvz_pinned_readout_query(state->panel, &query);
                if (readout != NULL)
                {
                    dvz_pinned_readout_set_format(
                        readout, &(DvzFormatDesc){DVZ_STRUCT_INIT_FIELDS(DvzFormatDesc), .precision = 3, .trim_trailing_zeros = true});
                    state->pinned_count++;
                    dvz_fprintf(stdout, "pinned readout %u\n", state->pinned_count);
                }
                else
                {
                    dvz_fprintf(stderr, "dvz_pinned_readout_query() failed\n");
                }
            }
            state->pin_next_result = false;
        }

        if (!_query_changed(state, &query))
            continue;

        if (
            query.status == DVZ_QUERY_STATUS_HIT && query.hit &&
            query.value_kind == DVZ_QUERY_VALUE_VEC4)
        {
            dvz_fprintf(
                stdout,
                "query x=%7.1f y=%7.1f rgba=(%0.3f, %0.3f, %0.3f, %0.3f)\n",
                query.has_data_position ? query.data_position[0] : state->cursor_x,
                query.has_data_position ? query.data_position[1] : state->cursor_y,
                query.vector[0], query.vector[1], query.vector[2], query.vector[3]);
        }
        else
        {
            dvz_fprintf(stdout, "query miss x=%7.1f y=%7.1f\n", state->cursor_x, state->cursor_y);
        }
        _store_query_result(state, &query);
    }

    if (state->cursor_valid)
    {
        if (dvz_panel_query_px(
                state->panel, state->cursor_x, state->cursor_y,
                &(DvzQueryRequest){
                    .request_id = 0,
                    .target = DVZ_SCENE_TARGET_PIXEL,
                }) != 0)
        {
            dvz_fprintf(stderr, "dvz_panel_query_px() failed\n");
        }
    }
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

    DvzVisual* image = dvz_image(scene, 0);
    EXAMPLE_CHECK(image != NULL, "dvz_image() failed");

    vec3 positions[4] = {
        {-0.85f, -0.85f, 0.0f},
        {-0.85f, 0.85f, 0.0f},
        {0.85f, -0.85f, 0.0f},
        {0.85f, 0.85f, 0.0f},
    };
    vec2 texcoords[4] = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
    };
    uint8_t pixels[IMG * IMG * 4] = {0};
    _fill_probe_image(pixels);

    int rc = dvz_visual_set_data(image, "position", positions, 4);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_data(position) failed");

    rc = dvz_visual_set_data(image, "texcoords", texcoords, 4);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_data(texcoords) failed");

    EXAMPLE_CHECK(
        example_visual_set_rgba8_field(scene, image, "field", (const uint8_t*)pixels, IMG, IMG,
                                       NULL),
        "example_visual_set_rgba8_field() failed");
    dvz_visual_set_query_capabilities(image, DVZ_QUERY_CAPABILITY_PIXEL);

    rc = dvz_panel_add_visual(panel, image, NULL);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual() failed");

    dvz_panel_set_background_color(panel, dvz_color_from_unit(0.04f, 0.05f, 0.06f, 1.0f));

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win = dvz_view_window(app, figure, WIDTH, HEIGHT, "image_probe");
    EXAMPLE_CHECK(win != NULL, "dvz_view_window() failed (GLFW unavailable?)");

    DvzInputRouter* router = dvz_view_input(win);
    EXAMPLE_CHECK(router != NULL, "dvz_view_input() failed");

    ImageProbeState state = {
        .scene = scene,
        .panel = panel,
    };
    dvz_input_subscribe_pointer(router, _image_probe_pointer, &state);
    dvz_view_set_frame_callback(win, _image_probe_frame, &state);

    dvz_app_run(app, example_frame_count(argc, argv));
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
