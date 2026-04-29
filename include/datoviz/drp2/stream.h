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
 * Append an EndRenderPass command.
 *
 * @param stream the command stream
 * @param pass_id the pass id
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_end_render_pass(DvzDrp2CommandStream* stream, uint64_t pass_id);



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
