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
 * Attach a debug label to a numeric DRP2 id in a command stream.
 *
 * Labels are diagnostic metadata only: runtimes ignore them and they are not executable commands.
 *
 * @param stream the command stream
 * @param id the DRP2 object or transient id
 * @param label the debug label, or NULL to clear it to an empty string
 * @return whether the label was recorded
 */
DVZ_EXPORT bool
dvz_drp2_stream_set_label(DvzDrp2CommandStream* stream, uint64_t id, const char* label);



/**
 * Return a debug label attached to a numeric DRP2 id.
 *
 * @param stream the command stream
 * @param id the DRP2 object or transient id
 * @return the label, or NULL when the stream has no label for the id
 */
DVZ_EXPORT const char*
dvz_drp2_stream_label(const DvzDrp2CommandStream* stream, uint64_t id);



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
 * Append a CreateTexture command for a 2D texture with explicit format and usage.
 *
 * @param stream the command stream
 * @param id the texture id
 * @param width the texture width
 * @param height the texture height
 * @param format texture format, using VkFormat values
 * @param usage texture usage flags
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_create_texture_2d_format_usage(
    DvzDrp2CommandStream* stream, uint64_t id, uint32_t width, uint32_t height, uint32_t format,
    uint32_t usage);



/**
 * Append a CreateTexture command for a 3D texture.
 *
 * @param stream the command stream
 * @param id the texture id
 * @param width the texture width
 * @param height the texture height
 * @param depth the texture depth (number of slices)
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_create_texture_3d(
    DvzDrp2CommandStream* stream, uint64_t id, uint32_t width, uint32_t height, uint32_t depth);


/**
 * Append a CreateTexture command for a 3D texture with explicit format and usage.
 *
 * @param stream the command stream
 * @param id the texture id
 * @param width the texture width
 * @param height the texture height
 * @param depth the texture depth (number of slices)
 * @param format texture format, using VkFormat values
 * @param usage texture usage flags
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_create_texture_3d_format_usage(
    DvzDrp2CommandStream* stream, uint64_t id, uint32_t width, uint32_t height, uint32_t depth,
    uint32_t format, uint32_t usage);


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
 * Append a CreateShaderModule command with an explicit shader source format.
 *
 * @param stream the command stream
 * @param id the shader module id
 * @param stage the shader stage
 * @param format the shader source format
 * @param code the shader source
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_create_shader_module_format(
    DvzDrp2CommandStream* stream, uint64_t id, const char* stage, const char* format,
    const char* code);



/**
 * Append a CreateShaderModule command from a precompiled SPIR-V binary (in-process path).
 *
 * The caller retains ownership of `spirv`; it must remain valid until the stream is executed.
 *
 * @param stream the command stream
 * @param id the shader module id
 * @param stage the shader stage ("VERTEX" or "FRAGMENT")
 * @param spirv pointer to SPIR-V bytecode
 * @param spirv_size size in bytes
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_create_shader_module_spirv(
    DvzDrp2CommandStream* stream, uint64_t id, const char* stage,
    const unsigned char* spirv, uint64_t spirv_size);



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
 * Append a CreateRenderPipeline command with explicit vertex input layout and topology.
 *
 * @param stream the command stream
 * @param id the pipeline id
 * @param vertex_shader_module_id the vertex shader module id
 * @param fragment_shader_module_id the fragment shader module id
 * @param vertex_buffer_slots the number of required vertex buffer slots
 * @param topology VkPrimitiveTopology value (e.g. VK_PRIMITIVE_TOPOLOGY_POINT_LIST)
 * @param binding_count number of vertex binding descriptors
 * @param binding_strides stride in bytes for each binding
 * @param attr_count number of vertex attribute descriptors
 * @param attr_bindings binding index for each attribute
 * @param attr_locations shader location for each attribute
 * @param attr_formats VkFormat for each attribute
 * @param attr_offsets byte offset within the binding for each attribute
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_create_render_pipeline_ex(
    DvzDrp2CommandStream* stream, uint64_t id, uint64_t vertex_shader_module_id,
    uint64_t fragment_shader_module_id, uint32_t vertex_buffer_slots,
    uint32_t topology,
    uint32_t binding_count, const uint32_t* binding_strides,
    uint32_t attr_count, const uint32_t* attr_bindings, const uint32_t* attr_locations,
    const uint32_t* attr_formats, const uint32_t* attr_offsets);



/**
 * Append a CreateRenderPipeline command with explicit vertex layout, topology, and step modes.
 *
 * @param stream the command stream
 * @param id the pipeline id
 * @param vertex_shader_module_id the vertex shader module id
 * @param fragment_shader_module_id the fragment shader module id
 * @param vertex_buffer_slots the number of required vertex buffer slots
 * @param topology VkPrimitiveTopology value (e.g. VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
 * @param binding_count number of vertex binding descriptors
 * @param binding_strides stride in bytes for each binding
 * @param binding_step_modes DvzDrp2VertexStepMode value for each binding
 * @param attr_count number of vertex attribute descriptors
 * @param attr_bindings binding index for each attribute
 * @param attr_locations shader location for each attribute
 * @param attr_formats VkFormat for each attribute
 * @param attr_offsets byte offset within the binding for each attribute
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_create_render_pipeline_ex2(
    DvzDrp2CommandStream* stream, uint64_t id, uint64_t vertex_shader_module_id,
    uint64_t fragment_shader_module_id, uint32_t vertex_buffer_slots,
    uint32_t topology,
    uint32_t binding_count, const uint32_t* binding_strides,
    const uint32_t* binding_step_modes,
    uint32_t attr_count, const uint32_t* attr_bindings, const uint32_t* attr_locations,
    const uint32_t* attr_formats, const uint32_t* attr_offsets);


/**
 * Attach a bind-group layout to the most recently appended CreateRenderPipeline command.
 *
 * Use after `dvz_drp2_stream_create_render_pipeline_ex` to combine an explicit vertex layout
 * with a bind-group layout (mirrors what `_with_bind_group_layout` does on its own).
 *
 * @param stream the command stream
 * @param bind_group_layout_id the bind-group layout id (0 = none)
 * @return whether the most recent command was a CreateRenderPipeline and was updated
 */
