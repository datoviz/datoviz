/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* image_probe — live image-probe smoke example.
 *
 * Opens a GLFW window showing a non-uniform image. Move the cursor over the panel to issue one
 * image probe per frame through the scene -> DRP2 -> vklite live app path. Resolved probe values
 * are printed when the sampled RGBA changes.
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
 * Return whether a probe result differs enough from the last printed value.
 *
 * @param state image probe example state
 * @param probe probe result to compare
 * @return true when the result should be printed
 */
static bool _probe_changed(const ImageProbeState* state, const DvzProbeResult* probe)
{
    if (state == NULL || probe == NULL)
        return false;
    if (!state->has_last_result || state->last_hit != probe->hit)
        return true;
    if (!probe->hit)
        return false;

    for (uint32_t i = 0; i < 4; i++)
    {
        double delta = probe->vector[i] - state->last_rgba[i];
        if (delta < 0)
            delta = -delta;
        if (delta >= (1.0 / 255.0))
            return true;
    }
    return false;
}



/**
 * Remember the last printed probe result.
 *
 * @param state image probe example state
 * @param probe probe result to store
 */
static void _store_probe_result(ImageProbeState* state, const DvzProbeResult* probe)
{
    if (state == NULL || probe == NULL)
        return;

    state->has_last_result = true;
    state->last_hit = probe->hit;
    for (uint32_t i = 0; i < 4; i++)
        state->last_rgba[i] = probe->vector[i];
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
}



/**
 * Poll probe results and queue the next cursor probe.
 *
 * @param win app-window whose frame just completed
 * @param user_data image probe example state
 */
static void _image_probe_frame(DvzAppWindow* win, void* user_data)
{
    (void)win;
    ImageProbeState* state = (ImageProbeState*)user_data;
    if (state == NULL)
        return;

    DvzProbeResult probe = {0};
    while (dvz_scene_poll_probe(state->scene, &probe))
    {
        if (!_probe_changed(state, &probe))
            continue;

        if (probe.hit && probe.value_kind == DVZ_PROBE_VALUE_VEC4)
        {
            printf(
                "probe x=%7.1f y=%7.1f rgba=(%0.3f, %0.3f, %0.3f, %0.3f)\n",
                probe.has_coordinate ? probe.coordinate[0] : state->cursor_x,
                probe.has_coordinate ? probe.coordinate[1] : state->cursor_y,
                probe.vector[0], probe.vector[1], probe.vector[2], probe.vector[3]);
        }
        else
        {
            printf("probe miss x=%7.1f y=%7.1f\n", state->cursor_x, state->cursor_y);
        }
        _store_probe_result(state, &probe);
    }

    if (state->cursor_valid)
    {
        if (dvz_panel_probe(
                state->panel, state->cursor_x, state->cursor_y,
                &(DvzProbeRequest){
                    .request_id = 0,
                    .target = DVZ_SCENE_TARGET_PIXEL,
                }) != 0)
        {
            fprintf(stderr, "dvz_panel_probe() failed\n");
        }
    }
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    DvzScene* scene = dvz_scene();
    if (scene == NULL)
    {
        fprintf(stderr, "dvz_scene() failed\n");
        return 1;
    }

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    if (figure == NULL)
    {
        fprintf(stderr, "dvz_figure() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzPanel* panel =
        dvz_panel(figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    if (panel == NULL)
    {
        fprintf(stderr, "dvz_panel() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzVisual* image = dvz_image(scene, 0);
    if (image == NULL)
    {
        fprintf(stderr, "dvz_image() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    float positions[4][3] = {
        {-0.85f, -0.85f, 0.0f},
        {-0.85f, 0.85f, 0.0f},
        {0.85f, -0.85f, 0.0f},
        {0.85f, 0.85f, 0.0f},
    };
    float texcoords[4][2] = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
    };
    uint8_t pixels[IMG * IMG * 4] = {0};
    _fill_probe_image(pixels);

    if (dvz_visual_set_data(image, "position", positions, 4) != 0 ||
        dvz_visual_set_data(image, "texcoords", texcoords, 4) != 0 ||
        dvz_visual_set_texture(image, pixels, IMG, IMG) != 0)
    {
        fprintf(stderr, "image visual setup failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }
    if (dvz_panel_add_visual(panel, image, NULL) != 0)
    {
        fprintf(stderr, "dvz_panel_add_visual() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    dvz_panel_set_background_color(panel, 0.04f, 0.05f, 0.06f, 1.0f);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        fprintf(stderr, "dvz_app() failed (no GPU or display?)\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzAppWindow* win = dvz_app_window_glfw(app, figure, WIDTH, HEIGHT, "image_probe");
    if (win == NULL)
    {
        fprintf(stderr, "dvz_app_window_glfw() failed (GLFW unavailable?)\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzInputRouter* router = dvz_app_window_input(win);
    if (router == NULL)
    {
        fprintf(stderr, "dvz_app_window_input() failed\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }

    ImageProbeState state = {
        .scene = scene,
        .panel = panel,
    };
    dvz_input_subscribe_pointer(router, _image_probe_pointer, &state);
    dvz_app_window_set_frame_callback(win, _image_probe_frame, &state);

    dvz_app_run(app, example_frame_count(argc, argv));

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}
