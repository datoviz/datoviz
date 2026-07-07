/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* compute_buffer_animation - This example shows a compute pass updating point positions.
 *
 * Scenario: feature.compute_buffer_animation
 * Style: features, graphite_cyan, 1280x720 window target, experimental scene compute
 *
 * Build:  just example-c features/compute_buffer_animation
 * Run:    ./build/examples/c/features/compute_buffer_animation --live
 * Smoke:  ./build/examples/c/features/compute_buffer_animation --png
 *
 * What to look for: the point visual reads its position attribute from a scene buffer that is also
 * bound as read-write storage for a compute shader. Static color and diameter_px arrays define the
 * appearance, while per-frame parameters update time and the shader writes new x/y coordinates.
 * Compare the moving markers with their stable colors and sizes; this demonstrates a useful
 * compute-to-render pattern for particle systems, simulations, and iterative GPU analyses.
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_alloc.h"
#include "datoviz/scene.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH          EXAMPLE_WINDOW_WIDTH
#define HEIGHT         EXAMPLE_WINDOW_HEIGHT
#define POINT_COUNT    6u
#define WORKGROUP_SIZE 1u

static const float COMPUTE_ANGULAR_SPEED = 3.30f;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct ComputeBufferAnimationState
{
    DvzSceneBuffer* param;
    vec4 params;
} ComputeBufferAnimationState;



/*************************************************************************************************/
/*  Shaders                                                                                      */
/*************************************************************************************************/

static const char* COMPUTE_WGSL =
    "struct Params { p: vec4f, }\n"
    "@group(0) @binding(0) var<storage, read> params: Params;\n"
    "@group(0) @binding(1) var<storage, read_write> positions: array<f32>;\n"
    "@group(0) @binding(2) var<storage, read> phases: array<f32>;\n"
    "@compute @workgroup_size(1)\n"
    "fn main(@builtin(global_invocation_id) global_id: vec3u) {\n"
    "    let i = global_id.x;\n"
    "    let count = u32(params.p.z);\n"
    "    if (i >= count) { return; }\n"
    "    let phase = phases[i] + params.p.x * params.p.y * (0.65 + 0.18 * f32(i));\n"
    "    let denom = max(1.0, f32(count - 1u));\n"
    "    let center = -0.70 + 1.40 * (f32(i) / denom);\n"
    "    let radius = 0.07 + 0.018 * f32((i * 7u) % 4u);\n"
    "    positions[3u * i + 0u] = center + radius * cos(phase);\n"
    "    positions[3u * i + 1u] = 0.18 * sin(0.7 * f32(i)) + radius * sin(phase);\n"
    "    positions[3u * i + 2u] = 0.0;\n"
    "}\n";

static const char* COMPUTE_GLSL =
    "#version 450\n"
    "layout(local_size_x = 1) in;\n"
    "layout(std430, set = 0, binding = 0) readonly buffer Params { vec4 p; } params;\n"
    "layout(std430, set = 0, binding = 1) buffer Positions { float x[]; } positions;\n"
    "layout(std430, set = 0, binding = 2) readonly buffer Phases { float x[]; } phases;\n"
    "void main() {\n"
    "    uint i = gl_GlobalInvocationID.x;\n"
    "    uint count = uint(params.p.z);\n"
    "    if (i >= count) return;\n"
    "    float phase = phases.x[i] + params.p.x * params.p.y * (0.65 + 0.18 * float(i));\n"
    "    float center = -0.70 + 1.40 * (float(i) / max(1.0, float(count - 1u)));\n"
    "    float radius = 0.07 + 0.018 * float((i * 7u) % 4u);\n"
    "    positions.x[3u * i + 0u] = center + radius * cos(phase);\n"
    "    positions.x[3u * i + 1u] = 0.18 * sin(0.7 * float(i)) + radius * sin(phase);\n"
    "    positions.x[3u * i + 2u] = 0.0;\n"
    "}\n";



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_example_compute_buffer_animation_scenario(void);



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Create a scene buffer with a copied payload.
 *
 * @param scene scene owning the buffer
 * @param usage buffer usage flags
 * @param stride item stride
 * @param data copied payload
 * @param byte_size payload size
 * @return scene buffer, or NULL
 */
