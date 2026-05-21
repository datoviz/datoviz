/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* primitive - live primitive-visual stress workbench.
 *
 * Build:  just build
 * Run:    just example-c visuals/primitive
 * Smoke:  ./build/examples/c/visuals/primitive 120
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
#include "example_common.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  1100u
#define HEIGHT 760u

#define MAX_TRIANGLES 65536u
#define VERTICES_PER_TRIANGLE 3u

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct PrimitiveState
{
    DvzVisual* visual;
    DvzAppWindow* win;
    float (*positions)[3];
    float (*normals)[3];
    DvzColor* colors;
    uint32_t max_triangles;
    uint32_t triangle_count;
    float triangle_value;
    float scale;
    float alpha;
    float phase;
    bool animate;
    bool blended;
} PrimitiveState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Normalize a 3D vector in place, falling back to +Z for degenerate inputs.
 *
 * @param v vector to normalize
 */
static void _normalize3(float v[3])
{
    ANN(v);

    const float len2 = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
    if (len2 <= 0.0f)
    {
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 1.0f;
        return;
    }

    const float inv_len = 1.0f / sqrtf(len2);
    v[0] *= inv_len;
    v[1] *= inv_len;
    v[2] *= inv_len;
}



/**
 * Compute a 3D cross product.
 *
 * @param a first input vector
 * @param b second input vector
 * @param out output vector
 */
static void _cross3(const float a[3], const float b[3], float out[3])
{
    ANN(a);
    ANN(b);
    ANN(out);

    out[0] = a[1] * b[2] - a[2] * b[1];
    out[1] = a[2] * b[0] - a[0] * b[2];
    out[2] = a[0] * b[1] - a[1] * b[0];
}



/**
 * Fill deterministic triangle-list buffers.
 *
 * @param state primitive workbench state
 * @param phase animation phase in radians
 */
static void _fill_triangles(PrimitiveState* state, float phase)
{
    ANN(state);
    ANN(state->positions);
    ANN(state->normals);
    ANN(state->colors);

    const uint32_t count = state->triangle_count;
    const uint32_t side = (uint32_t)ceilf(sqrtf((float)count));
    const float inv_side = side > 1 ? 1.0f / (float)(side - 1) : 1.0f;
    const float radius = 1.15f * state->scale / (float)side;

    for (uint32_t i = 0; i < count; i++)
    {
        const uint32_t x = i % side;
        const uint32_t y = i / side;
        const float u = (float)x * inv_side;
        const float v = (float)y * inv_side;
        const float px = 2.0f * u - 1.0f;
        const float py = 2.0f * v - 1.0f;
        const float r = sqrtf(px * px + py * py);
        const float theta = atan2f(py, px);
        const float twist = theta + 0.38f * sinf(phase * 0.45f + 4.0f * r);
        const float ridge = sinf(phase + 10.0f * r + 4.0f * theta);
        const float dome = fmaxf(0.0f, 1.0f - 0.55f * r * r);
        const float cx = 1.18f * r * cosf(twist);
        const float cy = 0.92f * r * sinf(twist);
        const float cz = 0.62f * dome + 0.16f * ridge - 0.30f;
        const float angle = phase + TAU * (0.13f * (float)x + 0.09f * (float)y);
        const uint32_t base = VERTICES_PER_TRIANGLE * i;
        float normal[3] = {
            -0.42f * px + 0.20f * cosf(phase + 6.0f * v),
            -0.42f * py + 0.20f * sinf(phase + 6.0f * u),
            1.0f,
        };
        _normalize3(normal);

        float tangent[3] = {-normal[1], normal[0], 0.0f};
        if (tangent[0] * tangent[0] + tangent[1] * tangent[1] <= 0.0001f)
        {
            tangent[0] = 1.0f;
            tangent[1] = 0.0f;
            tangent[2] = 0.0f;
        }
        _normalize3(tangent);

        float bitangent[3] = {0};
        _cross3(normal, tangent, bitangent);
        _normalize3(bitangent);

        for (uint32_t k = 0; k < VERTICES_PER_TRIANGLE; k++)
        {
            const float a = angle + TAU * (float)k / 3.0f;
            const float ca = cosf(a);
            const float sa = sinf(a);
            const float local = 0.82f + 0.22f * (float)k;
            state->positions[base + k][0] =
                cx + radius * local * (ca * tangent[0] + sa * bitangent[0]);
            state->positions[base + k][1] =
                cy + radius * local * (ca * tangent[1] + sa * bitangent[1]);
            state->positions[base + k][2] =
                cz + radius * local * (ca * tangent[2] + sa * bitangent[2]);
            state->normals[base + k][0] = normal[0];
            state->normals[base + k][1] = normal[1];
            state->normals[base + k][2] = normal[2];
            state->colors[base + k][0] =
                (uint8_t)(72u + (uint32_t)(145.0f * (0.5f + 0.5f * ridge)));
            state->colors[base + k][1] = (uint8_t)(64u + (uint32_t)(145.0f * dome));
            state->colors[base + k][2] = (uint8_t)(112u + (uint32_t)(108.0f * (1.0f - 0.35f * u)));
            state->colors[base + k][3] = (uint8_t)(255.0f * state->alpha);
        }
    }
}



/**
 * Upload the active primitive arrays.
 *
 * @param state primitive workbench state
 * @return true if the upload succeeded
 */
