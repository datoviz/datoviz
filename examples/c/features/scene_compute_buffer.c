/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* scene_compute_buffer - minimal scene compute pass writing a point position buffer.
 *
 * Scenario: feature.scene_compute_buffer
 * Style: features, graphite_cyan, 1600x1200 capture target, experimental scene compute
 *
 * Build:  just example-c features/scene_compute_buffer
 * Run:    ./build/examples/c/features/scene_compute_buffer --live
 * Smoke:  ./build/examples/c/features/scene_compute_buffer --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/scene.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH          1600u
#define HEIGHT         1200u
#define POINT_COUNT    6u
#define WORKGROUP_SIZE 1u



/*************************************************************************************************/
/*  Shaders                                                                                      */
/*************************************************************************************************/

static const char* COMPUTE_GLSL =
    "#version 450\n"
    "layout(local_size_x = 1) in;\n"
    "layout(std430, set = 0, binding = 0) readonly buffer Params { vec4 p; } params;\n"
    "layout(std430, set = 0, binding = 1) buffer Positions { float x[]; } positions;\n"
    "void main() {\n"
    "    uint i = gl_GlobalInvocationID.x;\n"
    "    uint count = uint(params.p.z);\n"
    "    if (i >= count) return;\n"
    "    positions.x[3u * i + 1u] += params.p.x;\n"
    "}\n";



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
    if (!dvz_scene_buffer_set_data(buffer, data, byte_size))
        return NULL;
    return buffer;
}



/**
 * Add the compute-fed point visual and attach the compute pass.
 *
 * @param scene scene owning objects
 * @param figure figure scheduling compute
 * @param panel panel receiving the visual
 * @return true on success
 */
static bool _add_compute_points(DvzScene* scene, DvzFigure* figure, DvzPanel* panel)
{
    const vec3 positions[POINT_COUNT] = {
        {-0.72f, -0.24f, 0.0f}, {-0.44f, +0.18f, 0.0f}, {-0.16f, -0.10f, 0.0f},
        {+0.16f, +0.10f, 0.0f}, {+0.44f, -0.18f, 0.0f}, {+0.72f, +0.24f, 0.0f},
    };
    const vec4 params = {0.18f, 0.0f, (float)POINT_COUNT, 0.0f};
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
    DvzSceneBuffer* param = _scene_buffer(
        scene, DVZ_SCENE_BUFFER_USAGE_STORAGE, sizeof(vec4), &params, sizeof(params));
    if (position == NULL || param == NULL)
        return false;

    DvzVisual* point = dvz_point(scene, 0);
    if (point == NULL)
        return false;
    if (!dvz_visual_set_attr_buffer(point, "position", position, 0, POINT_COUNT))
        return false;
    if (dvz_visual_set_data(point, "color", colors, POINT_COUNT) != 0)
        return false;
    if (dvz_visual_set_data(point, "diameter", diameters, POINT_COUNT) != 0)
        return false;

    DvzPointStyleDesc style = dvz_point_style_desc();
    style.aspect = DVZ_SHAPE_ASPECT_FILLED;
    style.stroke_width = 0.0f;
    if (dvz_point_set_style(point, &style) != 0)
        return false;
    if (dvz_visual_set_depth_test(point, false) != 0)
        return false;
    if (dvz_panel_add_visual(panel, point, NULL) != 0)
        return false;

    DvzSceneComputeDesc compute_desc = dvz_scene_compute_desc();
    compute_desc.label = "feature_scene_compute_buffer";
    compute_desc.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    compute_desc.shader_source = COMPUTE_GLSL;
    compute_desc.dispatch[0] = (POINT_COUNT + WORKGROUP_SIZE - 1u) / WORKGROUP_SIZE;
    compute_desc.dispatch[1] = 1u;
    compute_desc.dispatch[2] = 1u;

    DvzSceneCompute* compute = dvz_scene_compute(scene, &compute_desc);
    if (compute == NULL)
        return false;
    if (!dvz_scene_compute_set_buffer(
            compute, 0, param, DVZ_SCENE_COMPUTE_ACCESS_READ, 0, sizeof(params)))
        return false;
    if (!dvz_scene_compute_set_buffer(
            compute, 1, position, DVZ_SCENE_COMPUTE_ACCESS_READ_WRITE, 0, sizeof(positions)))
        return false;
    return dvz_figure_add_compute(figure, compute);
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
    if (ctx == NULL)
        return false;
    if (out_user != NULL)
        *out_user = NULL;

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        return false;

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    if (panel == NULL)
        return false;
    example_graphite_cyan_set_panel_background(panel);
    return _add_compute_points(ctx->scene, ctx->figure, panel);
}



/**
 * Return the scene-compute-buffer scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _scene_compute_buffer_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_scene_compute_buffer",
        .title = "scene_compute_buffer",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .init = _scenario_init,
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
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _scene_compute_buffer_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
