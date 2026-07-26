/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* triangle - shared RC3 pilot renderer for the Vulkan tutorial.
 *
 * This intentionally uses the current public API without proposing tutorial-only helpers. One
 * renderer records a shader-generated triangle into Canvas frames supplied by either a live GLFW
 * window or the deterministic offscreen backend.
 *
 * Build: just example-c tutorial/first_triangle
 * Run:   ./build/examples/c/tutorial/first_triangle --offscreen --frames 3
 *        ./build/examples/c/tutorial/first_triangle --live --frames 300
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "datoviz/canvas.h"
#include "datoviz/common/functions.h"
#include "datoviz/controller.h"
#include "datoviz/fileio/fileio.h"
#include "datoviz/shader.h"
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

#ifndef DVZ_TUTORIAL_SHADER_DIR
#define DVZ_TUTORIAL_SHADER_DIR "shaders"
#endif

#ifndef DVZ_TUTORIAL_USE_VERTEX_BUFFER
#define DVZ_TUTORIAL_USE_VERTEX_BUFFER 0
#endif

#ifndef DVZ_TUTORIAL_USE_INDEXED_DEPTH
#define DVZ_TUTORIAL_USE_INDEXED_DEPTH 0
#endif

#ifndef DVZ_TUTORIAL_USE_TEXTURE_UPLOAD
#define DVZ_TUTORIAL_USE_TEXTURE_UPLOAD 0
#endif

#ifndef DVZ_TUTORIAL_USE_ARCBALL
#define DVZ_TUTORIAL_USE_ARCBALL 0
#endif

#define SPIKE_PATH_MAX 4096



/*************************************************************************************************/
/*  State                                                                                        */
/*************************************************************************************************/

typedef struct
{
    bool live;
    bool validation;
    uint32_t frame_count;
    const char* png_path;
    const char* shader_dir;
    bool vertex_buffer;
    bool indexed_depth;
    bool texture_upload;
    bool arcball;
} SpikeOptions;



typedef struct
{
#if DVZ_TUTORIAL_USE_INDEXED_DEPTH || DVZ_TUTORIAL_USE_ARCBALL
    float position[3];
#else
    float position[2];
#endif
    float color[3];
#if DVZ_TUTORIAL_USE_TEXTURE_UPLOAD
    float texcoord[2];
#endif
} SpikeVertex;



#if DVZ_TUTORIAL_USE_ARCBALL
static const SpikeVertex TRIANGLE_VERTICES[8] = {
    {{-0.65f, -0.65f, -0.65f}, {1.00f, 0.25f, 0.25f}},
    {{+0.65f, -0.65f, -0.65f}, {1.00f, 0.75f, 0.25f}},
    {{+0.65f, +0.65f, -0.65f}, {0.30f, 0.90f, 0.40f}},
    {{-0.65f, +0.65f, -0.65f}, {0.20f, 0.75f, 1.00f}},
    {{-0.65f, -0.65f, +0.65f}, {0.55f, 0.30f, 1.00f}},
    {{+0.65f, -0.65f, +0.65f}, {1.00f, 0.30f, 0.75f}},
    {{+0.65f, +0.65f, +0.65f}, {0.30f, 1.00f, 0.90f}},
    {{-0.65f, +0.65f, +0.65f}, {0.90f, 0.90f, 0.25f}},
};
#elif DVZ_TUTORIAL_USE_INDEXED_DEPTH
static const SpikeVertex TRIANGLE_VERTICES[6] = {
    {{-0.70f, +0.55f, 0.20f}, {1.00f, 0.35f, 0.25f}},
    {{+0.35f, +0.55f, 0.20f}, {1.00f, 0.70f, 0.25f}},
    {{-0.15f, -0.70f, 0.20f}, {0.95f, 0.20f, 0.55f}},
    {{-0.35f, +0.70f, 0.70f}, {0.20f, 0.45f, 1.00f}},
    {{+0.70f, +0.70f, 0.70f}, {0.25f, 0.90f, 0.90f}},
    {{+0.15f, -0.55f, 0.70f}, {0.35f, 0.30f, 1.00f}},
};
#elif DVZ_TUTORIAL_USE_TEXTURE_UPLOAD
static const SpikeVertex TRIANGLE_VERTICES[3] = {
    {{0.00f, -0.75f}, {1.0f, 1.0f, 1.0f}, {0.50f, 1.00f}},
    {{0.75f, +0.70f}, {1.0f, 1.0f, 1.0f}, {1.00f, 0.00f}},
    {{-0.75f, +0.70f}, {1.0f, 1.0f, 1.0f}, {0.00f, 0.00f}},
};
#else
static const SpikeVertex TRIANGLE_VERTICES[3] = {
    {{0.00f, -0.65f}, {1.0f, 0.2f, 0.2f}},
    {{0.65f, +0.65f}, {0.2f, 1.0f, 0.2f}},
    {{-0.65f, +0.65f}, {0.2f, 0.4f, 1.0f}},
};
#endif
#if DVZ_TUTORIAL_USE_ARCBALL
static const uint16_t TRIANGLE_INDICES[36] = {
    0, 1, 2, 2, 3, 0, 4, 6, 5, 6, 4, 7, 0, 4, 5, 5, 1, 0,
    3, 2, 6, 6, 7, 3, 1, 5, 6, 6, 2, 1, 0, 3, 7, 7, 4, 0,
};
#define SPIKE_INDEX_COUNT 36
#else
static const uint16_t TRIANGLE_INDICES[6] = {0, 1, 2, 3, 4, 5};
#define SPIKE_INDEX_COUNT 6
#endif



