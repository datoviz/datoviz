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

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_status.h"
#include "_trace.h"
#include "datoviz/app.h"
#include "datoviz/scene.h"
#include "../drp2/_stream.h"

#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
#include "datoviz/canvas.h"
#include "datoviz/drp2/runtime.h"
#include "datoviz/drp2/stream.h"
#include "datoviz/input/pointer.h"
#include "datoviz/input/router.h"
#include "datoviz/scene/frame_plan.h"
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

#define DVZ_APP_MAX_WINDOWS        16
#define DVZ_APP_CANVAS_TARGET_BASE 0xF000ULL



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
    uint64_t frame_index;
    DvzAppFrameCallback frame_callback;
    void* frame_user_data;
    DvzAppTraceSnapshot last_trace_snapshot;
    bool has_last_trace_snapshot;
};


struct DvzApp
{
    DvzScene* scene;
#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    DvzGpuCtx*      gpu_ctx;
    DvzDrp2Runtime* runtime;
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
 * Print one detailed command line when the command carries useful trace payload.
 *
 * @param command command to print
 * @param index command index
 * @param include_transient_ids whether to include per-stream pass ids
 * @return true if a line was printed
 */
static bool _app_trace_print_command_detail(
    const DvzDrp2Command* command, uint32_t index, bool include_transient_ids)
{
    ANN(command);
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
        dvz_fprintf(
            stderr, "  %03u + CreateBuffer id=%" PRIu64 " size=%" PRIu64
                    " usage=0x%" PRIx32 "\n",
            index, command->u.create_buffer.id, command->u.create_buffer.size,
            command->u.create_buffer.usage);
        return true;
    case DVZ_DRP2_COMMAND_DESTROY_BUFFER:
        dvz_fprintf(
            stderr, "  %03u - DestroyBuffer id=%" PRIu64 "\n", index,
            command->u.destroy_buffer.buffer_id);
        return true;
    case DVZ_DRP2_COMMAND_CREATE_TEXTURE:
        dvz_fprintf(
            stderr, "  %03u + CreateTexture id=%" PRIu64 " size=(%" PRIu32 ",%" PRIu32
                    ",%" PRIu32 ") usage=0x%" PRIx32 "\n",
            index, command->u.create_texture.id, command->u.create_texture.width,
            command->u.create_texture.height, command->u.create_texture.depth,
            command->u.create_texture.usage);
        return true;
    case DVZ_DRP2_COMMAND_DESTROY_TEXTURE:
        dvz_fprintf(
            stderr, "  %03u - DestroyTexture id=%" PRIu64 "\n", index,
            command->u.destroy_texture.texture_id);
        return true;
    case DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE:
        dvz_fprintf(
            stderr, "  %03u + CreateShaderModule id=%" PRIu64 " stage=%s format=%s"
                    " code=%s spirv_size=%" PRIu64 "\n",
            index, command->u.create_shader_module.id, command->u.create_shader_module.stage,
            command->u.create_shader_module.format,
            command->u.create_shader_module.code != NULL ? "yes" : "no",
            command->u.create_shader_module.spirv_size);
        return true;
    case DVZ_DRP2_COMMAND_DESTROY_SHADER_MODULE:
        dvz_fprintf(
            stderr, "  %03u - DestroyShaderModule id=%" PRIu64 "\n", index,
            command->u.destroy_shader_module.shader_module_id);
        return true;
    case DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE:
        dvz_fprintf(
            stderr, "  %03u + CreateRenderPipeline id=%" PRIu64 " vs=%" PRIu64
                    " fs=%" PRIu64 " vslots=%" PRIu32 " bgl0=%" PRIu64 " bgl1=%" PRIu64
                    " depth=%s write=%s compare=%" PRIu32 " topology=%" PRIu32
                    " bindings=%" PRIu32 " attrs=%" PRIu32 "\n",
            index, command->u.create_render_pipeline.id,
            command->u.create_render_pipeline.vertex_shader_module_id,
            command->u.create_render_pipeline.fragment_shader_module_id,
            command->u.create_render_pipeline.vertex_buffer_slots,
            command->u.create_render_pipeline.bind_group_layout_id,
            command->u.create_render_pipeline.bind_group_layout_id2,
            command->u.create_render_pipeline.has_depth_attachment ? "yes" : "no",
            command->u.create_render_pipeline.depth_write_enabled ? "yes" : "no",
            command->u.create_render_pipeline.depth_compare_op,
            command->u.create_render_pipeline.topology,
            command->u.create_render_pipeline.binding_count,
            command->u.create_render_pipeline.attr_count);
        return true;
    case DVZ_DRP2_COMMAND_DESTROY_RENDER_PIPELINE:
        dvz_fprintf(
            stderr, "  %03u - DestroyRenderPipeline id=%" PRIu64 "\n", index,
            command->u.destroy_render_pipeline.render_pipeline_id);
        return true;
    case DVZ_DRP2_COMMAND_CREATE_COMPUTE_PIPELINE:
        dvz_fprintf(
            stderr, "  %03u + CreateComputePipeline id=%" PRIu64 " shader=%" PRIu64
                    " bgl=%" PRIu64 "\n",
            index, command->u.create_compute_pipeline.id,
            command->u.create_compute_pipeline.compute_shader_module_id,
            command->u.create_compute_pipeline.bind_group_layout_id);
        return true;
    case DVZ_DRP2_COMMAND_DESTROY_COMPUTE_PIPELINE:
        dvz_fprintf(
            stderr, "  %03u - DestroyComputePipeline id=%" PRIu64 "\n", index,
            command->u.destroy_compute_pipeline.compute_pipeline_id);
        return true;
    case DVZ_DRP2_COMMAND_CREATE_SAMPLER:
        dvz_fprintf(
            stderr, "  %03u + CreateSampler id=%" PRIu64 "\n", index,
            command->u.create_sampler.id);
        return true;
    case DVZ_DRP2_COMMAND_CREATE_BIND_GROUP_LAYOUT:
        dvz_fprintf(
            stderr, "  %03u + CreateBindGroupLayout id=%" PRIu64
                    " storage=%s uniform=%s\n",
            index, command->u.create_bind_group_layout.id,
            command->u.create_bind_group_layout.storage_buffers ? "yes" : "no",
            command->u.create_bind_group_layout.uniform_buffer ? "yes" : "no");
        return true;
    case DVZ_DRP2_COMMAND_CREATE_BIND_GROUP:
        dvz_fprintf(
            stderr, "  %03u + CreateBindGroup id=%" PRIu64 " layout=%" PRIu64
                    " texture=%" PRIu64 " sampler=%" PRIu64 " buffer0=%" PRIu64
                    " buffer1=%" PRIu64 " size=%" PRIu64 " offset0=%" PRIu64 "\n",
            index, command->u.create_bind_group.id,
            command->u.create_bind_group.bind_group_layout_id,
            command->u.create_bind_group.texture_id, command->u.create_bind_group.sampler_id,
            command->u.create_bind_group.buffer0_id, command->u.create_bind_group.buffer1_id,
            command->u.create_bind_group.buffer_size,
            command->u.create_bind_group.buffer0_offset);
        return true;
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
        dvz_fprintf(
            stderr, "  %03u ~ WriteBuffer buffer=%" PRIu64 " offset=%" PRIu64
                    " size=%" PRIu64 "\n",
            index, command->u.write_buffer.buffer_id, command->u.write_buffer.offset,
            command->u.write_buffer.size);
        return true;
    case DVZ_DRP2_COMMAND_WRITE_TEXTURE:
        dvz_fprintf(
            stderr, "  %03u ~ WriteTexture texture=%" PRIu64 " origin=(%" PRIu32
                    ",%" PRIu32 ",%" PRIu32 ") size=(%" PRIu32 ",%" PRIu32 ",%" PRIu32
                    ") bytes_per_row=%" PRIu32 "\n",
            index, command->u.write_texture.texture_id, command->u.write_texture.origin_x,
            command->u.write_texture.origin_y, command->u.write_texture.origin_z,
            command->u.write_texture.width, command->u.write_texture.height,
            command->u.write_texture.depth, command->u.write_texture.bytes_per_row);
        return true;
    case DVZ_DRP2_COMMAND_BEGIN_COMMAND_ENCODER:
        dvz_fprintf(
            stderr, "  %03u = BeginCommandEncoder id=%" PRIu64 "\n", index,
            command->u.begin_command_encoder.id);
        return true;
    case DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS:
        dvz_fprintf(
            stderr, "  %03u = BeginRenderPass id=%" PRIu64 " encoder=%" PRIu64
                    " target=%" PRIu64 " clear=%s depth=%s clear_depth=%.3g"
                    " viewport=(%.3g,%.3g %.3gx%.3g)\n",
            index, command->u.begin_render_pass.id, command->u.begin_render_pass.encoder_id,
            command->u.begin_render_pass.texture_id,
            command->u.begin_render_pass.clear ? "yes" : "load",
            command->u.begin_render_pass.has_depth_attachment ? "yes" : "no",
            (double)command->u.begin_render_pass.clear_depth,
            (double)command->u.begin_render_pass.viewport[0],
            (double)command->u.begin_render_pass.viewport[1],
            (double)command->u.begin_render_pass.viewport[2],
            (double)command->u.begin_render_pass.viewport[3]);
        return true;
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
        if (include_transient_ids)
            dvz_fprintf(
                stderr, "  %03u = SetPipeline pass=%" PRIu64 " pipeline=%" PRIu64 "\n",
                index, command->u.set_pipeline.pass_id, command->u.set_pipeline.pipeline_id);
        else
            dvz_fprintf(
                stderr, "  %03u = SetPipeline pipeline=%" PRIu64 "\n", index,
                command->u.set_pipeline.pipeline_id);
        return true;
    case DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER:
        if (include_transient_ids)
            dvz_fprintf(
                stderr, "  %03u = SetVertexBuffer pass=%" PRIu64 " slot=%" PRIu32
                        " buffer=%" PRIu64 " offset=%" PRIu64 "\n",
                index, command->u.set_vertex_buffer.pass_id, command->u.set_vertex_buffer.slot,
                command->u.set_vertex_buffer.buffer_id, command->u.set_vertex_buffer.offset);
        else
            dvz_fprintf(
                stderr, "  %03u = SetVertexBuffer slot=%" PRIu32
                        " buffer=%" PRIu64 " offset=%" PRIu64 "\n",
                index, command->u.set_vertex_buffer.slot, command->u.set_vertex_buffer.buffer_id,
                command->u.set_vertex_buffer.offset);
        return true;
    case DVZ_DRP2_COMMAND_SET_INDEX_BUFFER:
        if (include_transient_ids)
            dvz_fprintf(
                stderr, "  %03u = SetIndexBuffer pass=%" PRIu64
                        " buffer=%" PRIu64 " format=%s offset=%" PRIu64 "\n",
                index, command->u.set_index_buffer.pass_id,
                command->u.set_index_buffer.buffer_id, command->u.set_index_buffer.index_format,
                command->u.set_index_buffer.offset);
        else
            dvz_fprintf(
                stderr, "  %03u = SetIndexBuffer buffer=%" PRIu64
                        " format=%s offset=%" PRIu64 "\n",
                index, command->u.set_index_buffer.buffer_id,
                command->u.set_index_buffer.index_format,
                command->u.set_index_buffer.offset);
        return true;
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
    return false;
}


/**
 * Print the full normalized snapshot for one changed DRP2 stream.
 *
 * @param snapshot current normalized snapshot
 * @param command_count emitted command count
 * @param frame_index 0-based frame index for the owning window
 */
static void _app_trace_stream_normal(
    const DvzAppTraceSnapshot* snapshot, uint32_t command_count, uint64_t frame_index)
{
    ANN(snapshot);
    dvz_fprintf(
        stderr, "frame %08" PRIu64 " | changed | %u cmds | %u semantic lines\n",
        frame_index, command_count, snapshot->count);

    for (uint32_t i = 0; i < snapshot->count; i++)
        dvz_fprintf(stderr, "  + %s\n", snapshot->lines[i].text);
}


/**
 * Print an expanded human-readable dump for one DRP2 stream.
 *
 * @param stream emitted command stream
 * @param frame_index 0-based frame index for the owning window
 */
static void _app_trace_stream_full(
    const DvzDrp2CommandStream* stream, uint64_t frame_index)
{
    ANN(stream);
    uint32_t command_count = dvz_drp2_stream_count(stream);
    dvz_fprintf(stderr, "\nframe %08" PRIu64 " | full | %u cmds\n", frame_index, command_count);
    for (uint32_t i = 0; i < command_count; i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        if (command == NULL)
            continue;
        DvzDrp2CommandType type = dvz_drp2_command_type(command);
        if (!_app_trace_print_command_detail(command, i, true))
        {
            dvz_fprintf(
                stderr, "  %03u %c %s\n", i, _trace_command_prefix(type),
                _trace_command_name(type));
        }
    }
}


/**
 * Print or refresh the live DRP2 trace for one emitted stream.
 *
 * In normal mode, changed frames print semantic additions/removals while unchanged frames rewrite
 * one in-place status line without scrolling. In full mode, every frame prints an expanded
 * human-readable command list.
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
        _app_trace_stream_full(stream, win->frame_index);
        _dvz_app_status_trace(
            &win->app->status, win->frame_index, command_count, snapshot.count, true);
        _dvz_app_status_render(&win->app->status);
    }
    else
    {
        if (plan.event_kind == DVZ_APP_TRACE_EVENT_CHANGED)
        {
            _app_trace_stream_normal(&snapshot, command_count, win->frame_index);
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




static void _app_draw(DvzCanvas* canvas, const DvzStreamFrame* frame, void* user_data)
{
    (void)canvas;
    ANN(frame);
    DvzAppWindow* win = (DvzAppWindow*)user_data;
    ANN(win);
    DvzApp* app = win->app;
    ANN(app);

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

    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format         = DVZ_SCENE_SHADER_FORMAT_GLSL;
    cfg.external_color_target = true;
    cfg.color_target_id       = win->target_id;
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
        log_error("_app_draw runtime execution failed at command %" PRIu32, result.command_index);
    else
        (void)dvz_figure_process_requests(win->figure, app->runtime, &caps);
    dvz_drp2_stream_destroy(stream);

    if (win->frame_callback != NULL)
        win->frame_callback(win, win->frame_user_data);
    win->frame_index++;
}

#endif



/*************************************************************************************************/
/*  App lifecycle                                                                                */
/*************************************************************************************************/

DvzApp* dvz_app(DvzScene* scene)
{
    ANN(scene);

#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
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

    /* GPU context — request dynamic rendering, synchronization2, and timeline semaphores. */
    DvzGpuCtxConfig gpu_cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan12Features features12 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    features12.timelineSemaphore = true;
    dvz_gpu_ctx_config_features12(&gpu_cfg, &features12);
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);

#if DVZ_HAS_GLFW
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
                    dvz_gpu_ctx_config_add_instance_extension(&gpu_cfg, extensions[i]);
                dvz_gpu_ctx_config_enable_canvas_extensions(&gpu_cfg, true);
            }
        }
    }
