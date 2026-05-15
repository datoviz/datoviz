/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  App — presentation layer                                                                     */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_status.h"
#include "_time_utils.h"
#include "_trace.h"
#include "datoviz/app.h"
#include "datoviz/gui.h"
#include "datoviz/scene.h"
#include "../drp2/_stream.h"
#include "../scene/_scene.h"
#if defined(DVZ_HAS_GUI) && DVZ_HAS_GUI
#include "../gui/_gui.h"
#endif

#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
#include "datoviz/canvas.h"
#include "datoviz/drp2/recording.h"
#include "datoviz/drp2/runtime.h"
#include "datoviz/drp2/stream.h"
#include "datoviz/input/keyboard.h"
#include "datoviz/input/pointer.h"
#include "datoviz/input/router.h"
#include "datoviz/scene/frame_plan.h"
#include "datoviz/vk/device.h"
#include "datoviz/vk/gpu_ctx.h"
#include "datoviz/window.h"
#include "datoviz/window/backend.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_APP_MAX_WINDOWS 16

/* Keep borrowed canvas targets out of the scene emitter's low transient id range. */
#define DVZ_APP_CANVAS_TARGET_BASE UINT64_C(0xF000000000000000)

#define DVZ_TRACE_COLOR_RESET "\x1b[0m"
#define DVZ_TRACE_COLOR_DIM   "\x1b[90m"
#define DVZ_TRACE_COLOR_GREEN "\x1b[32m"
#define DVZ_TRACE_COLOR_YELLOW "\x1b[33m"
#define DVZ_TRACE_COLOR_CYAN  "\x1b[36m"
#define DVZ_TRACE_COLOR_BLUE  "\x1b[34m"
#define DVZ_TRACE_COLOR_MAGENTA "\x1b[35m"
#define DVZ_TRACE_COLOR_RED   "\x1b[31m"
#define DVZ_TRACE_COLOR_BOLD  "\x1b[1m"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzAppWindow
{
    DvzApp*    app;
    DvzFigure* figure;
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    DvzWindow* window;
    DvzCanvas* canvas;
#endif
    uint64_t target_id;
    bool is_interactive;
    bool render_enabled;
    uint64_t frame_index;
    DvzAppFrameCallback frame_callback;
    void* frame_user_data;
    DvzAppRequestFrameCallback request_frame_callback;
    void* request_frame_user_data;
    DvzAppTraceSnapshot last_trace_snapshot;
    bool has_last_trace_snapshot;
    DvzDrp2Recorder* recorder;
    DvzClock recording_clock;
    bool recording_target_created;
    DvzDrp2Recording* replay_recording;
    uint64_t replay_target_id;
    uint32_t replay_frame_index;
    DvzClock replay_clock;
    bool replay_clock_started;
    bool replay_paced;
    bool replay_loop;
    double replay_speed;
#if defined(DVZ_HAS_GUI) && DVZ_HAS_GUI
    DvzGui* gui;
#endif
};


