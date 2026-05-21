/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* text - minimal MSDF text visual smoke example.
 *
 * Build:  just example-c visuals/text
 * Run:    ./build/examples/c/visuals/text
 * Smoke:  ./build/examples/c/visuals/text 120
 */

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>
#include <stdlib.h>

#include "_compat.h"
#include "datoviz/app.h"
#include "datoviz/scene.h"
#include "example_common.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH      960u
#define HEIGHT     540u
#define TEXT_COUNT 5u



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the text visual smoke example.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    uint32_t frames = example_frame_count(argc, argv);

    DvzScene* scene = dvz_scene();
    if (scene == NULL)
    {
        dvz_fprintf(stderr, "dvz_scene() failed\n");
        return 1;
    }

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    if (figure == NULL)
    {
        dvz_fprintf(stderr, "dvz_figure() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzPanel* panel = dvz_panel_full(figure);
    if (panel == NULL)
    {
        dvz_fprintf(stderr, "dvz_panel() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_panel_set_background_color(panel, 0.055f, 0.065f, 0.085f, 1.0f);

    const char* strings[TEXT_COUNT] = {
        "MSDF text smoke",
        "e b S  space  0123456789",
        "The quick brown fox jumps over 13 lazy glyphs.",
        "visual coordinates, pixel-sized glyphs",
        "rotated text",
    };
    float positions[TEXT_COUNT][3] = {{0}};
    float anchors[TEXT_COUNT][2] = {
        {0.0f, 0.5f},
        {0.0f, 0.5f},
        {0.0f, 0.5f},
        {0.0f, 0.5f},
        {0.5f, 0.5f},
    };
    float sizes[TEXT_COUNT] = {72.0f, 48.0f, 28.0f, 22.0f, 30.0f};
    float angles[TEXT_COUNT] = {0.0f, 0.0f, 0.0f, 0.0f, -0.32f};
    DvzColor colors[TEXT_COUNT] = {
        {236, 244, 255, 255},
        {128, 220, 255, 255},
        {214, 224, 238, 255},
        {166, 178, 196, 255},
        {255, 198, 110, 255},
    };
    positions[0][0] = 54.0f;
    positions[0][1] = 112.0f;
    positions[1][0] = 56.0f;
    positions[1][1] = 220.0f;
    positions[2][0] = 58.0f;
    positions[2][1] = 318.0f;
    positions[3][0] = 60.0f;
    positions[3][1] = 382.0f;
    positions[4][0] = 760.0f;
    positions[4][1] = 410.0f;

    for (uint32_t i = 0; i < TEXT_COUNT; i++)
    {
        DvzText* text = dvz_text(panel, 0);
        if (text == NULL)
        {
            dvz_fprintf(stderr, "dvz_text() failed\n");
            dvz_scene_destroy(scene);
            return 1;
        }
        if (dvz_text_set_style(
                text,
                &(DvzTextStyle){
                    .size_px = sizes[i],
                    .renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS,
                    .color = {colors[i][0], colors[i][1], colors[i][2], colors[i][3]},
                }) != 0)
        {
            dvz_fprintf(stderr, "dvz_text_set_style() failed\n");
            dvz_scene_destroy(scene);
            return 1;
        }
        dvz_text_set_placement(
            text,
            &(DvzTextPlacement){
                .mode = DVZ_TEXT_PLACEMENT_SCREEN,
                .anchor = DVZ_SCENE_ANCHOR_PANEL_TOP_LEFT,
                .position = {positions[i][0], positions[i][1], positions[i][2]},
                .text_anchor = {anchors[i][0], anchors[i][1]},
                .has_text_anchor = true,
                .angle = angles[i],
            });
        dvz_text_set_string(text, strings[i]);
    }

    DvzAppConfig app_config = dvz_app_config();
    if (frames > 0)
        app_config.schedule_mode = DVZ_APP_SCHEDULE_CONTINUOUS;
    DvzApp* app = dvz_app_with_config(scene, &app_config);
    if (app == NULL)
    {
        dvz_fprintf(stderr, "dvz_app() failed (no GPU or display?)\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzAppWindow* win = dvz_app_window_glfw(app, figure, WIDTH, HEIGHT, "text");
    if (win == NULL)
    {
        dvz_fprintf(stderr, "dvz_app_window_glfw() failed (GLFW unavailable?)\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }
    DvzPanzoom* panzoom = dvz_app_window_panel_panzoom(win, panel, NULL);
    if (panzoom == NULL)
    {
        dvz_fprintf(stderr, "failed to create or bind panzoom controller\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_app_window_request_frame(win);

    dvz_app_run(app, frames);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}
