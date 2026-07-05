/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  DRP2 command stream                                                                          */
/*************************************************************************************************/
/* Advanced/unstable runtime protocol builder. This header intentionally exposes backend-adjacent
 * tokens for vklite/WebGPU runtime integration and fixture tests. */

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
 * Return the numeric DRP2 id attached to a debug label.
 *
 * @param stream the command stream
 * @param label the debug label
 * @return the id, or 0 when the stream has no id for the label
 */
DVZ_EXPORT uint64_t
dvz_drp2_stream_label_id(const DvzDrp2CommandStream* stream, const char* label);



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
 * Return the default CreateTexture descriptor.
 *
 * @return initialized descriptor
 */
DVZ_EXPORT DvzDrp2TextureDesc dvz_drp2_texture_desc(void);


/**
 * Append a CreateTexture command from a descriptor.
 *
 * @param stream the command stream
 * @param desc the texture descriptor
 * @return whether the command was appended
 */
DVZ_EXPORT bool
dvz_drp2_stream_create_texture(DvzDrp2CommandStream* stream, const DvzDrp2TextureDesc* desc);


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
 * @param format texture format token
 * @param usage texture usage flags
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_create_texture_2d_format_usage(
    DvzDrp2CommandStream* stream, uint64_t id, uint32_t width, uint32_t height, DvzFormat format,
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
 * @param format texture format token
 * @param usage texture usage flags
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_create_texture_3d_format_usage(
    DvzDrp2CommandStream* stream, uint64_t id, uint32_t width, uint32_t height, uint32_t depth,
    DvzFormat format, uint32_t usage);


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
 * @param stage the shader stage ("VERTEX", "FRAGMENT", or "COMPUTE")
 * @param spirv pointer to SPIR-V bytecode
 * @param spirv_size size in bytes
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_create_shader_module_spirv(
    DvzDrp2CommandStream* stream, uint64_t id, const char* stage,
    const unsigned char* spirv, uint64_t spirv_size);


/**
 * Attach optional built-in shader identity metadata to a CreateShaderModule command.
 *
 * @param stream the command stream
 * @param shader_module_id the shader module id
 * @param family stable built-in shader family id
 * @param variant stable built-in shader variant id
 * @param version built-in shader contract version
 * @return whether the matching command was found and updated
 */
DVZ_EXPORT bool dvz_drp2_stream_shader_set_builtin_identity(
    DvzDrp2CommandStream* stream, uint64_t shader_module_id, const char* family,
    const char* variant, uint32_t version);



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
 * Return the default CreateRenderPipeline descriptor.
 *
 * @return initialized descriptor
 */
DVZ_EXPORT DvzDrp2RenderPipelineDesc dvz_drp2_render_pipeline_desc(void);



/**
 * Append a CreateRenderPipeline command from a descriptor.
 *
 * `binding_count` and `attr_count` must be at most `DVZ_DRP2_MAX_BINDINGS`. Count-zero pointer
 * arrays may be NULL; non-zero counts require all matching arrays except `binding_step_modes`,
 * which may be NULL to select per-vertex stepping. `bind_group_layout_count` must be at most
 * `DVZ_DRP2_MAX_BIND_GROUPS`. Oversized or missing arrays are rejected and no command is appended.
 *
 * @param stream the command stream
 * @param desc the render-pipeline descriptor
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_create_render_pipeline(
    DvzDrp2CommandStream* stream, const DvzDrp2RenderPipelineDesc* desc);


/**
 * Attach a bind-group layout to the most recently appended CreateRenderPipeline command.
 *
 * Descriptor-based creation can set bind-group layouts directly. This helper remains available
 * for incremental stream-building code that appends optional layout state after creation.
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
 * @param depth_compare_op depth compare operation
 * @return whether the most recent command was a CreateRenderPipeline and was updated
 */
DVZ_EXPORT bool dvz_drp2_stream_pipeline_set_depth_state(
    DvzDrp2CommandStream* stream, bool depth_write_enabled, DvzCompareOp depth_compare_op);


/**
 * Attach raster state to the most recently appended CreateRenderPipeline command.
 *
 * @param stream the command stream
 * @param cull_mode face culling mode
 * @param front_face front-face winding
 * @return whether the most recent command was a CreateRenderPipeline and was updated
 */
DVZ_EXPORT bool dvz_drp2_stream_pipeline_set_raster_state(
    DvzDrp2CommandStream* stream, DvzCullMode cull_mode, DvzFrontFace front_face);


/**
 * Set multisampling state on the most recently appended CreateRenderPipeline command.
 *
 * @param stream the command stream
 * @param sample_count raster sample count, with 0 treated as 1
 * @param alpha_to_coverage_enabled whether alpha-to-coverage is enabled
 * @return whether the most recent command was a CreateRenderPipeline and was updated
 */
DVZ_EXPORT bool dvz_drp2_stream_pipeline_set_multisampling(
    DvzDrp2CommandStream* stream, uint32_t sample_count, bool alpha_to_coverage_enabled);


/**
 * Set one color target format on the most recently appended CreateRenderPipeline command.
 *
 * @param stream the command stream
 * @param idx the color target index
 * @param format texture format token
 * @return whether the most recent command was a CreateRenderPipeline and was updated
 */
DVZ_EXPORT bool dvz_drp2_stream_pipeline_set_color_target(
    DvzDrp2CommandStream* stream, uint32_t idx, DvzFormat format);


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
    DvzDrp2CommandStream* stream, uint32_t idx, DvzBlendFactor src_color, DvzBlendFactor dst_color,
    DvzBlendOp color_op, DvzBlendFactor src_alpha, DvzBlendFactor dst_alpha, DvzBlendOp alpha_op,
    DvzColorMask color_write_mask);


