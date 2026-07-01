/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  DRP2 command metadata                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stddef.h>

#include "_stream.h"
#include "command_metadata.h"
#include "packet_wire.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_DRP2_COMMAND_METADATA_COUNT                                                        \
    ((size_t)DVZ_DRP2_COMMAND_QUEUE_SUBMIT_REPLY + (size_t)1)
#define DVZ_DRP2_COMMAND_BODY_SIZE(name) sizeof(((DvzDrp2Command*)0)->u.name)



/*************************************************************************************************/
/*  Tables                                                                                       */
/*************************************************************************************************/

static const DvzDrp2CommandMetadata _none_metadata = {
    DVZ_DRP2_COMMAND_NONE,
    "None",
    DVZ_DRP2_PACKET_NONE,
    0,
};


static const DvzDrp2CommandMetadata _command_metadata[DVZ_DRP2_COMMAND_METADATA_COUNT] = {
    [DVZ_DRP2_COMMAND_NONE] = {DVZ_DRP2_COMMAND_NONE, "None", DVZ_DRP2_PACKET_NONE, 0},
    [DVZ_DRP2_COMMAND_HELLO_RENDERER] = {DVZ_DRP2_COMMAND_HELLO_RENDERER, "HelloRenderer",
                                         DVZ_DRP2_PACKET_SETUP,
                                         DVZ_DRP2_COMMAND_BODY_SIZE(handshake)},
    [DVZ_DRP2_COMMAND_RENDERER_HELLO_REPLY] = {
        DVZ_DRP2_COMMAND_RENDERER_HELLO_REPLY, "RendererHelloReply", DVZ_DRP2_PACKET_SETUP,
        DVZ_DRP2_COMMAND_BODY_SIZE(handshake)},
    [DVZ_DRP2_COMMAND_CREATE_BUFFER] = {DVZ_DRP2_COMMAND_CREATE_BUFFER, "CreateBuffer",
                                        DVZ_DRP2_PACKET_SETUP,
                                        DVZ_DRP2_COMMAND_BODY_SIZE(create_buffer)},
    [DVZ_DRP2_COMMAND_DESTROY_BUFFER] = {DVZ_DRP2_COMMAND_DESTROY_BUFFER, "DestroyBuffer",
                                         DVZ_DRP2_PACKET_SETUP,
                                         DVZ_DRP2_COMMAND_BODY_SIZE(destroy_buffer)},
    [DVZ_DRP2_COMMAND_CREATE_TEXTURE] = {DVZ_DRP2_COMMAND_CREATE_TEXTURE, "CreateTexture",
                                         DVZ_DRP2_PACKET_SETUP,
                                         DVZ_DRP2_COMMAND_BODY_SIZE(create_texture)},
    [DVZ_DRP2_COMMAND_DESTROY_TEXTURE] = {DVZ_DRP2_COMMAND_DESTROY_TEXTURE, "DestroyTexture",
                                          DVZ_DRP2_PACKET_SETUP,
                                          DVZ_DRP2_COMMAND_BODY_SIZE(destroy_texture)},
    [DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE] = {
        DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE, "CreateShaderModule", DVZ_DRP2_PACKET_SETUP,
        sizeof(PacketShaderBody)},
    [DVZ_DRP2_COMMAND_DESTROY_SHADER_MODULE] = {
        DVZ_DRP2_COMMAND_DESTROY_SHADER_MODULE, "DestroyShaderModule", DVZ_DRP2_PACKET_SETUP,
        DVZ_DRP2_COMMAND_BODY_SIZE(destroy_shader_module)},
    [DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE] = {
        DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE, "CreateRenderPipeline", DVZ_DRP2_PACKET_SETUP,
        DVZ_DRP2_COMMAND_BODY_SIZE(create_render_pipeline)},
    [DVZ_DRP2_COMMAND_DESTROY_RENDER_PIPELINE] = {
        DVZ_DRP2_COMMAND_DESTROY_RENDER_PIPELINE, "DestroyRenderPipeline", DVZ_DRP2_PACKET_SETUP,
        DVZ_DRP2_COMMAND_BODY_SIZE(destroy_render_pipeline)},
    [DVZ_DRP2_COMMAND_CREATE_COMPUTE_PIPELINE] = {
        DVZ_DRP2_COMMAND_CREATE_COMPUTE_PIPELINE, "CreateComputePipeline", DVZ_DRP2_PACKET_SETUP,
        DVZ_DRP2_COMMAND_BODY_SIZE(create_compute_pipeline)},
    [DVZ_DRP2_COMMAND_DESTROY_COMPUTE_PIPELINE] = {
        DVZ_DRP2_COMMAND_DESTROY_COMPUTE_PIPELINE, "DestroyComputePipeline", DVZ_DRP2_PACKET_SETUP,
        DVZ_DRP2_COMMAND_BODY_SIZE(destroy_compute_pipeline)},
    [DVZ_DRP2_COMMAND_CREATE_SAMPLER] = {DVZ_DRP2_COMMAND_CREATE_SAMPLER, "CreateSampler",
                                         DVZ_DRP2_PACKET_SETUP,
                                         DVZ_DRP2_COMMAND_BODY_SIZE(create_sampler)},
    [DVZ_DRP2_COMMAND_CREATE_BIND_GROUP_LAYOUT] = {
        DVZ_DRP2_COMMAND_CREATE_BIND_GROUP_LAYOUT, "CreateBindGroupLayout", DVZ_DRP2_PACKET_SETUP,
        DVZ_DRP2_COMMAND_BODY_SIZE(create_bind_group_layout)},
    [DVZ_DRP2_COMMAND_CREATE_BIND_GROUP] = {DVZ_DRP2_COMMAND_CREATE_BIND_GROUP,
                                            "CreateBindGroup", DVZ_DRP2_PACKET_SETUP,
                                            DVZ_DRP2_COMMAND_BODY_SIZE(create_bind_group)},
    [DVZ_DRP2_COMMAND_DESTROY_BIND_GROUP_LAYOUT] = {
        DVZ_DRP2_COMMAND_DESTROY_BIND_GROUP_LAYOUT, "DestroyBindGroupLayout",
        DVZ_DRP2_PACKET_SETUP, DVZ_DRP2_COMMAND_BODY_SIZE(destroy_bind_group_layout)},
    [DVZ_DRP2_COMMAND_DESTROY_BIND_GROUP] = {
        DVZ_DRP2_COMMAND_DESTROY_BIND_GROUP, "DestroyBindGroup", DVZ_DRP2_PACKET_SETUP,
        DVZ_DRP2_COMMAND_BODY_SIZE(destroy_bind_group)},
    [DVZ_DRP2_COMMAND_WRITE_BUFFER] = {DVZ_DRP2_COMMAND_WRITE_BUFFER, "WriteBuffer",
                                       DVZ_DRP2_PACKET_UPDATE, sizeof(PacketWriteBufferBody)},
    [DVZ_DRP2_COMMAND_WRITE_TEXTURE] = {DVZ_DRP2_COMMAND_WRITE_TEXTURE, "WriteTexture",
                                        DVZ_DRP2_PACKET_UPDATE, sizeof(PacketWriteTextureBody)},
    [DVZ_DRP2_COMMAND_BEGIN_COMMAND_ENCODER] = {
        DVZ_DRP2_COMMAND_BEGIN_COMMAND_ENCODER, "BeginCommandEncoder", DVZ_DRP2_PACKET_FRAME,
        DVZ_DRP2_COMMAND_BODY_SIZE(begin_command_encoder)},
    [DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS] = {DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS,
                                            "BeginRenderPass", DVZ_DRP2_PACKET_FRAME,
                                            DVZ_DRP2_COMMAND_BODY_SIZE(begin_render_pass)},
    [DVZ_DRP2_COMMAND_BEGIN_COMPUTE_PASS] = {DVZ_DRP2_COMMAND_BEGIN_COMPUTE_PASS,
                                             "BeginComputePass", DVZ_DRP2_PACKET_FRAME,
                                             DVZ_DRP2_COMMAND_BODY_SIZE(begin_compute_pass)},
    [DVZ_DRP2_COMMAND_SET_VIEWPORT] = {DVZ_DRP2_COMMAND_SET_VIEWPORT, "SetViewport",
                                       DVZ_DRP2_PACKET_FRAME,
                                       DVZ_DRP2_COMMAND_BODY_SIZE(set_viewport)},
    [DVZ_DRP2_COMMAND_SET_SCISSOR] = {DVZ_DRP2_COMMAND_SET_SCISSOR, "SetScissor",
                                      DVZ_DRP2_PACKET_FRAME,
                                      DVZ_DRP2_COMMAND_BODY_SIZE(set_scissor)},
    [DVZ_DRP2_COMMAND_SET_PIPELINE] = {DVZ_DRP2_COMMAND_SET_PIPELINE, "SetPipeline",
                                       DVZ_DRP2_PACKET_FRAME,
                                       DVZ_DRP2_COMMAND_BODY_SIZE(set_pipeline)},
    [DVZ_DRP2_COMMAND_SET_BIND_GROUP] = {DVZ_DRP2_COMMAND_SET_BIND_GROUP, "SetBindGroup",
                                         DVZ_DRP2_PACKET_FRAME,
                                         DVZ_DRP2_COMMAND_BODY_SIZE(set_bind_group)},
    [DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER] = {DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER,
                                            "SetVertexBuffer", DVZ_DRP2_PACKET_FRAME,
                                            DVZ_DRP2_COMMAND_BODY_SIZE(set_vertex_buffer)},
    [DVZ_DRP2_COMMAND_SET_INDEX_BUFFER] = {DVZ_DRP2_COMMAND_SET_INDEX_BUFFER,
                                           "SetIndexBuffer", DVZ_DRP2_PACKET_FRAME,
                                           DVZ_DRP2_COMMAND_BODY_SIZE(set_index_buffer)},
    [DVZ_DRP2_COMMAND_DRAW] = {DVZ_DRP2_COMMAND_DRAW, "Draw", DVZ_DRP2_PACKET_FRAME,
                               DVZ_DRP2_COMMAND_BODY_SIZE(draw)},
    [DVZ_DRP2_COMMAND_DRAW_INDEXED] = {DVZ_DRP2_COMMAND_DRAW_INDEXED, "DrawIndexed",
                                       DVZ_DRP2_PACKET_FRAME,
                                       DVZ_DRP2_COMMAND_BODY_SIZE(draw_indexed)},
    [DVZ_DRP2_COMMAND_END_RENDER_PASS] = {DVZ_DRP2_COMMAND_END_RENDER_PASS, "EndRenderPass",
                                          DVZ_DRP2_PACKET_FRAME,
                                          DVZ_DRP2_COMMAND_BODY_SIZE(end_render_pass)},
    [DVZ_DRP2_COMMAND_DISPATCH_WORKGROUPS] = {
        DVZ_DRP2_COMMAND_DISPATCH_WORKGROUPS, "DispatchWorkgroups", DVZ_DRP2_PACKET_FRAME,
        DVZ_DRP2_COMMAND_BODY_SIZE(dispatch)},
    [DVZ_DRP2_COMMAND_END_COMPUTE_PASS] = {DVZ_DRP2_COMMAND_END_COMPUTE_PASS,
                                           "EndComputePass", DVZ_DRP2_PACKET_FRAME,
                                           DVZ_DRP2_COMMAND_BODY_SIZE(end_compute_pass)},
    [DVZ_DRP2_COMMAND_RESOURCE_BARRIER] = {DVZ_DRP2_COMMAND_RESOURCE_BARRIER,
                                           "ResourceBarrier", DVZ_DRP2_PACKET_FRAME,
                                           DVZ_DRP2_COMMAND_BODY_SIZE(resource_barrier)},
    [DVZ_DRP2_COMMAND_COPY_BUFFER_TO_BUFFER] = {
        DVZ_DRP2_COMMAND_COPY_BUFFER_TO_BUFFER, "CopyBufferToBuffer", DVZ_DRP2_PACKET_FRAME,
        DVZ_DRP2_COMMAND_BODY_SIZE(copy_buffer_to_buffer)},
    [DVZ_DRP2_COMMAND_COPY_BUFFER_TO_TEXTURE] = {
        DVZ_DRP2_COMMAND_COPY_BUFFER_TO_TEXTURE, "CopyBufferToTexture", DVZ_DRP2_PACKET_FRAME,
        DVZ_DRP2_COMMAND_BODY_SIZE(copy_buffer_to_texture)},
    [DVZ_DRP2_COMMAND_COPY_TEXTURE_TO_BUFFER] = {
        DVZ_DRP2_COMMAND_COPY_TEXTURE_TO_BUFFER, "CopyTextureToBuffer", DVZ_DRP2_PACKET_FRAME,
        DVZ_DRP2_COMMAND_BODY_SIZE(copy_texture_to_buffer)},
    [DVZ_DRP2_COMMAND_COPY_TEXTURE_TO_TEXTURE] = {
        DVZ_DRP2_COMMAND_COPY_TEXTURE_TO_TEXTURE, "CopyTextureToTexture", DVZ_DRP2_PACKET_FRAME,
        DVZ_DRP2_COMMAND_BODY_SIZE(copy_texture_to_texture)},
    [DVZ_DRP2_COMMAND_FINISH_COMMAND_ENCODER] = {
        DVZ_DRP2_COMMAND_FINISH_COMMAND_ENCODER, "FinishCommandEncoder", DVZ_DRP2_PACKET_FRAME,
        DVZ_DRP2_COMMAND_BODY_SIZE(finish_command_encoder)},
    [DVZ_DRP2_COMMAND_QUEUE_SUBMIT] = {DVZ_DRP2_COMMAND_QUEUE_SUBMIT, "QueueSubmit",
                                       DVZ_DRP2_PACKET_FRAME,
                                       DVZ_DRP2_COMMAND_BODY_SIZE(queue_submit)},
    [DVZ_DRP2_COMMAND_QUEUE_SUBMIT_REPLY] = {DVZ_DRP2_COMMAND_QUEUE_SUBMIT_REPLY,
                                             "QueueSubmitReply", DVZ_DRP2_PACKET_FRAME,
                                             DVZ_DRP2_COMMAND_BODY_SIZE(queue_submit)},
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

const DvzDrp2CommandMetadata* _dvz_drp2_command_metadata(DvzDrp2CommandType type)
{
    const size_t index = (size_t)type;
    if (index >= DVZ_DRP2_COMMAND_METADATA_COUNT)
        return &_none_metadata;

    const DvzDrp2CommandMetadata* metadata = &_command_metadata[index];
    if (metadata->name == NULL || metadata->type != type)
        return &_none_metadata;
    return metadata;
}


const char* _dvz_drp2_command_name(DvzDrp2CommandType type)
{
    return _dvz_drp2_command_metadata(type)->name;
}


DvzDrp2PacketKind _dvz_drp2_command_packet_kind(DvzDrp2CommandType type)
{
    return _dvz_drp2_command_metadata(type)->packet_kind;
}


uint64_t _dvz_drp2_command_fixed_body_size(DvzDrp2CommandType type)
{
    return _dvz_drp2_command_metadata(type)->fixed_body_size;
}