DVZ_EXPORT bool dvz_drp2_stream_pipeline_set_bind_group_layout(
    DvzDrp2CommandStream* stream, uint64_t bind_group_layout_id);



/**
 * Attach a second bind-group layout (descriptor set 1) to the most recently appended
 * CreateRenderPipeline command.
 *
 * @param stream the command stream
 * @param bind_group_layout_id2 the second bind-group layout id (0 = none)
 * @return whether the most recent command was a CreateRenderPipeline and was updated
 */
DVZ_EXPORT bool dvz_drp2_stream_pipeline_set_bind_group_layout2(
    DvzDrp2CommandStream* stream, uint64_t bind_group_layout_id2);


/**
 * Attach an ordered bind-group layout array to the most recent CreateRenderPipeline command.
 *
 * @param stream the command stream
 * @param count the number of bind-group layout ids
 * @param bind_group_layout_ids ordered bind-group layout ids, one per slot
 * @return whether the most recent command was updated
 */
DVZ_EXPORT bool dvz_drp2_stream_pipeline_set_bind_group_layouts(
    DvzDrp2CommandStream* stream, uint32_t count, const uint64_t* bind_group_layout_ids);


/**
 * Attach depth state to the most recently appended CreateRenderPipeline command.
 *
 * @param stream the command stream
 * @param depth_write_enabled whether depth writes are enabled
 * @param depth_compare_op VkCompareOp value used for depth testing
 * @return whether the most recent command was a CreateRenderPipeline and was updated
 */
DVZ_EXPORT bool dvz_drp2_stream_pipeline_set_depth_state(
    DvzDrp2CommandStream* stream, bool depth_write_enabled, uint32_t depth_compare_op);


/**
 * Set one color target format on the most recently appended CreateRenderPipeline command.
 *
 * @param stream the command stream
 * @param idx the color target index
 * @param format backend-native texture format enum value
 * @return whether the most recent command was a CreateRenderPipeline and was updated
 */
DVZ_EXPORT bool dvz_drp2_stream_pipeline_set_color_target(
    DvzDrp2CommandStream* stream, uint32_t idx, uint32_t format);


/**
 * Set one color target blend state on the most recently appended CreateRenderPipeline command.
 *
 * @param stream the command stream
 * @param idx the color target index
 * @param src_color source color blend factor
 * @param dst_color destination color blend factor
 * @param color_op color blend operation
 * @param src_alpha source alpha blend factor
 * @param dst_alpha destination alpha blend factor
 * @param alpha_op alpha blend operation
 * @param color_write_mask color component write mask
 * @return whether the most recent command was a CreateRenderPipeline and was updated
 */
DVZ_EXPORT bool dvz_drp2_stream_pipeline_set_color_blend(
    DvzDrp2CommandStream* stream, uint32_t idx, uint32_t src_color, uint32_t dst_color,
    uint32_t color_op, uint32_t src_alpha, uint32_t dst_alpha, uint32_t alpha_op,
    uint32_t color_write_mask);



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
 * Append a CreateBindGroupLayout command for compute input/output storage buffers.
 *
 * @param stream the command stream
 * @param id the bind-group layout id
 * @return whether the command was appended
 */
