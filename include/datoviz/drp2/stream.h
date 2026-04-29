/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  DRP2 command stream                                                                          */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/common/macros.h"
#include "datoviz/drp2/types.h"



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Create an empty DRP2 command stream.
 *
 * @return the command stream
 */
DVZ_EXPORT DvzDrp2CommandStream* dvz_drp2_stream(void);



/**
 * Destroy a DRP2 command stream.
 *
 * @param stream the command stream
 */
DVZ_EXPORT void dvz_drp2_stream_destroy(DvzDrp2CommandStream* stream);



/**
 * Return the number of commands in a DRP2 command stream.
 *
 * @param stream the command stream
 * @return the number of commands
 */
DVZ_EXPORT uint32_t dvz_drp2_stream_count(const DvzDrp2CommandStream* stream);



/**
 * Return a command from a DRP2 command stream.
 *
 * @param stream the command stream
 * @param index the command index
 * @return the command, or NULL when index is out of bounds
 */
DVZ_EXPORT const DvzDrp2Command*
dvz_drp2_stream_get(const DvzDrp2CommandStream* stream, uint32_t index);



/**
 * Return a command type.
 *
 * @param command the command
 * @return the command type
 */
DVZ_EXPORT DvzDrp2CommandType dvz_drp2_command_type(const DvzDrp2Command* command);



/**
 * Append a HelloRenderer command.
 *
 * @param stream the command stream
 * @param client_name the client name
 * @return whether the command was appended
 */
DVZ_EXPORT bool
dvz_drp2_stream_hello_renderer(DvzDrp2CommandStream* stream, const char* client_name);



/**
 * Append a RendererHelloReply command.
 *
 * @param stream the command stream
 * @param renderer_name the renderer name
 * @return whether the command was appended
 */
DVZ_EXPORT bool
dvz_drp2_stream_renderer_hello_reply(DvzDrp2CommandStream* stream, const char* renderer_name);



/**
 * Append a CreateBuffer command.
 *
 * @param stream the command stream
 * @param id the buffer id
 * @param size the buffer size in bytes
 * @param usage buffer usage flags
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_create_buffer(
    DvzDrp2CommandStream* stream, uint64_t id, uint64_t size, uint32_t usage);



/**
 * Append a DestroyBuffer command.
 *
 * @param stream the command stream
 * @param buffer_id the buffer id
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_destroy_buffer(DvzDrp2CommandStream* stream, uint64_t buffer_id);



/**
 * Append a CreateTexture command for a 2D render attachment.
 *
 * @param stream the command stream
 * @param id the texture id
 * @param width the texture width
 * @param height the texture height
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_create_texture_2d(
    DvzDrp2CommandStream* stream, uint64_t id, uint32_t width, uint32_t height);



/**
 * Append a CreateTexture command for a 2D texture with explicit usage.
 *
 * @param stream the command stream
 * @param id the texture id
 * @param width the texture width
 * @param height the texture height
 * @param usage texture usage flags
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_create_texture_2d_usage(
    DvzDrp2CommandStream* stream, uint64_t id, uint32_t width, uint32_t height, uint32_t usage);



/**
 * Append a DestroyTexture command.
 *
 * @param stream the command stream
 * @param texture_id the texture id
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_destroy_texture(DvzDrp2CommandStream* stream, uint64_t texture_id);



/**
 * Append a CreateShaderModule command.
 *
 * @param stream the command stream
 * @param id the shader module id
 * @param stage the shader stage
 * @param code the WGSL shader source
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_create_shader_module(
    DvzDrp2CommandStream* stream, uint64_t id, const char* stage, const char* code);



/**
 * Append a DestroyShaderModule command.
 *
 * @param stream the command stream
 * @param shader_module_id the shader module id
 * @return whether the command was appended
 */
DVZ_EXPORT bool
dvz_drp2_stream_destroy_shader_module(DvzDrp2CommandStream* stream, uint64_t shader_module_id);



