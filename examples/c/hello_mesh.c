/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* hello_mesh — lit indexed cube mesh via dvz_mesh + scene/app.
 *
 * Creates one triangle-list cube mesh with explicit face normals, per-face colours, and a shared
 * scene index buffer. A perspective camera and arcball model transform provide the default 3D view.
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


static void _build_cube(
    float positions[24][3], DvzColor colors[24], float normals[24][3], DvzIndex indices[36])
{
    const float s = 0.58f;
    const float face_positions[6][4][3] = {
        {{-s, -s, +s}, {+s, -s, +s}, {+s, +s, +s}, {-s, +s, +s}},
        {{+s, -s, -s}, {-s, -s, -s}, {-s, +s, -s}, {+s, +s, -s}},
        {{-s, -s, -s}, {-s, -s, +s}, {-s, +s, +s}, {-s, +s, -s}},
        {{+s, -s, +s}, {+s, -s, -s}, {+s, +s, -s}, {+s, +s, +s}},
        {{-s, +s, +s}, {+s, +s, +s}, {+s, +s, -s}, {-s, +s, -s}},
        {{-s, -s, -s}, {+s, -s, -s}, {+s, -s, +s}, {-s, -s, +s}},
    };
    const float face_normals[6][3] = {
        {0.0f, 0.0f, +1.0f},
        {0.0f, 0.0f, -1.0f},
        {-1.0f, 0.0f, 0.0f},
        {+1.0f, 0.0f, 0.0f},
        {0.0f, +1.0f, 0.0f},
        {0.0f, -1.0f, 0.0f},
    };
    const DvzColor face_colors[6] = {
        {239, 83, 80, 255},
        {66, 165, 245, 255},
        {102, 187, 106, 255},
        {255, 202, 40, 255},
        {171, 71, 188, 255},
        {255, 112, 67, 255},
    };

    for (uint32_t face = 0; face < 6; face++)
    {
        for (uint32_t corner = 0; corner < 4; corner++)
        {
            const uint32_t vertex = 4 * face + corner;
            positions[vertex][0] = face_positions[face][corner][0];
            positions[vertex][1] = face_positions[face][corner][1];
            positions[vertex][2] = face_positions[face][corner][2];
            memcpy(colors[vertex], face_colors[face], sizeof(DvzColor));
            normals[vertex][0] = face_normals[face][0];
            normals[vertex][1] = face_normals[face][1];
            normals[vertex][2] = face_normals[face][2];
        }

        const uint32_t base = 4 * face;
        indices[6 * face + 0] = base + 0;
        indices[6 * face + 1] = base + 1;
        indices[6 * face + 2] = base + 2;
        indices[6 * face + 3] = base + 0;
        indices[6 * face + 4] = base + 2;
        indices[6 * face + 5] = base + 3;
    }
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

    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.eye[0] = 0.0f;
    camera_desc.eye[1] = 0.0f;
    camera_desc.eye[2] = 3.0f;
    camera_desc.target[0] = 0.0f;
    camera_desc.target[1] = 0.0f;
    camera_desc.target[2] = 0.0f;
    camera_desc.up[0] = 0.0f;
    camera_desc.up[1] = 1.0f;
    camera_desc.up[2] = 0.0f;
    camera_desc.fov_y = 0.78539816339f;
    camera_desc.near = 0.1f;
    camera_desc.far = 100.0f;
    if (!dvz_panel_set_camera(panel, &camera_desc))
    {
        fprintf(stderr, "dvz_panel_set_camera() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    dvz_panel_set_arcball(panel, NULL, 0);
    DvzArcball* arcball = dvz_panel_arcball(panel);
    if (!arcball) { fprintf(stderr, "dvz_panel_set_arcball() failed\n"); dvz_scene_destroy(scene); return 1; }
    dvz_arcball_initial(arcball, (vec3){+0.6f, -1.2f, +3.0f});

    DvzVisual* visual = dvz_mesh(scene, 0);
    if (!visual) { fprintf(stderr, "dvz_mesh() failed\n"); dvz_scene_destroy(scene); return 1; }

    float positions[24][3] = {0};
    DvzColor colors[24] = {0};
    float normals[24][3] = {0};
    DvzIndex indices[36] = {0};
    _build_cube(positions, colors, normals, indices);

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

    dvz_visual_set_data(visual, "position", positions, 24);
    dvz_visual_set_data(visual, "color", colors, 24);
    dvz_visual_set_data(visual, "normal", normals, 24);
    dvz_visual_set_buffer(visual, "index", index_buffer);
    dvz_visual_set_primitive_shading(
        visual,
        &(DvzPrimitiveShadingDesc){
            .light_direction = {0.35f, 0.55f, 0.75f},
            .ambient = 0.25f,
            .diffuse = 0.85f,
        });
    dvz_panel_add_visual(panel, visual, NULL);
    dvz_panel_set_background_color(panel, 0.05f, 0.05f, 0.08f, 1.0f);

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
