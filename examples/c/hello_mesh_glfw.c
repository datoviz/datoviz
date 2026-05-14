/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* hello_mesh_glfw — live rotating lit cube mesh via dvz_mesh + scene/app.
 *
 * Opens a GLFW window showing one indexed cube mesh with explicit face normals, per-face colours,
 * depth testing, and a perspective camera. The frame callback advances the panel arcball while the
 * user is idle so the retained 3D scene exercises the live scene -> DRP2 -> vklite/canvas path
 * continuously without fighting interactive arcball gestures.
 *
 * Build:  just example-c hello_mesh_glfw
 * Run:    ./build/examples/c/hello_mesh_glfw
 * Smoke:  ./build/examples/c/hello_mesh_glfw 60
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "_alloc.h"
#include "datoviz/app.h"
#include "datoviz/input/pointer.h"
#include "datoviz/scene.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  800
#define HEIGHT 600

#define ROTATION_SPEED_RAD_PER_SEC 0.9f



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct MeshGlfwState MeshGlfwState;

struct MeshGlfwState
{
    DvzArcball* arcball;
    uint64_t last_timestamp_ns;
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Build an indexed cube with duplicated vertices and per-face normals.
 *
 * @param positions output vertex positions
 * @param colors output vertex colors
 * @param normals output vertex normals
 * @param indices output triangle-list indices
 */
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
            dvz_memcpy(colors[vertex], sizeof(DvzColor), face_colors[face], sizeof(DvzColor));
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
    if (argc < 2 || argv == NULL || argv[1] == NULL)
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
 * Advance the cube orientation for the next frame.
 *
 * @param win app-window whose frame just completed
 * @param user_data mesh GLFW example state
 */
static void _mesh_glfw_frame(DvzAppWindow* win, void* user_data)
{
    (void)win;
    MeshGlfwState* state = (MeshGlfwState*)user_data;
    if (state == NULL || state->arcball == NULL)
        return;

    uint64_t now = dvz_input_timestamp_ns();
    if (state->last_timestamp_ns == 0)
        state->last_timestamp_ns = now;

    uint64_t elapsed_ns = now - state->last_timestamp_ns;
    state->last_timestamp_ns = now;
    float elapsed = (float)((double)elapsed_ns / 1000000000.0);
    if (elapsed > 0.1f)
        elapsed = 0.1f;

    if (!dvz_arcball_is_interacting(state->arcball))
    {
        dvz_arcball_rotate_axis(
            state->arcball, ROTATION_SPEED_RAD_PER_SEC * elapsed, (vec3){0.0f, 1.0f, 0.0f});
    }
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

int main(int argc, char** argv)
{
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
    camera_desc.eye[2] = 3.0f;
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

    DvzVisual* visual = dvz_mesh(scene, 0);
    if (visual == NULL)
    {
        fprintf(stderr, "dvz_mesh() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

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
    if (index_buffer == NULL)
    {
        fprintf(stderr, "dvz_scene_buffer() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }
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
    if (app == NULL)
    {
        fprintf(stderr, "dvz_app() failed (no GPU or display?)\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzAppWindow* win = dvz_app_window_glfw(app, figure, WIDTH, HEIGHT, "hello_mesh_glfw");
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

    MeshGlfwState state = {.arcball = arcball};
    dvz_app_window_set_frame_callback(win, _mesh_glfw_frame, &state);

    dvz_app_run(app, _frame_count(argc, argv));

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}