/**
 * Append a CreateRenderPipeline command.
 *
 * @param stream the command stream
 * @param id the pipeline id
 * @param vertex_shader_module_id the vertex shader module id
 * @param fragment_shader_module_id the fragment shader module id
 * @param vertex_buffer_slots the number of required vertex buffer slots
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_create_render_pipeline(
    DvzDrp2CommandStream* stream, uint64_t id, uint64_t vertex_shader_module_id,
    uint64_t fragment_shader_module_id, uint32_t vertex_buffer_slots);



/**
 * Append a CreateRenderPipeline command with one bind-group layout.
 *
 * @param stream the command stream
 * @param id the pipeline id
 * @param vertex_shader_module_id the vertex shader module id
 * @param fragment_shader_module_id the fragment shader module id
 * @param vertex_buffer_slots the number of required vertex buffer slots
 * @param bind_group_layout_id the bind-group layout id for slot 0
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_create_render_pipeline_with_bind_group_layout(
    DvzDrp2CommandStream* stream, uint64_t id, uint64_t vertex_shader_module_id,
    uint64_t fragment_shader_module_id, uint32_t vertex_buffer_slots,
    uint64_t bind_group_layout_id);



/**
 * Append a DestroyRenderPipeline command.
 *
 * @param stream the command stream
 * @param render_pipeline_id the render pipeline id
 * @return whether the command was appended
 */
DVZ_EXPORT bool
dvz_drp2_stream_destroy_render_pipeline(DvzDrp2CommandStream* stream, uint64_t render_pipeline_id);



/**
 * Append a CreateComputePipeline command.
 *
 * @param stream the command stream
 * @param id the pipeline id
 * @param compute_shader_module_id the compute shader module id
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_create_compute_pipeline(
    DvzDrp2CommandStream* stream, uint64_t id, uint64_t compute_shader_module_id);



/**
 * Append a CreateComputePipeline command with one bind-group layout.
 *
 * @param stream the command stream
 * @param id the pipeline id
 * @param compute_shader_module_id the compute shader module id
 * @param bind_group_layout_id the bind-group layout id for slot 0
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_create_compute_pipeline_with_bind_group_layout(
    DvzDrp2CommandStream* stream, uint64_t id, uint64_t compute_shader_module_id,
    uint64_t bind_group_layout_id);



/**
 * Append a DestroyComputePipeline command.
 *
 * @param stream the command stream
 * @param compute_pipeline_id the compute pipeline id
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_destroy_compute_pipeline(
    DvzDrp2CommandStream* stream, uint64_t compute_pipeline_id);



/**
 * Append a CreateSampler command.
 *
 * @param stream the command stream
 * @param id the sampler id
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_create_sampler(DvzDrp2CommandStream* stream, uint64_t id);



/**
 * Append a CreateBindGroupLayout command for one sampled texture and one sampler.
 *
 * @param stream the command stream
 * @param id the bind-group layout id
 * @return whether the command was appended
 */
DVZ_EXPORT bool
dvz_drp2_stream_create_texture_sampler_bind_group_layout(DvzDrp2CommandStream* stream, uint64_t id);



/**
 * Append a CreateBindGroupLayout command for two storage buffers.
 *
 * @param stream the command stream
 * @param id the bind-group layout id
 * @return whether the command was appended
 */
DVZ_EXPORT bool
dvz_drp2_stream_create_storage_bind_group_layout(DvzDrp2CommandStream* stream, uint64_t id);



/**
 * Append a CreateBindGroup command for one sampled texture and one sampler.
 *
 * @param stream the command stream
 * @param id the bind-group id
 * @param bind_group_layout_id the bind-group layout id
 * @param texture_id the sampled texture id
 * @param sampler_id the sampler id
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_create_texture_sampler_bind_group(
    DvzDrp2CommandStream* stream, uint64_t id, uint64_t bind_group_layout_id, uint64_t texture_id,
    uint64_t sampler_id);



/**
 * Append a CreateBindGroup command for two storage buffers.
 *
 * @param stream the command stream
 * @param id the bind-group id
 * @param bind_group_layout_id the bind-group layout id
 * @param buffer0_id the first storage buffer id
 * @param buffer1_id the second storage buffer id
 * @param buffer_size the bound range size for each buffer
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_create_storage_bind_group(
    DvzDrp2CommandStream* stream, uint64_t id, uint64_t bind_group_layout_id, uint64_t buffer0_id,
    uint64_t buffer1_id, uint64_t buffer_size);



/**
 * Append a DestroyBindGroupLayout command.
 *
 * @param stream the command stream
 * @param bind_group_layout_id the bind-group layout id
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_destroy_bind_group_layout(
    DvzDrp2CommandStream* stream, uint64_t bind_group_layout_id);



/**
 * Append a DestroyBindGroup command.
 *
 * @param stream the command stream
 * @param bind_group_id the bind-group id
 * @return whether the command was appended
 */
DVZ_EXPORT bool
dvz_drp2_stream_destroy_bind_group(DvzDrp2CommandStream* stream, uint64_t bind_group_id);