DVZ_EXPORT bool
dvz_drp2_stream_create_storage_bind_group_layout(DvzDrp2CommandStream* stream, uint64_t id);



/**
 * Append a CreateBindGroupLayout command for one uniform buffer (VS + FS visible).
 *
 * @param stream the command stream
 * @param id the bind-group layout id
 * @return whether the command was appended
 */
DVZ_EXPORT bool
dvz_drp2_stream_create_uniform_bind_group_layout(DvzDrp2CommandStream* stream, uint64_t id);


/**
 * Append a CreateBindGroupLayout command with explicit entries.
 *
 * @param stream the command stream
 * @param id the bind-group layout id
 * @param entry_count number of entries
 * @param entries bind-group layout entries
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_create_bind_group_layout_entries(
    DvzDrp2CommandStream* stream, uint64_t id, uint32_t entry_count,
    const DvzDrp2BindGroupLayoutEntry* entries);



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
 * Append a CreateBindGroup command for one uniform buffer with a sub-allocation offset.
 *
 * @param stream the command stream
 * @param id the bind-group id
 * @param bind_group_layout_id the bind-group layout id
 * @param buffer_id the uniform buffer id
 * @param offset byte offset into the buffer for this sub-allocation
 * @param size bound range size in bytes
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_create_uniform_bind_group(
    DvzDrp2CommandStream* stream, uint64_t id, uint64_t bind_group_layout_id, uint64_t buffer_id,
    uint64_t offset, uint64_t size);


/**
 * Append a CreateBindGroup command with explicit resource entries.
 *
 * @param stream the command stream
 * @param id the bind-group id
 * @param bind_group_layout_id the bind-group layout id
 * @param entry_count number of entries
 * @param entries bind-group resource entries
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_create_bind_group_entries(
    DvzDrp2CommandStream* stream, uint64_t id, uint64_t bind_group_layout_id,
    uint32_t entry_count, const DvzDrp2BindGroupEntry* entries);



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
 * Append a WriteBuffer command from raw bytes.
 *
 * In-process callers that have a raw byte pointer should prefer this entry
 * point over dvz_drp2_stream_write_buffer (which takes a pre-encoded base64
 * string and is intended for JSON wire-loading paths).
 *
 * IMPORTANT (lifetime): `data` is borrowed, NOT copied. The caller MUST keep
 * the buffer alive and unchanged from this call until the stream is executed
 * (or destroyed unused). Modifying or freeing `data` between emit and execute
 * is undefined behavior. The base64 string is computed lazily, only if the
 * stream is later serialized to JSON.
 *
 * size==0 is a valid WebGPU-shaped no-op: returns true without recording a
 * command and does not retain `data` (which may legitimately be NULL).
 *
 * @param stream the command stream
 * @param buffer_id the destination buffer id
 * @param offset byte offset within the buffer
 * @param size number of bytes to write (0 is a valid no-op)
 * @param data raw source bytes (must be non-NULL when size>0)
 * @return whether the call succeeded (true on size==0 no-op even though no
 *         command was recorded)
 */
DVZ_EXPORT bool dvz_drp2_stream_write_buffer_bytes(
    DvzDrp2CommandStream* stream, uint64_t buffer_id, uint64_t offset, uint64_t size,
    const void* data);



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
 * Append a WriteTexture command for a full 2D mip level using raw bytes.
 *
 * Mirrors `dvz_drp2_stream_write_buffer_bytes`: the runtime path consumes the borrowed
 * pointer directly with no base64 round-trip; JSON serialization re-encodes lazily.
 *
 * @param stream the command stream
 * @param texture_id the destination texture id
 * @param mip_level the destination mip level
 * @param width the written width
 * @param height the written height
 * @param bytes_per_row the source bytes per row
 * @param rows_per_image the source rows per image
 * @param data raw pixel bytes (must remain valid until the stream executes)
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_write_texture_2d_bytes(
    DvzDrp2CommandStream* stream, uint64_t texture_id, uint32_t mip_level, uint32_t width,
    uint32_t height, uint32_t bytes_per_row, uint32_t rows_per_image, const void* data);


