/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* hello_mesh_ssao_glfw - lit height-field mesh with internal SSAO controls.
 *
 * Build:  just example-c hello_mesh_ssao_glfw
 * Run:    ./build/examples/c/hello_mesh_ssao_glfw
 * Smoke:  ./build/examples/c/hello_mesh_ssao_glfw 60
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "datoviz/app.h"
#include "datoviz/gui.h"
#include "datoviz/scene.h"
#include "../../src/scene/_technique.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  1000u
#define HEIGHT 760u
#define GRID   64u

#define ROTATION_SPEED_RAD_PER_SEC 0.22f



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct SsaoExampleState
{
    DvzPanel* panel;
    DvzAnimation* spin;
    bool ssao_enabled;
    bool spin_enabled;
    float radius;
    float strength;
    float bias;
    float sample_count;
} SsaoExampleState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

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



/**
 * Return a smooth terrain height at normalized coordinates.
 *
 * @param x normalized X coordinate
 * @param z normalized Z coordinate
 * @return height value
 */
static float _height(float x, float z)
{
    float r2a = (x + 0.38f) * (x + 0.38f) + (z - 0.18f) * (z - 0.18f);
    float r2b = (x - 0.28f) * (x - 0.28f) + (z + 0.24f) * (z + 0.24f);
    float ridge = expf(-14.0f * r2a) + 0.72f * expf(-22.0f * r2b);
    float waves = 0.09f * sinf(10.0f * x + 3.0f * z) + 0.07f * cosf(8.0f * z);
    return 0.42f * ridge + waves - 0.18f;
}



/**
 * Normalize a vector in place.
 *
 * @param v vector to normalize
 */
static void _normalize3(float v[3])
{
    float n = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (n <= 1e-8f)
    {
        v[0] = 0.0f;
        v[1] = 1.0f;
        v[2] = 0.0f;
        return;
    }
    v[0] /= n;
    v[1] /= n;
    v[2] /= n;
}



/**
 * Convert a normalized float channel to an 8-bit color channel.
 *
 * @param value normalized channel value
 * @return clamped 8-bit channel value
 */
static uint8_t _u8(float value)
{
    if (value < 0.0f)
        value = 0.0f;
    if (value > 1.0f)
        value = 1.0f;
    return (uint8_t)(255.0f * value + 0.5f);
}



/**
 * Build a deterministic indexed height-field mesh.
 *
 * @param positions output positions
 * @param colors output colors
 * @param normals output normals
 * @param indices output triangle indices
 */
static void _build_heightfield(
    float (*positions)[3], DvzColor* colors, float (*normals)[3], DvzIndex* indices)
{
    ANN(positions);
    ANN(colors);
    ANN(normals);
    ANN(indices);

    const float step = 2.0f / (float)(GRID - 1);
    for (uint32_t j = 0; j < GRID; j++)
    {
        for (uint32_t i = 0; i < GRID; i++)
        {
            uint32_t idx = j * GRID + i;
            float x = -1.0f + step * (float)i;
            float z = -1.0f + step * (float)j;
            float y = _height(x, z);
            positions[idx][0] = x;
            positions[idx][1] = y;
            positions[idx][2] = z;

            float hx0 = _height(x - step, z);
            float hx1 = _height(x + step, z);
            float hz0 = _height(x, z - step);
            float hz1 = _height(x, z + step);
            normals[idx][0] = -(hx1 - hx0);
            normals[idx][1] = 2.0f * step;
            normals[idx][2] = -(hz1 - hz0);
            _normalize3(normals[idx]);

            float t = fminf(fmaxf((y + 0.35f) / 0.85f, 0.0f), 1.0f);
            colors[idx][0] = _u8(0.18f + 0.55f * t);
            colors[idx][1] = _u8(0.35f + 0.42f * (1.0f - fabsf(t - 0.55f)));
            colors[idx][2] = _u8(0.62f - 0.34f * t);
            colors[idx][3] = 255;
        }
    }

    uint32_t k = 0;
    for (uint32_t j = 0; j + 1 < GRID; j++)
    {
        for (uint32_t i = 0; i + 1 < GRID; i++)
        {
            uint32_t a = j * GRID + i;
            uint32_t b = j * GRID + i + 1;
            uint32_t c = (j + 1) * GRID + i;
            uint32_t d = (j + 1) * GRID + i + 1;
            indices[k++] = a;
            indices[k++] = b;
            indices[k++] = c;
            indices[k++] = c;
            indices[k++] = b;
            indices[k++] = d;
        }
    }
}



/**
 * Apply the internal SSAO state to the panel.
 *
 * @param state example state
 */
