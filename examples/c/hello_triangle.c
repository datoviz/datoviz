/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* hello_triangle — single colored triangle via dvz_primitive + scene/app.
 *
 * Builds the simplest possible primitive visual (TRIANGLE_LIST topology) with three
 * hardcoded vertices in clip space and per-vertex RGB colours.
 *
 * Build:  just example-c hello_triangle
 * Run:    ./build/examples/c/hello_triangle
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

    DvzPanelDesc desc = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, desc);
    if (!panel) { fprintf(stderr, "dvz_panel() failed\n"); dvz_scene_destroy(scene); return 1; }

    DvzVisual* visual = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    if (!visual) { fprintf(stderr, "dvz_primitive() failed\n"); dvz_scene_destroy(scene); return 1; }

    /* Three vertices in clip space: red top, green left, blue right. */
    float positions[3][3] = {
        { 0.0f,  0.6f, 0.0f},
        {-0.6f, -0.6f, 0.0f},
        { 0.6f, -0.6f, 0.0f},
    };
    uint8_t colors[3][4] = {
        {255,   0,   0, 255},
        {  0, 255,   0, 255},
        {  0,   0, 255, 255},
    };

    dvz_visual_set_data(visual, "position", positions, 3);
    dvz_visual_set_data(visual, "color",    colors,    3);
    dvz_panel_add_visual(panel, visual);

    DvzApp* app = dvz_app(scene);
    if (!app) { fprintf(stderr, "dvz_app() failed (no GPU?)\n"); dvz_scene_destroy(scene); return 1; }

    DvzAppWindow* win = dvz_app_window(app, figure, 800, 600);
    if (!win) { fprintf(stderr, "dvz_app_window() failed\n"); dvz_app_destroy(app); dvz_scene_destroy(scene); return 1; }

    dvz_app_run(app, 1);

    char out[512];
    _outpath(argv[0], "hello_triangle.png", out, sizeof(out));
    dvz_app_window_capture_png(win, out);
    printf("hello_triangle: saved %s\n", out);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}