/**
 * Attach optional built-in pipeline identity metadata to a CreateRenderPipeline command.
 *
 * @param stream the command stream
 * @param render_pipeline_id the render pipeline id
 * @param pipeline stable built-in pipeline id
 * @param version built-in pipeline contract version
 * @return whether the matching command was found and updated
 */
DVZ_EXPORT bool dvz_drp2_stream_pipeline_set_builtin_identity(
    DvzDrp2CommandStream* stream, uint64_t render_pipeline_id, const char* pipeline,
    uint32_t version);



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
 * Append a CreateSampler command with explicit min/mag filters.
 *
 * @param stream the command stream
 * @param id the sampler id
 * @param mag_filter magnification filter
 * @param min_filter minification filter
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_create_sampler_filter(
    DvzDrp2CommandStream* stream, uint64_t id, DvzDrp2FilterMode mag_filter,
    DvzDrp2FilterMode min_filter);



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
 * `entry_count` must be in `[1, DVZ_DRP2_MAX_BINDINGS]` and `entries` must not be NULL. Invalid
 * inputs are rejected and no command is appended.
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
 * `entry_count` must be in `[1, DVZ_DRP2_MAX_BINDINGS]` and `entries` must not be NULL. Invalid
 * inputs are rejected and no command is appended.
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
DVZ_EXPORT bool dvz_drp2_stream_write_buffer_base64(
    DvzDrp2CommandStream* stream, uint64_t buffer_id, uint64_t offset, uint64_t size,
    const char* data_base64);


/**
 * Append a WriteBuffer command from raw bytes.
 *
 * In-process callers that have a raw byte pointer should prefer this entry
 * point over dvz_drp2_stream_write_buffer_base64 (which takes a pre-encoded base64
 * string and is intended for JSON wire-loading paths).
 *
 * The stream owns a copy of `data`. The base64 string is computed lazily, only
 * if the stream is later serialized to JSON.
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
DVZ_EXPORT bool dvz_drp2_stream_write_texture_2d_base64(
    DvzDrp2CommandStream* stream, uint64_t texture_id, uint32_t mip_level, uint32_t width,
    uint32_t height, uint32_t bytes_per_row, uint32_t rows_per_image, const char* data_base64);


/**
 * Append a WriteTexture command for a full 2D mip level using borrowed raw bytes.
 *
 * The runtime path consumes the borrowed pointer directly with no base64 round-trip. JSON
 * serialization re-encodes lazily. The caller must keep `data` alive until the stream has executed
 * or has been serialized.
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
DVZ_EXPORT bool dvz_drp2_stream_write_texture_2d_borrowed(
    DvzDrp2CommandStream* stream, uint64_t texture_id, uint32_t mip_level, uint32_t width,
    uint32_t height, uint32_t bytes_per_row, uint32_t rows_per_image, const void* data);


/**
 * Append a WriteTexture command for a 2D sub-region using borrowed raw bytes.
 *
 * Mirrors `dvz_drp2_stream_write_texture_2d_borrowed` for the in-process runtime path.
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
DVZ_EXPORT bool dvz_drp2_stream_write_texture_2d_region_borrowed(
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
DVZ_EXPORT bool dvz_drp2_stream_write_texture_2d_region_base64(
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
DVZ_EXPORT bool dvz_drp2_stream_write_texture_3d_base64(
    DvzDrp2CommandStream* stream, uint64_t texture_id, uint32_t mip_level,
    uint32_t origin_x, uint32_t origin_y, uint32_t origin_z,
    uint32_t width, uint32_t height, uint32_t depth,
    uint32_t bytes_per_row, uint32_t rows_per_image, const char* data_base64);


/**
 * Append a WriteTexture command for a 3D sub-region using borrowed raw bytes.
 *
 * Mirrors `dvz_drp2_stream_write_texture_2d_borrowed` for 3D texture uploads.
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
DVZ_EXPORT bool dvz_drp2_stream_write_texture_3d_borrowed(
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
 * @param x normalized left coordinate in attachment space [0, 1]
 * @param y normalized top coordinate in attachment space [0, 1]
 * @param width normalized width in attachment space [0, 1]
 * @param height normalized height in attachment space [0, 1]
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
 * Set access intent on one color attachment of the most recent BeginRenderPass command.
 *
 * @param stream the command stream
 * @param attachment_index the color attachment index
 * @param access the attachment access intent
 * @return whether the most recent command was a BeginRenderPass and was updated
 */