/**
 * Append a WriteBuffer command.
 *
 * @param stream the command stream
 * @param buffer_id the buffer id
 * @param offset the byte offset
 * @param size the payload size in bytes
 * @param data_base64 base64-encoded payload
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_write_buffer(
    DvzDrp2CommandStream* stream, uint64_t buffer_id, uint64_t offset, uint64_t size,
    const char* data_base64);



/**
 * Append a WriteTexture command.
 *
 * @param stream the command stream
 * @param texture_id the destination texture id
 * @param mip_level the destination mip level
 * @param width the written width
 * @param height the written height
 * @param bytes_per_row the source bytes per row
 * @param rows_per_image the source rows per image
 * @param data_base64 base64-encoded payload
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_write_texture_2d(
    DvzDrp2CommandStream* stream, uint64_t texture_id, uint32_t mip_level, uint32_t width,
    uint32_t height, uint32_t bytes_per_row, uint32_t rows_per_image, const char* data_base64);



/**
 * Append a BeginCommandEncoder command.
 *
 * @param stream the command stream
 * @param id the encoder id
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_begin_command_encoder(DvzDrp2CommandStream* stream, uint64_t id);



/**
 * Append a BeginRenderPass command with one color texture attachment.
 *
 * @param stream the command stream
 * @param id the render pass id
 * @param encoder_id the encoder id
 * @param texture_id the color attachment texture id
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_begin_render_pass(
    DvzDrp2CommandStream* stream, uint64_t id, uint64_t encoder_id, uint64_t texture_id);



/**
 * Append a BeginComputePass command.
 *
 * @param stream the command stream
 * @param id the compute pass id
 * @param encoder_id the encoder id
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_begin_compute_pass(
    DvzDrp2CommandStream* stream, uint64_t id, uint64_t encoder_id);



/**
 * Append a SetPipeline command.
 *
 * @param stream the command stream
 * @param pass_id the pass id
 * @param pipeline_id the pipeline id
 * @return whether the command was appended
 */
DVZ_EXPORT bool
dvz_drp2_stream_set_pipeline(DvzDrp2CommandStream* stream, uint64_t pass_id, uint64_t pipeline_id);