/**
 * Append a WriteTexture command for a 2D sub-region using raw bytes.
 *
 * Mirrors `dvz_drp2_stream_write_texture_2d_bytes` for the in-process runtime path.
 *
 * @param stream the command stream
 * @param texture_id the destination texture id
 * @param mip_level the destination mip level
 * @param origin_x x offset in texels
 * @param origin_y y offset in texels
 * @param width the written width
 * @param height the written height
 * @param bytes_per_row the source bytes per row
 * @param rows_per_image the source rows per image
 * @param data raw pixel bytes (must remain valid until the stream executes)
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_write_texture_2d_region_bytes(
    DvzDrp2CommandStream* stream, uint64_t texture_id, uint32_t mip_level, uint32_t origin_x,
    uint32_t origin_y, uint32_t width, uint32_t height, uint32_t bytes_per_row,
    uint32_t rows_per_image, const void* data);


/**
 * Append a WriteTexture command for a 2D sub-region with explicit origin.
 *
 * @param stream the command stream
 * @param texture_id the destination texture id
 * @param mip_level the destination mip level
 * @param origin_x x offset in texels
 * @param origin_y y offset in texels
 * @param width the written width
 * @param height the written height
 * @param bytes_per_row the source bytes per row
 * @param rows_per_image the source rows per image
 * @param data_base64 base64-encoded payload
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_write_texture_2d_region(
    DvzDrp2CommandStream* stream, uint64_t texture_id, uint32_t mip_level,
    uint32_t origin_x, uint32_t origin_y,
    uint32_t width, uint32_t height,
    uint32_t bytes_per_row, uint32_t rows_per_image, const char* data_base64);


/**
 * Append a WriteTexture command for a 3D sub-region.
 *
 * @param stream the command stream
 * @param texture_id the destination texture id
 * @param mip_level the destination mip level
 * @param origin_x x offset in texels
 * @param origin_y y offset in texels
 * @param origin_z z offset in texels
 * @param width the written width
 * @param height the written height
 * @param depth the written depth
 * @param bytes_per_row the source bytes per row
 * @param rows_per_image the source rows per image
 * @param data_base64 base64-encoded payload
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_write_texture_3d(
    DvzDrp2CommandStream* stream, uint64_t texture_id, uint32_t mip_level,
    uint32_t origin_x, uint32_t origin_y, uint32_t origin_z,
    uint32_t width, uint32_t height, uint32_t depth,
    uint32_t bytes_per_row, uint32_t rows_per_image, const char* data_base64);


/**
 * Append a WriteTexture command for a 3D sub-region using raw bytes.
 *
 * Mirrors `dvz_drp2_stream_write_texture_2d_bytes` for 3D texture uploads.
 *
 * @param stream the command stream
 * @param texture_id the destination texture id
 * @param mip_level the destination mip level
 * @param origin_x x offset in texels
 * @param origin_y y offset in texels
 * @param origin_z z offset in texels
 * @param width the written width
 * @param height the written height
 * @param depth the written depth
 * @param bytes_per_row the source bytes per row
 * @param rows_per_image the source rows per image
 * @param data raw pixel bytes (must remain valid until the stream executes)
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_write_texture_3d_bytes(
    DvzDrp2CommandStream* stream, uint64_t texture_id, uint32_t mip_level,
    uint32_t origin_x, uint32_t origin_y, uint32_t origin_z,
    uint32_t width, uint32_t height, uint32_t depth,
    uint32_t bytes_per_row, uint32_t rows_per_image, const void* data);



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
 * Append a BeginRenderPass command with an explicit RGBA clear color.
 *
 * @param stream the command stream
 * @param id the render pass id
 * @param encoder_id the encoder id
 * @param texture_id the color attachment texture id
 * @param r red clear value [0, 1]
 * @param g green clear value [0, 1]
 * @param b blue clear value [0, 1]
 * @param a alpha clear value [0, 1]
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_begin_render_pass_clear(
    DvzDrp2CommandStream* stream, uint64_t id, uint64_t encoder_id, uint64_t texture_id,
    float r, float g, float b, float a);


/**
 * Append a BeginRenderPass command for a normalized target sub-region.
 *
 * The render pass targets the rectangle `(x, y, width, height)` in normalized
 * [0, 1] attachment coordinates. When `clear` is true, the runtime clears the
 * full target before rendering; when false, existing target contents are
 * preserved and only subsequent draw commands are clipped to the region.
 *
 * @param stream the command stream
 * @param id the render pass id
 * @param encoder_id the encoder id
 * @param texture_id the color attachment texture id
 * @param r clear color red channel
 * @param g clear color green channel
 * @param b clear color blue channel
 * @param a clear color alpha channel
 * @param x normalized left coordinate in [0, 1]
 * @param y normalized top coordinate in [0, 1]
 * @param width normalized width in [0, 1]
 * @param height normalized height in [0, 1]
 * @param clear whether to clear the target at render-pass begin
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_begin_render_pass_region_clear(
    DvzDrp2CommandStream* stream, uint64_t id, uint64_t encoder_id, uint64_t texture_id,
    float r, float g, float b, float a, float x, float y, float width, float height, bool clear);


/**
 * Add a color attachment to the most recently appended BeginRenderPass command.
 *
 * @param stream the command stream
 * @param texture_id the color attachment texture id
 * @param r clear color red channel
 * @param g clear color green channel
 * @param b clear color blue channel
 * @param a clear color alpha channel
 * @param clear whether to clear this attachment at render-pass begin
 * @return whether the most recent command was a BeginRenderPass and was updated
 */
