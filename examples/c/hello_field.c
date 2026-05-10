/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* hello_field — scalar sampled field rendered through dvz_image + scale/colormap.
 *
 * Builds a 64x64 scalar field, binds it through the retained sampled-field API, and
 * renders it on a fullscreen quad.
 *
 * Build:  just example-c hello_field
 * Run:    ./build/examples/c/hello_field
 */

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "datoviz/app.h"
#include "datoviz/scene.h"

#define IMG 64

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

    DvzScale* scale = dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CONTINUOUS});
    if (!scale) { fprintf(stderr, "dvz_scale() failed\n"); dvz_scene_destroy(scene); return 1; }
    dvz_scale_set_domain(scale, 0.0, 1.0);
    DvzColormap* colormap = dvz_colormap_builtin(scene, DVZ_BUILTIN_COLORMAP_TURBO);
    if (!colormap) { fprintf(stderr, "dvz_colormap_builtin() failed\n"); dvz_scene_destroy(scene); return 1; }
    dvz_scale_set_colormap(scale, colormap);

    DvzVisual* visual = dvz_image(scene, 0);
    if (!visual) { fprintf(stderr, "dvz_image() failed\n"); dvz_scene_destroy(scene); return 1; }

    float positions[4][3] = {
        {-1.0f, -1.0f, 0.0f},
        {-1.0f,  1.0f, 0.0f},
        { 1.0f, -1.0f, 0.0f},
        { 1.0f,  1.0f, 0.0f},
    };
    float texcoords[4][2] = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
    };

    static float values[IMG * IMG];
    for (uint32_t y = 0; y < IMG; y++)
    {
        for (uint32_t x = 0; x < IMG; x++)
        {
            float fx = ((float)x / (float)(IMG - 1)) * 2.0f - 1.0f;
            float fy = ((float)y / (float)(IMG - 1)) * 2.0f - 1.0f;
            float r = sqrtf(fx * fx + fy * fy);
            values[y * IMG + x] = 0.5f + 0.5f * cosf(10.0f * r);
        }
    }

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_R32_FLOAT,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = IMG,
                   .height = IMG,
                   .depth = 1,
               });
    if (!field) { fprintf(stderr, "dvz_sampled_field() failed\n"); dvz_scene_destroy(scene); return 1; }
    dvz_sampled_field_set_geometry(
        field, &(DvzFieldGeometry){
                   .axis_order = {0, 1, 2},
                   .axis_flip = {false, false, false},
                   .origin = {0.0, 0.0, 0.0},
                   .spacing = {1.0, 1.0, 1.0},
                   .unit = "px",
               });
    dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){
                   .data = values,
                   .bytes_per_row = IMG * sizeof(float),
                   .rows_per_image = IMG,
               });

    dvz_visual_set_data(visual, "position", positions, 4);
    dvz_visual_set_data(visual, "texcoords", texcoords, 4);
    dvz_visual_set_field(visual, "field", field);
    dvz_visual_set_scale(visual, "colormap", scale);
    dvz_panel_add_visual(panel, visual, NULL);

    DvzApp* app = dvz_app(scene);
    if (!app) { fprintf(stderr, "dvz_app() failed (no GPU?)\n"); dvz_scene_destroy(scene); return 1; }

    DvzAppWindow* win = dvz_app_window(app, figure, 800, 600);
    if (!win) { fprintf(stderr, "dvz_app_window() failed\n"); dvz_app_destroy(app); dvz_scene_destroy(scene); return 1; }

    dvz_app_run(app, 1);

    char out[512];
    _outpath(argv[0], "hello_field.png", out, sizeof(out));
    dvz_app_window_capture_png(win, out);
    printf("hello_field: saved %s\n", out);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}