static DvzSceneBuffer* _scene_buffer(
    DvzScene* scene, uint32_t usage, uint32_t stride, const void* data, uint64_t byte_size)
{
    DvzSceneBufferDesc desc = dvz_scene_buffer_desc();
    desc.usage = usage;
    desc.stride = stride;
    desc.byte_size = byte_size;
    DvzSceneBuffer* buffer = dvz_scene_buffer(scene, &desc);
    if (buffer == NULL)
        return NULL;
    if (dvz_scene_buffer_set_data(buffer, data, byte_size) != DVZ_OK)
        return NULL;
    return buffer;
}


/**
 * Upload compute parameters for one scenario time.
 *
 * @param state scenario state
 * @param t scenario time in seconds
 * @return true on success
 */
static bool _upload_compute_params(ComputeBufferAnimationState* state, double t)
{
    if (state == NULL || state->param == NULL)
        return false;
    state->params[0] = (float)t;
    state->params[1] = COMPUTE_ANGULAR_SPEED;
    state->params[2] = (float)POINT_COUNT;
    state->params[3] = 0.0f;
    return dvz_scene_buffer_set_data(state->param, state->params, sizeof(state->params)) == DVZ_OK;
}



/**
 * Add the compute-fed point visual and attach the compute pass.
 *
 * @param scene scene owning objects
 * @param figure figure scheduling compute
 * @param panel panel receiving the visual
 * @param state scenario state receiving the parameter buffer
 * @return true on success
 */