static void _apply_ssao(SsaoExampleState* state)
{
    ANN(state);
    ANN(state->panel);

    if (!state->ssao_enabled)
    {
        (void)_scene_technique_state_set_ssao(&state->panel->techniques, NULL);
        return;
    }
    if (state->sample_count < 4.0f)
        state->sample_count = 4.0f;
    if (state->sample_count > 16.0f)
        state->sample_count = 16.0f;

    DvzSceneSsaoDesc desc = {
        .radius = state->radius,
        .strength = state->strength,
        .bias = state->bias,
        .sample_count = (uint32_t)(state->sample_count + 0.5f),
    };
    if (!_scene_technique_state_set_ssao(&state->panel->techniques, &desc))
        dvz_fprintf(stderr, "_scene_technique_state_set_ssao() failed\n");
}



/**
 * Apply the retained spin control to the scene animation.
 *
 * @param state example state
 */
static void _apply_spin(SsaoExampleState* state)
{
    ANN(state);
    if (state->spin == NULL)
        return;
    if (state->spin_enabled)
        dvz_anim_start(state->spin, 0.0);
    else
        dvz_anim_stop(state->spin);
}



/**
 * Reset the live SSAO controls to useful defaults.
 *
 * @param state example state
 */
static void _reset_ssao(SsaoExampleState* state)
{
    ANN(state);
    state->ssao_enabled = true;
    state->spin_enabled = true;
    state->radius = 1.15f;
    state->strength = 3.2f;
    state->bias = 0.015f;
    state->sample_count = 16.0f;
    _apply_ssao(state);
    _apply_spin(state);
}



/**
 * Build the live SSAO controls.
 *
 * @param gui GUI overlay
 * @param win app window
 * @param user_data example state
 */