#endif

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

    return app;
#else
    (void)scene;
    return NULL;
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
        if (win->canvas != NULL)
        {
            dvz_canvas_destroy(win->canvas);
            win->canvas = NULL;
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
    /* DVZ_PRESENT_MODE: fifo (default, vsync), mailbox (vsync+latest), immediate (no vsync). */
    const char* pm_env = getenv("DVZ_PRESENT_MODE");
    if (pm_env != NULL)
    {
        if (strcmp(pm_env, "immediate") == 0)
            ccfg.present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        else if (strcmp(pm_env, "mailbox") == 0)
            ccfg.present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
        else if (strcmp(pm_env, "fifo") == 0)
            ccfg.present_mode = VK_PRESENT_MODE_FIFO_KHR;
        else
            log_warn("ignoring DVZ_PRESENT_MODE='%s' (expected fifo|mailbox|immediate)", pm_env);
    }
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



/*************************************************************************************************/
/*  Window accessors                                                                             */
/*************************************************************************************************/

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


void dvz_app_window_set_frame_callback(
    DvzAppWindow* win, DvzAppFrameCallback callback, void* user_data)
{
    ANN(win);
    win->frame_callback = callback;
    win->frame_user_data = user_data;
}



/*************************************************************************************************/
/*  Frame loop                                                                                   */
/*************************************************************************************************/

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
                int rc = dvz_canvas_frame(win->canvas);
                if (rc == DVZ_CANVAS_FRAME_READY)
                    dvz_canvas_submit(win->canvas);
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
                int rc = dvz_canvas_frame(win->canvas);
                if (rc == DVZ_CANVAS_FRAME_READY)
                    dvz_canvas_submit(win->canvas);
            }
        }
    }
    _dvz_app_status_finish(&app->status);
#else
    (void)frame_count;
#endif
}
