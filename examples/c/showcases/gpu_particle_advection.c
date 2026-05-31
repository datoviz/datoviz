/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* gpu_particle_advection - DRP2 compute-to-graphics particle showcase.
 *
 * Scenario: gpu_particle_advection
 * Style: showcase, graphite_cyan, GPU-only frame loop, final PNG capture
 *
 * Build:  just example-c showcases/gpu_particle_advection
 * Run:    ./build/examples/c/showcases/gpu_particle_advection 120
 * Output: gpu_particle_advection.png beside the executable
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <vulkan/vulkan_core.h>

#include "datoviz/drp2.h"
#include "datoviz/fileio/fileio.h"
#include "datoviz/vk/gpu_ctx.h"
#include "example_common.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH          1600u
#define HEIGHT         1000u
#define PARTICLE_COUNT 65536u
#define WORKGROUP_SIZE 128u
#define DEFAULT_FRAMES 180u

#define ID_PARAM_BUFFER   1
#define ID_PARTICLE_BUF   2
#define ID_READBACK_BUF   3
#define ID_BIND_LAYOUT    4
#define ID_BIND_GROUP     5
#define ID_COMPUTE_SHADER 6
#define ID_VERTEX_SHADER  7
#define ID_FRAGMENT_SHADER 8
#define ID_COMPUTE_PIPE   9
#define ID_RENDER_PIPE    10
#define ID_COLOR_TARGET   11
#define ID_SUBMIT_BASE    100000
#define ID_FRAME_BASE     1000



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct Particle
{
    float pos_size[4];
    float vel_age[4];
    float color[4];
} Particle;



/*************************************************************************************************/
/*  Shaders                                                                                      */
/*************************************************************************************************/

static const char* COMPUTE_GLSL =
    "#version 450\n"
    "layout(local_size_x = 128) in;\n"
    "struct Particle {\n"
    "    vec4 pos_size;\n"
    "    vec4 vel_age;\n"
    "    vec4 color;\n"
    "};\n"
    "layout(std430, set = 0, binding = 0) readonly buffer Params { vec4 sim; } params;\n"
    "layout(std430, set = 0, binding = 1) buffer Particles { Particle p[]; } particles;\n"
    "void main() {\n"
    "    uint i = gl_GlobalInvocationID.x;\n"
    "    uint count = uint(params.sim.z);\n"
    "    if (i >= count) return;\n"
    "    float t = params.sim.x;\n"
    "    float dt = params.sim.y;\n"
    "    vec2 x = particles.p[i].pos_size.xy;\n"
    "    vec2 v = particles.p[i].vel_age.xy;\n"
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
    "    float speed = clamp(length(v) * 5.0, 0.0, 1.0);\n"
    "    particles.p[i].pos_size.xy = x;\n"
    "    particles.p[i].vel_age.xy = v;\n"
    "    particles.p[i].vel_age.z += dt;\n"
    "    particles.p[i].color = vec4(0.10 + 0.20 * speed, 0.55 + 0.35 * speed,\n"
    "                                0.85 + 0.15 * sin(t + float(i) * 0.013), 1.0);\n"
    "}\n";

static const char* VERTEX_GLSL =
    "#version 450\n"
    "layout(location = 0) in vec4 in_pos_size;\n"
    "layout(location = 1) in vec4 in_color;\n"
    "layout(location = 0) out vec4 frag_color;\n"
    "void main() {\n"
    "    gl_Position = vec4(in_pos_size.xy, 0.0, 1.0);\n"
    "    gl_PointSize = in_pos_size.w;\n"
    "    frag_color = in_color;\n"
    "}\n";

static const char* FRAGMENT_GLSL =
    "#version 450\n"
    "layout(location = 0) in vec4 frag_color;\n"
    "layout(location = 0) out vec4 out_color;\n"
    "void main() {\n"
    "    vec2 p = gl_PointCoord * 2.0 - 1.0;\n"
    "    float r2 = dot(p, p);\n"
    "    if (r2 > 1.0) discard;\n"
    "    float core = smoothstep(1.0, 0.0, r2);\n"
    "    vec3 color = frag_color.rgb * (0.35 + 0.90 * core);\n"
    "    out_color = vec4(color, 1.0);\n"
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