typedef struct
{
    DvzDevice* device;
    DvzVma* allocator;
    DvzSlots* slots;
    DvzGraphics* pipeline;
    DvzBuffer* vertex_buffer;
    DvzBuffer* index_buffer;
    DvzImages* texture;
    DvzImageViews* texture_view;
    DvzSampler* sampler;
    DvzDescriptors* descriptors;
    DvzArcball* arcball;
    DvzCamera* camera;
    DvzInputRouter* input_router;
    DvzCommands* commands;
    DvzRendering* rendering;
    VkFormat color_format;
    const char* shader_dir;
    bool use_vertex_buffer;
    bool use_indexed_depth;
    bool use_texture_upload;
    bool use_arcball;
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
    fprintf(
        stderr,
        "Usage: %s [--offscreen|--live] [--frames N] [--validate] [--png PATH] "
        "[--shader-dir PATH] [--vertex-buffer]\n",
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
        .png_path = "triangle.png",
        .shader_dir = DVZ_TUTORIAL_SHADER_DIR,
        .vertex_buffer = DVZ_TUTORIAL_USE_VERTEX_BUFFER != 0,
        .indexed_depth = DVZ_TUTORIAL_USE_INDEXED_DEPTH != 0,
        .texture_upload = DVZ_TUTORIAL_USE_TEXTURE_UPLOAD != 0,
        .arcball = DVZ_TUTORIAL_USE_ARCBALL != 0,
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
        else if (strcmp(argv[i], "--shader-dir") == 0 && i + 1 < argc)
        {
            options->shader_dir = argv[++i];
        }
        else if (strcmp(argv[i], "--vertex-buffer") == 0)
        {
            options->vertex_buffer = true;
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
 * Multiply two column-major 4x4 matrices.
 *
 * @param left left matrix
 * @param right right matrix
 * @param[out] out product matrix
 */
static void _mat4_multiply(const mat4 left, const mat4 right, mat4 out)
{
    mat4 product = {{0}};
    for (uint32_t column = 0; column < 4; column++)
    {
        for (uint32_t row = 0; row < 4; row++)
        {
            for (uint32_t k = 0; k < 4; k++)
                product[column][row] += left[k][row] * right[column][k];
        }
    }
    memcpy(out, product, sizeof(product));
}



/**
 * Destroy renderer-owned vklite objects.
 *
 * @param renderer renderer to destroy
 */
static void _renderer_destroy(SpikeRenderer* renderer)
{
    if (renderer->arcball != NULL && renderer->input_router != NULL)
        (void)dvz_arcball_disconnect(renderer->arcball, renderer->input_router);
    if (renderer->camera != NULL)
    {
        dvz_camera_destroy(renderer->camera);
        renderer->camera = NULL;
    }
    if (renderer->arcball != NULL)
    {
        dvz_arcball_destroy(renderer->arcball);
        renderer->arcball = NULL;
    }
    renderer->input_router = NULL;
    dvz_commands_free(renderer->commands);
    renderer->commands = NULL;
    dvz_rendering_free(renderer->rendering);
    renderer->rendering = NULL;
    if (renderer->vertex_buffer != NULL)
    {
        dvz_buffer_destroy(renderer->vertex_buffer);
        dvz_buffer_free(renderer->vertex_buffer);
        renderer->vertex_buffer = NULL;
    }
    if (renderer->index_buffer != NULL)
    {
        dvz_buffer_destroy(renderer->index_buffer);
        dvz_buffer_free(renderer->index_buffer);
        renderer->index_buffer = NULL;
    }
    dvz_descriptors_free(renderer->descriptors);
    renderer->descriptors = NULL;
    if (renderer->sampler != NULL)
    {
        dvz_sampler_destroy(renderer->sampler);
        dvz_sampler_free(renderer->sampler);
        renderer->sampler = NULL;
    }
    if (renderer->texture_view != NULL)
    {
        dvz_image_views_destroy(renderer->texture_view);
        dvz_image_views_free(renderer->texture_view);
        renderer->texture_view = NULL;
    }
    if (renderer->texture != NULL)
    {
        dvz_images_destroy(renderer->texture);
        dvz_images_free(renderer->texture);
        renderer->texture = NULL;
    }
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
 * Read and compile one external tutorial shader.
 *
 * @param shader_dir directory containing the tutorial shaders
 * @param filename shader filename
 * @param stage shader stage
 * @param[out] result owned compilation result
 * @return 0 on success or -1 on failure
 */
static int _compile_shader(
    const char* shader_dir, const char* filename, DvzShaderStage stage,
    DvzShaderCompileResult* result)
{
    char path[SPIKE_PATH_MAX] = {0};
    int length = snprintf(path, sizeof(path), "%s/%s", shader_dir, filename);
    if (length < 0 || (size_t)length >= sizeof(path))
    {
        fprintf(stderr, "triangle: shader path is too long\n");
        return -1;
    }

    DvzSize source_size = 0;
    char* source = dvz_read_text(path, &source_size);
    if (source == NULL)
    {
        fprintf(stderr, "triangle: unable to read shader %s\n", path);
        return -1;
    }

    DvzShaderCompileRequest request = {
        .stage = stage,
        .profile = DVZ_SHADER_PROFILE_GRAPHICS,
        .source = source,
        .source_size = source_size,
        .source_name = path,
        .entry_point = "main",
    };
    DvzShaderCompileStatus status = dvz_shader_compile(&request, result);
    dvz_memory_free(source);
    if (status != DVZ_SHADER_COMPILE_SUCCESS)
    {
        fprintf(
            stderr, "triangle: unable to compile %s (%s): %s\n", path,
            dvz_shader_compile_status_name(status),
            result->diagnostics != NULL ? result->diagnostics : "no diagnostics");
        return -1;
    }
    return 0;
}



/**
 * Create and upload a small procedural sRGB texture.
 *
 * The upload remains deliberately explicit: host-visible staging storage, two image-layout
 * transitions, the copy, blocking submission, view, sampler, and descriptor are separate objects.
 *
 * @param renderer renderer receiving the owned texture resources
 * @return 0 on success or -1 on failure
 */
static int _renderer_create_texture(SpikeRenderer* renderer)
{
    static const uint8_t PIXELS[4 * 4 * 4] = {
        255, 64,  64,  255, 255, 64,  64,  255, 32,  210, 220, 255, 32, 210, 220, 255,
        255, 190, 32,  255, 255, 255, 255, 255, 60,  70,  130, 255, 32, 210, 220, 255,
        180, 60,  220, 255, 180, 60,  220, 255, 255, 255, 255, 255, 40, 210, 110, 255,
        180, 60,  220, 255, 20,  25,  45,  255, 40,  210, 110, 255, 40, 210, 110, 255,
    };

    DvzBuffer* staging = dvz_buffer_create_wrapper();
    DvzCommands* upload = dvz_commands_create_wrapper();
    renderer->texture = dvz_images_create_wrapper();
    renderer->texture_view = dvz_image_views_create_wrapper();
    renderer->sampler = dvz_sampler_create_wrapper();
    renderer->descriptors = dvz_descriptors_create_wrapper();
    if (staging == NULL || upload == NULL || renderer->texture == NULL ||
        renderer->texture_view == NULL || renderer->sampler == NULL ||
        renderer->descriptors == NULL)
    {
        goto error;
    }

    dvz_buffer(renderer->device, renderer->allocator, staging);
    dvz_buffer_size(staging, sizeof(PIXELS));
    dvz_buffer_usage(staging, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    dvz_buffer_flags(staging, DVZ_ALLOC_MAPPED | DVZ_ALLOC_HOST_ACCESS_SEQUENTIAL_WRITE);
    if (dvz_buffer_create(staging) != 0)
        goto error;
    dvz_buffer_upload(staging, 0, sizeof(PIXELS), PIXELS);

    dvz_images(renderer->device, renderer->allocator, VK_IMAGE_TYPE_2D, 1, renderer->texture);
    dvz_images_format(renderer->texture, VK_FORMAT_R8G8B8A8_SRGB);
    dvz_images_size(renderer->texture, 4, 4, 1);
    dvz_images_usage(
        renderer->texture, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    if (dvz_images_create(renderer->texture) != 0)
        goto error;

    dvz_image_views(renderer->texture, renderer->texture_view);
    if (dvz_image_views_create(renderer->texture_view) != 0)
        goto error;

    dvz_sampler(renderer->device, renderer->sampler);
    dvz_sampler_min_filter(renderer->sampler, VK_FILTER_NEAREST);
    dvz_sampler_mag_filter(renderer->sampler, VK_FILTER_NEAREST);
    dvz_sampler_address_mode(
        renderer->sampler, DVZ_SAMPLER_AXIS_U, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    dvz_sampler_address_mode(
        renderer->sampler, DVZ_SAMPLER_AXIS_V, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    if (dvz_sampler_create(renderer->sampler) != 0)
        goto error;

    dvz_commands(renderer->device, dvz_device_queue(renderer->device, DVZ_QUEUE_MAIN), 1, upload);
    if (dvz_cmd_begin_result(upload) != 0)
        goto error;

    DvzBarriers barriers = {0};
    dvz_barriers(&barriers);
    DvzBarrierImage* image_barrier =
        dvz_barriers_image(&barriers, dvz_image_handle(renderer->texture, 0));
    if (image_barrier == NULL)
        goto error;
    dvz_barrier_image_stage(image_barrier, VK_PIPELINE_STAGE_2_NONE, VK_PIPELINE_STAGE_2_COPY_BIT);
    dvz_barrier_image_access(image_barrier, 0, VK_ACCESS_2_TRANSFER_WRITE_BIT);
    dvz_barrier_image_layout(
        image_barrier, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    dvz_barrier_image_aspect(image_barrier, VK_IMAGE_ASPECT_COLOR_BIT);
    dvz_cmd_barriers(upload, &barriers);

    DvzImageRegion region = {0};
    dvz_image_region(&region);
    dvz_image_region_extent(&region, 4, 4, 1);
    dvz_cmd_copy_buffer_to_image(
        upload, dvz_buffer_handle(staging), 0, dvz_image_handle(renderer->texture, 0),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &region);

    dvz_barrier_image_stage(
        image_barrier, VK_PIPELINE_STAGE_2_COPY_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
    dvz_barrier_image_access(
        image_barrier, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT);
    dvz_barrier_image_layout(
        image_barrier, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    dvz_cmd_barriers(upload, &barriers);
    if (dvz_cmd_end_result(upload) != 0 || dvz_cmd_submit_result(upload) != 0)
        goto error;

    dvz_descriptors(renderer->slots, renderer->descriptors);
    dvz_descriptors_image(
        renderer->descriptors, 0, 0, 0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        dvz_image_views_handle(renderer->texture_view, 0), dvz_sampler_handle(renderer->sampler));
    if (dvz_descriptors_handle(renderer->descriptors, 0) == VK_NULL_HANDLE)
        goto error;

    dvz_commands_destroy(upload);
    dvz_commands_free(upload);
    dvz_buffer_destroy(staging);
    dvz_buffer_free(staging);
    return 0;

error:
    if (upload != NULL)
    {
        dvz_commands_destroy(upload);
        dvz_commands_free(upload);
    }
    if (staging != NULL)
    {
        dvz_buffer_destroy(staging);
        dvz_buffer_free(staging);
    }
    return -1;
}



/**
 * Compile the external spike shaders and create the graphics pipeline.
 *
 * @param renderer renderer receiving owned vklite objects
 * @param device borrowed device that outlives the renderer
 * @param allocator borrowed allocator that outlives the renderer
 * @param color_format Canvas attachment format used by the pipeline
 * @return 0 on success or -1 on failure
 */
static int _renderer_create(
    SpikeRenderer* renderer, DvzDevice* device, DvzVma* allocator, VkFormat color_format)
{
    renderer->device = device;
    renderer->allocator = allocator;
    renderer->color_format = color_format;

    DvzShaderCompileResult vertex = {0};
    DvzShaderCompileResult fragment = {0};
    if (_compile_shader(
            renderer->shader_dir, "vklite_triangle.vert", DVZ_SHADER_STAGE_VERTEX, &vertex) != 0 ||
        _compile_shader(
            renderer->shader_dir, "vklite_triangle.frag", DVZ_SHADER_STAGE_FRAGMENT, &fragment) != 0)
    {
        dvz_shader_compile_result_destroy(&vertex);
        dvz_shader_compile_result_destroy(&fragment);
        return -1;
    }

    DvzShader* vertex_shader = dvz_shader_create_wrapper();
    DvzShader* fragment_shader = dvz_shader_create_wrapper();
    if (vertex_shader == NULL || fragment_shader == NULL)
    {
        dvz_shader_free(vertex_shader);
        dvz_shader_free(fragment_shader);
        dvz_shader_compile_result_destroy(&vertex);
        dvz_shader_compile_result_destroy(&fragment);
        return -1;
    }
    dvz_shader(device, vertex.spirv_size, vertex.spirv, vertex_shader);
    dvz_shader(device, fragment.spirv_size, fragment.spirv, fragment_shader);
    dvz_shader_compile_result_destroy(&vertex);
    dvz_shader_compile_result_destroy(&fragment);

    renderer->slots = dvz_slots_create_wrapper();
    renderer->pipeline = dvz_graphics_create_wrapper();
    if (renderer->slots == NULL || renderer->pipeline == NULL)
        goto error;

    dvz_slots(device, renderer->slots);
    if (renderer->use_texture_upload)
    {
        dvz_slots_binding(
            renderer->slots, 0, 0, 1, VK_SHADER_STAGE_FRAGMENT_BIT,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    }
    if (renderer->use_arcball)
        dvz_slots_push(renderer->slots, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4));
    if (dvz_slots_create(renderer->slots) != 0)
        goto error;

    dvz_graphics(device, renderer->pipeline);
    dvz_graphics_shader(
        renderer->pipeline, VK_SHADER_STAGE_VERTEX_BIT, dvz_shader_handle(vertex_shader));
    dvz_graphics_shader(
        renderer->pipeline, VK_SHADER_STAGE_FRAGMENT_BIT, dvz_shader_handle(fragment_shader));
    if (renderer->use_vertex_buffer)
    {
        dvz_graphics_vertex_binding(
            renderer->pipeline, 0, sizeof(SpikeVertex), VK_VERTEX_INPUT_RATE_VERTEX);
        dvz_graphics_vertex_attr(
            renderer->pipeline, 0, 0,
            renderer->use_indexed_depth ? VK_FORMAT_R32G32B32_SFLOAT : VK_FORMAT_R32G32_SFLOAT,
            offsetof(SpikeVertex, position));
        dvz_graphics_vertex_attr(
            renderer->pipeline, 0, 1, VK_FORMAT_R32G32B32_SFLOAT,
            offsetof(SpikeVertex, color));
#if DVZ_TUTORIAL_USE_TEXTURE_UPLOAD
        dvz_graphics_vertex_attr(
            renderer->pipeline, 0, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(SpikeVertex, texcoord));
#endif
    }
    dvz_graphics_primitive(renderer->pipeline, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    dvz_graphics_attachment_color(renderer->pipeline, 0, color_format);
    if (renderer->use_indexed_depth)
    {
        dvz_graphics_attachment_depth(renderer->pipeline, VK_FORMAT_D32_SFLOAT);
        dvz_graphics_depth(
            renderer->pipeline, false, true, VK_COMPARE_OP_LESS, DVZ_GRAPHICS_FLAGS_FIXED);
    }
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

    if (renderer->use_vertex_buffer)
    {
        renderer->vertex_buffer = dvz_buffer_create_wrapper();
        if (renderer->vertex_buffer == NULL)
        {
            _renderer_destroy(renderer);
            return -1;
        }
        dvz_buffer(device, allocator, renderer->vertex_buffer);
        dvz_buffer_size(renderer->vertex_buffer, sizeof(TRIANGLE_VERTICES));
        dvz_buffer_usage(renderer->vertex_buffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        dvz_buffer_flags(
            renderer->vertex_buffer,
            DVZ_ALLOC_MAPPED | DVZ_ALLOC_HOST_ACCESS_SEQUENTIAL_WRITE);
        if (dvz_buffer_create(renderer->vertex_buffer) != 0)
        {
            _renderer_destroy(renderer);
            return -1;
        }
        dvz_buffer_upload(
            renderer->vertex_buffer, 0, sizeof(TRIANGLE_VERTICES), TRIANGLE_VERTICES);
    }
    if (renderer->use_indexed_depth)
    {
        renderer->index_buffer = dvz_buffer_create_wrapper();
        if (renderer->index_buffer == NULL)
        {
            _renderer_destroy(renderer);
            return -1;
        }
        dvz_buffer(device, allocator, renderer->index_buffer);
        dvz_buffer_size(renderer->index_buffer, sizeof(TRIANGLE_INDICES));
        dvz_buffer_usage(renderer->index_buffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
        dvz_buffer_flags(
            renderer->index_buffer,
            DVZ_ALLOC_MAPPED | DVZ_ALLOC_HOST_ACCESS_SEQUENTIAL_WRITE);
        if (dvz_buffer_create(renderer->index_buffer) != 0)
        {
            _renderer_destroy(renderer);
            return -1;
        }
        dvz_buffer_upload(
            renderer->index_buffer, 0, sizeof(TRIANGLE_INDICES), TRIANGLE_INDICES);
    }
    if (renderer->use_texture_upload && _renderer_create_texture(renderer) != 0)
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
    if (
        renderer->use_indexed_depth &&
        (!frame->depth_valid || !frame->depth_image_borrowed || !frame->depth_view_borrowed ||
         frame->depth_format != VK_FORMAT_D32_SFLOAT ||
         frame->depth_layout != VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL))
    {
        renderer->invalid_frame_contract_count++;
        return;
    }

    if (renderer->pipeline == NULL)
    {
        if (_renderer_create(
                renderer, renderer->device, renderer->allocator, frame->color_format) != 0)
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
    if (renderer->use_indexed_depth)
    {
        DvzAttachment* depth = dvz_rendering_depth(rendering);
        dvz_attachment_image(depth, frame->depth_view, frame->depth_layout);
        dvz_attachment_ops(depth, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);
        dvz_attachment_clear(depth, (VkClearValue){.depthStencil = {1.0f, 0}});
    }
    if (renderer->use_arcball)
    {
        if (
            renderer->arcball == NULL || renderer->camera == NULL ||
            dvz_camera_resize(
                renderer->camera, (float)frame->extent.width, (float)frame->extent.height) !=
                DVZ_OK)
        {
            renderer->invalid_frame_contract_count++;
            (void)dvz_commands_unwrap(commands);
            return;
        }
        DvzMVP mvp = {0};
        mat4 view_model = {{0}};
        mat4 transform = {{0}};
        dvz_camera_mvp(renderer->camera, &mvp);
        dvz_arcball_mvp(renderer->arcball, &mvp);
        _mat4_multiply(mvp.view, mvp.model, view_model);
        _mat4_multiply(mvp.proj, view_model, transform);
        if (
            dvz_cmd_push_constants(
                commands, renderer->slots, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(transform),
                transform) != DVZ_OK)
        {
            renderer->invalid_frame_contract_count++;
            (void)dvz_commands_unwrap(commands);
            return;
        }
    }
    dvz_cmd_rendering_begin(commands, rendering);
    dvz_cmd_bind_graphics(commands, renderer->pipeline);
    if (renderer->use_vertex_buffer)
    {
        DvzSize offset = 0;
        dvz_cmd_bind_vertex_buffers(commands, 0, 1, renderer->vertex_buffer, &offset);
    }
    if (renderer->use_indexed_depth)
    {
        dvz_cmd_bind_index_buffer(
            commands, renderer->index_buffer, 0, VK_INDEX_TYPE_UINT16);
    }

    dvz_cmd_set_viewport_scissor(commands, frame->extent);
    if (renderer->use_texture_upload)
    {
        dvz_cmd_bind_descriptors(
            commands, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->descriptors, 0, 1, 0, NULL);
    }
    if (renderer->use_indexed_depth)
        dvz_cmd_draw_indexed(commands, 0, 0, SPIKE_INDEX_COUNT, 0, 1);
    else
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
    window_config.title = "Modern GPU Graphics in C";
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
    if (options->indexed_depth)
        canvas_config.depth_format = VK_FORMAT_D32_SFLOAT;
    runtime->canvas = dvz_canvas_create(&canvas_config);
    if (runtime->canvas == NULL)
        return -1;

    runtime->renderer.device = dvz_gpu_ctx_device(runtime->gpu);
    runtime->renderer.allocator = dvz_gpu_ctx_alloc(runtime->gpu);
    runtime->renderer.shader_dir = options->shader_dir;
    runtime->renderer.use_vertex_buffer =
        options->vertex_buffer || options->indexed_depth || options->texture_upload;
    runtime->renderer.use_indexed_depth = options->indexed_depth;
    runtime->renderer.use_texture_upload = options->texture_upload;
    runtime->renderer.use_arcball = options->arcball;
    runtime->renderer.commands = dvz_commands_create_wrapper();
    runtime->renderer.rendering = dvz_rendering_create_wrapper();
    if (runtime->renderer.commands == NULL || runtime->renderer.rendering == NULL)
        return -1;
    if (runtime->renderer.use_arcball)
    {
        runtime->renderer.input_router = dvz_canvas_input(runtime->canvas);
        runtime->renderer.arcball = dvz_arcball_create(NULL);
        runtime->renderer.camera = dvz_camera_create(NULL);
        if (
            runtime->renderer.input_router == NULL || runtime->renderer.arcball == NULL ||
            runtime->renderer.camera == NULL ||
            dvz_arcball_initial(
                runtime->renderer.arcball, (vec3){0.45f, -0.55f, 0.15f}) != DVZ_OK ||
            dvz_arcball_connect(
                runtime->renderer.arcball, runtime->renderer.input_router) != DVZ_OK)
        {
            return -1;
        }
    }
    VkFormat frame_format = dvz_canvas_frame_format(runtime->canvas);
    if (frame_format != VK_FORMAT_UNDEFINED &&
        _renderer_create(
            &runtime->renderer, runtime->renderer.device, runtime->renderer.allocator,
            frame_format) != 0)
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
        fprintf(
            stdout, "triangle: saved %s after %u frames\n", options->png_path,
            submitted_count);
    }
    fprintf(
        stdout,
        "triangle: submitted=%u drawn=%llu extent_changes=%llu "
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
        fprintf(
            stderr, "triangle: %u Vulkan validation error(s)\n",
            runtime.validation_error_count);
        result = -1;
    }
    if (result != 0)
        fprintf(stderr, "triangle: failed\n");
    return result == 0 ? 0 : 1;
}