static bool _add_compute_points(
    DvzScene* scene, DvzFigure* figure, DvzPanel* panel, ComputeBufferAnimationState* state)
{
    if (state == NULL)
        return false;
    const vec3 positions[POINT_COUNT] = {
        {-0.72f, -0.24f, 0.0f}, {-0.44f, +0.18f, 0.0f}, {-0.16f, -0.10f, 0.0f},
        {+0.16f, +0.10f, 0.0f}, {+0.44f, -0.18f, 0.0f}, {+0.72f, +0.24f, 0.0f},
    };
    const float phases[POINT_COUNT] = {0.0f, 0.7f, 1.6f, 2.4f, 3.2f, 4.1f};
    DvzColor colors[POINT_COUNT] = {
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
    };
    const float diameters[POINT_COUNT] = {34.0f, 42.0f, 50.0f, 50.0f, 42.0f, 34.0f};

    DvzSceneBuffer* position = _scene_buffer(
        scene, DVZ_SCENE_BUFFER_USAGE_VERTEX | DVZ_SCENE_BUFFER_USAGE_STORAGE, sizeof(vec3),
        positions, sizeof(positions));
    state->params[0] = 0.0f;
    state->params[1] = COMPUTE_ANGULAR_SPEED;
    state->params[2] = (float)POINT_COUNT;
    state->params[3] = 0.0f;
    DvzSceneBuffer* param = _scene_buffer(
        scene, DVZ_SCENE_BUFFER_USAGE_STORAGE, sizeof(vec4), state->params, sizeof(state->params));
    DvzSceneBuffer* phase = _scene_buffer(
        scene, DVZ_SCENE_BUFFER_USAGE_STORAGE, sizeof(float), phases, sizeof(phases));
    if (position == NULL || param == NULL || phase == NULL)
        return false;
    state->param = param;

    DvzVisual* point = dvz_point(scene, 0);
    if (point == NULL)
        return false;
    if (dvz_visual_set_attr_buffer(point, "position", position, 0, POINT_COUNT) != DVZ_OK)
        return false;
    if (dvz_visual_set_data(point, "color", colors, POINT_COUNT) != 0)
        return false;
    if (dvz_visual_set_data(point, "diameter_px", diameters, POINT_COUNT) != 0)
        return false;

    DvzPointStyleDesc style = dvz_point_style_desc();
    style.aspect = DVZ_SHAPE_ASPECT_FILLED;
    style.stroke_width_px = 0.0f;
    if (dvz_point_set_style(point, &style) != 0)
        return false;
    if (dvz_visual_set_depth_test(point, false) != 0)
        return false;
    if (dvz_panel_add_visual(panel, point, NULL) != 0)
        return false;

    DvzSceneComputeDesc compute_desc = dvz_scene_compute_desc();
    compute_desc.label = "feature_compute_buffer_animation";
#ifdef DVZ_EXAMPLE_NO_APP
    compute_desc.shader_format = DVZ_SCENE_SHADER_FORMAT_WGSL;
    compute_desc.shader_source = COMPUTE_WGSL;
#else
    compute_desc.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    compute_desc.shader_source = COMPUTE_GLSL;
#endif
    compute_desc.dispatch[0] = (POINT_COUNT + WORKGROUP_SIZE - 1u) / WORKGROUP_SIZE;
    compute_desc.dispatch[1] = 1u;
    compute_desc.dispatch[2] = 1u;

    DvzSceneCompute* compute = dvz_scene_compute(scene, &compute_desc);
    if (compute == NULL)
        return false;
    if (dvz_scene_compute_set_buffer(
            compute, 0, param, DVZ_SCENE_COMPUTE_ACCESS_READ, 0, sizeof(state->params)) != DVZ_OK)
        return false;
    if (dvz_scene_compute_set_buffer(
            compute, 1, position, DVZ_SCENE_COMPUTE_ACCESS_READ_WRITE, 0, sizeof(positions)) !=
        DVZ_OK)
        return false;
    if (dvz_scene_compute_set_buffer(
            compute, 2, phase, DVZ_SCENE_COMPUTE_ACCESS_READ, 0, sizeof(phases)) != DVZ_OK)
        return false;
    return dvz_figure_add_compute(figure, compute) == DVZ_OK;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the scene-compute-buffer feature scenario.
 *
 * @param ctx scenario context
 * @param out_user scenario state output
 * @return true on success
 */
static bool _scenario_init(DvzScenarioContext* ctx, void** out_user)
{
    if (ctx == NULL || out_user == NULL)
        return false;

    ComputeBufferAnimationState* state =
        (ComputeBufferAnimationState*)dvz_calloc(1, sizeof(ComputeBufferAnimationState));
    if (state == NULL)
        return false;

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        goto error;

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    if (panel == NULL)
        goto error;
    example_graphite_cyan_set_panel_background(panel);
    if (!_add_compute_points(ctx->scene, ctx->figure, panel, state))
        goto error;

    *out_user = state;
    return true;

error:
    dvz_free(state);
    return false;
}



/**
 * Advance the scene-compute-buffer scenario for one frame.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_frame(DvzScenarioContext* ctx, void* user)
{
    if (ctx == NULL || user == NULL)
        return;
    const double time = ctx->preview_mode ? dvz_scenario_preview_time(ctx) : ctx->time;
    (void)_upload_compute_params((ComputeBufferAnimationState*)user, time);
}



/**
 * Destroy the scene-compute-buffer scenario.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    dvz_free(user);
}



/**
 * Return the scene-compute-buffer scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_example_compute_buffer_animation_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_compute_buffer_animation",
        .title = "Compute Buffer Animation",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_POINT_VISUAL | DVZ_SCENARIO_REQ_SCENE_BUFFERS |
                        DVZ_SCENARIO_REQ_STORAGE_BUFFERS | DVZ_SCENARIO_REQ_SCENE_COMPUTE |
                        DVZ_SCENARIO_REQ_FRAME_CALLBACKS | DVZ_SCENARIO_REQ_CONTINUOUS_FRAMES,
        .continuous_frames = true,
        .init = _scenario_init,
        .frame = _scenario_frame,
        .destroy = _scenario_destroy,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the scene-compute-buffer feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_example_compute_buffer_animation_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
