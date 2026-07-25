/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* vklite_triangle_spike - RC3 first-result spike for the Vulkan tutorial.
 *
 * This intentionally uses the current public API without proposing tutorial-only helpers. One
 * renderer records a shader-generated triangle into Canvas frames supplied by either a live GLFW
 * window or the deterministic offscreen backend.
 *
 * Build: just example-c tutorial/vklite_triangle_spike
 * Run:   ./build/examples/c/tutorial/vklite_triangle_spike --offscreen --frames 3
 *        ./build/examples/c/tutorial/vklite_triangle_spike --live --frames 300
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <volk.h>
#include <vulkan/vulkan_core.h>

#include "_compat.h"
#include "datoviz/canvas.h"
#include "datoviz/common/functions.h"
#include "datoviz/stream/frame_stream.h"
#include "datoviz/vk/enums.h"
#include "datoviz/vk/gpu_ctx.h"
#include "datoviz/vklite.h"
#include "datoviz/window.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define SPIKE_WIDTH  800
#define SPIKE_HEIGHT 600



/*************************************************************************************************/
/*  Shaders                                                                                      */
/*************************************************************************************************/

static const char* VERT_GLSL = "#version 450\n"
                               "layout(location = 0) out vec3 color;\n"
                               "const vec2 positions[3] = vec2[3](\n"
                               "    vec2( 0.0, -0.65),\n"
                               "    vec2( 0.65, 0.65),\n"
                               "    vec2(-0.65, 0.65));\n"
                               "const vec3 colors[3] = vec3[3](\n"
                               "    vec3(1.0, 0.2, 0.2),\n"
                               "    vec3(0.2, 1.0, 0.2),\n"
                               "    vec3(0.2, 0.4, 1.0));\n"
                               "void main() {\n"
                               "    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);\n"
                               "    color = colors[gl_VertexIndex];\n"
                               "}\n";

static const char* FRAG_GLSL = "#version 450\n"
                               "layout(location = 0) in vec3 color;\n"
                               "layout(location = 0) out vec4 out_color;\n"
                               "void main() { out_color = vec4(color, 1.0); }\n";



/*************************************************************************************************/
/*  State                                                                                        */
/*************************************************************************************************/

typedef struct
{
    bool live;
    bool validation;
    uint32_t frame_count;
    const char* png_path;
} SpikeOptions;



typedef struct
{
    DvzDevice* device;
    DvzSlots* slots;
    DvzGraphics* pipeline;
    DvzCommands* commands;
    DvzRendering* rendering;
    VkFormat color_format;
    bool create_failed;
    uint64_t draw_count;
    uint64_t extent_change_count;
    uint64_t generation_change_count;
    uint64_t format_mismatch_count;
    uint64_t invalid_frame_contract_count;
    uint64_t last_resource_generation;
    VkExtent2D last_extent;
} SpikeRenderer;



typedef struct
{
    DvzWindowHost* host;
    DvzGpuCtx* gpu;
    DvzWindow* window;
    DvzCanvas* canvas;
    SpikeRenderer renderer;
    uint32_t validation_error_count;
} SpikeRuntime;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Print command-line usage.
 *
 * @param executable executable path
 */
static void _print_usage(const char* executable)
{
    dvz_fprintf(
        stderr, "Usage: %s [--offscreen|--live] [--frames N] [--validate] [--png PATH]\n",
        executable);
}



/**
 * Parse the spike command line.
 *
 * @param argc argument count
 * @param argv argument values
 * @param options output options
 * @return 0 on success, 1 when help was printed, or -1 on invalid input
 */
static int _parse_options(int argc, char** argv, SpikeOptions* options)
{
    bool frames_explicit = false;
    *options = (SpikeOptions){
        .live = false,
        .validation = false,
        .frame_count = 3,
        .png_path = "vklite_triangle_spike.png",
    };

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--offscreen") == 0)
        {
            options->live = false;
        }
        else if (strcmp(argv[i], "--live") == 0)
        {
            options->live = true;
            if (!frames_explicit)
                options->frame_count = 300;
        }
        else if (strcmp(argv[i], "--validate") == 0)
        {
            options->validation = true;
        }
        else if (strcmp(argv[i], "--frames") == 0 && i + 1 < argc)
        {
            char* end = NULL;
            unsigned long value = strtoul(argv[++i], &end, 10);
            if (end == argv[i] || *end != '\0' || value == 0 || value > UINT32_MAX)
                return -1;
            options->frame_count = (uint32_t)value;
            frames_explicit = true;
        }
        else if (strcmp(argv[i], "--png") == 0 && i + 1 < argc)
        {
            options->png_path = argv[++i];
        }
        else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
        {
            _print_usage(argv[0]);
            return 1;
        }
        else
        {
            return -1;
        }
    }
    return 0;
}



