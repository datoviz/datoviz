/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* hello_mesh — minimal indexed quad mesh via dvz_mesh + scene/app.
 *
 * Creates one triangle-list mesh with positions, normals, and a shared scene index buffer.
 * The mesh intentionally omits vertex colours so the retained mesh path exercises its default
 * opaque-white colour generation.
 *
 * Build:  just example-c hello_mesh
 * Run:    ./build/examples/c/hello_mesh
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
    else snprintf(buf, n, "%s", name);
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

    DvzVisual* visual = dvz_mesh(scene, 0);
    if (!visual) { fprintf(stderr, "dvz_mesh() failed\n"); dvz_scene_destroy(scene); return 1; }

    float positions[4][3] = {
        {-0.8f, -0.8f, 0.0f}, {-0.8f, 0.8f, 0.0f},
        {0.8f, -0.8f, 0.0f},  {0.8f, 0.8f, 0.0f},
    };
    float normals[4][3] = {
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
    };
    DvzIndex indices[6] = {0, 1, 2, 2, 1, 3};

    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    if (!index_buffer) { fprintf(stderr, "dvz_scene_buffer() failed\n"); dvz_scene_destroy(scene); return 1; }
    if (!dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)))
    {
        fprintf(stderr, "dvz_scene_buffer_set_data() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    dvz_visual_set_data(visual, "position", positions, 4);
    dvz_visual_set_data(visual, "normal", normals, 4);
    dvz_visual_set_buffer(visual, "index", index_buffer);
    dvz_panel_add_visual(panel, visual, NULL);

    DvzApp* app = dvz_app(scene);
    if (!app) { fprintf(stderr, "dvz_app() failed (no GPU?)\n"); dvz_scene_destroy(scene); return 1; }

    DvzAppWindow* win = dvz_app_window(app, figure, 800, 600);
    if (!win) { fprintf(stderr, "dvz_app_window() failed\n"); dvz_app_destroy(app); dvz_scene_destroy(scene); return 1; }

    dvz_app_run(app, 1);

    char out[512];
    _outpath(argv[0], "hello_mesh.png", out, sizeof(out));
    dvz_app_window_capture_png(win, out);
    printf("hello_mesh: saved %s\n", out);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}