DVZ_EXPORT bool dvz_drp2_stream_begin_render_pass_add_color_attachment(
    DvzDrp2CommandStream* stream, uint64_t texture_id, float r, float g, float b, float a,
    bool clear);


/**
 * Set load/store operations on one color attachment of the most recent BeginRenderPass command.
 *
 * @param stream the command stream
 * @param attachment_index the color attachment index
 * @param load_op the attachment load operation
 * @param store_op the attachment store operation
 * @return whether the most recent command was a BeginRenderPass and was updated
 */
DVZ_EXPORT bool dvz_drp2_stream_begin_render_pass_set_color_attachment_ops(
    DvzDrp2CommandStream* stream, uint32_t attachment_index, DvzDrp2AttachmentLoadOp load_op,
    DvzDrp2AttachmentStoreOp store_op);


/**
 * Attach a transient depth attachment request to the most recently appended BeginRenderPass
 * command.
 *
 * @param stream the command stream
 * @param clear_depth the depth clear value used when the pass clears attachments
 * @return whether the most recent command was a BeginRenderPass and was updated
 */
DVZ_EXPORT bool
dvz_drp2_stream_begin_render_pass_set_depth(DvzDrp2CommandStream* stream, float clear_depth);


/**
 * Attach a named depth texture to the most recently appended BeginRenderPass command.
 *
 * A zero `depth_texture_id` keeps the existing transient depth attachment behavior.
 *
 * @param stream the command stream
 * @param depth_texture_id the depth attachment texture id, or 0 for transient depth
 * @param clear_depth the depth clear value used when the pass clears attachments
 * @return whether the most recent command was a BeginRenderPass and was updated
 */
DVZ_EXPORT bool dvz_drp2_stream_begin_render_pass_set_depth_texture(
    DvzDrp2CommandStream* stream, uint64_t depth_texture_id, float clear_depth);


/**
 * Set load/store operations on the depth attachment of the most recent BeginRenderPass command.
 *
 * @param stream the command stream
 * @param load_op the depth attachment load operation
 * @param store_op the depth attachment store operation
 * @return whether the most recent command was a BeginRenderPass and was updated
 */
DVZ_EXPORT bool dvz_drp2_stream_begin_render_pass_set_depth_ops(
    DvzDrp2CommandStream* stream, DvzDrp2AttachmentLoadOp load_op,
    DvzDrp2AttachmentStoreOp store_op);



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
 * Append a SetViewport command.
 *
 * @param stream the command stream
 * @param pass_id the render pass id
 * @param x normalized left coordinate in [0, 1]
 * @param y normalized top coordinate in [0, 1]
 * @param width normalized width in [0, 1]
 * @param height normalized height in [0, 1]
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_set_viewport(
    DvzDrp2CommandStream* stream, uint64_t pass_id, float x, float y, float width, float height);



/**
 * Append a SetScissor command.
 *
 * @param stream the command stream
 * @param pass_id the render pass id
 * @param x normalized left coordinate in [0, 1]
 * @param y normalized top coordinate in [0, 1]
 * @param width normalized width in [0, 1]
 * @param height normalized height in [0, 1]
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_set_scissor(
    DvzDrp2CommandStream* stream, uint64_t pass_id, float x, float y, float width, float height);



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
 * Append a SetBindGroup command with dynamic offsets.
 *
 * @param stream the command stream
 * @param pass_id the pass id
 * @param slot the bind-group slot
 * @param bind_group_id the bind-group id
 * @param dynamic_offset_count number of dynamic offsets
 * @param dynamic_offsets dynamic offsets consumed in layout-entry order
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_set_bind_group_dynamic(
    DvzDrp2CommandStream* stream, uint64_t pass_id, uint32_t slot, uint64_t bind_group_id,
    uint32_t dynamic_offset_count, const uint64_t* dynamic_offsets);



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