/**
 * Destroy renderer-owned vklite objects.
 *
 * @param renderer renderer to destroy
 */
static void _renderer_destroy(SpikeRenderer* renderer)
{
    dvz_commands_free(renderer->commands);
    renderer->commands = NULL;
    dvz_rendering_free(renderer->rendering);
    renderer->rendering = NULL;
    if (renderer->pipeline != NULL)
    {
        dvz_graphics_destroy(renderer->pipeline);
        dvz_graphics_free(renderer->pipeline);
        renderer->pipeline = NULL;
    }
    if (renderer->slots != NULL)
    {
        dvz_slots_destroy(renderer->slots);
        dvz_slots_free(renderer->slots);
        renderer->slots = NULL;
    }
}



/**
 * Compile the inline spike shaders and create the graphics pipeline.
 *
 * @param renderer renderer receiving owned vklite objects
 * @param device borrowed device that outlives the renderer
 * @param color_format Canvas attachment format used by the pipeline
 * @return 0 on success or -1 on failure
 */
static int _renderer_create(SpikeRenderer* renderer, DvzDevice* device, VkFormat color_format)
{
    renderer->device = device;
    renderer->color_format = color_format;

    uint64_t vertex_size = 0;
    uint64_t fragment_size = 0;
    uint32_t* vertex_spirv = dvz_compile_glsl("vertex", VERT_GLSL, &vertex_size);
    uint32_t* fragment_spirv = dvz_compile_glsl("fragment", FRAG_GLSL, &fragment_size);
    if (vertex_spirv == NULL || fragment_spirv == NULL)
    {
        dvz_memory_free(vertex_spirv);
        dvz_memory_free(fragment_spirv);
        dvz_fprintf(stderr, "vklite_triangle_spike: GLSL compilation failed\n");
        return -1;
    }

    DvzShader* vertex_shader = dvz_shader_create_wrapper();
    DvzShader* fragment_shader = dvz_shader_create_wrapper();
    if (vertex_shader == NULL || fragment_shader == NULL)
    {
        dvz_shader_free(vertex_shader);
        dvz_shader_free(fragment_shader);
        dvz_memory_free(vertex_spirv);
        dvz_memory_free(fragment_spirv);
        return -1;
    }
    dvz_shader(device, vertex_size, vertex_spirv, vertex_shader);
    dvz_shader(device, fragment_size, fragment_spirv, fragment_shader);
    dvz_memory_free(vertex_spirv);
    dvz_memory_free(fragment_spirv);

    renderer->slots = dvz_slots_create_wrapper();
    renderer->pipeline = dvz_graphics_create_wrapper();
    if (renderer->slots == NULL || renderer->pipeline == NULL)
        goto error;

    dvz_slots(device, renderer->slots);
    dvz_slots_create(renderer->slots);

    dvz_graphics(device, renderer->pipeline);
    dvz_graphics_shader(
        renderer->pipeline, VK_SHADER_STAGE_VERTEX_BIT, dvz_shader_handle(vertex_shader));
    dvz_graphics_shader(
        renderer->pipeline, VK_SHADER_STAGE_FRAGMENT_BIT, dvz_shader_handle(fragment_shader));
    dvz_graphics_primitive(renderer->pipeline, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    dvz_graphics_attachment_color(renderer->pipeline, 0, color_format);
    dvz_graphics_layout(renderer->pipeline, dvz_slots_handle(renderer->slots));
    dvz_graphics_viewport(renderer->pipeline, 0, 0, 0, 0, 0, 1, DVZ_GRAPHICS_FLAGS_DYNAMIC);
    dvz_graphics_scissor(renderer->pipeline, 0, 0, 0, 0, DVZ_GRAPHICS_FLAGS_DYNAMIC);

    int create_result = dvz_graphics_create(renderer->pipeline);
    dvz_shader_destroy(vertex_shader);
    dvz_shader_free(vertex_shader);
    dvz_shader_destroy(fragment_shader);
    dvz_shader_free(fragment_shader);
    if (create_result != 0)
    {
        _renderer_destroy(renderer);
        return -1;
    }
    return 0;

error:
    dvz_shader_destroy(vertex_shader);
    dvz_shader_free(vertex_shader);
    dvz_shader_destroy(fragment_shader);
    dvz_shader_free(fragment_shader);
    _renderer_destroy(renderer);
    return -1;
}



/**
 * Record one triangle into a borrowed Canvas frame.
 *
 * @param canvas owning canvas (unused)
 * @param frame borrowed frame valid only during this callback
 * @param user_data renderer state
 */
static void _draw_triangle(DvzCanvas* canvas, const DvzStreamFrame* frame, void* user_data)
{
    SpikeRenderer* renderer = (SpikeRenderer*)user_data;
    if (renderer == NULL || frame == NULL || frame->command_buffer == VK_NULL_HANDLE ||
        frame->image_view == VK_NULL_HANDLE || frame->extent.width == 0 ||
        frame->extent.height == 0 || renderer->create_failed)
    {
        return;
    }

    if (!frame->command_buffer_recording || !frame->command_buffer_borrowed ||
        !frame->image_view_borrowed || !frame->image_valid || frame->resource_generation == 0)
    {
        renderer->invalid_frame_contract_count++;
        return;
    }

    if (renderer->pipeline == NULL)
    {
        if (_renderer_create(renderer, renderer->device, frame->color_format) != 0)
        {
            renderer->create_failed = true;
            return;
        }
    }

    if (renderer->last_extent.width != 0 && (renderer->last_extent.width != frame->extent.width ||
                                             renderer->last_extent.height != frame->extent.height))
    {
        renderer->extent_change_count++;
    }
    renderer->last_extent = frame->extent;
    if (renderer->last_resource_generation != 0 &&
        renderer->last_resource_generation != frame->resource_generation)
    {
        renderer->generation_change_count++;
    }
    renderer->last_resource_generation = frame->resource_generation;
    if (frame->color_format != renderer->color_format ||
        dvz_canvas_frame_format(canvas) != frame->color_format)
    {
        renderer->format_mismatch_count++;
        return;
    }

    DvzCommands* commands = renderer->commands;
    DvzRendering* rendering = renderer->rendering;
    if (commands == NULL || rendering == NULL)
        return;
    dvz_commands_wrap_borrowed_recording(renderer->device, frame->command_buffer, commands);
    dvz_cmd_rendering_default(
        commands, frame->image_view, frame->extent.width, frame->extent.height,
        (VkClearValue){.color.float32 = {0.03f, 0.04f, 0.07f, 1.0f}}, rendering);
    dvz_cmd_rendering_begin(commands, rendering);
    dvz_cmd_bind_graphics(commands, renderer->pipeline);

    dvz_cmd_set_viewport_scissor(commands, frame->extent);
    dvz_cmd_draw(commands, 0, 3, 0, 1);
    dvz_cmd_rendering_end(commands);

    renderer->draw_count++;
    if (dvz_commands_unwrap(commands) != DVZ_OK)
        renderer->invalid_frame_contract_count++;
}



/**
 * Configure a GPU context for the selected Canvas backend.
 *
 * @param runtime runtime containing a live window host
 * @param options selected execution options
 * @return 0 on success or -1 on failure
 */
static int _create_gpu(SpikeRuntime* runtime, const SpikeOptions* options)
{
    DvzGpuCtxConfig gpu_config = dvz_gpu_ctx_config();
    dvz_gpu_ctx_config_validation(&gpu_config, options->validation);
    DvzBackend backend = options->live ? DVZ_BACKEND_GLFW : DVZ_BACKEND_OFFSCREEN;
    DvzCanvasRenderMode render_mode =
        options->live ? DVZ_CANVAS_RENDER_MODE_PRESENT : DVZ_CANVAS_RENDER_MODE_OFFSCREEN;
    if (dvz_canvas_configure_gpu_ctx(runtime->host, backend, render_mode, &gpu_config) != DVZ_OK)
        return -1;

    runtime->gpu = dvz_gpu_ctx(&gpu_config);
    return runtime->gpu != NULL ? 0 : -1;
}



/**
 * Destroy all resources in dependency-safe order.
 *
 * @param runtime runtime to destroy
 */
static void _runtime_destroy(SpikeRuntime* runtime)
{
    dvz_canvas_destroy(runtime->canvas);
    runtime->canvas = NULL;
    _renderer_destroy(&runtime->renderer);
    dvz_window_destroy(runtime->window);
    runtime->window = NULL;
    dvz_window_host_destroy(runtime->host);
    runtime->host = NULL;
    if (runtime->gpu != NULL)
        runtime->validation_error_count = dvz_gpu_ctx_error_count(runtime->gpu);
    dvz_gpu_ctx_destroy(runtime->gpu);
    runtime->gpu = NULL;
}



/**
 * Create the shared renderer and the selected Canvas backend.
 *
 * @param runtime output runtime
 * @param options selected execution options
 * @return 0 on success or -1 on failure
 */
static int _runtime_create(SpikeRuntime* runtime, const SpikeOptions* options)
{
    runtime->host = dvz_window_host();
    if (runtime->host == NULL)
        return -1;
    if (_create_gpu(runtime, options) != 0)
        return -1;

    DvzWindowConfig window_config = dvz_window_config();
    window_config.width = SPIKE_WIDTH;
    window_config.height = SPIKE_HEIGHT;
    window_config.title = "Datoviz vklite triangle spike";
    runtime->window = dvz_window_create(
        runtime->host, options->live ? DVZ_BACKEND_GLFW : DVZ_BACKEND_OFFSCREEN, &window_config);
    if (runtime->window == NULL || dvz_window_backend_type(runtime->window) !=
                                       (options->live ? DVZ_BACKEND_GLFW : DVZ_BACKEND_OFFSCREEN))
    {
        return -1;
    }

    DvzCanvasConfig canvas_config = dvz_canvas_config();
    canvas_config.window = runtime->window;
    canvas_config.device = dvz_gpu_ctx_device(runtime->gpu);
    canvas_config.render_mode =
        options->live ? DVZ_CANVAS_RENDER_MODE_PRESENT : DVZ_CANVAS_RENDER_MODE_OFFSCREEN;
    runtime->canvas = dvz_canvas_create(&canvas_config);
    if (runtime->canvas == NULL)
        return -1;

    runtime->renderer.device = dvz_gpu_ctx_device(runtime->gpu);
    runtime->renderer.commands = dvz_commands_create_wrapper();
    runtime->renderer.rendering = dvz_rendering_create_wrapper();
    if (runtime->renderer.commands == NULL || runtime->renderer.rendering == NULL)
        return -1;
    VkFormat frame_format = dvz_canvas_frame_format(runtime->canvas);
    if (frame_format != VK_FORMAT_UNDEFINED &&
        _renderer_create(&runtime->renderer, runtime->renderer.device, frame_format) != 0)
    {
        return -1;
    }
    dvz_canvas_set_draw_callback(runtime->canvas, _draw_triangle, &runtime->renderer);
    return 0;
}



/**
 * Run a bounded live or offscreen frame loop.
 *
 * @param runtime initialized runtime
 * @param options selected execution options
 * @return 0 on success or -1 on frame or submission failure
 */
static int _run(SpikeRuntime* runtime, const SpikeOptions* options)
{
    uint32_t submitted_count = 0;
    while (submitted_count < options->frame_count &&
           (!options->live || !dvz_window_should_close(runtime->window)))
    {
        dvz_window_host_poll(runtime->host);
        int frame_result = dvz_canvas_frame(runtime->canvas);
        if (frame_result == DVZ_CANVAS_FRAME_WAIT_SURFACE)
            continue;
        if (frame_result != DVZ_CANVAS_FRAME_READY)
            return -1;
        int submit_result = dvz_canvas_submit(runtime->canvas);
        if (submit_result != 0)
            return -1;
        submitted_count++;
    }

    if (!options->live)
    {
        int capture_result = dvz_canvas_capture_png(runtime->canvas, options->png_path);
        if (capture_result != 0)
            return -1;
        dvz_fprintf(
            stdout, "vklite_triangle_spike: saved %s after %u frames\n", options->png_path,
            submitted_count);
    }
    dvz_fprintf(
        stdout,
        "vklite_triangle_spike: submitted=%u drawn=%llu extent_changes=%llu "
        "generation_changes=%llu "
        "format_mismatches=%llu "
        "invalid_frame_contracts=%llu create_failed=%d\n",
        submitted_count, (unsigned long long)runtime->renderer.draw_count,
        (unsigned long long)runtime->renderer.extent_change_count,
        (unsigned long long)runtime->renderer.generation_change_count,
        (unsigned long long)runtime->renderer.format_mismatch_count,
        (unsigned long long)runtime->renderer.invalid_frame_contract_count,
        runtime->renderer.create_failed);
    return submitted_count > 0 && runtime->renderer.draw_count == submitted_count &&
                   runtime->renderer.format_mismatch_count == 0 &&
                   runtime->renderer.invalid_frame_contract_count == 0 &&
                   !runtime->renderer.create_failed
               ? 0
               : -1;
}



/*************************************************************************************************/
/*  Main                                                                                         */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    SpikeOptions options = {0};
    int parse_result = _parse_options(argc, argv, &options);
    if (parse_result != 0)
    {
        if (parse_result < 0)
            _print_usage(argv[0]);
        return parse_result < 0 ? 2 : 0;
    }

    SpikeRuntime runtime = {0};
    int result = _runtime_create(&runtime, &options);
    if (result == 0)
        result = _run(&runtime, &options);
    _runtime_destroy(&runtime);
    if (options.validation && runtime.validation_error_count != 0)
    {
        dvz_fprintf(
            stderr, "vklite_triangle_spike: %u Vulkan validation error(s)\n",
            runtime.validation_error_count);
        result = -1;
    }
    if (result != 0)
        dvz_fprintf(stderr, "vklite_triangle_spike: failed\n");
    return result == 0 ? 0 : 1;
}
