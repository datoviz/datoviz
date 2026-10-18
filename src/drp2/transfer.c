/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  DRP2 vklite runtime transfers                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include <volk.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_base64.h"
#include "_runtime.h"
#include "_stream.h"
#include "../vklite/_buffers.h"
#include "datoviz/vklite/sync.h"



#if DVZ_DRP2_HAS_VKLITE
/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static void _vklite_region(
    DvzImageRegion* region, uint32_t width, uint32_t height, uint32_t depth,
    uint32_t bytes_per_row, uint32_t rows_per_image, uint32_t bytes_per_texel)
{
    ANN(region);
    dvz_image_region(region);
    dvz_image_region_extent(region, width, height, depth);
    region->bufferRowLength = bytes_per_row / bytes_per_texel;
    region->bufferImageHeight = rows_per_image;
}


static void _vklite_region_offset(
    DvzImageRegion* region, uint32_t x, uint32_t y, uint32_t z)
{
    ANN(region);
    dvz_image_region_offset(region, (int32_t)x, (int32_t)y, (int32_t)z);
}


/**
 * Return whether a texture format contains a depth component.
 *
 * @param format Vulkan image format
 * @return whether the format has depth
 */
static bool _vklite_format_has_depth(uint32_t format)
{
    switch ((VkFormat)format)
    {
    case VK_FORMAT_D16_UNORM:
    case VK_FORMAT_X8_D24_UNORM_PACK32:
    case VK_FORMAT_D32_SFLOAT:
    case VK_FORMAT_D16_UNORM_S8_UINT:
    case VK_FORMAT_D24_UNORM_S8_UINT:
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        return true;
    default:
        return false;
    }
}


/**
 * Return the image aspect mask for a texture object.
 *
 * @param object vklite texture object
 * @return Vulkan image aspect mask
 */
static VkImageAspectFlags _vklite_texture_aspect(const Drp2VkliteObject* object)
{
    ANN(object);
    return _vklite_format_has_depth(object->format) ? VK_IMAGE_ASPECT_DEPTH_BIT :
                                                      VK_IMAGE_ASPECT_COLOR_BIT;
}


/**
 * Return the Vulkan layout implied by one DRP2 texture access.
 *
 * @param access DRP2 texture access
 * @return Vulkan image layout
 */
VkImageLayout _vklite_texture_access_layout(Drp2TextureAccess access)
{
    switch (access)
    {
    case DRP2_TEXTURE_ACCESS_TRANSFER_READ:
        return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    case DRP2_TEXTURE_ACCESS_TRANSFER_WRITE:
        return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    case DRP2_TEXTURE_ACCESS_SAMPLED_READ:
        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    case DRP2_TEXTURE_ACCESS_COLOR_ATTACHMENT:
    case DRP2_TEXTURE_ACCESS_COLOR_ATTACHMENT_READ:
    case DRP2_TEXTURE_ACCESS_COLOR_ATTACHMENT_WRITE:
        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    case DRP2_TEXTURE_ACCESS_DEPTH_ATTACHMENT_READ:
        return VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
    case DRP2_TEXTURE_ACCESS_DEPTH_ATTACHMENT:
    case DRP2_TEXTURE_ACCESS_DEPTH_ATTACHMENT_WRITE:
        return VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
    case DRP2_TEXTURE_ACCESS_STORAGE_TEXTURE_READ:
    case DRP2_TEXTURE_ACCESS_STORAGE_TEXTURE_READ_WRITE:
        return VK_IMAGE_LAYOUT_GENERAL;
    default:
        return VK_IMAGE_LAYOUT_UNDEFINED;
    }
}


/**
 * Return pipeline stage and access masks implied by one DRP2 texture access.
 *
 * @param access DRP2 texture access
 * @param stage output Vulkan pipeline stage mask
 * @param access_mask output Vulkan access mask
 */
