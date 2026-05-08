/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* hello_path — minimal line-strip path via dvz_path + scene/app.
 *
 * Creates a single zig-zag path with per-vertex colours and renders one offscreen frame.
 *
 * Build:  just example-c hello_path
 * Run:    ./build/examples/c/hello_path
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "datoviz/app.h"
#include "datoviz/scene.h"

static void _outpath(const char* exe, const char* name, char* buf, size_t n)
{
    const char* slash = strrchr(exe, '/');
    if (slash) snprintf(buf, n, "%.*s/%s", (int)(slash - exe), exe, name);
    else        snprintf(buf, n, "%s", name);
}

int main(int argc, char** argv)
{
    (void)argc;

    DvzScene* scene = dvz_scene();
    if (!scene) { fprintf(stderr, "dvz_scene() failed\n"); return 1; }

    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    if (!figure) { fprintf(stderr, "dvz_figure() failed\n"); dvz_scene_destroy(scene); return 1; }

    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    if (!panel) { fprintf(stderr, "dvz_panel() failed\n"); dvz_scene_destroy(scene); return 1; }

    DvzVisual* visual = dvz_path(scene, 0);
    if (!visual) { fprintf(stderr, "dvz_path() failed\n"); dvz_scene_destroy(scene); return 1; }

    float positions[5][3] = {
        {-0.8f, -0.5f, 0.0f},
        {-0.4f, 0.1f, 0.0f},
        {0.0f, -0.2f, 0.0f},
        {0.4f, 0.5f, 0.0f},
        {0.8f, -0.1f, 0.0f},
    };
    uint8_t colors[5][4] = {
        {255, 64, 64, 255},
        {255, 192, 64, 255},
        {255, 255, 64, 255},
        {64, 220, 180, 255},
        {64, 128, 255, 255},
    };

    dvz_visual_set_data(visual, "position", positions, 5);
    dvz_visual_set_data(visual, "color", colors, 5);
    dvz_panel_add_visual(panel, visual, NULL);

    DvzApp* app = dvz_app(scene);
    if (!app) { fprintf(stderr, "dvz_app() failed (no GPU?)\n"); dvz_scene_destroy(scene); return 1; }

    DvzAppWindow* win = dvz_app_window(app, figure, 800, 600);
    if (!win) { fprintf(stderr, "dvz_app_window() failed\n"); dvz_app_destroy(app); dvz_scene_destroy(scene); return 1; }

    dvz_app_run(app, 1);

    char out[512];
    _outpath(argv[0], "hello_path.png", out, sizeof(out));
    dvz_app_window_capture_png(win, out);
    printf("hello_path: saved %s\n", out);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}