static void _init_particles(Particle* particles)
{
    for (uint32_t i = 0; i < PARTICLE_COUNT; i++)
    {
        const float a = 6.28318530718f * _hash01(i * 3u + 1u);
        const float r = 0.18f + 0.78f * sqrtf(_hash01(i * 3u + 2u));
        const float jitter = _hash01(i * 3u + 3u);
        const float x = r * cosf(a);
        const float y = r * sinf(a) * 0.92f;
        particles[i].pos_size[0] = x;
        particles[i].pos_size[1] = y;
        particles[i].pos_size[2] = 0.0f;
        particles[i].pos_size[3] = 1.6f + 2.2f * jitter;
        particles[i].vel_age[0] = -0.16f * y;
        particles[i].vel_age[1] = +0.16f * x;
        particles[i].vel_age[2] = 0.0f;
        particles[i].vel_age[3] = 1.0f;
        particles[i].color[0] = 0.10f;
        particles[i].color[1] = 0.72f;
        particles[i].color[2] = 0.95f;
        particles[i].color[3] = 1.0f;
    }
}



static uint32_t _frame_count_from_args(int argc, char** argv)
{
    if (argc < 2)
        return DEFAULT_FRAMES;
    const long value = strtol(argv[1], NULL, 10);
    if (value <= 0)
        return DEFAULT_FRAMES;
    if (value > 1000)
        return 1000u;
    return (uint32_t)value;
}



static void _append_frame(DvzDrp2CommandStream* stream, uint32_t frame, const float params[4])
{
    const uint64_t base = ID_FRAME_BASE + (uint64_t)frame * 10u;
    const uint64_t enc = base + 1u;
    const uint64_t cpass = base + 2u;
    const uint64_t rpass = base + 3u;
    const uint64_t cmdbuf = base + 4u;
    const uint64_t submit = ID_SUBMIT_BASE + frame;
    const uint64_t particle_bytes = (uint64_t)PARTICLE_COUNT * sizeof(Particle);

    dvz_drp2_stream_write_buffer_bytes(stream, ID_PARAM_BUFFER, 0, 4u * sizeof(float), params);
    dvz_drp2_stream_begin_command_encoder(stream, enc);
    dvz_drp2_stream_begin_compute_pass(stream, cpass, enc);
    dvz_drp2_stream_set_pipeline(stream, cpass, ID_COMPUTE_PIPE);
    dvz_drp2_stream_set_bind_group(stream, cpass, 0, ID_BIND_GROUP);
    dvz_drp2_stream_dispatch_workgroups(
        stream, cpass, (PARTICLE_COUNT + WORKGROUP_SIZE - 1u) / WORKGROUP_SIZE, 1, 1);
    dvz_drp2_stream_end_compute_pass(stream, cpass);
    dvz_drp2_stream_resource_barrier(
        stream, enc, ID_PARTICLE_BUF, "COMPUTE", "STORAGE_WRITE", "VERTEX_INPUT",
        "VERTEX_READ", 0, particle_bytes);
    dvz_drp2_stream_begin_render_pass_clear(
        stream, rpass, enc, ID_COLOR_TARGET, 0.015f, 0.021f, 0.028f, 1.0f);
    dvz_drp2_stream_set_pipeline(stream, rpass, ID_RENDER_PIPE);
    dvz_drp2_stream_set_vertex_buffer(stream, rpass, 0, ID_PARTICLE_BUF, 0);
    dvz_drp2_stream_draw(stream, rpass, PARTICLE_COUNT, 1, 0, 0);
    dvz_drp2_stream_end_render_pass(stream, rpass);
    dvz_drp2_stream_copy_texture_to_buffer(
        stream, enc, ID_COLOR_TARGET, ID_READBACK_BUF, 0, WIDTH, HEIGHT, WIDTH * 4u, HEIGHT);
    dvz_drp2_stream_finish_command_encoder(stream, enc, cmdbuf);
    dvz_drp2_stream_queue_submit(stream, cmdbuf, submit);
}