DVZ_EXPORT bool dvz_drp2_stream_begin_render_pass_set_color_attachment_access(
    DvzDrp2CommandStream* stream, uint32_t attachment_index, DvzDrp2AttachmentAccess access);


/**
 * Set the resolve target on one color attachment of the most recent BeginRenderPass command.
 *
 * @param stream the command stream
 * @param attachment_index the color attachment index
 * @param resolve_texture_id the single-sample resolve texture id, or 0 to disable resolve
 * @param resolve_mode backend-native resolve mode, with 0 treated as average
 * @return whether the most recent command was a BeginRenderPass and was updated
 */
DVZ_EXPORT bool dvz_drp2_stream_begin_render_pass_set_color_attachment_resolve(
    DvzDrp2CommandStream* stream, uint32_t attachment_index, uint64_t resolve_texture_id,
    uint32_t resolve_mode);


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
 * Set access intent on the depth attachment of the most recent BeginRenderPass command.
 *
 * @param stream the command stream
 * @param access the depth attachment access intent
 * @return whether the most recent command was a BeginRenderPass and was updated
 */
DVZ_EXPORT bool dvz_drp2_stream_begin_render_pass_set_depth_access(
    DvzDrp2CommandStream* stream, DvzDrp2AttachmentAccess access);



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
 * @param x normalized left coordinate in attachment space [0, 1]
 * @param y normalized top coordinate in attachment space [0, 1]
 * @param width normalized width in attachment space [0, 1]
 * @param height normalized height in attachment space [0, 1]
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_set_viewport(
    DvzDrp2CommandStream* stream, uint64_t pass_id, float x, float y, float width, float height);



/**
 * Append a SetScissor command.
 *
 * @param stream the command stream
 * @param pass_id the render pass id
 * @param x normalized left coordinate in attachment space [0, 1]
 * @param y normalized top coordinate in attachment space [0, 1]
 * @param width normalized width in attachment space [0, 1]
 * @param height normalized height in attachment space [0, 1]
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
 * Append a ResourceBarrier command for a buffer range.
 *
 * The first active DRP2 barrier slice covers compute storage writes made visible to a later vertex
 * input or copy read in the same command encoder.
 *
 * @param stream the command stream
 * @param encoder_id the open command encoder id
 * @param buffer_id the buffer id
 * @param src_stage the producer stage, such as "COMPUTE"
 * @param src_access the producer access, such as "STORAGE_WRITE"
 * @param dst_stage the consumer stage, such as "VERTEX_INPUT" or "COPY"
 * @param dst_access the consumer access, such as "VERTEX_READ" or "COPY_READ"
 * @param offset the first byte in the synchronized range
 * @param size the synchronized byte size, or 0 for the rest of the buffer
 * @return whether the command was appended
 */
DVZ_EXPORT bool dvz_drp2_stream_resource_barrier(
    DvzDrp2CommandStream* stream, uint64_t encoder_id, uint64_t buffer_id,
    const char* src_stage, const char* src_access, const char* dst_stage, const char* dst_access,
    uint64_t offset, uint64_t size);



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
 * Serialize a command stream as JSON with write payloads referenced by index.
 *
 * WriteBuffer and WriteTexture commands backed by in-process raw bytes are serialized with a
 * `data_ref` index and `data_encoding: "wasm-memory"` instead of inline base64. The referenced
 * payloads are available through `dvz_drp2_stream_payload_*()`. Commands that only have a base64
 * payload keep the normal inline `data` field.
 *
 * @param stream the command stream
 * @param name the fixture name
 * @return an owned NUL-terminated JSON string
 */
DVZ_EXPORT char*
dvz_drp2_stream_json_payload_refs(const DvzDrp2CommandStream* stream, const char* name);


/**
 * Return the number of raw binary write payloads in a command stream.
 *
 * @param stream the command stream
 * @return the number of raw payload spans
 */
DVZ_EXPORT uint32_t dvz_drp2_stream_payload_count(const DvzDrp2CommandStream* stream);


/**
 * Return the command index owning a raw binary payload.
 *
 * @param stream the command stream
 * @param payload_index the payload index
 * @return the command index, or UINT32_MAX when not found
 */
DVZ_EXPORT uint32_t
dvz_drp2_stream_payload_command_index(const DvzDrp2CommandStream* stream, uint32_t payload_index);


/**
 * Return the raw binary payload pointer for a payload index.
 *
 * @param stream the command stream
 * @param payload_index the payload index
 * @return the borrowed payload pointer, or NULL when not found
 */
DVZ_EXPORT const void*
dvz_drp2_stream_payload_ptr(const DvzDrp2CommandStream* stream, uint32_t payload_index);


/**
 * Return the raw binary payload byte size for a payload index.
 *
 * @param stream the command stream
 * @param payload_index the payload index
 * @return the payload byte size, or 0 when not found
 */
DVZ_EXPORT uint64_t
dvz_drp2_stream_payload_size(const DvzDrp2CommandStream* stream, uint32_t payload_index);



/**
 * Destroy a JSON string returned by dvz_drp2_stream_json().
 *
 * @param json the JSON string
 */
DVZ_EXPORT void dvz_drp2_stream_json_destroy(char* json);

EXTERN_C_OFF