struct DvzApp
{
    DvzScene* scene;
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    DvzGpuCtx*      gpu_ctx;
    DvzDrp2Runtime* runtime;
    DvzSceneRequestExecutor request_executor;
    DvzWindowHost*  window_host;
#endif
    uint32_t     window_count;
    DvzAppWindow windows[DVZ_APP_MAX_WINDOWS];
    DvzAppStatus status;
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE

/**
 * Add Vulkan instance extensions required by an external host.
 *
 * @param gpu_cfg GPU-context configuration receiving extension names
 * @param count number of extension names
 * @param extensions extension-name array, or NULL when count is zero
 * @return true on success, false on invalid input
 */
static bool _app_gpu_config_add_instance_extensions(
    DvzGpuCtxConfig* gpu_cfg, uint32_t count, const char* const* extensions)
{
    ANN(gpu_cfg);
    if (count == 0)
        return true;
    if (extensions == NULL)
    {
        log_error("app config requires an extension-name array when extension count is nonzero");
        return false;
    }
    if (count > 16)
    {
        log_error("app config supports at most 16 Vulkan instance extensions");
        return false;
    }
    for (uint32_t i = 0; i < count; i++)
    {
        if (extensions[i] == NULL)
        {
            log_error("app config contains a NULL Vulkan instance extension name");
            return false;
        }
        dvz_gpu_ctx_config_add_instance_extension(gpu_cfg, extensions[i]);
    }
    return true;
}



/**
 * Add GLFW surface extensions when the default app configuration asks for them.
 *
 * @param app partially initialized app with a window host
 * @param gpu_cfg GPU-context configuration receiving extension names
 * @param config app configuration
 */
static void _app_gpu_config_add_glfw_extensions(
    DvzApp* app, DvzGpuCtxConfig* gpu_cfg, const DvzAppConfig* config)
{
    ANN(app);
    ANN(gpu_cfg);
    ANN(config);
#if DVZ_HAS_GLFW
    if (!config->enable_glfw_extensions)
        return;
    /* If GLFW is available, add surface extensions so the same instance supports windowed mode. */
    if (dvz_window_glfw_init())
    {
        uint32_t ext_count =
            dvz_window_host_required_extension_count(app->window_host, DVZ_BACKEND_GLFW);
        if (ext_count > 0)
        {
            const char* extensions[16] = {0};
            int written = dvz_window_host_required_extensions(
                app->window_host, DVZ_BACKEND_GLFW, ext_count, extensions);
            if (written == (int)ext_count)
            {
                for (uint32_t i = 0; i < ext_count; i++)
                    dvz_gpu_ctx_config_add_instance_extension(gpu_cfg, extensions[i]);
                dvz_gpu_ctx_config_enable_canvas_extensions(gpu_cfg, true);
            }
        }
    }
#else
    (void)app;
    (void)gpu_cfg;
    (void)config;
#endif
}



/**
 * Return a readable label for one DRP2 command type.
 *
 * @param type command type enum value
 * @return static command label
 */
static const char* _trace_command_name(DvzDrp2CommandType type)
{
    switch (type)
    {
    case DVZ_DRP2_COMMAND_HELLO_RENDERER:
        return "HelloRenderer";
    case DVZ_DRP2_COMMAND_RENDERER_HELLO_REPLY:
        return "RendererHelloReply";
    case DVZ_DRP2_COMMAND_CREATE_BUFFER:
        return "CreateBuffer";
    case DVZ_DRP2_COMMAND_DESTROY_BUFFER:
        return "DestroyBuffer";
    case DVZ_DRP2_COMMAND_CREATE_TEXTURE:
        return "CreateTexture";
    case DVZ_DRP2_COMMAND_DESTROY_TEXTURE:
        return "DestroyTexture";
    case DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE:
        return "CreateShaderModule";
    case DVZ_DRP2_COMMAND_DESTROY_SHADER_MODULE:
        return "DestroyShaderModule";
    case DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE:
        return "CreateRenderPipeline";
    case DVZ_DRP2_COMMAND_DESTROY_RENDER_PIPELINE:
        return "DestroyRenderPipeline";
    case DVZ_DRP2_COMMAND_CREATE_COMPUTE_PIPELINE:
        return "CreateComputePipeline";
    case DVZ_DRP2_COMMAND_DESTROY_COMPUTE_PIPELINE:
        return "DestroyComputePipeline";
    case DVZ_DRP2_COMMAND_CREATE_SAMPLER:
        return "CreateSampler";
    case DVZ_DRP2_COMMAND_CREATE_BIND_GROUP_LAYOUT:
        return "CreateBindGroupLayout";
    case DVZ_DRP2_COMMAND_CREATE_BIND_GROUP:
        return "CreateBindGroup";
    case DVZ_DRP2_COMMAND_DESTROY_BIND_GROUP_LAYOUT:
        return "DestroyBindGroupLayout";
    case DVZ_DRP2_COMMAND_DESTROY_BIND_GROUP:
        return "DestroyBindGroup";
    case DVZ_DRP2_COMMAND_WRITE_BUFFER:
        return "WriteBuffer";
    case DVZ_DRP2_COMMAND_WRITE_TEXTURE:
        return "WriteTexture";
    case DVZ_DRP2_COMMAND_BEGIN_COMMAND_ENCODER:
        return "BeginCommandEncoder";
    case DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS:
        return "BeginRenderPass";
    case DVZ_DRP2_COMMAND_BEGIN_COMPUTE_PASS:
        return "BeginComputePass";
    case DVZ_DRP2_COMMAND_SET_VIEWPORT:
        return "SetViewport";
    case DVZ_DRP2_COMMAND_SET_SCISSOR:
        return "SetScissor";
    case DVZ_DRP2_COMMAND_SET_PIPELINE:
        return "SetPipeline";
    case DVZ_DRP2_COMMAND_SET_BIND_GROUP:
        return "SetBindGroup";
    case DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER:
        return "SetVertexBuffer";
    case DVZ_DRP2_COMMAND_SET_INDEX_BUFFER:
        return "SetIndexBuffer";
    case DVZ_DRP2_COMMAND_DRAW:
        return "Draw";
    case DVZ_DRP2_COMMAND_DRAW_INDEXED:
        return "DrawIndexed";
    case DVZ_DRP2_COMMAND_END_RENDER_PASS:
        return "EndRenderPass";
    case DVZ_DRP2_COMMAND_DISPATCH_WORKGROUPS:
        return "DispatchWorkgroups";
    case DVZ_DRP2_COMMAND_END_COMPUTE_PASS:
        return "EndComputePass";
    case DVZ_DRP2_COMMAND_COPY_BUFFER_TO_BUFFER:
        return "CopyBufferToBuffer";
    case DVZ_DRP2_COMMAND_COPY_BUFFER_TO_TEXTURE:
        return "CopyBufferToTexture";
    case DVZ_DRP2_COMMAND_COPY_TEXTURE_TO_BUFFER:
        return "CopyTextureToBuffer";
    case DVZ_DRP2_COMMAND_COPY_TEXTURE_TO_TEXTURE:
        return "CopyTextureToTexture";
    case DVZ_DRP2_COMMAND_FINISH_COMMAND_ENCODER:
        return "FinishCommandEncoder";
    case DVZ_DRP2_COMMAND_QUEUE_SUBMIT:
        return "QueueSubmit";
    case DVZ_DRP2_COMMAND_QUEUE_SUBMIT_REPLY:
        return "QueueSubmitReply";
    case DVZ_DRP2_COMMAND_NONE:
    default:
        return "None";
    }
}


/**
 * Return the display prefix used for one DRP2 command type in human trace mode.
 *
 * @param type command type enum value
 * @return one-character semantic prefix
 */
static char _trace_command_prefix(DvzDrp2CommandType type)
{
    switch (type)
    {
    case DVZ_DRP2_COMMAND_CREATE_BUFFER:
    case DVZ_DRP2_COMMAND_CREATE_TEXTURE:
    case DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE:
    case DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE:
    case DVZ_DRP2_COMMAND_CREATE_COMPUTE_PIPELINE:
    case DVZ_DRP2_COMMAND_CREATE_SAMPLER:
    case DVZ_DRP2_COMMAND_CREATE_BIND_GROUP_LAYOUT:
    case DVZ_DRP2_COMMAND_CREATE_BIND_GROUP:
        return '+';
    case DVZ_DRP2_COMMAND_DESTROY_BUFFER:
    case DVZ_DRP2_COMMAND_DESTROY_TEXTURE:
    case DVZ_DRP2_COMMAND_DESTROY_SHADER_MODULE:
    case DVZ_DRP2_COMMAND_DESTROY_RENDER_PIPELINE:
    case DVZ_DRP2_COMMAND_DESTROY_COMPUTE_PIPELINE:
    case DVZ_DRP2_COMMAND_DESTROY_BIND_GROUP_LAYOUT:
    case DVZ_DRP2_COMMAND_DESTROY_BIND_GROUP:
        return '-';
    case DVZ_DRP2_COMMAND_WRITE_BUFFER:
    case DVZ_DRP2_COMMAND_WRITE_TEXTURE:
    case DVZ_DRP2_COMMAND_COPY_BUFFER_TO_BUFFER:
    case DVZ_DRP2_COMMAND_COPY_BUFFER_TO_TEXTURE:
    case DVZ_DRP2_COMMAND_COPY_TEXTURE_TO_BUFFER:
    case DVZ_DRP2_COMMAND_COPY_TEXTURE_TO_TEXTURE:
        return '~';
    default:
        return '=';
    }
}


/**
 * Return whether DRP2 trace color should be emitted on stderr.
 *
 * @return whether ANSI color should be used
 */
static bool _trace_color_enabled(void)
{
    const char* env = getenv("DVZ_DRP2_TRACE_COLOR");
    if (env != NULL)
    {
        if (strcmp(env, "0") == 0 || strcmp(env, "false") == 0 || strcmp(env, "FALSE") == 0 ||
            strcmp(env, "off") == 0 || strcmp(env, "OFF") == 0)
            return false;
        if (strcmp(env, "1") == 0 || strcmp(env, "true") == 0 || strcmp(env, "TRUE") == 0 ||
            strcmp(env, "on") == 0 || strcmp(env, "ON") == 0)
            return true;
    }

    if (getenv("NO_COLOR") != NULL)
        return false;

    return true;
}


/**
 * Return the ANSI color for one command category.
 *
 * @param type command type enum value
 * @return static ANSI color string
 */
static const char* _trace_command_color(DvzDrp2CommandType type)
{
    switch (type)
    {
    case DVZ_DRP2_COMMAND_CREATE_BUFFER:
    case DVZ_DRP2_COMMAND_CREATE_TEXTURE:
    case DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE:
    case DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE:
    case DVZ_DRP2_COMMAND_CREATE_COMPUTE_PIPELINE:
    case DVZ_DRP2_COMMAND_CREATE_SAMPLER:
    case DVZ_DRP2_COMMAND_CREATE_BIND_GROUP_LAYOUT:
    case DVZ_DRP2_COMMAND_CREATE_BIND_GROUP:
        return DVZ_TRACE_COLOR_GREEN;
    case DVZ_DRP2_COMMAND_DESTROY_BUFFER:
    case DVZ_DRP2_COMMAND_DESTROY_TEXTURE:
    case DVZ_DRP2_COMMAND_DESTROY_SHADER_MODULE:
    case DVZ_DRP2_COMMAND_DESTROY_RENDER_PIPELINE:
    case DVZ_DRP2_COMMAND_DESTROY_COMPUTE_PIPELINE:
    case DVZ_DRP2_COMMAND_DESTROY_BIND_GROUP_LAYOUT:
    case DVZ_DRP2_COMMAND_DESTROY_BIND_GROUP:
        return DVZ_TRACE_COLOR_RED;
    case DVZ_DRP2_COMMAND_WRITE_BUFFER:
    case DVZ_DRP2_COMMAND_WRITE_TEXTURE:
    case DVZ_DRP2_COMMAND_COPY_BUFFER_TO_BUFFER:
    case DVZ_DRP2_COMMAND_COPY_BUFFER_TO_TEXTURE:
    case DVZ_DRP2_COMMAND_COPY_TEXTURE_TO_BUFFER:
    case DVZ_DRP2_COMMAND_COPY_TEXTURE_TO_TEXTURE:
        return DVZ_TRACE_COLOR_YELLOW;
    case DVZ_DRP2_COMMAND_BEGIN_COMMAND_ENCODER:
    case DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS:
    case DVZ_DRP2_COMMAND_BEGIN_COMPUTE_PASS:
    case DVZ_DRP2_COMMAND_END_RENDER_PASS:
    case DVZ_DRP2_COMMAND_END_COMPUTE_PASS:
    case DVZ_DRP2_COMMAND_FINISH_COMMAND_ENCODER:
    case DVZ_DRP2_COMMAND_QUEUE_SUBMIT:
    case DVZ_DRP2_COMMAND_QUEUE_SUBMIT_REPLY:
        return DVZ_TRACE_COLOR_CYAN;
    case DVZ_DRP2_COMMAND_SET_VIEWPORT:
    case DVZ_DRP2_COMMAND_SET_SCISSOR:
    case DVZ_DRP2_COMMAND_SET_PIPELINE:
    case DVZ_DRP2_COMMAND_SET_BIND_GROUP:
    case DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER:
    case DVZ_DRP2_COMMAND_SET_INDEX_BUFFER:
        return DVZ_TRACE_COLOR_BLUE;
    case DVZ_DRP2_COMMAND_DRAW:
    case DVZ_DRP2_COMMAND_DRAW_INDEXED:
    case DVZ_DRP2_COMMAND_DISPATCH_WORKGROUPS:
        return DVZ_TRACE_COLOR_MAGENTA;
    default:
        return DVZ_TRACE_COLOR_DIM;
    }
}


/**
 * Format a DRP2 id with an optional stream debug label.
 *
 * @param stream command stream carrying optional labels
 * @param id numeric DRP2 id
 * @param out output string buffer
 * @param out_size output buffer size
 */
static void _trace_format_id(
    const DvzDrp2CommandStream* stream, uint64_t id, char* out, size_t out_size)
{
    ANN(out);
    if (out_size == 0)
        return;
    const char* label = dvz_drp2_stream_label(stream, id);
    if (label != NULL)
        dvz_snprintf(out, out_size, "%" PRIu64 "(%s)", id, label);
    else
        dvz_snprintf(out, out_size, "%" PRIu64, id);
}


/**
 * Print one detailed command line when the command carries useful trace payload.
 *
 * @param stream command stream carrying optional labels
 * @param command command to print
 * @param index command index
 * @param include_transient_ids whether to include per-stream pass ids
 * @return true if a line was printed
 */
static bool _app_trace_print_command_detail(
    const DvzDrp2CommandStream* stream, const DvzDrp2Command* command, uint32_t index,
    bool include_transient_ids)
{
    ANN(command);
#define TRACE_ID(name, value)                                                                       \
    char name[DVZ_DRP2_LABEL_SIZE + 64];                                                            \
    _trace_format_id(stream, (value), name, sizeof(name))

    DvzDrp2CommandType type = command->type;
    switch (type)
    {
    case DVZ_DRP2_COMMAND_HELLO_RENDERER:
        dvz_fprintf(
            stderr, "  %03u = HelloRenderer name=%s\n", index, command->u.handshake.name);
        return true;
    case DVZ_DRP2_COMMAND_RENDERER_HELLO_REPLY:
        dvz_fprintf(
            stderr, "  %03u = RendererHelloReply name=%s\n", index, command->u.handshake.name);
        return true;
    case DVZ_DRP2_COMMAND_CREATE_BUFFER:
    {
        TRACE_ID(id, command->u.create_buffer.id);
        dvz_fprintf(
            stderr, "  %03u + CreateBuffer id=%s size=%" PRIu64 " usage=0x%" PRIx32 "\n",
            index, id, command->u.create_buffer.size,
            command->u.create_buffer.usage);
        return true;
    }
    case DVZ_DRP2_COMMAND_DESTROY_BUFFER:
    {
        TRACE_ID(id, command->u.destroy_buffer.buffer_id);
        dvz_fprintf(
            stderr, "  %03u - DestroyBuffer id=%s\n", index, id);
        return true;
    }
    case DVZ_DRP2_COMMAND_CREATE_TEXTURE:
    {
        TRACE_ID(id, command->u.create_texture.id);
        dvz_fprintf(
            stderr, "  %03u + CreateTexture id=%s size=(%" PRIu32 ",%" PRIu32
                    ",%" PRIu32 ") usage=0x%" PRIx32 "\n",
            index, id, command->u.create_texture.width,
            command->u.create_texture.height, command->u.create_texture.depth,
            command->u.create_texture.usage);
        return true;
    }
    case DVZ_DRP2_COMMAND_DESTROY_TEXTURE:
    {
        TRACE_ID(id, command->u.destroy_texture.texture_id);
        dvz_fprintf(
            stderr, "  %03u - DestroyTexture id=%s\n", index, id);
        return true;
    }
    case DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE:
    {
        TRACE_ID(id, command->u.create_shader_module.id);
        dvz_fprintf(
            stderr, "  %03u + CreateShaderModule id=%s stage=%s format=%s"
                    " code=%s spirv_size=%" PRIu64 "\n",
            index, id, command->u.create_shader_module.stage,
            command->u.create_shader_module.format,
            command->u.create_shader_module.code != NULL ? "yes" : "no",
            command->u.create_shader_module.spirv_size);
        return true;
    }
    case DVZ_DRP2_COMMAND_DESTROY_SHADER_MODULE:
    {
        TRACE_ID(id, command->u.destroy_shader_module.shader_module_id);
        dvz_fprintf(
            stderr, "  %03u - DestroyShaderModule id=%s\n", index, id);
        return true;
    }
    case DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE:
    {
        TRACE_ID(id, command->u.create_render_pipeline.id);
        TRACE_ID(vs, command->u.create_render_pipeline.vertex_shader_module_id);
        TRACE_ID(fs, command->u.create_render_pipeline.fragment_shader_module_id);
        dvz_fprintf(
            stderr, "  %03u + CreateRenderPipeline id=%s vs=%s fs=%s"
                    " vslots=%" PRIu32 " bgls=%" PRIu32
                    " depth=%s write=%s compare=%" PRIu32 " topology=%" PRIu32
                    " bindings=%" PRIu32 " attrs=%" PRIu32 "\n",
            index, id, vs, fs,
            command->u.create_render_pipeline.vertex_buffer_slots,
            command->u.create_render_pipeline.bind_group_layout_count,
            command->u.create_render_pipeline.has_depth_attachment ? "yes" : "no",
            command->u.create_render_pipeline.depth_write_enabled ? "yes" : "no",
            command->u.create_render_pipeline.depth_compare_op,
            command->u.create_render_pipeline.topology,
            command->u.create_render_pipeline.binding_count,
            command->u.create_render_pipeline.attr_count);
        return true;
    }
    case DVZ_DRP2_COMMAND_DESTROY_RENDER_PIPELINE:
    {
        TRACE_ID(id, command->u.destroy_render_pipeline.render_pipeline_id);
        dvz_fprintf(
            stderr, "  %03u - DestroyRenderPipeline id=%s\n", index, id);
        return true;
    }
    case DVZ_DRP2_COMMAND_CREATE_COMPUTE_PIPELINE:
        dvz_fprintf(
            stderr, "  %03u + CreateComputePipeline id=%" PRIu64 " shader=%" PRIu64
                    " bgls=%" PRIu32 "\n",
            index, command->u.create_compute_pipeline.id,
            command->u.create_compute_pipeline.compute_shader_module_id,
            command->u.create_compute_pipeline.bind_group_layout_count);
        return true;
    case DVZ_DRP2_COMMAND_DESTROY_COMPUTE_PIPELINE:
        dvz_fprintf(
            stderr, "  %03u - DestroyComputePipeline id=%" PRIu64 "\n", index,
            command->u.destroy_compute_pipeline.compute_pipeline_id);
        return true;
    case DVZ_DRP2_COMMAND_CREATE_SAMPLER:
    {
        TRACE_ID(id, command->u.create_sampler.id);
        dvz_fprintf(
            stderr, "  %03u + CreateSampler id=%s\n", index, id);
        return true;
    }
    case DVZ_DRP2_COMMAND_CREATE_BIND_GROUP_LAYOUT:
    {
        TRACE_ID(id, command->u.create_bind_group_layout.id);
        dvz_fprintf(
            stderr, "  %03u + CreateBindGroupLayout id=%s entries=%" PRIu32 "\n",
            index, id,
            command->u.create_bind_group_layout.entry_count);
        return true;
    }
    case DVZ_DRP2_COMMAND_CREATE_BIND_GROUP:
    {
        TRACE_ID(id, command->u.create_bind_group.id);
        TRACE_ID(layout, command->u.create_bind_group.bind_group_layout_id);
        dvz_fprintf(
            stderr, "  %03u + CreateBindGroup id=%s layout=%s entries=%" PRIu32 "\n",
            index, id, layout,
            command->u.create_bind_group.entry_count);
        return true;
    }
    case DVZ_DRP2_COMMAND_DESTROY_BIND_GROUP_LAYOUT:
        dvz_fprintf(
            stderr, "  %03u - DestroyBindGroupLayout id=%" PRIu64 "\n", index,
            command->u.destroy_bind_group_layout.bind_group_layout_id);
        return true;
    case DVZ_DRP2_COMMAND_DESTROY_BIND_GROUP:
        dvz_fprintf(
            stderr, "  %03u - DestroyBindGroup id=%" PRIu64 "\n", index,
            command->u.destroy_bind_group.bind_group_id);
        return true;
    case DVZ_DRP2_COMMAND_WRITE_BUFFER:
    {
        TRACE_ID(buffer, command->u.write_buffer.buffer_id);
        dvz_fprintf(
            stderr, "  %03u ~ WriteBuffer buffer=%s offset=%" PRIu64
                    " size=%" PRIu64 "\n",
            index, buffer, command->u.write_buffer.offset,
            command->u.write_buffer.size);
        return true;
    }
    case DVZ_DRP2_COMMAND_WRITE_TEXTURE:
    {
        TRACE_ID(texture, command->u.write_texture.texture_id);
        dvz_fprintf(
            stderr, "  %03u ~ WriteTexture texture=%s origin=(%" PRIu32
                    ",%" PRIu32 ",%" PRIu32 ") size=(%" PRIu32 ",%" PRIu32 ",%" PRIu32
                    ") bytes_per_row=%" PRIu32 "\n",
            index, texture, command->u.write_texture.origin_x,
            command->u.write_texture.origin_y, command->u.write_texture.origin_z,
            command->u.write_texture.width, command->u.write_texture.height,
            command->u.write_texture.depth, command->u.write_texture.bytes_per_row);
        return true;
    }
    case DVZ_DRP2_COMMAND_BEGIN_COMMAND_ENCODER:
        dvz_fprintf(
            stderr, "  %03u = BeginCommandEncoder id=%" PRIu64 "\n", index,
            command->u.begin_command_encoder.id);
        return true;
    case DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS:
    {
        TRACE_ID(id, command->u.begin_render_pass.id);
        TRACE_ID(encoder, command->u.begin_render_pass.encoder_id);
        TRACE_ID(target, command->u.begin_render_pass.texture_id);
        TRACE_ID(depth, command->u.begin_render_pass.depth_texture_id);
        dvz_fprintf(
            stderr, "  %03u = BeginRenderPass id=%s encoder=%s"
                    " target=%s clear=%s depth=%s depth_target=%s clear_depth=%.3g"
                    " viewport=(%.3g,%.3g %.3gx%.3g)\n",
            index, id, encoder, target,
            command->u.begin_render_pass.clear ? "yes" : "load",
            command->u.begin_render_pass.has_depth_attachment ? "yes" : "no",
            depth,
            (double)command->u.begin_render_pass.clear_depth,
            (double)command->u.begin_render_pass.viewport[0],
            (double)command->u.begin_render_pass.viewport[1],
            (double)command->u.begin_render_pass.viewport[2],
            (double)command->u.begin_render_pass.viewport[3]);
        return true;
    }
    case DVZ_DRP2_COMMAND_BEGIN_COMPUTE_PASS:
        dvz_fprintf(
            stderr, "  %03u = BeginComputePass id=%" PRIu64 " encoder=%" PRIu64 "\n",
            index, command->u.begin_compute_pass.id, command->u.begin_compute_pass.encoder_id);
        return true;
    case DVZ_DRP2_COMMAND_SET_VIEWPORT:
        dvz_fprintf(
            stderr, "  %03u = SetViewport pass=%" PRIu64
                    " viewport=(%.3g,%.3g %.3gx%.3g)\n",
            index, command->u.set_viewport.pass_id,
            (double)command->u.set_viewport.viewport[0],
            (double)command->u.set_viewport.viewport[1],
            (double)command->u.set_viewport.viewport[2],
            (double)command->u.set_viewport.viewport[3]);
        return true;
    case DVZ_DRP2_COMMAND_SET_SCISSOR:
        dvz_fprintf(
            stderr, "  %03u = SetScissor pass=%" PRIu64
                    " scissor=(%.3g,%.3g %.3gx%.3g)\n",
            index, command->u.set_scissor.pass_id,
            (double)command->u.set_scissor.scissor[0],
            (double)command->u.set_scissor.scissor[1],
            (double)command->u.set_scissor.scissor[2],
            (double)command->u.set_scissor.scissor[3]);
        return true;
    case DVZ_DRP2_COMMAND_COPY_BUFFER_TO_BUFFER:
        if (include_transient_ids)
            dvz_fprintf(
                stderr, "  %03u ~ CopyBufferToBuffer encoder=%" PRIu64
                        " src=%" PRIu64 ":%" PRIu64 " dst=%" PRIu64 ":%" PRIu64
                        " size=%" PRIu64 "\n",
                index, command->u.copy_buffer_to_buffer.encoder_id,
                command->u.copy_buffer_to_buffer.src_buffer_id,
                command->u.copy_buffer_to_buffer.src_offset,
                command->u.copy_buffer_to_buffer.dst_buffer_id,
                command->u.copy_buffer_to_buffer.dst_offset,
                command->u.copy_buffer_to_buffer.size);
        else
            dvz_fprintf(
                stderr, "  %03u ~ CopyBufferToBuffer src=%" PRIu64 ":%" PRIu64
                        " dst=%" PRIu64 ":%" PRIu64 " size=%" PRIu64 "\n",
                index, command->u.copy_buffer_to_buffer.src_buffer_id,
                command->u.copy_buffer_to_buffer.src_offset,
                command->u.copy_buffer_to_buffer.dst_buffer_id,
                command->u.copy_buffer_to_buffer.dst_offset,
                command->u.copy_buffer_to_buffer.size);
        return true;
    case DVZ_DRP2_COMMAND_COPY_BUFFER_TO_TEXTURE:
        dvz_fprintf(
            stderr, "  %03u ~ CopyBufferToTexture encoder=%" PRIu64
                    " buffer=%" PRIu64 ":%" PRIu64 " texture=%" PRIu64
                    " origin=(%" PRIu32 ",%" PRIu32 ",%" PRIu32 ") size=(%" PRIu32
                    ",%" PRIu32 ",%" PRIu32 ") bytes_per_row=%" PRIu32 "\n",
            index, command->u.copy_buffer_to_texture.encoder_id,
            command->u.copy_buffer_to_texture.src_buffer_id,
            command->u.copy_buffer_to_texture.src_offset,
            command->u.copy_buffer_to_texture.dst_texture_id,
            command->u.copy_buffer_to_texture.dst_origin_x,
            command->u.copy_buffer_to_texture.dst_origin_y,
            command->u.copy_buffer_to_texture.dst_origin_z,
            command->u.copy_buffer_to_texture.width,
            command->u.copy_buffer_to_texture.height,
            command->u.copy_buffer_to_texture.depth,
            command->u.copy_buffer_to_texture.bytes_per_row);
        return true;
    case DVZ_DRP2_COMMAND_COPY_TEXTURE_TO_BUFFER:
        if (include_transient_ids)
            dvz_fprintf(
                stderr, "  %03u ~ CopyTextureToBuffer encoder=%" PRIu64
                        " texture=%" PRIu64 " buffer=%" PRIu64 ":%" PRIu64
                        " size=(%" PRIu32 ",%" PRIu32 ") bytes_per_row=%" PRIu32 "\n",
                index, command->u.copy_texture_to_buffer.encoder_id,
                command->u.copy_texture_to_buffer.src_texture_id,
                command->u.copy_texture_to_buffer.dst_buffer_id,
                command->u.copy_texture_to_buffer.dst_offset,
                command->u.copy_texture_to_buffer.width,
                command->u.copy_texture_to_buffer.height,
                command->u.copy_texture_to_buffer.bytes_per_row);
        else
            dvz_fprintf(
                stderr, "  %03u ~ CopyTextureToBuffer texture=%" PRIu64
                        " buffer=%" PRIu64 ":%" PRIu64 " size=(%" PRIu32 ",%" PRIu32
                        ") bytes_per_row=%" PRIu32 "\n",
                index, command->u.copy_texture_to_buffer.src_texture_id,
                command->u.copy_texture_to_buffer.dst_buffer_id,
                command->u.copy_texture_to_buffer.dst_offset,
                command->u.copy_texture_to_buffer.width,
                command->u.copy_texture_to_buffer.height,
                command->u.copy_texture_to_buffer.bytes_per_row);
        return true;
    case DVZ_DRP2_COMMAND_COPY_TEXTURE_TO_TEXTURE:
        dvz_fprintf(
            stderr, "  %03u ~ CopyTextureToTexture encoder=%" PRIu64
                    " src=%" PRIu64 " dst=%" PRIu64
                    " origin=(%" PRIu32 ",%" PRIu32 ",%" PRIu32 ") size=(%" PRIu32
                    ",%" PRIu32 ",%" PRIu32 ")\n",
            index, command->u.copy_texture_to_texture.encoder_id,
            command->u.copy_texture_to_texture.src_texture_id,
            command->u.copy_texture_to_texture.dst_texture_id,
            command->u.copy_texture_to_texture.dst_origin_x,
            command->u.copy_texture_to_texture.dst_origin_y,
            command->u.copy_texture_to_texture.dst_origin_z,
            command->u.copy_texture_to_texture.width,
            command->u.copy_texture_to_texture.height,
            command->u.copy_texture_to_texture.depth);
        return true;
    case DVZ_DRP2_COMMAND_SET_PIPELINE:
    {
        TRACE_ID(pipeline, command->u.set_pipeline.pipeline_id);
        if (include_transient_ids)
        {
            TRACE_ID(pass, command->u.set_pipeline.pass_id);
            dvz_fprintf(
                stderr, "  %03u = SetPipeline pass=%s pipeline=%s\n", index, pass, pipeline);
        }
        else
            dvz_fprintf(
                stderr, "  %03u = SetPipeline pipeline=%s\n", index, pipeline);
        return true;
    }
    case DVZ_DRP2_COMMAND_SET_BIND_GROUP:
    {
        TRACE_ID(bind_group, command->u.set_bind_group.bind_group_id);
        if (include_transient_ids)
        {
            TRACE_ID(pass, command->u.set_bind_group.pass_id);
            dvz_fprintf(
                stderr, "  %03u = SetBindGroup pass=%s slot=%" PRIu32
                        " bind_group=%s offsets=%" PRIu32 "\n",
                index, pass, command->u.set_bind_group.slot, bind_group,
                command->u.set_bind_group.dynamic_offset_count);
        }
        else
            dvz_fprintf(
                stderr, "  %03u = SetBindGroup slot=%" PRIu32 " bind_group=%s offsets=%" PRIu32
                        "\n",
                index, command->u.set_bind_group.slot, bind_group,
                command->u.set_bind_group.dynamic_offset_count);
        return true;
    }
    case DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER:
    {
        TRACE_ID(buffer, command->u.set_vertex_buffer.buffer_id);
        if (include_transient_ids)
        {
            TRACE_ID(pass, command->u.set_vertex_buffer.pass_id);
            dvz_fprintf(
                stderr, "  %03u = SetVertexBuffer pass=%s slot=%" PRIu32
                        " buffer=%s offset=%" PRIu64 "\n",
                index, pass, command->u.set_vertex_buffer.slot, buffer,
                command->u.set_vertex_buffer.offset);
        }
        else
            dvz_fprintf(
                stderr, "  %03u = SetVertexBuffer slot=%" PRIu32
                        " buffer=%s offset=%" PRIu64 "\n",
                index, command->u.set_vertex_buffer.slot, buffer,
                command->u.set_vertex_buffer.offset);
        return true;
    }
    case DVZ_DRP2_COMMAND_SET_INDEX_BUFFER:
    {
        TRACE_ID(buffer, command->u.set_index_buffer.buffer_id);
        if (include_transient_ids)
        {
            TRACE_ID(pass, command->u.set_index_buffer.pass_id);
            dvz_fprintf(
                stderr, "  %03u = SetIndexBuffer pass=%s"
                        " buffer=%s format=%s offset=%" PRIu64 "\n",
                index, pass, buffer, command->u.set_index_buffer.index_format,
                command->u.set_index_buffer.offset);
        }
        else
            dvz_fprintf(
                stderr, "  %03u = SetIndexBuffer buffer=%s"
                        " format=%s offset=%" PRIu64 "\n",
                index, buffer, command->u.set_index_buffer.index_format,
                command->u.set_index_buffer.offset);
        return true;
    }
    case DVZ_DRP2_COMMAND_DRAW:
        if (include_transient_ids)
            dvz_fprintf(
                stderr, "  %03u = Draw pass=%" PRIu64 " vertices=%" PRIu32
                        " first=%" PRIu32 " instances=%" PRIu32 "\n",
                index, command->u.draw.pass_id, command->u.draw.vertex_count,
                command->u.draw.first_vertex, command->u.draw.instance_count);
        else
            dvz_fprintf(
                stderr, "  %03u = Draw vertices=%" PRIu32
                        " first=%" PRIu32 " instances=%" PRIu32 "\n",
                index, command->u.draw.vertex_count, command->u.draw.first_vertex,
                command->u.draw.instance_count);
        return true;
    case DVZ_DRP2_COMMAND_DRAW_INDEXED:
        if (include_transient_ids)
            dvz_fprintf(
                stderr, "  %03u = DrawIndexed pass=%" PRIu64 " indices=%" PRIu32
                        " first=%" PRIu32 " base=%" PRId32 "\n",
                index, command->u.draw_indexed.pass_id, command->u.draw_indexed.index_count,
                command->u.draw_indexed.first_index, command->u.draw_indexed.base_vertex);
        else
            dvz_fprintf(
                stderr, "  %03u = DrawIndexed indices=%" PRIu32
                        " first=%" PRIu32 " base=%" PRId32 "\n",
                index, command->u.draw_indexed.index_count,
                command->u.draw_indexed.first_index, command->u.draw_indexed.base_vertex);
        return true;
    case DVZ_DRP2_COMMAND_END_RENDER_PASS:
        dvz_fprintf(
            stderr, "  %03u = EndRenderPass pass=%" PRIu64 "\n", index,
            command->u.end_render_pass.pass_id);
        return true;
    case DVZ_DRP2_COMMAND_DISPATCH_WORKGROUPS:
        dvz_fprintf(
            stderr, "  %03u = DispatchWorkgroups pass=%" PRIu64 " groups=(%" PRIu32
                    ",%" PRIu32 ",%" PRIu32 ")\n",
            index, command->u.dispatch.pass_id, command->u.dispatch.x,
            command->u.dispatch.y, command->u.dispatch.z);
        return true;
    case DVZ_DRP2_COMMAND_END_COMPUTE_PASS:
        dvz_fprintf(
            stderr, "  %03u = EndComputePass pass=%" PRIu64 "\n", index,
            command->u.end_compute_pass.pass_id);
        return true;
    case DVZ_DRP2_COMMAND_FINISH_COMMAND_ENCODER:
        dvz_fprintf(
            stderr, "  %03u = FinishCommandEncoder encoder=%" PRIu64
                    " command_buffer=%" PRIu64 "\n",
            index, command->u.finish_command_encoder.encoder_id,
            command->u.finish_command_encoder.command_buffer_id);
        return true;
    case DVZ_DRP2_COMMAND_QUEUE_SUBMIT:
        dvz_fprintf(
            stderr, "  %03u = QueueSubmit command_buffer=%" PRIu64
                    " submission=%" PRIu64 " readback=%s buffer=%" PRIu64
                    " offset=%" PRIu64 " size=%" PRIu64 " data=%s\n",
            index, command->u.queue_submit.command_buffer_id,
            command->u.queue_submit.submission_id,
            command->u.queue_submit.has_readback ? "yes" : "no",
            command->u.queue_submit.buffer_id, command->u.queue_submit.offset,
            command->u.queue_submit.size,
            command->u.queue_submit.data_base64[0] != '\0' ? "yes" : "no");
        return true;
    case DVZ_DRP2_COMMAND_QUEUE_SUBMIT_REPLY:
        dvz_fprintf(
            stderr, "  %03u = QueueSubmitReply submission=%" PRIu64
                    " readback=%s buffer=%" PRIu64 " offset=%" PRIu64
                    " size=%" PRIu64 " data=%s\n",
            index, command->u.queue_submit.submission_id,
            command->u.queue_submit.has_readback ? "yes" : "no",
            command->u.queue_submit.buffer_id, command->u.queue_submit.offset,
            command->u.queue_submit.size,
            command->u.queue_submit.data_base64[0] != '\0' ? "yes" : "no");
        return true;
    case DVZ_DRP2_COMMAND_NONE:
        dvz_fprintf(stderr, "  %03u = None\n", index);
        return true;
    default:
        break;
    }
#undef TRACE_ID
    return false;
}


/**
 * Print an expanded human-readable dump for one DRP2 stream.
 *
 * @param stream emitted command stream
 * @param frame_index 0-based frame index for the owning window
 * @param label short trace label for the frame header
 */
static void _app_trace_stream_full(
    const DvzDrp2CommandStream* stream, uint64_t frame_index, const char* label)
{
    ANN(stream);
    ANN(label);
    uint32_t command_count = dvz_drp2_stream_count(stream);
    bool use_color = _trace_color_enabled();
    dvz_fprintf(
        stderr, "\n%sframe %08" PRIu64 " | %s | %u cmds%s\n",
        use_color ? DVZ_TRACE_COLOR_BOLD : "", frame_index, label, command_count,
        use_color ? DVZ_TRACE_COLOR_RESET : "");
    for (uint32_t i = 0; i < command_count; i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        if (command == NULL)
            continue;
        DvzDrp2CommandType type = dvz_drp2_command_type(command);
        if (use_color)
            dvz_fprintf(stderr, "%s", _trace_command_color(type));
        if (!_app_trace_print_command_detail(stream, command, i, true))
        {
            dvz_fprintf(
                stderr, "  %03u %c %s\n", i, _trace_command_prefix(type),
                _trace_command_name(type));
        }
    }
    if (use_color)
        dvz_fprintf(stderr, "%s", DVZ_TRACE_COLOR_RESET);
}


/**
 * Print or refresh the live DRP2 trace for one emitted stream.
 *
 * In normal mode, changed frames print one expanded human-readable command list while unchanged
 * frames rewrite one in-place status line without scrolling. In full mode, every frame prints an
 * expanded command list.
 *
 * @param win app-window owning the trace state
 * @param stream the emitted command stream
 */
static void _app_trace_stream(DvzAppWindow* win, const DvzDrp2CommandStream* stream)
{
    ANN(win);
    ANN(stream);
    const char* trace_env = getenv("DVZ_DRP2_TRACE");
    DvzAppTraceMode mode = _dvz_app_trace_mode_from_env(trace_env);
    if (mode == DVZ_APP_TRACE_NONE)
        return;

    uint32_t command_count = dvz_drp2_stream_count(stream);

    DvzAppTraceSnapshot snapshot;
    _dvz_app_trace_snapshot_init(&snapshot);
    if (!_dvz_app_trace_snapshot_build(&snapshot, stream))
    {
        log_error("failed to build normalized DRP2 trace snapshot");
        return;
    }

    bool changed = !win->has_last_trace_snapshot ||
                   !_dvz_app_trace_snapshot_equal(&win->last_trace_snapshot, &snapshot);
    DvzAppTracePlan plan =
        _dvz_app_trace_plan(mode, win->app->status.line_open, changed);

    if (plan.prepend_newline)
        _dvz_app_status_break_line(&win->app->status);

    if (mode == DVZ_APP_TRACE_FULL)
    {
        _app_trace_stream_full(stream, win->frame_index, "full");
        _dvz_app_status_trace(
            &win->app->status, win->frame_index, command_count, snapshot.count, true);
        _dvz_app_status_render(&win->app->status);
    }
    else
    {
        if (plan.event_kind == DVZ_APP_TRACE_EVENT_CHANGED)
        {
            _app_trace_stream_full(stream, win->frame_index, "changed");
        }
        _dvz_app_status_trace(
            &win->app->status, win->frame_index, command_count, snapshot.count, changed);
        _dvz_app_status_render(&win->app->status);
    }
    _dvz_app_trace_snapshot_destroy(&win->last_trace_snapshot);
    win->last_trace_snapshot = snapshot;
    win->has_last_trace_snapshot = true;
}

#endif



/*************************************************************************************************/
/*  Draw callback                                                                                */
/*************************************************************************************************/

#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE

/**
 * Synchronize the figure size with the current output before emitting a frame.
 *
 * @param win app window being drawn
 * @param frame canvas frame attached to the DRP2 runtime
 */
static void _app_sync_figure_size(DvzAppWindow* win, const DvzStreamFrame* frame)
{
    ANN(win);
    ANN(win->figure);
    ANN(frame);

    uint32_t width = frame->extent.width;
    uint32_t height = frame->extent.height;

    if (win->is_interactive && win->canvas != NULL)
    {
        DvzInputRouter* router = dvz_canvas_input(win->canvas);
        DvzInputResizeEvent resize = {0};
        if (router != NULL && dvz_input_router_last_resize(router, &resize) &&
            resize.window_width > 0 && resize.window_height > 0)
        {
            width = resize.window_width;
            height = resize.window_height;
        }
    }

    if (width > 0 && height > 0)
        dvz_figure_resize(win->figure, width, height);
}




/**
 * Report one failed DRP2 runtime execution.
 *
 * @param prefix short failure context
 * @param stream the emitted command stream
 * @param result the failed validation result
 */
static void _app_log_runtime_failure(
    const char* prefix, const DvzDrp2CommandStream* stream, DvzDrp2ValidationResult result)
{
    ANN(prefix);
    ANN(stream);

    DvzDrp2CommandType type = DVZ_DRP2_COMMAND_NONE;
    const DvzDrp2Command* failed = dvz_drp2_stream_get(stream, result.command_index);
    if (failed != NULL)
        type = dvz_drp2_command_type(failed);
    uint64_t id = 0;
    if (failed != NULL && type == DVZ_DRP2_COMMAND_BEGIN_COMMAND_ENCODER)
        id = failed->u.begin_command_encoder.id;
    log_error(
        "%s: code=%d command=%" PRIu32 " type=%d (%s) id=%" PRIu64, prefix,
        (int)result.code, result.command_index, (int)type, _trace_command_name(type), id);
}



/**
 * Return whether a frame callback may safely run mutation-oriented user code.
 *
 * @param win app window owning the callback
 * @return true when no emitted scene stream is still live
 */
static bool _app_frame_callback_allowed(DvzAppWindow* win)
{
    ANN(win);
    if (win->app == NULL || win->app->scene == NULL)
        return true;
    return _scene_visual_mutation_allowed(win->app->scene, "run app frame callback");
}



/**
 * Append one emitted app stream to the active DVZR recorder.
 *
 * @param win app window owning the recorder
 * @param frame borrowed stream frame for target dimensions
 * @param stream emitted scene stream
 */
static void _app_record_stream(
    DvzAppWindow* win, const DvzStreamFrame* frame, const DvzDrp2CommandStream* stream)
{
    ANN(win);
    ANN(frame);
    ANN(stream);
    if (win->recorder == NULL)
        return;

    double t_present = dvz_clock_get(&win->recording_clock);
    if (!win->recording_target_created)
    {
        DvzDrp2CommandStream* setup = dvz_drp2_stream();
        if (setup == NULL ||
            !dvz_drp2_stream_hello_renderer(setup, "app-recording") ||
            !dvz_drp2_stream_renderer_hello_reply(setup, "datoviz-drp2-runtime") ||
            !dvz_drp2_stream_create_texture_2d_format_usage(
                setup, win->target_id, frame->extent.width, frame->extent.height,
                VK_FORMAT_R8G8B8A8_UNORM,
                DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC) ||
            !dvz_drp2_recorder_write_stream(win->recorder, t_present, setup))
        {
            log_error("_app_draw failed to append DVZR target setup stream");
            dvz_drp2_stream_destroy(setup);
            return;
        }
        dvz_drp2_stream_destroy(setup);
        win->recording_target_created = true;
    }

    const DvzDrp2CommandStream* recorded = stream;
    DvzDrp2CommandStream filtered = {0};
    if (dvz_drp2_stream_count(stream) >= 2 &&
        dvz_drp2_command_type(dvz_drp2_stream_get(stream, 0)) ==
            DVZ_DRP2_COMMAND_HELLO_RENDERER &&
        dvz_drp2_command_type(dvz_drp2_stream_get(stream, 1)) ==
            DVZ_DRP2_COMMAND_RENDERER_HELLO_REPLY)
    {
        filtered.count = dvz_drp2_stream_count(stream) - 2;
        filtered.commands = &stream->commands[2];
        recorded = &filtered;
    }

    if (!dvz_drp2_recorder_write_stream(win->recorder, t_present, recorded))
        log_error("_app_draw failed to append DRP2 stream to DVZR recording");
}



/**
 * Return whether a replay command creates or destroys the synthetic recorded frame target.
 *
 * @param command replay command
 * @param target_id recorded app target id
 * @return whether the command should be skipped for live replay
 */
static bool _app_replay_command_is_synthetic_target(
    const DvzDrp2Command* command, uint64_t target_id)
{
    ANN(command);
    if (target_id == 0)
        return false;

    if (command->type == DVZ_DRP2_COMMAND_CREATE_TEXTURE &&
        command->u.create_texture.id == target_id)
    {
        return true;
    }
    if (command->type == DVZ_DRP2_COMMAND_DESTROY_TEXTURE &&
        command->u.destroy_texture.texture_id == target_id)
    {
        return true;
    }
    return false;
}



/**
 * Remove synthetic target setup commands from a mutable per-frame replay stream.
 *
 * @param stream mutable frame command stream
 * @param target_id recorded app target id
 */
static void _app_replay_filter_synthetic_target(
    DvzDrp2CommandStream* stream, uint64_t target_id)
{
    if (stream == NULL || stream->commands == NULL || target_id == 0)
        return;

    uint32_t write = 0;
    for (uint32_t read = 0; read < stream->count; read++)
    {
        if (_app_replay_command_is_synthetic_target(&stream->commands[read], target_id))
            continue;
        if (write != read)
        {
            dvz_memmove(
                &stream->commands[write], sizeof(DvzDrp2Command),
                &stream->commands[read], sizeof(DvzDrp2Command));
        }
        write++;
    }
    stream->count = write;
}



/**
 * Find the app recording target id in a loaded recording.
 *
 * @param recording loaded recording
 * @param out_target_id output target id
 * @return whether a renderable target id was found
 */
static bool _app_replay_find_target_id(
    const DvzDrp2Recording* recording, uint64_t* out_target_id)
{
    ANN(out_target_id);
    *out_target_id = 0;
    const DvzDrp2CommandStream* stream = dvz_drp2_recording_stream(recording);
    if (stream == NULL)
        return false;

    uint32_t count = dvz_drp2_stream_count(stream);
    for (uint32_t i = 0; i < count; i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        if (command == NULL || command->type != DVZ_DRP2_COMMAND_CREATE_TEXTURE)
            continue;
        if ((command->u.create_texture.usage & DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT) == 0)
            continue;
        *out_target_id = command->u.create_texture.id;
        return *out_target_id != 0;
    }
    return false;
}



/**
 * Reset live replay timing and runtime state.
 *
 * @param win replay app-window
 */
static void _app_replay_restart(DvzAppWindow* win)
{
    ANN(win);
    if (win->app != NULL && win->app->runtime != NULL)
        dvz_drp2_runtime_reset(win->app->runtime);
    win->replay_frame_index = 0;
    win->replay_clock = dvz_clock();
    win->replay_clock_started = true;
}



/**
 * Sleep until a recorded frame timestamp should be presented.
 *
 * @param win replay app-window
 * @param frame recorded frame metadata
 */
static void _app_replay_pace(DvzAppWindow* win, const DvzDrp2RecordedFrame* frame)
{
    ANN(win);
    if (!win->replay_paced || frame == NULL || frame->t_present <= 0)
        return;
    if (!win->replay_clock_started)
    {
        win->replay_clock = dvz_clock();
        win->replay_clock_started = true;
    }

    double speed = win->replay_speed > 0 ? win->replay_speed : 1.0;
    double target = frame->t_present / speed;
    double now = dvz_clock_get(&win->replay_clock);
    double delay = target - now;
    if (delay <= 0)
        return;

    double delay_us = delay * 1000000.0;
    int sleep_us = delay_us > (double)INT32_MAX ? INT32_MAX : (int)delay_us;
    dvz_sleep_us(sleep_us);
}



/**
 * Replay one recorded frame into the current live canvas frame.
 *
 * @param win replay app-window
 * @param frame borrowed canvas frame
 */
static void _app_draw_replay(DvzAppWindow* win, const DvzStreamFrame* frame)
{
    ANN(win);
    ANN(frame);
    if (win->app == NULL || win->app->runtime == NULL || win->replay_recording == NULL)
        return;

    uint32_t frame_count = dvz_drp2_recording_frame_count(win->replay_recording);
    if (frame_count == 0)
        return;

    if (win->replay_frame_index >= frame_count)
    {
        if (!win->replay_loop)
        {
            win->render_enabled = false;
            return;
        }
        _app_replay_restart(win);
    }

    const DvzDrp2RecordedFrame* recorded =
        dvz_drp2_recording_frame(win->replay_recording, win->replay_frame_index);
    _app_replay_pace(win, recorded);

    if (!dvz_drp2_runtime_attach_frame_target(
            win->app->runtime, win->replay_target_id, frame))
    {
        log_error("_app_draw_replay failed to attach canvas frame target");
        win->render_enabled = false;
        return;
    }

    DvzDrp2CommandStream* stream =
        dvz_drp2_recording_frame_stream(win->replay_recording, win->replay_frame_index);
    if (stream == NULL)
    {
        log_error("_app_draw_replay failed to load frame stream");
        win->render_enabled = false;
        return;
    }
    _app_replay_filter_synthetic_target(stream, win->replay_target_id);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(win->app->runtime, stream);
    if (!result.ok)
    {
        _app_log_runtime_failure("_app_draw_replay runtime execution failed", stream, result);
        win->render_enabled = false;
    }
    else
    {
        win->replay_frame_index++;
        win->frame_index++;
    }
    dvz_drp2_stream_destroy(stream);
}



/**
 * Apply the DVZ_PRESENT_MODE environment override to a present canvas configuration.
 *
 * @param ccfg canvas configuration to mutate
 */
static void _app_canvas_config_apply_present_mode_env(DvzCanvasConfig* ccfg)
{
    ANN(ccfg);
    /* DVZ_PRESENT_MODE: fifo (default, vsync), mailbox (vsync+latest), immediate (no vsync). */
    const char* pm_env = getenv("DVZ_PRESENT_MODE");
    if (pm_env != NULL)
    {
        if (strcmp(pm_env, "immediate") == 0)
            ccfg->present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        else if (strcmp(pm_env, "mailbox") == 0)
            ccfg->present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
        else if (strcmp(pm_env, "fifo") == 0)
            ccfg->present_mode = VK_PRESENT_MODE_FIFO_KHR;
        else
            log_warn("ignoring DVZ_PRESENT_MODE='%s' (expected fifo|mailbox|immediate)", pm_env);
    }
}



static void _app_draw(DvzCanvas* canvas, const DvzStreamFrame* frame, void* user_data)
{
    (void)canvas;
    ANN(frame);
    DvzAppWindow* win = (DvzAppWindow*)user_data;
    ANN(win);
    DvzApp* app = win->app;
    ANN(app);

    if (win->replay_recording != NULL)
    {
        _app_draw_replay(win, frame);
        return;
    }

#if defined(DVZ_HAS_GUI) && DVZ_HAS_GUI
    if (win->gui != NULL)
        _dvz_gui_begin_frame(win->gui, win, frame);
#endif

    _dvz_scene_animations_step(app->scene, dvz_input_timestamp_ns());
    _app_sync_figure_size(win, frame);

    /* Attach the canvas frame to the reserved DRP2 texture ID. */
    if (!dvz_drp2_runtime_attach_frame_target(app->runtime, win->target_id, frame))
    {
        log_error("_app_draw failed to attach canvas frame target");
        return;
    }

    /* Emit the DRP2 command stream with the canvas as external color target. */
    DvzCapabilitySnapshot caps;
    dvz_capability_snapshot_default(&caps);
    caps.shader_format_glsl = true;
    caps.max_color_attachments = 2;
    caps.render_target_format_rgba16float = true;
    caps.render_target_format_r16float = true;
    caps.supports_render_target_sampling = true;
    caps.supports_color_blending = true;

    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format         = DVZ_SCENE_SHADER_FORMAT_GLSL;
    cfg.external_color_target = true;
    cfg.color_target_id       = win->target_id;
    cfg.target_width          = frame->extent.width;
    cfg.target_height         = frame->extent.height;
    cfg.clear_color[0]        = 0.05f;
    cfg.clear_color[1]        = 0.05f;
    cfg.clear_color[2]        = 0.08f;
    cfg.clear_color[3]        = 1.0f;

    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzDrp2CommandStream* stream = dvz_figure_emit_ex(win->figure, &caps, &report, &cfg);
    if (stream == NULL)
    {
        uint32_t n = dvz_diagnostic_report_count(&report);
        for (uint32_t i = 0; i < n; i++)
            log_error("_app_draw emit failed: %s", dvz_diagnostic_report_get(&report, i));
        return;
    }

    _app_trace_stream(win, stream);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(app->runtime, stream);
    if (!result.ok)
        _app_log_runtime_failure("_app_draw runtime execution failed", stream, result);
    else
        (void)_dvz_figure_process_requests_with_executor(
            win->figure, app->runtime, &app->request_executor, &caps);

    if (result.ok)
        _app_record_stream(win, frame, stream);
    dvz_drp2_stream_destroy(stream);

#if defined(DVZ_HAS_GUI) && DVZ_HAS_GUI
    if (win->gui != NULL)
        _dvz_gui_render_frame(win->gui, frame);
#endif

    if (win->frame_callback != NULL && _app_frame_callback_allowed(win))
        win->frame_callback(win, win->frame_user_data);
    win->frame_index++;
}

#endif



/*************************************************************************************************/
/*  App lifecycle                                                                                */
/*************************************************************************************************/

DvzAppConfig dvz_app_config(void)
{
    DvzAppConfig config = {
        .instance_extension_count = 0,
        .instance_extensions = NULL,
        .enable_canvas_extensions = false,
        .enable_glfw_extensions = true,
    };
    return config;
}



DvzApp* dvz_app(DvzScene* scene)
{
    DvzAppConfig config = dvz_app_config();
    return dvz_app_with_config(scene, &config);
}



DvzApp* dvz_app_with_config(DvzScene* scene, const DvzAppConfig* config)
{
    ANN(scene);

#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    DvzAppConfig resolved = config != NULL ? *config : dvz_app_config();

    DvzApp* app = (DvzApp*)dvz_calloc(1, sizeof(DvzApp));
    if (app == NULL)
        return NULL;
    app->scene = scene;
    _dvz_app_status_init(&app->status);

    /* Window host first — needed to query GLFW surface extensions before building the instance. */
    app->window_host = dvz_window_host();
    if (app->window_host == NULL)
    {
        dvz_free(app);
        return NULL;
    }

    /* GPU context — request independent blending, dynamic rendering, synchronization2, and
     * timeline semaphores. */
    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceFeatures features10 = {0};
    features10.independentBlend = true;
    dvz_gpu_ctx_config_features10(&gpu_cfg, &features10);
    VkPhysicalDeviceVulkan12Features features12 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    features12.timelineSemaphore = true;
    dvz_gpu_ctx_config_features12(&gpu_cfg, &features12);
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);

