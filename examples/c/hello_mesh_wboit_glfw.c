/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* hello_mesh_wboit_glfw — transparent mesh shell via scene WBOIT + app/GLFW.
 *
 * Opens a GLFW window showing an opaque cube inside an explicitly WBOIT transparent cube shell.
 * The visual alpha-mode opt-in exercises scene planning, DRP2 WBOIT accumulation/resolve, and the
 * vklite runtime while retaining the normal app/canvas presentation path.
 *
 * Build:  just example-c hello_mesh_wboit_glfw
 * Run:    ./build/examples/c/hello_mesh_wboit_glfw
 * Smoke:  ./build/examples/c/hello_mesh_wboit_glfw 60
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "_alloc.h"
#include "datoviz/app.h"
#include "datoviz/scene.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  800
#define HEIGHT 600

#define ROTATION_SPEED_RAD_PER_SEC 0.65f



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct MeshWboitState MeshWboitState;

struct MeshWboitState
{
    DvzArcball* arcball;
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Build an indexed cube with duplicated vertices, per-face normals, and one shell color.
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



/**
 * Parse an optional bounded frame count from the command line.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return requested frame count, or 0 for the interactive loop
 */
static uint32_t _frame_count(int argc, char** argv)
{
    if (argc < 2 || argv == NULL)
        return 0;
    char* end = NULL;
    unsigned long value = strtoul(argv[1], &end, 10);
    if (end == argv[1] || (end != NULL && *end != '\0'))
        return 0;
    if (value > UINT32_MAX)
        return UINT32_MAX;
    return (uint32_t)value;
}



/*************************************************************************************************/
/*  Callbacks                                                                                    */
/*************************************************************************************************/

/**
 * Advance the arcball orientation from the scene clock.
 *
 * @param animation timer animation
 * @param t current scene-clock time
 * @param dt elapsed scene-clock time since the previous step
 * @param user_data mesh WBOIT example state
 */
static void _mesh_wboit_timer(DvzAnimation* animation, double t, double dt, void* user_data)
{
    (void)animation;
    (void)t;
    MeshWboitState* state = (MeshWboitState*)user_data;
    if (state == NULL || state->arcball == NULL)
        return;
    if (!dvz_arcball_is_interacting(state->arcball))
        dvz_arcball_rotate_axis(
            state->arcball, ROTATION_SPEED_RAD_PER_SEC * (float)dt,
            (vec3){0.0f, 1.0f, 0.0f});
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    uint32_t frame_count = _frame_count(argc, argv);

    DvzScene* scene = dvz_scene();
    if (scene == NULL)
    {
        fprintf(stderr, "dvz_scene() failed\n");
        return 1;
    }

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    if (figure == NULL)
    {
        fprintf(stderr, "dvz_figure() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzPanel* panel =
        dvz_panel(figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    if (panel == NULL)
    {
        fprintf(stderr, "dvz_panel() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.eye[2] = 3.2f;
    camera_desc.up[1] = 1.0f;
    camera_desc.fov_y = 0.78539816339f;
    camera_desc.near = 0.1f;
    camera_desc.far = 100.0f;
    if (!dvz_panel_set_camera(panel, &camera_desc))
    {
        fprintf(stderr, "dvz_panel_set_camera() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzVisual* inner = dvz_mesh(scene, 0);
    DvzVisual* shell = dvz_mesh(scene, 0);
    if (inner == NULL || shell == NULL)
    {
        fprintf(stderr, "dvz_mesh() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    float inner_positions[24][3] = {0};
    DvzColor inner_colors[24] = {0};
    float inner_normals[24][3] = {0};
    DvzIndex indices[36] = {0};
    DvzColor inner_color = {245, 245, 245, 255};
    _build_cube(0.42f, inner_color, inner_positions, inner_colors, inner_normals, indices);

    float shell_positions[24][3] = {0};
    DvzColor shell_colors[24] = {0};
    float shell_normals[24][3] = {0};
    DvzColor shell_color = {56, 220, 255, 72};
    _build_cube(0.72f, shell_color, shell_positions, shell_colors, shell_normals, indices);

    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    if (index_buffer == NULL ||
        !dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)))
    {
        fprintf(stderr, "index buffer setup failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    dvz_visual_set_data(inner, "position", inner_positions, 24);
    dvz_visual_set_data(inner, "color", inner_colors, 24);
    dvz_visual_set_data(inner, "normal", inner_normals, 24);
    dvz_visual_set_buffer(inner, "index", index_buffer);
    dvz_visual_set_primitive_shading(
        inner,
        &(DvzPrimitiveShadingDesc){
            .light_direction = {0.35f, 0.55f, 0.75f},
            .ambient = 0.25f,
            .diffuse = 0.85f,
        });

    dvz_visual_set_data(shell, "position", shell_positions, 24);
    dvz_visual_set_data(shell, "color", shell_colors, 24);
    dvz_visual_set_data(shell, "normal", shell_normals, 24);
    dvz_visual_set_buffer(shell, "index", index_buffer);
    dvz_visual_set_primitive_shading(
        shell,
        &(DvzPrimitiveShadingDesc){
            .light_direction = {0.25f, 0.70f, 0.45f},
            .ambient = 0.18f,
            .diffuse = 0.95f,
        });
    dvz_visual_set_alpha_mode(shell, DVZ_ALPHA_WBOIT);

    dvz_panel_add_visual(panel, inner, NULL);
    dvz_panel_add_visual(panel, shell, NULL);
    dvz_panel_set_background_color(panel, 0.05f, 0.05f, 0.08f, 1.0f);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        fprintf(stderr, "dvz_app() failed (no GPU or display?)\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzAppWindow* win = dvz_app_window_glfw(app, figure, WIDTH, HEIGHT, "hello_mesh_wboit_glfw");
    if (win == NULL)
    {
        fprintf(stderr, "dvz_app_window_glfw() failed (GLFW unavailable?)\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }

    dvz_panel_set_arcball(panel, dvz_app_window_input(win), 0);
    DvzArcball* arcball = dvz_panel_arcball(panel);
    if (arcball == NULL)
    {
        fprintf(stderr, "dvz_panel_set_arcball() failed\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_arcball_set(arcball, (vec3){+0.65f, 0.0f, +0.35f});

    dvz_scene_set_clock_mode(scene, DVZ_CLOCK_REALTIME);
    dvz_scene_set_fps(scene, 60.0);

    MeshWboitState state = {.arcball = arcball};
    DvzAnimation* spin = dvz_anim_timer(scene, 0.0, _mesh_wboit_timer, &state);
    if (spin == NULL)
    {
        fprintf(stderr, "dvz_anim_timer() failed\n");
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
