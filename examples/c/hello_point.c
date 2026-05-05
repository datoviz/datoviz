/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* hello_point — minimal scene+app example.
 *
 * Creates three colored points and renders one offscreen frame.
 *
 * Build:  just example-c hello_point
 * Run:    ./build/examples/c/hello_point
 */

#include <stdio.h>
#include <stdint.h>

#include "datoviz/app.h"
#include "datoviz/scene.h"

int main(void)
{
    /* Scene */
    DvzScene* scene = dvz_scene();
    if (!scene) { fprintf(stderr, "dvz_scene() failed\n"); return 1; }

    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    if (!figure) { fprintf(stderr, "dvz_figure() failed\n"); dvz_scene_destroy(scene); return 1; }

    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    if (!panel) { fprintf(stderr, "dvz_panel() failed\n"); dvz_scene_destroy(scene); return 1; }

    DvzVisual* visual = dvz_point(scene, 0);
    if (!visual) { fprintf(stderr, "dvz_point() failed\n"); dvz_scene_destroy(scene); return 1; }

    /* Data */
    float positions[3][3] = {
        {-0.5f, -0.5f, 0.0f},
        { 0.5f, -0.5f, 0.0f},
        { 0.0f,  0.5f, 0.0f},
    };
    uint8_t colors[3][4] = {
        {255,   0,   0, 255},
        {  0, 255,   0, 255},
        {  0,   0, 255, 255},
    };
    float sizes[3] = {20.0f, 20.0f, 20.0f};

    dvz_visual_set_data(visual, "position", positions, 3);
    dvz_visual_set_data(visual, "color",    colors,    3);
    dvz_visual_set_data(visual, "size",     sizes,     3);
    dvz_panel_add_visual(panel, visual);

    /* App */
    DvzApp* app = dvz_app(scene);
    if (!app) { fprintf(stderr, "dvz_app() failed (no GPU?)\n"); dvz_scene_destroy(scene); return 1; }

    DvzAppWindow* win = dvz_app_window(app, figure, 800, 600);
    if (!win) { fprintf(stderr, "dvz_app_window() failed\n"); dvz_app_destroy(app); dvz_scene_destroy(scene); return 1; }

    /* Render one frame */
    dvz_app_run(app, 1);

    printf("hello_point: rendered one frame successfully\n");

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}