void _vklite_texture_access_scope(
    Drp2TextureAccess access, VkPipelineStageFlags2* stage, VkAccessFlags2* access_mask)
{
    ANN(stage);
    ANN(access_mask);
    switch (access)
    {
    case DRP2_TEXTURE_ACCESS_TRANSFER_READ:
        *stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        *access_mask = VK_ACCESS_2_TRANSFER_READ_BIT;
        break;
    case DRP2_TEXTURE_ACCESS_TRANSFER_WRITE:
        *stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        *access_mask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        break;
    case DRP2_TEXTURE_ACCESS_SAMPLED_READ:
        *stage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
        *access_mask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
        break;
    case DRP2_TEXTURE_ACCESS_COLOR_ATTACHMENT:
        *stage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        *access_mask =
            VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
        break;
    case DRP2_TEXTURE_ACCESS_COLOR_ATTACHMENT_READ:
        *stage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        *access_mask = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT;
        break;
    case DRP2_TEXTURE_ACCESS_COLOR_ATTACHMENT_WRITE:
        *stage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        *access_mask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
        break;
    case DRP2_TEXTURE_ACCESS_DEPTH_ATTACHMENT:
        *stage = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                 VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
        *access_mask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                       VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;
    case DRP2_TEXTURE_ACCESS_DEPTH_ATTACHMENT_READ:
        *stage = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                 VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT |
                 VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
        *access_mask =
            VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
        break;
    case DRP2_TEXTURE_ACCESS_DEPTH_ATTACHMENT_WRITE:
        *stage = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                 VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
        *access_mask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;
    case DRP2_TEXTURE_ACCESS_STORAGE_TEXTURE_READ:
        *stage = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT |
                 VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
                 VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
        *access_mask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT;
        break;
    case DRP2_TEXTURE_ACCESS_STORAGE_TEXTURE_READ_WRITE:
        *stage = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT |
                 VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
                 VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
        *access_mask =
            VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
        break;
    default:
        *stage = VK_PIPELINE_STAGE_2_NONE;
        *access_mask = 0;
        break;
    }
}


/**
 * Infer the previous access class from a tracked Vulkan image layout.
 *
 * @param object vklite texture object
 * @return inferred DRP2 texture access
 */
static Drp2TextureAccess _vklite_texture_access_from_layout(const Drp2VkliteObject* object)
{
    ANN(object);
    switch (object->image_layout)
    {
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        return DRP2_TEXTURE_ACCESS_TRANSFER_READ;
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        return DRP2_TEXTURE_ACCESS_TRANSFER_WRITE;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        return DRP2_TEXTURE_ACCESS_SAMPLED_READ;
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        return DRP2_TEXTURE_ACCESS_COLOR_ATTACHMENT;
    case VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL:
        return _vklite_format_has_depth(object->format) ? DRP2_TEXTURE_ACCESS_DEPTH_ATTACHMENT :
                                                          DRP2_TEXTURE_ACCESS_COLOR_ATTACHMENT;
    case VK_IMAGE_LAYOUT_GENERAL:
        return DRP2_TEXTURE_ACCESS_STORAGE_TEXTURE_READ_WRITE;
    default:
        return DRP2_TEXTURE_ACCESS_NONE;
    }
}


/**
 * Transition a vklite texture object to the layout required for one DRP2 access.
 *
 * @param cmds command buffer wrapper
 * @param object vklite texture object
 * @param access requested DRP2 texture access
 */
void _vklite_transition_image_access(
    DvzCommands* cmds, Drp2VkliteObject* object, Drp2TextureAccess access)
{
    ANN(cmds);
    ANN(object);
    ANN(object->images);
    VkImageLayout layout = _vklite_texture_access_layout(access);
    Drp2TextureAccess previous_access = object->texture_access != DRP2_TEXTURE_ACCESS_NONE
                                            ? object->texture_access
                                            : _vklite_texture_access_from_layout(object);
    if (object->image_layout == layout && previous_access == access)
        return;

    VkPipelineStageFlags2 src_stage = VK_PIPELINE_STAGE_2_NONE;
    VkAccessFlags2 src_access = 0;
    VkPipelineStageFlags2 dst_stage = VK_PIPELINE_STAGE_2_NONE;
    VkAccessFlags2 dst_access = 0;
    _vklite_texture_access_scope(previous_access, &src_stage, &src_access);
    _vklite_texture_access_scope(access, &dst_stage, &dst_access);

    DvzBarriers barriers = {0};
    dvz_barriers(&barriers);
    DvzBarrierImage* bimg = dvz_barriers_image(&barriers, dvz_image_handle(object->images, 0));
    ANN(bimg);
    dvz_barrier_image_stage(bimg, src_stage, dst_stage);
    dvz_barrier_image_access(bimg, src_access, dst_access);
    dvz_barrier_image_layout(bimg, object->image_layout, layout);
    dvz_barrier_image_aspect(bimg, _vklite_texture_aspect(object));
    dvz_cmd_barriers(cmds, &barriers);
    object->image_layout = layout;
    object->texture_access = access;
}


