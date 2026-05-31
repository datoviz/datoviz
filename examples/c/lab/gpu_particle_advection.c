/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* gpu_particle_advection - scene API compute-to-graphics lab.
 *
 * Scenario: lab_gpu_particle_advection
 * Style: experimental scene compute, graphite_cyan, optional capture
 *
 * Build:  just example-c lab/gpu_particle_advection
 * Run:    ./build/examples/c/lab/gpu_particle_advection 120
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "_alloc.h"
#include "_assertions.h"
#include "datoviz/app.h"
#include "datoviz/scene.h"
#include "example_common.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH          1600u
#define HEIGHT         1000u
#define PARTICLE_COUNT 65536u
#define WORKGROUP_SIZE 128u
#define DEFAULT_FRAMES 180u

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct ParticleState
{
    DvzSceneBuffer* params;
    uint32_t frame_index;
} ParticleState;



/*************************************************************************************************/
/*  Shaders                                                                                      */
/*************************************************************************************************/

static const char* COMPUTE_GLSL =
    "#version 450\n"
    "layout(local_size_x = 128) in;\n"
    "layout(std430, set = 0, binding = 0) readonly buffer Params { vec4 sim; } params;\n"
    "layout(std430, set = 0, binding = 1) buffer Positions { float x[]; } positions;\n"
    "layout(std430, set = 0, binding = 2) buffer Velocities { float v[]; } velocities;\n"
    "void main() {\n"
    "    uint i = gl_GlobalInvocationID.x;\n"
    "    uint count = uint(params.sim.z);\n"
    "    if (i >= count) return;\n"
    "    float t = params.sim.x;\n"
    "    float dt = params.sim.y;\n"
    "    uint j = 3u * i;\n"
    "    vec2 x = vec2(positions.x[j + 0u], positions.x[j + 1u]);\n"
    "    vec2 v = vec2(velocities.v[j + 0u], velocities.v[j + 1u]);\n"
    "    float k = 2.6 + 0.6 * sin(t * 0.17);\n"
    "    vec2 swirl = vec2(-x.y, x.x) / (0.16 + dot(x, x));\n"
    "    vec2 wave = vec2(\n"
    "        sin(7.0 * x.y + 1.9 * t) + 0.45 * sin(13.0 * x.x - 0.7 * t),\n"
    "        cos(6.0 * x.x - 1.4 * t) - 0.40 * sin(11.0 * x.y + 0.5 * t));\n"
    "    vec2 center = 0.20 * vec2(sin(0.31 * t), cos(0.23 * t));\n"
    "    vec2 pull = normalize(center - x + 1e-4) * 0.08;\n"
    "    v += dt * (0.50 * k * swirl + 0.16 * wave + pull);\n"
    "    v *= 0.991;\n"
    "    x += dt * v;\n"
    "    if (x.x < -1.05) x.x += 2.10;\n"
    "    if (x.x >  1.05) x.x -= 2.10;\n"
    "    if (x.y < -1.05) x.y += 2.10;\n"
    "    if (x.y >  1.05) x.y -= 2.10;\n"
    "    positions.x[j + 0u] = x.x;\n"
    "    positions.x[j + 1u] = x.y;\n"
    "    positions.x[j + 2u] = 0.0;\n"
    "    velocities.v[j + 0u] = v.x;\n"
    "    velocities.v[j + 1u] = v.y;\n"
    "    velocities.v[j + 2u] = 0.0;\n"
    "}\n";



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static float _hash01(uint32_t x)
{
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return (float)(x & 0x00ffffffu) / (float)0x01000000u;
}


static float _clamp01(float x)
{
    if (x < 0.0f)
        return 0.0f;
    if (x > 1.0f)
        return 1.0f;
    return x;
}


static DvzColor _particle_color(float radius, float phase)
{
    const float t = _clamp01((radius - 0.18f) / 0.78f);
    const uint8_t r = (uint8_t)(32.0f + 90.0f * t + 20.0f * sinf(phase));
    const uint8_t g = (uint8_t)(135.0f + 85.0f * (1.0f - 0.35f * t));
    const uint8_t b = (uint8_t)(205.0f + 45.0f * sinf(0.7f + phase));
    return dvz_color_rgba(r, g, b, 235);
}