    if (!_app_gpu_config_add_instance_extensions(
            &gpu_cfg, resolved.instance_extension_count, resolved.instance_extensions))
    {
        dvz_window_host_destroy(app->window_host);
        dvz_free(app);
        return NULL;
    }
    if (resolved.enable_canvas_extensions)
        dvz_gpu_ctx_config_enable_canvas_extensions(&gpu_cfg, true);
    _app_gpu_config_add_glfw_extensions(app, &gpu_cfg, &resolved);

    app->gpu_ctx = dvz_gpu_ctx(&gpu_cfg);
    if (app->gpu_ctx == NULL)
    {
        dvz_window_host_destroy(app->window_host);
        dvz_free(app);
        return NULL;
    }

    /* DRP2 runtime backed by vklite. */
    DvzDrp2RuntimeConfig runtime_cfg = dvz_drp2_runtime_vklite_config(
        dvz_gpu_ctx_device(app->gpu_ctx), dvz_gpu_ctx_alloc(app->gpu_ctx));
    app->runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    if (app->runtime == NULL)
    {
        dvz_gpu_ctx_destroy(app->gpu_ctx);
        dvz_window_host_destroy(app->window_host);
        dvz_free(app);
        return NULL;
    }
    _scene_request_executor_init(&app->request_executor);