static bool _vklite_create_staging_buffer(
    Drp2VkliteState* state, uint64_t size, DvzBuffer** buffer, VkBufferUsageFlags usage)
{
    ANN(state);
    ANN(buffer);
    *buffer = NULL;
    if (size == 0)
        return false;

    DvzBuffer* out = dvz_buffer_create_wrapper();
    if (out == NULL)
        return false;

    dvz_buffer(state->runtime->device, state->runtime->allocator, out);
    dvz_buffer_size(out, size);
    dvz_buffer_usage(out, usage);
    dvz_buffer_flags(out, DVZ_ALLOC_HOST_ACCESS_SEQUENTIAL_WRITE);
    if (dvz_buffer_create(out) != 0)
    {
        dvz_buffer_free(out);
        return false;
    }
    *buffer = out;
    return true;
}


/**
 * Order one borrowed-frame buffer update against earlier or later main-queue accesses.
 *
 * @param cmds borrowed recording wrapper outside a rendering scope
 * @param buffer destination buffer handle
 * @param offset first updated byte
 * @param size number of updated bytes
 * @param before whether this is the dependency preceding the transfer write
 */
static void _vklite_buffer_write_barrier(
    DvzCommands* cmds, VkBuffer buffer, uint64_t offset, uint64_t size, bool before)
{
    DvzBarriers barriers = {0};
    dvz_barriers(&barriers);
    DvzBarrierBuffer* barrier = dvz_barriers_buffer(&barriers, buffer, offset, size);
    ANN(barrier);
    VkPipelineStageFlags2 stages = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    VkAccessFlags2 accesses = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
    dvz_barrier_buffer_stage(
        barrier, before ? stages : VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        before ? VK_PIPELINE_STAGE_2_TRANSFER_BIT : stages);
    dvz_barrier_buffer_access(
        barrier, before ? accesses : VK_ACCESS_2_TRANSFER_WRITE_BIT,
        before ? VK_ACCESS_2_TRANSFER_WRITE_BIT : accesses);
    dvz_cmd_barriers(cmds, &barriers);
}



/**
 * Record a borrowed-frame write without mapping storage still read by pending GPU frames.
 *
 * @param state runtime state with an active borrowed recording command buffer
 * @param buffer destination buffer
 * @param offset first updated byte
 * @param size number of updated bytes
 * @param data source bytes copied during recording
 * @return whether recording and any staging-resource retention succeeded
 */
static bool _vklite_record_buffer_write(
    Drp2VkliteState* state, DvzBuffer* buffer, uint64_t offset, uint64_t size, const void* data)
{
    ANN(state);
    ANN(buffer);
    ANN(data);
    if ((dvz_buffer_usage_value(buffer) & VK_BUFFER_USAGE_TRANSFER_DST_BIT) == 0)
    {
        log_error("borrowed frame buffer writes require transfer-destination storage");
        return false;
    }
    // Vulkan transfer commands and their buffer barriers cannot be recorded inside rendering.
    for (uint32_t i = 0; i < state->count; i++)
        if (!state->objects[i].destroyed && state->objects[i].kind == DRP2_OBJECT_RENDER_PASS &&
            state->objects[i].commands != NULL)
        {
            log_error("borrowed frame buffer writes must precede or follow render passes");
            return false;
        }

    DvzCommands* cmds = _vklite_borrowed_frame_commands_create(
        state->runtime->device, state->active_borrowed_command_buffer);
    if (cmds == NULL)
        return false;
    DvzBuffer* staging = NULL;
    const bool inline_update = size <= 65536 && offset % 4 == 0 && size % 4 == 0;
    if (!inline_update)
    {
        // Reserve before recording: a failed allocation must never free a recorded staging use.
        if (!_vklite_deferred_reserve(state, 1) ||
            !_vklite_create_staging_buffer(state, size, &staging, VK_BUFFER_USAGE_TRANSFER_SRC_BIT))
        {
            _vklite_borrowed_frame_commands_free(cmds);
            return false;
        }
        if (dvz_allocator_copy_to(staging->allocator, staging->alloc, 0, data, size) != 0)
        {
            dvz_buffer_destroy(staging);
            dvz_buffer_free(staging);
            _vklite_borrowed_frame_commands_free(cmds);
            return false;
        }
        Drp2VkliteObject retirement = {.kind = DRP2_OBJECT_BUFFER, .buffer = staging};
        if (!_vklite_defer_destroy_object(
                state, &retirement, state->active_borrowed_command_buffer))
        {
            _vklite_destroy_object(&retirement);
            _vklite_borrowed_frame_commands_free(cmds);
            return false;
        }
    }

    VkBuffer destination = dvz_buffer_handle(buffer);
    _vklite_buffer_write_barrier(cmds, destination, offset, size, true);
    if (inline_update)
        vkCmdUpdateBuffer(dvz_commands_handle(cmds), destination, offset, size, data);
    else
    {
        VkBufferCopy copy = {.dstOffset = offset, .size = size};
        vkCmdCopyBuffer(
            dvz_commands_handle(cmds), dvz_buffer_handle(staging), destination, 1, &copy);
    }
    _vklite_buffer_write_barrier(cmds, destination, offset, size, false);
    _vklite_borrowed_frame_commands_free(cmds);
    return true;
}



