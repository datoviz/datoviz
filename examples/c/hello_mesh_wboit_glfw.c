/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* hello_mesh_wboit_glfw — transparent mesh shell via scene WBOIT + app/GLFW.
 *
 * Opens a GLFW window showing a transparent WBOIT cube inside an explicitly WBOIT transparent cube
 * shell. A GUI overlay exposes live shell color, alpha, ambient/diffuse, and light-direction sliders.
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
#include "_assertions.h"
#include "datoviz/app.h"
#include "datoviz/gui.h"
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
    DvzVisual* shell;
    uint32_t shell_vertex_count;
    DvzColor shell_colors[24];
    float shell_rgb[3];
    float shell_alpha;
    float shell_light_direction[3];
    float shell_ambient;
    float shell_diffuse;
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
 * Convert a normalized float channel to an 8-bit color channel.
 *
 * @param value normalized channel value
 * @return clamped 8-bit channel value
 */
static uint8_t _u8_from_unit(float value)
{
    if (value < 0.0f)
        value = 0.0f;
    if (value > 1.0f)
        value = 1.0f;
    return (uint8_t)(255.0f * value + 0.5f);
}



/**
 * Upload the GUI-controlled WBOIT shell material.
 *
 * @param state mesh WBOIT example state
 */
static void _mesh_wboit_update_shell(MeshWboitState* state)
{
    ANN(state);
    if (state->shell == NULL || state->shell_vertex_count == 0)
        return;

    DvzColor color = {
        _u8_from_unit(state->shell_rgb[0]),
        _u8_from_unit(state->shell_rgb[1]),
        _u8_from_unit(state->shell_rgb[2]),
        _u8_from_unit(state->shell_alpha),
    };
    for (uint32_t i = 0; i < state->shell_vertex_count; i++)
        dvz_memcpy(state->shell_colors[i], sizeof(DvzColor), color, sizeof(DvzColor));

    dvz_visual_set_data(state->shell, "color", state->shell_colors, state->shell_vertex_count);
    dvz_visual_set_primitive_shading(
        state->shell,
        &(DvzPrimitiveShadingDesc){
            .light_direction = {
                state->shell_light_direction[0],
                state->shell_light_direction[1],
                state->shell_light_direction[2],
            },
            .ambient = state->shell_ambient,
            .diffuse = state->shell_diffuse,
        });
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



/**
 * Build the live WBOIT material controls.
 *
 * @param gui GUI overlay
 * @param win app window
 * @param user_data mesh WBOIT example state
 */
static void _mesh_wboit_gui(DvzGui* gui, DvzAppWindow* win, void* user_data)
{
    (void)win;
    MeshWboitState* state = (MeshWboitState*)user_data;
    if (state == NULL)
        return;

    bool changed = false;
    if (dvz_gui_begin(gui, "WBOIT shell", NULL, 0))
    {
        changed |= dvz_gui_slider_float(gui, "Red", &state->shell_rgb[0], 0.0f, 1.0f);
        changed |= dvz_gui_slider_float(gui, "Green", &state->shell_rgb[1], 0.0f, 1.0f);
        changed |= dvz_gui_slider_float(gui, "Blue", &state->shell_rgb[2], 0.0f, 1.0f);
        changed |= dvz_gui_slider_float(gui, "Alpha", &state->shell_alpha, 0.02f, 0.65f);
        changed |= dvz_gui_slider_float(gui, "Ambient", &state->shell_ambient, 0.0f, 1.0f);
        changed |= dvz_gui_slider_float(gui, "Diffuse", &state->shell_diffuse, 0.0f, 1.5f);
        changed |=
            dvz_gui_slider_float(gui, "Light X", &state->shell_light_direction[0], -1.0f, 1.0f);
        changed |=
            dvz_gui_slider_float(gui, "Light Y", &state->shell_light_direction[1], -1.0f, 1.0f);
        changed |=
            dvz_gui_slider_float(gui, "Light Z", &state->shell_light_direction[2], -1.0f, 1.0f);
        if (dvz_gui_button(gui, "Reset"))
        {
            state->shell_rgb[0] = 56.0f / 255.0f;
            state->shell_rgb[1] = 220.0f / 255.0f;
            state->shell_rgb[2] = 1.0f;
            state->shell_alpha = 72.0f / 255.0f;
            state->shell_light_direction[0] = 0.25f;
            state->shell_light_direction[1] = 0.70f;
            state->shell_light_direction[2] = 0.45f;
            state->shell_ambient = 0.18f;
            state->shell_diffuse = 0.95f;
            changed = true;
        }
    }
    dvz_gui_end(gui);

    if (changed)
        _mesh_wboit_update_shell(state);
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
    DvzColor inner_color = {245, 245, 245, 160};
    _build_cube(0.42f, inner_color, inner_positions, inner_colors, inner_normals, indices);

    float shell_positions[24][3] = {0};
    DvzColor shell_colors[24] = {0};
    float shell_normals[24][3] = {0};
    DvzColor shell_color = {56, 220, 255, 72};
    _build_cube(0.72f, shell_color, shell_positions, shell_colors, shell_normals, indices);

    MeshWboitState state = {
        .shell = shell,
        .shell_vertex_count = 24,
        .shell_rgb = {56.0f / 255.0f, 220.0f / 255.0f, 1.0f},
        .shell_alpha = 72.0f / 255.0f,
        .shell_light_direction = {0.25f, 0.70f, 0.45f},
        .shell_ambient = 0.18f,
        .shell_diffuse = 0.95f,
    };
    dvz_memcpy(state.shell_colors, sizeof(state.shell_colors), shell_colors, sizeof(shell_colors));

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
    dvz_visual_set_alpha_mode(inner, DVZ_ALPHA_WBOIT);
    dvz_visual_set_primitive_shading(
        inner,
        &(DvzPrimitiveShadingDesc){
            .light_direction = {0.35f, 0.55f, 0.75f},
            .ambient = 0.25f,
            .diffuse = 0.85f,
        });

    dvz_visual_set_data(shell, "position", shell_positions, 24);
    dvz_visual_set_data(shell, "normal", shell_normals, 24);
    dvz_visual_set_buffer(shell, "index", index_buffer);
    _mesh_wboit_update_shell(&state);
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
    state.arcball = arcball;

    DvzGuiConfig gui_config = dvz_gui_config();
    DvzGui* gui = dvz_app_window_gui(win, &gui_config);
    if (gui == NULL)
    {
        fprintf(stderr, "dvz_app_window_gui() failed\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_app_window_set_gui_callback(win, _mesh_wboit_gui, &state);

    dvz_scene_set_clock_mode(scene, DVZ_CLOCK_REALTIME);
    dvz_scene_set_fps(scene, 60.0);

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