    return app;
#else
    (void)scene;
    return NULL;
#endif
}



/**
 * Return the borrowed Vulkan instance handle owned by the app.
 *
 * @param app app to query
 * @return Vulkan instance handle, or VK_NULL_HANDLE when unavailable
 */
VkInstance dvz_app_vk_instance(DvzApp* app)
{
    ANN(app);
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    if (app->gpu_ctx == NULL)
        return VK_NULL_HANDLE;
    DvzInstance* instance = dvz_gpu_ctx_instance(app->gpu_ctx);
    if (instance == NULL)
        return VK_NULL_HANDLE;
    return dvz_instance_handle(instance);
#else
    return VK_NULL_HANDLE;
#endif
}


void dvz_app_destroy(DvzApp* app)
{
    if (app == NULL)
        return;

#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    _dvz_app_status_finish(&app->status);
    for (uint32_t i = 0; i < app->window_count; i++)
    {
        DvzAppWindow* win = &app->windows[i];
#if defined(DVZ_HAS_GUI) && DVZ_HAS_GUI
        if (win->gui != NULL)
        {
            _dvz_gui_destroy(win->gui);
            win->gui = NULL;
        }
#endif
        if (win->canvas != NULL)
        {
            dvz_canvas_destroy(win->canvas);
            win->canvas = NULL;
        }
        if (win->recorder != NULL)
        {
            (void)dvz_drp2_recorder_close(win->recorder);
            win->recorder = NULL;
        }
        if (win->replay_recording != NULL)
        {
            dvz_drp2_recording_close(win->replay_recording);
            win->replay_recording = NULL;
        }
        if (win->window != NULL)
        {
            dvz_window_destroy(win->window);
            win->window = NULL;
        }
        _dvz_app_trace_snapshot_destroy(&win->last_trace_snapshot);
        win->has_last_trace_snapshot = false;
    }
    if (app->window_host != NULL)
    {
        dvz_window_host_destroy(app->window_host);
        app->window_host = NULL;
    }
    _scene_request_executor_destroy(&app->request_executor);
    if (app->runtime != NULL)
    {
        dvz_drp2_runtime_destroy(app->runtime);
        app->runtime = NULL;
    }
    if (app->gpu_ctx != NULL)
    {
        dvz_gpu_ctx_destroy(app->gpu_ctx);
        app->gpu_ctx = NULL;
    }
#endif

    dvz_free(app);
}