DvzDrp2ValidationResult _vklite_write_buffer(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);
    Drp2VkliteObject* object = _vklite_find(state, command->u.write_buffer.buffer_id);
    if (object == NULL || object->buffer == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    uint64_t offset = command->u.write_buffer.offset;
    uint64_t size   = command->u.write_buffer.size;

    /* WebGPU-shaped: size==0 is a valid no-op. */
    if (size == 0)
        return _drp2_ok();

    const void* data = command->u.write_buffer.data_raw;
    uint8_t* decoded = NULL;
    if (data == NULL && command->u.write_buffer.data_base64 != NULL)
    {
        if (!_dvz_b64_decode_exact(command->u.write_buffer.data_base64, size, &decoded))
            return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, command_index);
        data = decoded;
    }
    if (data == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, command_index);

    bool ok = true;
    if (state->active_borrowed_command_buffer != VK_NULL_HANDLE)
    {
        // Only the active recording authorizes commands. The retirement owner may refer to an
        // earlier submitted frame and must never be used as a recording destination.
        ok = _vklite_record_buffer_write(state, object->buffer, offset, size, data);
    }
    else
        dvz_buffer_upload(object->buffer, offset, size, data);
    dvz_free(decoded);
    return ok ? _drp2_ok() : _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
}


DvzDrp2ValidationResult _vklite_write_texture(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);
    Drp2VkliteObject* texture = _vklite_find(state, command->u.write_texture.texture_id);
    if (texture == NULL || texture->images == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    uint64_t size = _drp2_texture_layout_size(
        command->u.write_texture.depth, command->u.write_texture.bytes_per_row,
        command->u.write_texture.rows_per_image);
    uint32_t bytes_per_texel = 0;
    if (!_drp2_texture_format_bytes_per_texel(texture->format, &bytes_per_texel))
        return _drp2_fail(DVZ_DRP2_VALIDATION_USAGE, command_index);

    /* WebGPU-shaped: zero-size write is a valid no-op. */
    if (size == 0)
        return _drp2_ok();

    const void* upload_src = command->u.write_texture.data_raw;
    uint8_t* decoded = NULL;
    if (upload_src == NULL)
    {
        if (command->u.write_texture.data_base64 == NULL ||
            !_dvz_b64_decode_exact(command->u.write_texture.data_base64, size, &decoded))
            return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, command_index);
        upload_src = decoded;
    }

    DvzBuffer* staging = NULL;
    if (!_vklite_create_staging_buffer(state, size, &staging, VK_BUFFER_USAGE_TRANSFER_SRC_BIT))
    {
        dvz_free(decoded);
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }
    if (dvz_allocator_copy_to(staging->allocator, staging->alloc, 0, upload_src, size) != 0)
    {
        dvz_free(decoded);
        dvz_buffer_destroy(staging);
        dvz_buffer_free(staging);
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }
    dvz_free(decoded);

    bool borrowed = false;
    DvzCommands* cmds = _vklite_commands_begin(state, &borrowed);
    if (cmds == NULL)
    {
        dvz_buffer_destroy(staging);
        dvz_buffer_free(staging);
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }

    DvzImageRegion region = {0};
    _vklite_region(
        &region, command->u.write_texture.width, command->u.write_texture.height,
        command->u.write_texture.depth, command->u.write_texture.bytes_per_row,
        command->u.write_texture.rows_per_image, bytes_per_texel);
    _vklite_region_offset(
        &region, command->u.write_texture.origin_x, command->u.write_texture.origin_y,
        command->u.write_texture.origin_z);

    if (borrowed)
    {
        Drp2VkliteObject retirement = {.kind = DRP2_OBJECT_BUFFER, .buffer = staging};
        if (!_vklite_deferred_reserve(state, 1) ||
            !_vklite_defer_destroy_object(
                state, &retirement, state->active_borrowed_command_buffer))
        {
            _vklite_destroy_object(&retirement);
            _vklite_commands_destroy(cmds, borrowed);
            return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
        }
    }
    _vklite_transition_image_access(cmds, texture, DRP2_TEXTURE_ACCESS_TRANSFER_WRITE);
    dvz_cmd_copy_buffer_to_image(
        cmds, dvz_buffer_handle(staging), 0, dvz_image_handle(texture->images, 0),
        _vklite_texture_access_layout(DRP2_TEXTURE_ACCESS_TRANSFER_WRITE), &region);
    DvzDrp2ValidationResult result =
        _vklite_commands_end_submit(cmds, borrowed, command_index);
    _vklite_commands_destroy(cmds, borrowed);
    if (!borrowed)
    {
        dvz_buffer_destroy(staging);
        dvz_buffer_free(staging);
    }
    return result;
}