/**
 * Append a SetBindGroup command.
 *
 * @param stream the command stream
 * @param pass_id the pass id
 * @param slot the bind-group slot
 * @param bind_group_id the bind-group id
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_set_bind_group(
    DvzDrp2CommandStream* stream, uint64_t pass_id, uint32_t slot, uint64_t bind_group_id);



/**
 * Append a SetVertexBuffer command.
 *
 * @param stream the command stream
 * @param pass_id the pass id
 * @param slot the vertex buffer slot
 * @param buffer_id the buffer id
 * @param offset the byte offset
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_set_vertex_buffer(
    DvzDrp2CommandStream* stream, uint64_t pass_id, uint32_t slot, uint64_t buffer_id,
    uint64_t offset);



/**
 * Append a SetIndexBuffer command.
 *
 * @param stream the command stream
 * @param pass_id the pass id
 * @param buffer_id the index buffer id
 * @param index_format the index format token
 * @param offset the byte offset
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_set_index_buffer(
    DvzDrp2CommandStream* stream, uint64_t pass_id, uint64_t buffer_id, const char* index_format,
    uint64_t offset);



/**
 * Append a Draw command.
 *
 * @param stream the command stream
 * @param pass_id the pass id
 * @param vertex_count the vertex count
 * @param instance_count the instance count
 * @param first_vertex the first vertex
 * @param first_instance the first instance
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_draw(
    DvzDrp2CommandStream* stream, uint64_t pass_id, uint32_t vertex_count,
    uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance);



/**
 * Append a DrawIndexed command.
 *
 * @param stream the command stream
 * @param pass_id the pass id
 * @param index_count the index count
 * @param instance_count the instance count
 * @param first_index the first index
 * @param base_vertex the base vertex
 * @param first_instance the first instance
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_draw_indexed(
    DvzDrp2CommandStream* stream, uint64_t pass_id, uint32_t index_count,
    uint32_t instance_count, uint32_t first_index, int32_t base_vertex, uint32_t first_instance);



/**
 * Append an EndRenderPass command.
 *
 * @param stream the command stream
 * @param pass_id the pass id
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_end_render_pass(DvzDrp2CommandStream* stream, uint64_t pass_id);



/**
 * Append a DispatchWorkgroups command.
 *
 * @param stream the command stream
 * @param pass_id the compute pass id
 * @param x the x workgroup count
 * @param y the y workgroup count
 * @param z the z workgroup count
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_dispatch_workgroups(
    DvzDrp2CommandStream* stream, uint64_t pass_id, uint32_t x, uint32_t y, uint32_t z);



/**
 * Append an EndComputePass command.
 *
 * @param stream the command stream
 * @param pass_id the compute pass id
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_end_compute_pass(DvzDrp2CommandStream* stream, uint64_t pass_id);



/**
 * Append a CopyBufferToBuffer command.
 *
 * @param stream the command stream
 * @param encoder_id the encoder id
 * @param src_buffer_id the source buffer id
 * @param src_offset the source byte offset
 * @param dst_buffer_id the destination buffer id
 * @param dst_offset the destination byte offset
 * @param size the copied byte size
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_copy_buffer_to_buffer(
    DvzDrp2CommandStream* stream, uint64_t encoder_id, uint64_t src_buffer_id,
    uint64_t src_offset, uint64_t dst_buffer_id, uint64_t dst_offset, uint64_t size);



/**
 * Append a CopyBufferToTexture command.
 *
 * @param stream the command stream
 * @param encoder_id the encoder id
 * @param src_buffer_id the source buffer id
 * @param src_offset the source byte offset
 * @param dst_texture_id the destination texture id
 * @param width the copy width in pixels
 * @param height the copy height in pixels
 * @param bytes_per_row the source bytes per row
 * @param rows_per_image the source rows per image
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_copy_buffer_to_texture(
    DvzDrp2CommandStream* stream, uint64_t encoder_id, uint64_t src_buffer_id,
    uint64_t src_offset, uint64_t dst_texture_id, uint32_t width, uint32_t height,
    uint32_t bytes_per_row, uint32_t rows_per_image);



/**
 * Append a CopyTextureToBuffer command.
 *
 * @param stream the command stream
 * @param encoder_id the encoder id
 * @param src_texture_id the source texture id
 * @param dst_buffer_id the destination buffer id
 * @param dst_offset the destination byte offset
 * @param width the copy width in pixels
 * @param height the copy height in pixels
 * @param bytes_per_row the destination bytes per row
 * @param rows_per_image the destination rows per image
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_copy_texture_to_buffer(
    DvzDrp2CommandStream* stream, uint64_t encoder_id, uint64_t src_texture_id,
    uint64_t dst_buffer_id, uint64_t dst_offset, uint32_t width, uint32_t height,
    uint32_t bytes_per_row, uint32_t rows_per_image);



/**
 * Append a CopyTextureToTexture command.
 *
 * @param stream the command stream
 * @param encoder_id the encoder id
 * @param src_texture_id the source texture id
 * @param dst_texture_id the destination texture id
 * @param width the copy width in pixels
 * @param height the copy height in pixels
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_copy_texture_to_texture(
    DvzDrp2CommandStream* stream, uint64_t encoder_id, uint64_t src_texture_id,
    uint64_t dst_texture_id, uint32_t width, uint32_t height);



/**
 * Append a FinishCommandEncoder command.
 *
 * @param stream the command stream
 * @param encoder_id the encoder id
 * @param command_buffer_id the command buffer id
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_finish_command_encoder(
    DvzDrp2CommandStream* stream, uint64_t encoder_id, uint64_t command_buffer_id);



/**
 * Append a QueueSubmit command with one command buffer and no readback.
 *
 * @param stream the command stream
 * @param command_buffer_id the command buffer id
 * @param submission_id the submission id
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_queue_submit(
    DvzDrp2CommandStream* stream, uint64_t command_buffer_id, uint64_t submission_id);



/**
 * Append a QueueSubmit command with one command buffer and one readback request.
 *
 * @param stream the command stream
 * @param command_buffer_id the command buffer id
 * @param submission_id the submission id
 * @param buffer_id the readback buffer id
 * @param offset the readback byte offset
 * @param size the readback byte size
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_queue_submit_readback(
    DvzDrp2CommandStream* stream, uint64_t command_buffer_id, uint64_t submission_id,
    uint64_t buffer_id, uint64_t offset, uint64_t size);



/**
 * Serialize a command stream as a DRP2 fixture JSON document.
 *
 * @param stream the command stream
 * @param name the fixture name
 * @return an owned NUL-terminated JSON string
 */
DVZ_EXPORT char*
dvz_drp2_stream_json(const DvzDrp2CommandStream* stream, const char* name);



/**
 * Destroy a JSON string returned by dvz_drp2_stream_json().
 *
 * @param json the JSON string
 */
DVZ_EXPORT void dvz_drp2_stream_json_destroy(char* json);

EXTERN_C_OFF