/*************************************************************************************************/
/*  Window management                                                                            */
/*************************************************************************************************/

DvzAppWindow*
dvz_app_window(DvzApp* app, DvzFigure* figure, uint32_t width, uint32_t height)
{
    ANN(app);
    ANN(figure);

#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    if (app->window_count >= DVZ_APP_MAX_WINDOWS)
        return NULL;

    /* Create the offscreen window. */
    DvzWindowConfig wcfg = dvz_window_default_config();
    wcfg.width           = width;
    wcfg.height          = height;
    DvzWindow* window = dvz_window_create(app->window_host, DVZ_BACKEND_OFFSCREEN, &wcfg);
    if (window == NULL)
        return NULL;

    /* Create an offscreen canvas. */
    DvzCanvasConfig ccfg = dvz_canvas_default_config();
    ccfg.window          = window;
    ccfg.device          = dvz_gpu_ctx_device(app->gpu_ctx);
    ccfg.render_mode     = DVZ_CANVAS_RENDER_MODE_OFFSCREEN;
    DvzCanvas* canvas = dvz_canvas_create(&ccfg);
    if (canvas == NULL)
    {
        dvz_window_destroy(window);
        return NULL;
    }

    DvzAppWindow* win = &app->windows[app->window_count];
    win->app           = app;
    win->figure        = figure;
    win->window        = window;
    win->canvas        = canvas;
    win->target_id     = DVZ_APP_CANVAS_TARGET_BASE + (uint64_t)app->window_count;
    win->render_enabled = true;
    app->window_count++;

    dvz_canvas_set_draw_callback(canvas, _app_draw, win);
    return win;
#else
    (void)width;
    (void)height;
    return NULL;
#endif
}



