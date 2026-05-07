/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* hello_texture — single textured quad via dvz_image + scene/app.
 *
 * Builds a 16x16 procedural RGBA image and renders it on a quad covering most of the
 * panel. Demonstrates `dvz_image` with `position`, `texcoords`, and a sampled texture.
 *
 * Build:  just example-c hello_texture
 * Run:    ./build/examples/c/hello_texture
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "datoviz/app.h"
#include "datoviz/scene.h"

#define IMG 16

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

    DvzVisual* visual = dvz_image(scene, 0);
    if (!visual) { fprintf(stderr, "dvz_image() failed\n"); dvz_scene_destroy(scene); return 1; }

    /* TRIANGLE_STRIP corner order: TL, BL, TR, BR. */
    float positions[4][3] = {
        {-0.7f, -0.7f, 0.0f},
        {-0.7f,  0.7f, 0.0f},
        { 0.7f, -0.7f, 0.0f},
        { 0.7f,  0.7f, 0.0f},
    };
    float texcoords[4][2] = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
    };

    /* Procedural 16x16 RGBA gradient with a checker overlay. */
    static uint8_t pixels[IMG * IMG * 4];
    for (uint32_t y = 0; y < IMG; y++)
    {
        for (uint32_t x = 0; x < IMG; x++)
        {
            uint32_t i = (y * IMG + x) * 4;
            uint8_t checker = ((x ^ y) & 1) ? 80 : 0;
            pixels[i + 0] = (uint8_t)((x * 255) / (IMG - 1));        /* red ramp */
            pixels[i + 1] = (uint8_t)((y * 255) / (IMG - 1));        /* green ramp */
            pixels[i + 2] = (uint8_t)(160 + checker);                /* blue + checker */
            pixels[i + 3] = 255;
        }
    }

    dvz_visual_set_data(visual, "position", positions, 4);
    dvz_visual_set_data(visual, "texcoords", texcoords, 4);
    dvz_visual_set_texture(visual, pixels, IMG, IMG);
    dvz_panel_add_visual(panel, visual);

    DvzApp* app = dvz_app(scene);
    if (!app) { fprintf(stderr, "dvz_app() failed (no GPU?)\n"); dvz_scene_destroy(scene); return 1; }

    DvzAppWindow* win = dvz_app_window(app, figure, 800, 600);
    if (!win) { fprintf(stderr, "dvz_app_window() failed\n"); dvz_app_destroy(app); dvz_scene_destroy(scene); return 1; }

    dvz_app_run(app, 1);

    char out[512];
    _outpath(argv[0], "hello_texture.png", out, sizeof(out));
    dvz_app_window_capture_png(win, out);
    printf("hello_texture: saved %s\n", out);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}