DvzDrp2ValidationResult _vklite_copy_buffer_to_buffer(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);
    Drp2VkliteObject* src = _vklite_find(state, command->u.copy_buffer_to_buffer.src_buffer_id);
    Drp2VkliteObject* dst = _vklite_find(state, command->u.copy_buffer_to_buffer.dst_buffer_id);
    if (src == NULL || src->buffer == NULL || dst == NULL || dst->buffer == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    bool borrowed = false;
    DvzCommands* cmds = _vklite_commands_begin(state, &borrowed);
    if (cmds == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    VkBufferCopy region = {0};
    region.srcOffset = command->u.copy_buffer_to_buffer.src_offset;
    region.dstOffset = command->u.copy_buffer_to_buffer.dst_offset;
    region.size = command->u.copy_buffer_to_buffer.size;

    vkCmdCopyBuffer(
        dvz_commands_handle(cmds), dvz_buffer_handle(src->buffer), dvz_buffer_handle(dst->buffer),
        1, &region);
    DvzDrp2ValidationResult result = _vklite_commands_end_submit(cmds, borrowed, command_index);
    if (!result.ok)
    {
        _vklite_commands_destroy(cmds, borrowed);
        return result;
    }
    _vklite_commands_destroy(cmds, borrowed);
    return _drp2_ok();
}


DvzDrp2ValidationResult _vklite_copy_buffer_to_texture(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);
    Drp2VkliteObject* src = _vklite_find(state, command->u.copy_buffer_to_texture.src_buffer_id);
    Drp2VkliteObject* dst = _vklite_find(state, command->u.copy_buffer_to_texture.dst_texture_id);
    if (src == NULL || src->buffer == NULL || dst == NULL || dst->images == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    uint32_t bytes_per_texel = 0;
    if (!_drp2_texture_format_bytes_per_texel(dst->format, &bytes_per_texel))
        return _drp2_fail(DVZ_DRP2_VALIDATION_USAGE, command_index);

    if (src->external_timeline_pending && state->active_borrowed_command_buffer != VK_NULL_HANDLE)
    {
        log_error("external timeline copies require standalone execution outside borrowed frames");
        return _drp2_fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
    }

    bool borrowed = false;
    DvzCommands* cmds = _vklite_commands_begin(state, &borrowed);
    if (cmds == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    DvzImageRegion region = {0};
    _vklite_region(
        &region, command->u.copy_buffer_to_texture.width,
        command->u.copy_buffer_to_texture.height, command->u.copy_buffer_to_texture.depth,
        command->u.copy_buffer_to_texture.bytes_per_row,
        command->u.copy_buffer_to_texture.rows_per_image, bytes_per_texel);
    _vklite_region_offset(
        &region, command->u.copy_buffer_to_texture.dst_origin_x,
        command->u.copy_buffer_to_texture.dst_origin_y,
        command->u.copy_buffer_to_texture.dst_origin_z);

    bool timeline_handoff = src->external_timeline_pending;
    DvzSemaphore* timeline_semaphore = src->external_timeline_semaphore;
    Drp2Object* semantic_src = _drp2_find_any_object(
        state->runtime->semantic_state, command->u.copy_buffer_to_texture.src_buffer_id);
    if (timeline_handoff &&
        (timeline_semaphore == NULL || semantic_src == NULL ||
         !semantic_src->external_timeline_pending))
    {
        _vklite_commands_destroy(cmds, borrowed);
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }

    if (timeline_handoff)
    {
        DvzBarriers acquire = {0};
        dvz_barriers(&acquire);
        DvzBarrierBuffer* buffer_barrier = dvz_barriers_buffer(
            &acquire, dvz_buffer_handle(src->buffer), 0, (VkDeviceSize)semantic_src->size);
        dvz_barrier_buffer_stage(
            buffer_barrier, VK_PIPELINE_STAGE_2_NONE, VK_PIPELINE_STAGE_2_TRANSFER_BIT);
        dvz_barrier_buffer_access(
            buffer_barrier, VK_ACCESS_2_NONE, VK_ACCESS_2_TRANSFER_READ_BIT);
        dvz_cmd_barriers(cmds, &acquire);
    }
    _vklite_transition_image_access(cmds, dst, DRP2_TEXTURE_ACCESS_TRANSFER_WRITE);
    dvz_cmd_copy_buffer_to_image(
        cmds, dvz_buffer_handle(src->buffer), command->u.copy_buffer_to_texture.src_offset,
        dvz_image_handle(dst->images, 0),
        _vklite_texture_access_layout(DRP2_TEXTURE_ACCESS_TRANSFER_WRITE), &region);

    if (timeline_handoff)
    {
        DvzBarriers release = {0};
        dvz_barriers(&release);
        DvzBarrierBuffer* buffer_barrier = dvz_barriers_buffer(
            &release, dvz_buffer_handle(src->buffer), 0, (VkDeviceSize)semantic_src->size);
        dvz_barrier_buffer_stage(
            buffer_barrier, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_PIPELINE_STAGE_2_NONE);
        dvz_barrier_buffer_access(
            buffer_barrier, VK_ACCESS_2_TRANSFER_READ_BIT, VK_ACCESS_2_NONE);
        dvz_cmd_barriers(cmds, &release);
    }

    DvzDrp2ValidationResult result = _drp2_ok();
    if (timeline_handoff)
    {
        if (_vklite_owned_commands_end(cmds) != 0)
        {
            result = _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
        }
        else
        {
            DvzQueue* queue = dvz_device_queue(state->runtime->device, DVZ_QUEUE_MAIN);
            DvzSubmit* submit = queue != NULL ? dvz_submit_create_wrapper() : NULL;
            if (submit == NULL)
            {
                result = _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
            }
            else
            {
                dvz_submit(submit);
                dvz_submit_wait(
                    submit, dvz_semaphore_handle(timeline_semaphore),
                    semantic_src->external_timeline_wait_value, VK_PIPELINE_STAGE_2_TRANSFER_BIT);
                dvz_submit_command(submit, dvz_commands_handle(cmds));
                dvz_submit_signal(
                    submit, dvz_semaphore_handle(timeline_semaphore),
                    semantic_src->external_timeline_signal_value, VK_PIPELINE_STAGE_2_TRANSFER_BIT);
                if (dvz_submit_send(submit, dvz_queue_handle(queue), VK_NULL_HANDLE) != VK_SUCCESS)
                    result = _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
                else
                    dvz_queue_wait(queue);
                dvz_submit_free(submit);
            }
        }
    }
    else
    {
        result = _vklite_commands_end_submit(cmds, borrowed, command_index);
    }

    if (result.ok && timeline_handoff)
    {
        src->external_timeline_pending = false;
        src->external_timeline_semaphore = NULL;
        semantic_src->external_timeline_pending = false;
    }
    _vklite_commands_destroy(cmds, borrowed);
    return result;
}


DvzDrp2ValidationResult _vklite_copy_texture_to_buffer(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);
    Drp2VkliteObject* src = _vklite_find(state, command->u.copy_texture_to_buffer.src_texture_id);
    Drp2VkliteObject* dst = _vklite_find(state, command->u.copy_texture_to_buffer.dst_buffer_id);
    if (src == NULL || src->images == NULL || dst == NULL || dst->buffer == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    uint32_t bytes_per_texel = 0;
    if (!_drp2_texture_format_bytes_per_texel(src->format, &bytes_per_texel))
        return _drp2_fail(DVZ_DRP2_VALIDATION_USAGE, command_index);

    bool borrowed = false;
    DvzCommands* cmds = _vklite_commands_begin(state, &borrowed);
    if (cmds == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    DvzImageRegion region = {0};
    _vklite_region(
        &region, command->u.copy_texture_to_buffer.width,
        command->u.copy_texture_to_buffer.height, 1,
        command->u.copy_texture_to_buffer.bytes_per_row,
        command->u.copy_texture_to_buffer.rows_per_image, bytes_per_texel);

    _vklite_transition_image_access(cmds, src, DRP2_TEXTURE_ACCESS_TRANSFER_READ);
    dvz_cmd_copy_image_to_buffer(
        cmds, dvz_image_handle(src->images, 0),
        _vklite_texture_access_layout(DRP2_TEXTURE_ACCESS_TRANSFER_READ), &region,
        dvz_buffer_handle(dst->buffer), command->u.copy_texture_to_buffer.dst_offset);
    DvzDrp2ValidationResult result = _vklite_commands_end_submit(cmds, borrowed, command_index);
    if (!result.ok)
    {
        _vklite_commands_destroy(cmds, borrowed);
        return result;
    }

    _vklite_commands_destroy(cmds, borrowed);
    return _drp2_ok();
}


DvzDrp2ValidationResult _vklite_copy_texture_to_texture(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);
    Drp2VkliteObject* src = _vklite_find(state, command->u.copy_texture_to_texture.src_texture_id);
    Drp2VkliteObject* dst = _vklite_find(state, command->u.copy_texture_to_texture.dst_texture_id);
    if (src == NULL || src->images == NULL || dst == NULL || dst->images == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    bool borrowed = false;
    DvzCommands* cmds = _vklite_commands_begin(state, &borrowed);
    if (cmds == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    DvzImageCopy* copy = dvz_image_copy_create();
    if (copy == NULL)
    {
        _vklite_commands_destroy(cmds, borrowed);
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }
    dvz_cmd_copy_source(
        copy, dvz_image_handle(src->images, 0),
        _vklite_texture_access_layout(DRP2_TEXTURE_ACCESS_TRANSFER_READ),
        (int32_t)command->u.copy_texture_to_texture.src_origin_x,
        (int32_t)command->u.copy_texture_to_texture.src_origin_y,
        (int32_t)command->u.copy_texture_to_texture.src_origin_z,
        command->u.copy_texture_to_texture.width, command->u.copy_texture_to_texture.height,
        command->u.copy_texture_to_texture.depth);
    dvz_cmd_copy_destination(
        copy, dvz_image_handle(dst->images, 0),
        _vklite_texture_access_layout(DRP2_TEXTURE_ACCESS_TRANSFER_WRITE),
        (int32_t)command->u.copy_texture_to_texture.dst_origin_x,
        (int32_t)command->u.copy_texture_to_texture.dst_origin_y,
        (int32_t)command->u.copy_texture_to_texture.dst_origin_z);

    _vklite_transition_image_access(cmds, src, DRP2_TEXTURE_ACCESS_TRANSFER_READ);
    _vklite_transition_image_access(cmds, dst, DRP2_TEXTURE_ACCESS_TRANSFER_WRITE);
    dvz_cmd_copy_image(cmds, copy);
    DvzDrp2ValidationResult result = _vklite_commands_end_submit(cmds, borrowed, command_index);
    if (!result.ok)
    {
        dvz_image_copy_free(copy);
        _vklite_commands_destroy(cmds, borrowed);
        return result;
    }

    dvz_image_copy_free(copy);
    _vklite_commands_destroy(cmds, borrowed);
    return _drp2_ok();
}



#endif