DvzAppWindow*
dvz_app_window_glfw(DvzApp* app, DvzFigure* figure, uint32_t width, uint32_t height,
                    const char* title)
{
    ANN(app);
    ANN(figure);

#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE && DVZ_HAS_GLFW
    if (app->window_count >= DVZ_APP_MAX_WINDOWS)
        return NULL;

    DvzWindowConfig wcfg = dvz_window_default_config();
    wcfg.width  = width;
    wcfg.height = height;
    if (title != NULL)
        wcfg.title = title;
    DvzWindow* window = dvz_window_create(app->window_host, DVZ_BACKEND_GLFW, &wcfg);
    if (window == NULL || dvz_window_backend_type(window) != DVZ_BACKEND_GLFW)
    {
        if (window != NULL)
            dvz_window_destroy(window);
        return NULL;
    }

    /* Poll once so the initial resize event sets the surface extent. */
    dvz_window_host_poll(app->window_host);

    DvzCanvasConfig ccfg = dvz_canvas_default_config();
    ccfg.window = window;
    ccfg.device = dvz_gpu_ctx_device(app->gpu_ctx);
    /* render_mode defaults to DVZ_CANVAS_RENDER_MODE_PRESENT */
    _app_canvas_config_apply_present_mode_env(&ccfg);
    DvzCanvas* canvas = dvz_canvas_create(&ccfg);
    if (canvas == NULL)
    {
        dvz_window_destroy(window);
        return NULL;
    }

    DvzAppWindow* win = &app->windows[app->window_count];
    win->app            = app;
    win->figure         = figure;
    win->window         = window;
    win->canvas         = canvas;
    win->target_id      = DVZ_APP_CANVAS_TARGET_BASE + (uint64_t)app->window_count;
    win->is_interactive = true;
    win->render_enabled = true;
    app->window_count++;

    dvz_canvas_set_draw_callback(canvas, _app_draw, win);
    return win;
#else
    (void)width;
    (void)height;
    (void)title;
    return NULL;
#endif
}