static void _init_particles(vec3* positions, vec3* velocities, DvzColor* colors, float* sizes)
{
    ANN(positions);
    ANN(velocities);
    ANN(colors);
    ANN(sizes);

    for (uint32_t i = 0; i < PARTICLE_COUNT; i++)
    {
        const float a = TAU * _hash01(i * 3u + 1u);
        const float r = 0.18f + 0.78f * sqrtf(_hash01(i * 3u + 2u));
        const float jitter = _hash01(i * 3u + 3u);
        const float x = r * cosf(a);
        const float y = r * sinf(a) * 0.92f;
        positions[i][0] = x;
        positions[i][1] = y;
        positions[i][2] = 0.0f;
        velocities[i][0] = -0.16f * y;
        velocities[i][1] = +0.16f * x;
        velocities[i][2] = 0.0f;
        colors[i] = _particle_color(r, a);
        sizes[i] = 1.6f + 2.2f * jitter;
    }
}


static uint32_t _frame_count(int argc, char** argv)
{
    uint32_t frames = example_frame_count(argc, argv);
    if (frames == 0)
        frames = DEFAULT_FRAMES;
    if (frames > 1000)
        frames = 1000;
    return frames;
}


static void _params_for_frame(uint32_t frame, vec4 params)
{
    params[0] = (float)frame / 60.0f;
    params[1] = 1.0f / 60.0f;
    params[2] = (float)PARTICLE_COUNT;
    params[3] = 0.0f;
}


static void _particle_frame(DvzView* win, void* user_data)
{
    ParticleState* state = (ParticleState*)user_data;
    if (state == NULL || state->params == NULL)
        return;

    vec4 params = {0};
    _params_for_frame(state->frame_index++, params);
    (void)dvz_scene_buffer_set_data(state->params, params, sizeof(vec4));
    dvz_view_request_frame(win);
}