/*************************************************************************************************/
/*  Main                                                                                         */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    const uint32_t frame_count = _frame_count_from_args(argc, argv);
    const uint64_t particle_bytes = (uint64_t)PARTICLE_COUNT * sizeof(Particle);
    const uint64_t pixel_bytes = (uint64_t)WIDTH * HEIGHT * 4u;

    Particle* particles = (Particle*)calloc(PARTICLE_COUNT, sizeof(Particle));
    uint8_t* pixels = (uint8_t*)calloc((size_t)pixel_bytes, 1);
    if (particles == NULL || pixels == NULL)
    {
        fprintf(stderr, "gpu_particle_advection: allocation failed\n");
        free(particles);
        free(pixels);
        return 1;
    }
    _init_particles(particles);

    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan12Features feat12 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .timelineSemaphore = VK_TRUE,
    };
    dvz_gpu_ctx_config_features12(&gpu_cfg, &feat12);
    VkPhysicalDeviceVulkan13Features feat13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .dynamicRendering = VK_TRUE,
        .synchronization2 = VK_TRUE,
    };
    dvz_gpu_ctx_config_features13(&gpu_cfg, &feat13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        fprintf(stderr, "gpu_particle_advection: GPU context creation failed\n");
        free(particles);
        free(pixels);
        return 1;
    }

    DvzDrp2RuntimeConfig rt_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&rt_cfg);
    if (runtime == NULL)
    {
        fprintf(stderr, "gpu_particle_advection: DRP2 runtime creation failed\n");
        dvz_gpu_ctx_destroy(ctx);
        free(particles);
        free(pixels);
        return 1;
    }

    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    if (stream == NULL)
    {
        fprintf(stderr, "gpu_particle_advection: stream allocation failed\n");
        dvz_drp2_runtime_destroy(runtime);
        dvz_gpu_ctx_destroy(ctx);
        free(particles);
        free(pixels);
        return 1;
    }

    dvz_drp2_stream_hello_renderer(stream, "gpu_particle_advection");
    dvz_drp2_stream_renderer_hello_reply(stream, "vklite");
    dvz_drp2_stream_create_buffer(
        stream, ID_PARAM_BUFFER, 4u * sizeof(float),
        DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_STORAGE);
    dvz_drp2_stream_create_buffer(
        stream, ID_PARTICLE_BUF, particle_bytes,
        DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_STORAGE |
            DVZ_DRP2_BUFFER_USAGE_VERTEX);
    dvz_drp2_stream_write_buffer_bytes(
        stream, ID_PARTICLE_BUF, 0, particle_bytes, particles);
    dvz_drp2_stream_create_buffer(
        stream, ID_READBACK_BUF, pixel_bytes, DVZ_DRP2_BUFFER_USAGE_COPY_DST);

    DvzDrp2BindGroupLayoutEntry layout[2] = {
        {
            .binding = 0,
            .binding_type = DVZ_DRP2_BINDING_TYPE_STORAGE_BUFFER,
            .visibility = DVZ_DRP2_SHADER_STAGE_COMPUTE,
            .access = DVZ_DRP2_BINDING_ACCESS_READ,
        },
        {
            .binding = 1,
            .binding_type = DVZ_DRP2_BINDING_TYPE_STORAGE_BUFFER,
            .visibility = DVZ_DRP2_SHADER_STAGE_COMPUTE,
            .access = DVZ_DRP2_BINDING_ACCESS_READ_WRITE,
        },
    };
    dvz_drp2_stream_create_bind_group_layout_entries(stream, ID_BIND_LAYOUT, 2, layout);

    DvzDrp2BindGroupEntry entries[2] = {
        {
            .binding = 0,
            .binding_type = DVZ_DRP2_BINDING_TYPE_STORAGE_BUFFER,
            .resource_kind = DVZ_DRP2_BINDING_RESOURCE_BUFFER,
            .resource_id = ID_PARAM_BUFFER,
            .size = 4u * sizeof(float),
        },
        {
            .binding = 1,
            .binding_type = DVZ_DRP2_BINDING_TYPE_STORAGE_BUFFER,
            .resource_kind = DVZ_DRP2_BINDING_RESOURCE_BUFFER,
            .resource_id = ID_PARTICLE_BUF,
            .size = particle_bytes,
        },
    };
    dvz_drp2_stream_create_bind_group_entries(stream, ID_BIND_GROUP, ID_BIND_LAYOUT, 2, entries);
    dvz_drp2_stream_create_shader_module_format(
        stream, ID_COMPUTE_SHADER, "compute", "glsl", COMPUTE_GLSL);
    dvz_drp2_stream_create_shader_module_format(
        stream, ID_VERTEX_SHADER, "vertex", "glsl", VERTEX_GLSL);
    dvz_drp2_stream_create_shader_module_format(
        stream, ID_FRAGMENT_SHADER, "fragment", "glsl", FRAGMENT_GLSL);
    dvz_drp2_stream_create_compute_pipeline_with_bind_group_layout(
        stream, ID_COMPUTE_PIPE, ID_COMPUTE_SHADER, ID_BIND_LAYOUT);

    uint32_t strides[1] = {sizeof(Particle)};
    uint32_t bindings[2] = {0, 0};
    uint32_t locations[2] = {0, 1};
    uint32_t formats[2] = {VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT};
    uint32_t offsets[2] = {0, 8u * sizeof(float)};
    dvz_drp2_stream_create_render_pipeline_ex(
        stream, ID_RENDER_PIPE, ID_VERTEX_SHADER, ID_FRAGMENT_SHADER, 1,
        VK_PRIMITIVE_TOPOLOGY_POINT_LIST, 1, strides, 2, bindings, locations, formats, offsets);
    dvz_drp2_stream_create_texture_2d_usage(
        stream, ID_COLOR_TARGET, WIDTH, HEIGHT,
        DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC);

    for (uint32_t frame = 0; frame < frame_count; frame++)
    {
        const float params[4] = {
            (float)frame / 60.0f,
            1.0f / 60.0f,
            (float)PARTICLE_COUNT,
            0.0f,
        };
        _append_frame(stream, frame, params);
    }

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    if (!result.ok)
    {
        fprintf(
            stderr, "gpu_particle_advection: DRP2 execution failed at command %u code %d\n",
            result.command_index, result.code);
        dvz_drp2_stream_destroy(stream);
        dvz_drp2_runtime_destroy(runtime);
        dvz_gpu_ctx_destroy(ctx);
        free(particles);
        free(pixels);
        return 1;
    }

    if (!dvz_drp2_runtime_download_buffer(runtime, ID_READBACK_BUF, 0, pixel_bytes, pixels))
    {
        fprintf(stderr, "gpu_particle_advection: readback failed\n");
        dvz_drp2_stream_destroy(stream);
        dvz_drp2_runtime_destroy(runtime);
        dvz_gpu_ctx_destroy(ctx);
        free(particles);
        free(pixels);
        return 1;
    }

    char path[512];
    example_outpath(argv[0], "gpu_particle_advection.png", path, sizeof(path));
    if (dvz_write_png(path, WIDTH, HEIGHT, pixels) != 0)
    {
        fprintf(stderr, "gpu_particle_advection: failed to write %s\n", path);
        dvz_drp2_stream_destroy(stream);
        dvz_drp2_runtime_destroy(runtime);
        dvz_gpu_ctx_destroy(ctx);
        free(particles);
        free(pixels);
        return 1;
    }

    printf(
        "gpu_particle_advection: %u particles, %u frames, saved %s\n", PARTICLE_COUNT,
        frame_count, path);

    dvz_drp2_stream_destroy(stream);
    dvz_drp2_runtime_destroy(runtime);
    dvz_gpu_ctx_destroy(ctx);
    free(particles);
    free(pixels);
    return 0;
}