/**
 * Create an app-window around an externally-owned Vulkan surface.
 *
 * @param app app that owns the rendering runtime
 * @param figure figure rendered into the surface
 * @param surface external surface description
 * @return app-window handle, or NULL on failure
 */
DvzAppWindow* dvz_app_window_external_surface(
    DvzApp* app, DvzFigure* figure, const DvzWindowExternalSurfaceInfo* surface)
{
    ANN(app);
    ANN(figure);
    ANN(surface);

#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    if (app->window_count >= DVZ_APP_MAX_WINDOWS)
        return NULL;

    DvzWindowConfig wcfg = dvz_window_default_config();
    wcfg.width = surface->extent.width;
    wcfg.height = surface->extent.height;
    DvzWindow* window = dvz_window_create(app->window_host, DVZ_BACKEND_WRAP, &wcfg);
    if (window == NULL || dvz_window_backend_type(window) != DVZ_BACKEND_WRAP)
    {
        if (window != NULL)
            dvz_window_destroy(window);
        return NULL;
    }
    if (dvz_window_wrap_attach_surface(window, surface) != 0)
    {
        dvz_window_destroy(window);
        return NULL;
    }

    DvzCanvasConfig ccfg = dvz_canvas_default_config();
    ccfg.window = window;
    ccfg.device = dvz_gpu_ctx_device(app->gpu_ctx);
    _app_canvas_config_apply_present_mode_env(&ccfg);
    DvzCanvas* canvas = dvz_canvas_create(&ccfg);
    if (canvas == NULL)
    {
        dvz_window_destroy(window);
        return NULL;
    }

    DvzAppWindow* win = &app->windows[app->window_count];
    win->app        = app;
    win->figure     = figure;
    win->window     = window;
    win->canvas     = canvas;
    win->target_id  = DVZ_APP_CANVAS_TARGET_BASE + (uint64_t)app->window_count;
    win->render_enabled = true;
    app->window_count++;

    dvz_canvas_set_draw_callback(canvas, _app_draw, win);
    return win;
#else
    return NULL;
#endif
}



/*************************************************************************************************/
/*  Window accessors                                                                             */
/*************************************************************************************************/

/**
 * Return the content scale currently cached for an app-window.
 *
 * @param win app-window to query
 * @return horizontal content scale, or 1 when unavailable
 */
static float _app_window_content_scale(DvzAppWindow* win)
{
    ANN(win);
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    if (win->window == NULL)
        return 1.0f;
    const DvzWindowSurface* surface = dvz_window_surface(win->window);
    if (surface == NULL || surface->scale_x <= 0.0f)
        return 1.0f;
    return surface->scale_x;
#else
    return 1.0f;
#endif
}



/**
 * Update the external Vulkan surface associated with a hosted app-window.
 *
 * @param win hosted app-window
 * @param surface external surface description
 * @return 0 on success, negative on error
 */
int dvz_app_window_update_external_surface(
    DvzAppWindow* win, const DvzWindowExternalSurfaceInfo* surface)
{
    ANN(win);
    ANN(surface);
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    if (win->window == NULL || dvz_window_backend_type(win->window) != DVZ_BACKEND_WRAP)
        return -1;
    return dvz_window_wrap_update_surface(win->window, surface);
#else
    return -1;
#endif
}



/**
 * Release a hosted external Vulkan surface before the host destroys it.
 *
 * @param win hosted app-window
 * @return DVZ_CANVAS_FRAME_WAIT_SURFACE on clean release, or negative on error
 */
int dvz_app_window_release_external_surface(DvzAppWindow* win)
{
    ANN(win);
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    if (win->window == NULL || dvz_window_backend_type(win->window) != DVZ_BACKEND_WRAP)
        return -1;

    dvz_app_window_set_request_frame_callback(win, NULL, NULL);

    DvzWindowExternalSurfaceInfo surface = {0};
    surface.scale_x = 1.0f;
    surface.scale_y = 1.0f;
    if (dvz_app_window_update_external_surface(win, &surface) != 0)
        return -1;
    return dvz_app_window_render_once(win);
#else
    return -1;
#endif
}



/**
 * Emit a hosted resize event for an app-window.
 *
 * @param win app-window receiving the event
 * @param framebuffer_width framebuffer width in physical pixels
 * @param framebuffer_height framebuffer height in physical pixels
 * @param window_width logical host-window width
 * @param window_height logical host-window height
 * @param content_scale_x horizontal content scale
 * @param content_scale_y vertical content scale
 * @return 0 on success, negative on error
 */
int dvz_app_window_emit_resize(
    DvzAppWindow* win, uint32_t framebuffer_width, uint32_t framebuffer_height,
    uint32_t window_width, uint32_t window_height, float content_scale_x, float content_scale_y)
{
    ANN(win);
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    if (win->window == NULL)
        return -1;
    dvz_window_backend_emit_resize(
        win->window, framebuffer_width, framebuffer_height, window_width, window_height,
        content_scale_x, content_scale_y);
    dvz_app_window_request_frame(win);
    return 0;
#else
    return -1;
#endif
}



/**
 * Emit a hosted pointer position/button event for an app-window.
 *
 * @param win app-window receiving the event
 * @param type pointer event type
 * @param x pointer x position in logical host-window coordinates
 * @param y pointer y position in logical host-window coordinates
 * @param window_width logical host-window width
 * @param window_height logical host-window height
 * @param button pointer button
 * @param mods keyboard modifier bit mask
 * @return 0 on success, negative on error
 */
int dvz_app_window_emit_pointer(
    DvzAppWindow* win, DvzPointerEventType type, float x, float y, float window_width,
    float window_height, DvzPointerButton button, int mods)
{
    ANN(win);
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    if (win->canvas == NULL)
        return -1;
    DvzInputRouter* router = dvz_canvas_input(win->canvas);
    if (router == NULL)
        return -1;
    dvz_pointer_emit_position(
        router, type, x, y, window_width, window_height, button, mods,
        _app_window_content_scale(win), dvz_input_timestamp_ns(),
        win->window != NULL ? dvz_window_user_data(win->window) : NULL);
    dvz_app_window_request_frame(win);
    return 0;
#else
    return -1;
#endif
}



/**
 * Emit a hosted pointer wheel event for an app-window.
 *
 * @param win app-window receiving the event
 * @param x pointer x position in logical host-window coordinates
 * @param y pointer y position in logical host-window coordinates
 * @param window_width logical host-window width
 * @param window_height logical host-window height
 * @param dx horizontal wheel delta
 * @param dy vertical wheel delta
 * @param mods keyboard modifier bit mask
 * @return 0 on success, negative on error
 */
int dvz_app_window_emit_wheel(
    DvzAppWindow* win, float x, float y, float window_width, float window_height, float dx,
    float dy, int mods)
{
    ANN(win);
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    if (win->canvas == NULL)
        return -1;
    DvzInputRouter* router = dvz_canvas_input(win->canvas);
    if (router == NULL)
        return -1;
    dvz_pointer_emit_wheel(
        router, x, y, window_width, window_height, dx, dy, mods, _app_window_content_scale(win),
        dvz_input_timestamp_ns(), win->window != NULL ? dvz_window_user_data(win->window) : NULL);
    dvz_app_window_request_frame(win);
    return 0;
#else
    return -1;
#endif
}



/**
 * Emit a hosted keyboard event for an app-window.
 *
 * @param win app-window receiving the event
 * @param type keyboard event type
 * @param key Datoviz key code
 * @param mods keyboard modifier bit mask
 * @return 0 on success, negative on error
 */
int
dvz_app_window_emit_key(DvzAppWindow* win, DvzKeyboardEventType type, DvzKeyCode key, int mods)
{
    ANN(win);
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    if (win->canvas == NULL)
        return -1;
    DvzInputRouter* router = dvz_canvas_input(win->canvas);
    if (router == NULL)
        return -1;
    dvz_keyboard_emit(
        router, type, key, mods, win->window != NULL ? dvz_window_user_data(win->window) : NULL);
    dvz_app_window_request_frame(win);
    return 0;
#else
    return -1;
#endif
}




struct DvzCanvas* dvz_app_window_canvas(DvzAppWindow* win)
{
    ANN(win);
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    return win->canvas;
#else
    return NULL;
#endif
}


struct DvzInputRouter* dvz_app_window_input(DvzAppWindow* win)
{
    ANN(win);
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    return win->canvas ? dvz_canvas_input(win->canvas) : NULL;
#else
    return NULL;
#endif
}


int dvz_app_window_capture_png(DvzAppWindow* win, const char* path)
{
    ANN(win);
    ANN(path);
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    return dvz_canvas_capture_png(win->canvas, path);
#else
    return -1;
#endif
}


/**
 * Start app-window DVZR recording.
 *
 * @param win the app-window
 * @param path output recording directory path
 * @return 0 on success, negative on error
 */
