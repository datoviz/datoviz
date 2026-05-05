/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* hello_scatter — 1 000 random colored points via scene+app.
 *
 * Demonstrates dvz_visual_set_data() with non-trivial arrays and
 * independent per-point position, colour, and size.
 *
 * Build:  just example-c hello_scatter
 * Run:    ./build/examples/c/hello_scatter
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "datoviz/app.h"
#include "datoviz/scene.h"

#define N 1000

static void _outpath(const char* exe, const char* name, char* buf, size_t n)
{
    const char* slash = strrchr(exe, '/');
    if (slash) snprintf(buf, n, "%.*s/%s", (int)(slash - exe), exe, name);
    else        snprintf(buf, n, "%s", name);
}

static float randf(void) { return (float)rand() / (float)RAND_MAX; }

int main(int argc, char** argv)
{
    (void)argc;
    srand(42);

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
    float    positions[N][3];
    uint8_t  colors[N][4];
    float    sizes[N];

    for (int i = 0; i < N; i++) {
        positions[i][0] = randf() * 2.0f - 1.0f;  /* x in [-1, 1] */
        positions[i][1] = randf() * 2.0f - 1.0f;  /* y in [-1, 1] */
        positions[i][2] = 0.0f;

        colors[i][0] = (uint8_t)(randf() * 255);
        colors[i][1] = (uint8_t)(randf() * 255);
        colors[i][2] = (uint8_t)(randf() * 255);
        colors[i][3] = 200;

        sizes[i] = 4.0f + randf() * 12.0f;  /* 4–16 px diameter */
    }

    dvz_visual_set_data(visual, "position", positions, N);
    dvz_visual_set_data(visual, "color",    colors,    N);
    dvz_visual_set_data(visual, "size",     sizes,     N);
    dvz_panel_add_visual(panel, visual);

    /* App */
    DvzApp* app = dvz_app(scene);
    if (!app) { fprintf(stderr, "dvz_app() failed (no GPU?)\n"); dvz_scene_destroy(scene); return 1; }

    DvzAppWindow* win = dvz_app_window(app, figure, 800, 600);
    if (!win) { fprintf(stderr, "dvz_app_window() failed\n"); dvz_app_destroy(app); dvz_scene_destroy(scene); return 1; }

    dvz_app_run(app, 1);

    char out[512];
    _outpath(argv[0], "hello_scatter.png", out, sizeof(out));
    dvz_app_window_capture_png(win, out);
    printf("hello_scatter: saved %s\n", out);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}
