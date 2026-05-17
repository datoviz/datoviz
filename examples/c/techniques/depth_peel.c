/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* depth_peel — transparent mesh via scene depth peeling + app/GLFW.
 *
 * Opens a GLFW window showing one transparent depth-peel cube with opaque reference geometry. The
 * alpha-mode opt-in exercises retained scene planning, graph-backed DRP2 lowering, and the vklite
 * runtime while retaining the normal app/canvas presentation path.
 *
 * Build:  just example-c techniques/depth_peel
 * Run:    ./build/examples/c/techniques/depth_peel
 * Smoke:  ./build/examples/c/techniques/depth_peel 60
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "datoviz/app.h"
#include "datoviz/scene.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  800
#define HEIGHT 600

#define ROTATION_SPEED_RAD_PER_SEC 0.55f



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Build an indexed cube with duplicated vertices, per-face normals, and one color.
 *
 * @param scale cube half-extent
 * @param color cube vertex color
 * @param positions output vertex positions
 * @param colors output vertex colors
 * @param normals output vertex normals
 * @param indices output triangle-list indices
 */
static void _build_cube(
    float scale, DvzColor color, float positions[24][3], DvzColor colors[24],
    float normals[24][3], DvzIndex indices[36])
{
    const float s = scale;
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

    for (uint32_t face = 0; face < 6; face++)
    {
        for (uint32_t corner = 0; corner < 4; corner++)
        {
            const uint32_t vertex = 4 * face + corner;
            positions[vertex][0] = face_positions[face][corner][0];
            positions[vertex][1] = face_positions[face][corner][1];
            positions[vertex][2] = face_positions[face][corner][2];
            dvz_memcpy(colors[vertex], sizeof(DvzColor), color, sizeof(DvzColor));
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



/*************************************************************************************************/
/*  Entry point                                                                                  */
/*************************************************************************************************/

/**
 * Run the interactive depth-peeling mesh example.
 *
 * @param argc argument count
 * @param argv arguments; argv[1] may contain a bounded frame count
 * @return process status
 */
int main(int argc, char** argv)
{
    uint64_t frame_count = 0;
    if (argc >= 2)
        frame_count = (uint64_t)strtoull(argv[1], NULL, 10);

    DvzScene* scene = dvz_scene();
    if (scene == NULL)
        return 1;
    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.max_color_attachments = 3;
    caps.render_target_format_rgba16float = true;
    caps.supports_render_target_sampling = true;
    dvz_scene_set_capabilities(scene, &caps);

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    if (figure == NULL || panel == NULL)
    {
        dvz_fprintf(stderr, "scene setup failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_panel_set_background_color(panel, 0.05f, 0.05f, 0.08f, 1.0f);

    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.eye[2] = 3.2f;
    camera_desc.up[1] = 1.0f;
    camera_desc.fov_y = 0.78539816339f;
    camera_desc.near = 0.1f;
    camera_desc.far = 100.0f;
    if (!dvz_panel_set_camera(panel, &camera_desc))
    {
        dvz_fprintf(stderr, "dvz_panel_set_camera() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzVisual* reference = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    DvzVisual* cube = dvz_mesh(scene, 0);
    if (reference == NULL || cube == NULL)
    {
        dvz_fprintf(stderr, "visual creation failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    float reference_positions[12][3] = {
        {-0.95f, -0.95f, -1.05f},
        {+0.95f, -0.95f, -1.05f},
        {+0.95f, +0.95f, -1.05f},
        {-0.95f, -0.95f, -1.05f},
        {+0.95f, +0.95f, -1.05f},
        {-0.95f, +0.95f, -1.05f},
        {-0.16f, -0.86f, +1.05f},
        {+0.16f, -0.86f, +1.05f},
        {+0.16f, +0.86f, +1.05f},
        {-0.16f, -0.86f, +1.05f},
        {+0.16f, +0.86f, +1.05f},
        {-0.16f, +0.86f, +1.05f},
    };
    DvzColor reference_colors[12] = {
        {255, 230, 80, 255},
        {255, 230, 80, 255},
        {255, 80, 180, 255},
        {255, 230, 80, 255},
        {255, 80, 180, 255},
        {80, 200, 255, 255},
        {32, 32, 32, 255},
        {32, 32, 32, 255},
        {120, 255, 150, 255},
        {32, 32, 32, 255},
        {120, 255, 150, 255},
        {32, 32, 32, 255},
    };

    float positions[24][3] = {0};
    DvzColor colors[24] = {0};
    float normals[24][3] = {0};
    DvzIndex indices[36] = {0};
    DvzColor cube_color = {48, 170, 220, 82};
    _build_cube(0.72f, cube_color, positions, colors, normals, indices);

    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    if (index_buffer == NULL ||
        !dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)))
    {
        dvz_fprintf(stderr, "index buffer setup failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    dvz_visual_set_data(reference, "position", reference_positions, 12);
    dvz_visual_set_data(reference, "color", reference_colors, 12);

    dvz_visual_set_data(cube, "position", positions, 24);
    dvz_visual_set_data(cube, "color", colors, 24);
    dvz_visual_set_data(cube, "normal", normals, 24);
    dvz_visual_set_buffer(cube, "index", index_buffer);
    dvz_visual_set_alpha_mode(cube, DVZ_ALPHA_DEPTH_PEEL);

    dvz_panel_add_visual(panel, reference, NULL);
    dvz_panel_add_visual(panel, cube, NULL);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        dvz_fprintf(stderr, "dvz_app() failed (no GPU or display?)\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzAppWindow* win =
        dvz_app_window_glfw(app, figure, WIDTH, HEIGHT, "depth_peel");
    if (win == NULL)
    {
        dvz_fprintf(stderr, "dvz_app_window_glfw() failed (GLFW unavailable?)\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }

    dvz_panel_set_arcball(panel, dvz_app_window_input(win), 0);
    DvzArcball* arcball = dvz_panel_arcball(panel);
    if (arcball == NULL)
    {
        dvz_fprintf(stderr, "dvz_panel_set_arcball() failed\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_arcball_set(arcball, (vec3){+0.65f, 0.0f, +0.35f});

    dvz_scene_set_clock_mode(scene, DVZ_CLOCK_REALTIME);
    dvz_scene_set_fps(scene, 60.0);

    DvzAnimation* spin = dvz_anim_arcball_spin(
        scene, arcball, (vec3){0.0f, 1.0f, 0.0f}, ROTATION_SPEED_RAD_PER_SEC,
        DVZ_ARCBALL_SPIN_FLAGS_PAUSE_ON_INTERACTION);
    if (spin == NULL)
    {
        dvz_fprintf(stderr, "dvz_anim_arcball_spin() failed\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_anim_start(spin, 0.0);

    dvz_app_run(app, frame_count);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}