/*************************************************************************************************/
/*  Main                                                                                         */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    int rc = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;
    vec3* positions = NULL;
    vec3* velocities = NULL;
    DvzColor* colors = NULL;
    float* sizes = NULL;

    const uint32_t frames = _frame_count(argc, argv);
    positions = (vec3*)dvz_calloc(PARTICLE_COUNT, sizeof(vec3));
    velocities = (vec3*)dvz_calloc(PARTICLE_COUNT, sizeof(vec3));
    colors = (DvzColor*)dvz_calloc(PARTICLE_COUNT, sizeof(DvzColor));
    sizes = (float*)dvz_calloc(PARTICLE_COUNT, sizeof(float));
    EXAMPLE_CHECK(
        positions != NULL && velocities != NULL && colors != NULL && sizes != NULL,
        "gpu_particle_advection: allocation failed");
    _init_particles(positions, velocities, colors, sizes);

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");
    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    EXAMPLE_CHECK(panel != NULL, "dvz_panel() failed");

    DvzSceneBuffer* position_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){
                   .usage = DVZ_SCENE_BUFFER_USAGE_VERTEX | DVZ_SCENE_BUFFER_USAGE_STORAGE,
                   .stride = sizeof(vec3),
                   .byte_size = (uint64_t)PARTICLE_COUNT * sizeof(vec3),
               });
    DvzSceneBuffer* velocity_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){
                   .usage = DVZ_SCENE_BUFFER_USAGE_STORAGE,
                   .stride = sizeof(vec3),
                   .byte_size = (uint64_t)PARTICLE_COUNT * sizeof(vec3),
               });
    DvzSceneBuffer* param_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){
                   .usage = DVZ_SCENE_BUFFER_USAGE_STORAGE,
                   .stride = sizeof(vec4),
                   .byte_size = sizeof(vec4),
               });
    EXAMPLE_CHECK(
        position_buffer != NULL && velocity_buffer != NULL && param_buffer != NULL,
        "dvz_scene_buffer() failed");
    vec4 params = {0};
    _params_for_frame(0, params);
    EXAMPLE_CHECK(
        dvz_scene_buffer_set_data(
            position_buffer, positions, (uint64_t)PARTICLE_COUNT * sizeof(vec3)),
        "position buffer upload failed");
    EXAMPLE_CHECK(
        dvz_scene_buffer_set_data(
            velocity_buffer, velocities, (uint64_t)PARTICLE_COUNT * sizeof(vec3)),
        "velocity buffer upload failed");
    EXAMPLE_CHECK(
        dvz_scene_buffer_set_data(param_buffer, params, sizeof(vec4)),
        "param buffer upload failed");

    DvzVisual* points = dvz_point(scene, 0);
    EXAMPLE_CHECK(points != NULL, "dvz_point() failed");
    EXAMPLE_CHECK(
        dvz_visual_set_attr_buffer(points, "position", position_buffer, 0, PARTICLE_COUNT),
        "bind point position buffer failed");
    EXAMPLE_CHECK(
        dvz_visual_set_data(points, "color", colors, PARTICLE_COUNT) == 0,
        "set point colors failed");
    EXAMPLE_CHECK(
        dvz_visual_set_data(points, "size", sizes, PARTICLE_COUNT) == 0,
        "set point sizes failed");
    EXAMPLE_CHECK(dvz_visual_set_depth_test(points, false) == 0, "disable depth test failed");
    EXAMPLE_CHECK(dvz_panel_add_visual(panel, points, NULL) == 0, "add point visual failed");

    DvzSceneCompute* compute = dvz_scene_compute(
        scene, &(DvzSceneComputeDesc){
                   .label = "gpu_particle_advection",
                   .shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL,
                   .shader_source = COMPUTE_GLSL,
                   .dispatch = {(PARTICLE_COUNT + WORKGROUP_SIZE - 1u) / WORKGROUP_SIZE, 1, 1},
               });
    EXAMPLE_CHECK(compute != NULL, "dvz_scene_compute() failed");
    EXAMPLE_CHECK(
        dvz_scene_compute_set_buffer(
            compute, 0, param_buffer, DVZ_SCENE_COMPUTE_ACCESS_READ, 0, sizeof(vec4)),
        "bind compute params failed");
    EXAMPLE_CHECK(
        dvz_scene_compute_set_buffer(
            compute, 1, position_buffer, DVZ_SCENE_COMPUTE_ACCESS_READ_WRITE, 0,
            (uint64_t)PARTICLE_COUNT * sizeof(vec3)),
        "bind compute positions failed");
    EXAMPLE_CHECK(
        dvz_scene_compute_set_buffer(
            compute, 2, velocity_buffer, DVZ_SCENE_COMPUTE_ACCESS_READ_WRITE, 0,
            (uint64_t)PARTICLE_COUNT * sizeof(vec3)),
        "bind compute velocities failed");
    EXAMPLE_CHECK(dvz_figure_add_compute(figure, compute), "attach compute pass failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");
    DvzView* win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "gpu_particle_advection");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    ParticleState state = {.params = param_buffer, .frame_index = 1};
    dvz_view_set_frame_callback(win, _particle_frame, &state);
    dvz_view_request_frame(win);

    DvzAppCaptureConfig capture = dvz_app_capture_config_from_env("gpu_particle_advection");
    EXAMPLE_CHECK(dvz_view_capture_start(win, &capture) == 0, "dvz_view_capture_start() failed");
    dvz_app_run(app, frames);
    EXAMPLE_CHECK(dvz_view_capture_stop(win) == 0, "dvz_view_capture_stop() failed");

    printf(
        "gpu_particle_advection: %u particles, %u frames, scene compute path\n",
        PARTICLE_COUNT, frames);
    rc = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    else if (scene != NULL)
        dvz_scene_destroy(scene);
    dvz_free(sizes);
    dvz_free(colors);
    dvz_free(velocities);
    dvz_free(positions);
    return rc;
}