int dvz_app_window_record_start(DvzAppWindow* win, const char* path)
{
    ANN(win);
    ANN(path);
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    if (win->recorder != NULL)
        return -1;
    DvzDrp2RecordingInfo info = {
        .width = 0,
        .height = 0,
        .duration_s = 0.0,
        .t_present = 0.0,
        .backend_hint = "app",
    };
    win->recorder = dvz_drp2_recorder_open(path, &info);
    if (win->recorder == NULL)
        return -1;
    win->recording_clock = dvz_clock();
    win->recording_target_created = false;
    return 0;
#else
    return -1;
#endif
}



/**
 * Stop app-window DVZR recording.
 *
 * @param win the app-window
 * @return 0 on success, negative on error
 */
int dvz_app_window_record_stop(DvzAppWindow* win)
{
    ANN(win);
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    if (win->recorder == NULL)
        return -1;
    bool ok = dvz_drp2_recorder_close(win->recorder);
    win->recorder = NULL;
    win->recording_target_created = false;
    return ok ? 0 : -1;
#else
    return -1;
#endif
}



/**
 * Start app-window DVZR live replay.
 *
 * @param win the app-window
 * @param path input recording directory path
 * @return 0 on success, negative on error
 */
int dvz_app_window_replay_start(DvzAppWindow* win, const char* path)
{
    ANN(win);
    ANN(path);
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    if (win->app == NULL || win->app->runtime == NULL || win->replay_recording != NULL)
        return -1;

    DvzDrp2Recording* recording = dvz_drp2_recording_open(path);
    if (recording == NULL)
        return -1;

    uint64_t target_id = 0;
    if (!_app_replay_find_target_id(recording, &target_id))
    {
        dvz_drp2_recording_close(recording);
        return -1;
    }

    win->replay_recording = recording;
    win->replay_target_id = target_id;
    win->replay_frame_index = 0;
    win->replay_clock = dvz_clock();
    win->replay_clock_started = true;
    win->replay_paced = true;
    win->replay_loop = false;
    win->replay_speed = 1.0;
    win->render_enabled = true;
    dvz_drp2_runtime_reset(win->app->runtime);
    return 0;
#else
    return -1;
#endif
}



/**
 * Stop app-window DVZR live replay.
 *
 * @param win the app-window
 * @return 0 on success, negative on error
 */
int dvz_app_window_replay_stop(DvzAppWindow* win)
{
    ANN(win);
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    if (win->replay_recording == NULL)
        return -1;
    dvz_drp2_recording_close(win->replay_recording);
    win->replay_recording = NULL;
    win->replay_target_id = 0;
    win->replay_frame_index = 0;
    win->replay_clock_started = false;
    return 0;
#else
    return -1;
#endif
}



/**
 * Enable or disable timestamp-paced live replay.
 *
 * @param win the app-window
 * @param paced whether replay waits for recorded timestamps
 */
void dvz_app_window_replay_set_paced(DvzAppWindow* win, bool paced)
{
    ANN(win);
    win->replay_paced = paced;
}



/**
 * Set the live replay speed multiplier.
 *
 * @param win the app-window
 * @param speed speed multiplier
 */
void dvz_app_window_replay_set_speed(DvzAppWindow* win, double speed)
{
    ANN(win);
    if (speed > 0)
        win->replay_speed = speed;
}



/**
 * Enable or disable live replay looping.
 *
 * @param win the app-window
 * @param loop whether replay should loop
 */
void dvz_app_window_replay_set_loop(DvzAppWindow* win, bool loop)
{
    ANN(win);
    win->replay_loop = loop;
}



/**
 * Return the active live replay frame count.
 *
 * @param win the app-window
 * @return replay frame count, or 0 when no replay is active
 */
uint32_t dvz_app_window_replay_frame_count(const DvzAppWindow* win)
{
    ANN(win);
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    if (win->replay_recording == NULL)
        return 0;
    return dvz_drp2_recording_frame_count(win->replay_recording);
#else
    return 0;
#endif
}



/**
 * Resize an app-window's logical and framebuffer extent.
 *
 * @param win the app-window
 * @param width width in pixels
 * @param height height in pixels
 * @return 0 on success, negative on error
 */
int dvz_app_window_resize(DvzAppWindow* win, uint32_t width, uint32_t height)
{
    ANN(win);
    if (width == 0 || height == 0)
        return -1;

#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    if (win->window == NULL || win->figure == NULL)
        return -1;
    dvz_window_backend_emit_resize(win->window, width, height, width, height, 1.0f, 1.0f);
    dvz_figure_resize(win->figure, width, height);
    dvz_app_window_request_frame(win);
    return 0;
#else
    (void)width;
    (void)height;
    return -1;
#endif
}


/**
 * Enable or disable rendering for an app-window.
 *
 * @param win app-window to update
 * @param enabled whether rendering should be enabled
 */
void dvz_app_window_set_render_enabled(DvzAppWindow* win, bool enabled)
{
    ANN(win);
    win->render_enabled = enabled;
}



/**
 * Return whether rendering is enabled for an app-window.
 *
 * @param win app-window to query
 * @return whether rendering is enabled
 */
bool dvz_app_window_render_enabled(const DvzAppWindow* win)
{
    ANN(win);
    return win->render_enabled;
}



/**
 * Request that the host schedules another frame for an app-window.
 *
 * @param win app-window requesting a frame
 */
void dvz_app_window_request_frame(DvzAppWindow* win)
{
    ANN(win);
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    if (win->app != NULL && win->app->window_host != NULL && win->window != NULL)
        dvz_window_host_request_frame(win->app->window_host, win->window);
#endif
    if (win->request_frame_callback != NULL)
        win->request_frame_callback(win, win->request_frame_user_data);
}



/**
 * Register a callback invoked whenever Datoviz requests another frame.
 *
 * @param win app-window receiving the callback
 * @param callback callback pointer, or NULL to clear it
 * @param user_data opaque pointer forwarded to the callback
 */
void dvz_app_window_set_request_frame_callback(
    DvzAppWindow* win, DvzAppRequestFrameCallback callback, void* user_data)
{
    ANN(win);
    win->request_frame_callback = callback;
    win->request_frame_user_data = user_data;
}



void dvz_app_window_set_frame_callback(
    DvzAppWindow* win, DvzAppFrameCallback callback, void* user_data)
{
    ANN(win);
    win->frame_callback = callback;
    win->frame_user_data = user_data;
}



/*************************************************************************************************/
/*  GUI                                                                                          */
/*************************************************************************************************/

DvzGui* dvz_app_window_gui(DvzAppWindow* win, const DvzGuiConfig* config)
{
    ANN(win);
#if defined(DVZ_HAS_GUI) && DVZ_HAS_GUI
    if (win->gui != NULL)
        return win->gui;
    if (win->app == NULL || win->app->gpu_ctx == NULL || win->window == NULL)
        return NULL;
    win->gui = _dvz_gui_create(win->app, win->app->gpu_ctx, win->window, config);
    return win->gui;
#else
    (void)config;
    return NULL;
#endif
}



void dvz_app_window_set_gui_callback(
    DvzAppWindow* win, DvzGuiCallback callback, void* user_data)
{
    ANN(win);
#if defined(DVZ_HAS_GUI) && DVZ_HAS_GUI
    if (win->gui != NULL)
        _dvz_gui_set_callback(win->gui, callback, user_data);
#else
    (void)callback;
    (void)user_data;
#endif
}



/*************************************************************************************************/
/*  Frame loop                                                                                   */
/*************************************************************************************************/

/**
 * Render one frame for a single app-window.
 *
 * @param win app-window to render
 * @return canvas frame status or negative on error
 */
int dvz_app_window_render_once(DvzAppWindow* win)
{
    ANN(win);

#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    if (!win->render_enabled)
        return 0;
    if (win->canvas == NULL)
        return -1;
    int rc = dvz_canvas_frame(win->canvas);
    if (rc == DVZ_CANVAS_FRAME_READY)
    {
        if (dvz_canvas_submit(win->canvas) != 0)
            return -1;
        if (win->app != NULL && win->app->scene != NULL &&
            dvz_scene_has_active_animations(win->app->scene))
        {
            dvz_app_window_request_frame(win);
        }
    }
    return rc;
#else
    return -1;
#endif
}



/**
 * Render one frame for every app-window without polling events.
 *
 * @param app app whose windows should render
 * @return 0 on success, wait-surface status, or negative on error
 */
int dvz_app_render_once(DvzApp* app)
{
    ANN(app);

#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    int result = 0;
    for (uint32_t i = 0; i < app->window_count; i++)
    {
        int rc = dvz_app_window_render_once(&app->windows[i]);
        if (rc < 0)
            result = -1;
        else if (result == 0 && rc == DVZ_CANVAS_FRAME_WAIT_SURFACE)
            result = DVZ_CANVAS_FRAME_WAIT_SURFACE;
    }
    return result;
#else
    return -1;
#endif
}



void dvz_app_run(DvzApp* app, uint32_t frame_count)
{
    ANN(app);

#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    /* FPS counter — opt in via DVZ_FPS=1 (default on for interactive runs to aid profiling). */
    const char* fps_env = getenv("DVZ_FPS");
    bool fps_enabled = (fps_env == NULL) ? (frame_count == 0)
                                         : (strcmp(fps_env, "0") != 0);
    uint64_t fps_window_start = fps_enabled ? dvz_input_timestamp_ns() : 0;
    uint32_t fps_window_frames = 0;

    if (frame_count == 0)
    {
        /* Interactive mode: loop until every interactive window requests close. */
        for (;;)
        {
            dvz_window_host_poll(app->window_host);
            bool any_open = false;
            for (uint32_t i = 0; i < app->window_count; i++)
            {
                DvzAppWindow* win = &app->windows[i];
                if (win->is_interactive && !dvz_window_should_close(win->window))
                    any_open = true;
            }
            if (!any_open)
                break;
            for (uint32_t i = 0; i < app->window_count; i++)
            {
                DvzAppWindow* win = &app->windows[i];
                (void)dvz_app_window_render_once(win);
            }
            if (fps_enabled)
            {
                fps_window_frames++;
                uint64_t now = dvz_input_timestamp_ns();
                uint64_t elapsed_ns = now - fps_window_start;
                if (elapsed_ns >= 1000000000ULL)
                {
                    double fps = (double)fps_window_frames * 1e9 / (double)elapsed_ns;
                    _dvz_app_status_fps(
                        &app->status, fps, fps_window_frames, (double)elapsed_ns / 1e9);
                    _dvz_app_status_render(&app->status);
                    fps_window_start = now;
                    fps_window_frames = 0;
                }
            }
        }
    }
    else
    {
        for (uint32_t f = 0; f < frame_count; f++)
        {
            dvz_window_host_poll(app->window_host);
            for (uint32_t i = 0; i < app->window_count; i++)
            {
                DvzAppWindow* win = &app->windows[i];
                (void)dvz_app_window_render_once(win);
            }
        }
    }
    DvzDevice* device = dvz_gpu_ctx_device(app->gpu_ctx);
    if (device != NULL)
        dvz_device_wait(device);
    _dvz_app_status_finish(&app->status);
#else
    (void)frame_count;
#endif
}