static bool _upload_triangles(PrimitiveState* state)
{
    ANN(state);
    ANN(state->visual);

    const uint32_t vertex_count = VERTICES_PER_TRIANGLE * state->triangle_count;
    if (dvz_primitive_data(
            state->visual, state->positions, state->colors, state->normals, vertex_count) != 0)
        return false;

    DvzAlphaMode alpha_mode =
        state->blended || state->alpha < 0.999f ? DVZ_ALPHA_BLENDED : DVZ_ALPHA_OPAQUE;
    return dvz_visual_set_alpha_mode(state->visual, alpha_mode) == 0;
}



/**
 * Render primitive visual controls.
 *
 * @param gui GUI context
 * @param win app window
 * @param user_data primitive workbench state
 */
static void _gui_callback(DvzGui* gui, DvzAppWindow* win, void* user_data)
{
    (void)win;
    PrimitiveState* state = (PrimitiveState*)user_data;
    ANN(state);

    bool changed = false;
    if (dvz_gui_begin(gui, "Primitive", NULL, 0))
    {
        changed |=
            dvz_gui_slider_float(gui, "Triangles", &state->triangle_value, 256.0f, MAX_TRIANGLES);
        changed |= dvz_gui_slider_float(gui, "Scale", &state->scale, 0.5f, 2.5f);
        changed |= dvz_gui_slider_float(gui, "Alpha", &state->alpha, 0.02f, 1.0f);
        changed |= dvz_gui_checkbox(gui, "Blended", &state->blended);
        (void)dvz_gui_checkbox(gui, "Animate", &state->animate);
    }
    dvz_gui_end(gui);

    if (changed)
    {
        state->triangle_count = (uint32_t)state->triangle_value;
        if (state->triangle_count < 1)
            state->triangle_count = 1;
        if (state->triangle_count > state->max_triangles)
            state->triangle_count = state->max_triangles;
        _fill_triangles(state, 0.0f);
        (void)_upload_triangles(state);
        dvz_app_window_request_frame(state->win);
    }
}



/**
 * Update animated primitive data before each frame.
 *
 * @param win app window
 * @param user_data primitive workbench state
 */
static void _frame_callback(DvzAppWindow* win, void* user_data)
{
    PrimitiveState* state = (PrimitiveState*)user_data;
    if (state == NULL || !state->animate)
        return;

    state->phase += 0.025f;
    _fill_triangles(state, state->phase);
    (void)_upload_triangles(state);
    dvz_app_window_request_frame(win);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the primitive visual stress workbench.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    PrimitiveState state = {
        .max_triangles = MAX_TRIANGLES,
        .triangle_count = 8192u,
        .triangle_value = 8192.0f,
        .scale = 1.35f,
        .alpha = 1.0f,
        .animate = true,
    };
    state.positions = dvz_calloc(MAX_TRIANGLES * VERTICES_PER_TRIANGLE, sizeof(*state.positions));
    state.normals = dvz_calloc(MAX_TRIANGLES * VERTICES_PER_TRIANGLE, sizeof(*state.normals));
    state.colors = dvz_calloc(MAX_TRIANGLES * VERTICES_PER_TRIANGLE, sizeof(*state.colors));
    if (state.positions == NULL || state.normals == NULL || state.colors == NULL)
    {
        dvz_fprintf(stderr, "primitive buffer allocation failed\n");
        return 1;
    }

    DvzScene* scene = dvz_scene();
    if (scene == NULL)
    {
        dvz_fprintf(stderr, "dvz_scene() failed\n");
        return 1;
    }
    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    DvzPanel* panel = figure != NULL ? dvz_panel_full(figure) : NULL;
    state.visual =
        panel != NULL ? dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0) : NULL;
    if (figure == NULL || panel == NULL || state.visual == NULL)
    {
        dvz_fprintf(stderr, "primitive scene setup failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.eye[2] = 3.6f;
    camera_desc.near = 0.1f;
    camera_desc.far = 100.0f;
    if (!dvz_panel_set_camera(panel, &camera_desc))
    {
        dvz_fprintf(stderr, "dvz_panel_set_camera() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    dvz_panel_set_background_color(panel, 0.040f, 0.043f, 0.052f, 1.0f);
    _fill_triangles(&state, 0.0f);
    if (!_upload_triangles(&state) ||
        dvz_visual_set_primitive_shading(
            state.visual,
            &(DvzPrimitiveShadingDesc){
                .light_direction = {0.32f, 0.46f, 0.82f},
                .ambient = 0.23f,
                .diffuse = 0.78f,
                .specular = 0.34f,
                .shininess = 48.0f,
            }) != 0 ||
        dvz_panel_add_visual(panel, state.visual, NULL) != 0)
    {
        dvz_fprintf(stderr, "primitive visual setup failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        dvz_fprintf(stderr, "dvz_app() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }
    state.win = dvz_app_window_glfw(app, figure, WIDTH, HEIGHT, "primitive");
    if (state.win == NULL)
    {
        dvz_fprintf(stderr, "dvz_app_window_glfw() failed\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }
    DvzArcball* arcball = dvz_app_window_panel_arcball(state.win, panel, NULL);
    if (arcball == NULL)
    {
        dvz_fprintf(stderr, "failed to create or bind arcball controller\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_arcball_set(arcball, (vec3){+0.54f, -0.10f, +0.26f});
    DvzGui* gui = dvz_app_window_gui(state.win, NULL);
    if (gui != NULL)
        dvz_app_window_set_gui_callback(state.win, _gui_callback, &state);
    dvz_app_window_set_frame_callback(state.win, _frame_callback, &state);

    dvz_app_run(app, example_frame_count(argc, argv));

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    dvz_free(state.colors);
    dvz_free(state.normals);
    dvz_free(state.positions);
    return 0;
}