static void _ssao_gui(DvzGui* gui, DvzAppWindow* win, void* user_data)
{
    (void)win;
    SsaoExampleState* state = (SsaoExampleState*)user_data;
    if (state == NULL)
        return;

    bool ssao_changed = false;
    bool spin_changed = false;
    if (dvz_gui_begin(gui, "SSAO", NULL, 0))
    {
        spin_changed |= dvz_gui_checkbox(gui, "Auto rotate", &state->spin_enabled);
        ssao_changed |= dvz_gui_checkbox(gui, "Enable SSAO", &state->ssao_enabled);
        ssao_changed |= dvz_gui_slider_float(gui, "Radius", &state->radius, 0.05f, 4.0f);
        ssao_changed |= dvz_gui_slider_float(gui, "Strength", &state->strength, 0.0f, 12.0f);
        ssao_changed |= dvz_gui_slider_float(gui, "Bias", &state->bias, 0.0f, 0.12f);
        ssao_changed |= dvz_gui_slider_float(gui, "Samples", &state->sample_count, 4.0f, 16.0f);
        if (dvz_gui_button(gui, "Reset"))
        {
            _reset_ssao(state);
            ssao_changed = false;
            spin_changed = false;
        }
    }
    dvz_gui_end(gui);

    if (ssao_changed)
        _apply_ssao(state);
    if (spin_changed)
        _apply_spin(state);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    DvzScene* scene = dvz_scene();
    if (scene == NULL)
    {
        dvz_fprintf(stderr, "dvz_scene() failed\n");
        return 1;
    }

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    if (figure == NULL)
    {
        dvz_fprintf(stderr, "dvz_figure() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzPanel* panel =
        dvz_panel(figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    if (panel == NULL)
    {
        dvz_fprintf(stderr, "dvz_panel() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.eye[2] = 3.35f;
    camera_desc.up[1] = 1.0f;
    camera_desc.fov_y = 0.72f;
    camera_desc.near = 0.1f;
    camera_desc.far = 100.0f;
    if (!dvz_panel_set_camera(panel, &camera_desc))
    {
        dvz_fprintf(stderr, "dvz_panel_set_camera() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    const uint32_t vertex_count = GRID * GRID;
    const uint32_t index_count = (GRID - 1) * (GRID - 1) * 6;
    float(*positions)[3] = (float(*)[3])dvz_calloc(vertex_count, sizeof(*positions));
    DvzColor* colors = (DvzColor*)dvz_calloc(vertex_count, sizeof(DvzColor));
    float(*normals)[3] = (float(*)[3])dvz_calloc(vertex_count, sizeof(*normals));
    DvzIndex* indices = (DvzIndex*)dvz_calloc(index_count, sizeof(DvzIndex));
    if (positions == NULL || colors == NULL || normals == NULL || indices == NULL)
    {
        dvz_fprintf(stderr, "mesh allocation failed\n");
        dvz_free(indices);
        dvz_free(normals);
        dvz_free(colors);
        dvz_free(positions);
        dvz_scene_destroy(scene);
        return 1;
    }
    _build_heightfield(positions, colors, normals, indices);

    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    if (index_buffer == NULL || !dvz_scene_buffer_set_data(
                                    index_buffer, indices, index_count * sizeof(DvzIndex)))
    {
        dvz_fprintf(stderr, "index buffer setup failed\n");
        dvz_free(indices);
        dvz_free(normals);
        dvz_free(colors);
        dvz_free(positions);
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzVisual* visual = dvz_mesh(scene, 0);
    if (visual == NULL)
    {
        dvz_fprintf(stderr, "dvz_mesh() failed\n");
        dvz_free(indices);
        dvz_free(normals);
        dvz_free(colors);
        dvz_free(positions);
        dvz_scene_destroy(scene);
        return 1;
    }
    if (dvz_visual_set_data(visual, "position", positions, vertex_count) != 0 ||
        dvz_visual_set_data(visual, "color", colors, vertex_count) != 0 ||
        dvz_visual_set_data(visual, "normal", normals, vertex_count) != 0 ||
        !dvz_visual_set_buffer(visual, "index", index_buffer) ||
        dvz_panel_add_visual(panel, visual, NULL) != 0)
    {
        dvz_fprintf(stderr, "mesh visual setup failed\n");
        dvz_free(indices);
        dvz_free(normals);
        dvz_free(colors);
        dvz_free(positions);
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_visual_set_primitive_shading(
        visual,
        &(DvzPrimitiveShadingDesc){
            .light_direction = {0.30f, 0.70f, 0.62f},
            .ambient = 0.34f,
            .diffuse = 0.82f,
        });
    dvz_panel_set_background_color(panel, 0.035f, 0.040f, 0.052f, 1.0f);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        dvz_fprintf(stderr, "dvz_app() failed (no GPU or display?)\n");
        dvz_free(indices);
        dvz_free(normals);
        dvz_free(colors);
        dvz_free(positions);
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzAppWindow* win = dvz_app_window_glfw(app, figure, WIDTH, HEIGHT, "hello_mesh_ssao_glfw");
    if (win == NULL)
    {
        dvz_fprintf(stderr, "dvz_app_window_glfw() failed (GLFW unavailable?)\n");
        dvz_app_destroy(app);
        dvz_free(indices);
        dvz_free(normals);
        dvz_free(colors);
        dvz_free(positions);
        dvz_scene_destroy(scene);
        return 1;
    }

    dvz_panel_set_arcball(panel, dvz_app_window_input(win), 0);
    DvzArcball* arcball = dvz_panel_arcball(panel);
    if (arcball == NULL)
    {
        dvz_fprintf(stderr, "dvz_panel_set_arcball() failed\n");
        dvz_app_destroy(app);
        dvz_free(indices);
        dvz_free(normals);
        dvz_free(colors);
        dvz_free(positions);
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_arcball_set(arcball, (vec3){+0.92f, 0.0f, +0.18f});
    dvz_scene_set_clock_mode(scene, DVZ_CLOCK_REALTIME);
    dvz_scene_set_fps(scene, 60.0);

    DvzAnimation* spin = dvz_anim_arcball_spin(
        scene, arcball, (vec3){0.0f, 1.0f, 0.0f}, ROTATION_SPEED_RAD_PER_SEC,
        DVZ_ARCBALL_SPIN_FLAGS_PAUSE_ON_INTERACTION);
    if (spin == NULL)
    {
        dvz_fprintf(stderr, "dvz_anim_arcball_spin() failed\n");
        dvz_app_destroy(app);
        dvz_free(indices);
        dvz_free(normals);
        dvz_free(colors);
        dvz_free(positions);
        dvz_scene_destroy(scene);
        return 1;
    }

    SsaoExampleState state = {
        .panel = panel,
        .spin = spin,
    };
    _reset_ssao(&state);

    DvzGuiConfig gui_config = dvz_gui_config();
    DvzGui* gui = dvz_app_window_gui(win, &gui_config);
    if (gui == NULL)
    {
        dvz_fprintf(stderr, "dvz_app_window_gui() failed\n");
        dvz_app_destroy(app);
        dvz_free(indices);
        dvz_free(normals);
        dvz_free(colors);
        dvz_free(positions);
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_app_window_set_gui_callback(win, _ssao_gui, &state);

    dvz_app_run(app, _frame_count(argc, argv));

    dvz_app_destroy(app);
    dvz_free(indices);
    dvz_free(normals);
    dvz_free(colors);
    dvz_free(positions);
    dvz_scene_destroy(scene);
    return 0;
}
