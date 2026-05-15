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
#include "datoviz/vklite/sync.h"



#if DVZ_DRP2_HAS_VKLITE
/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static void _vklite_region(
    DvzImageRegion* region, uint32_t width, uint32_t height, uint32_t depth,
    uint32_t bytes_per_row, uint32_t rows_per_image)
{
    ANN(region);
    dvz_image_region(region);
    dvz_image_region_extent(region, width, height, depth);
    region->bufferRowLength = bytes_per_row / DVZ_DRP2_RGBA8_BYTES_PER_TEXEL;
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
        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    case DRP2_TEXTURE_ACCESS_DEPTH_ATTACHMENT:
        return VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
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
static void _vklite_texture_access_scope(
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
    case DRP2_TEXTURE_ACCESS_DEPTH_ATTACHMENT:
        *stage = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                 VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
        *access_mask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                       VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
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
    if (object->image_layout == layout)
        return;

    VkPipelineStageFlags2 src_stage = VK_PIPELINE_STAGE_2_NONE;
    VkAccessFlags2 src_access = 0;
    VkPipelineStageFlags2 dst_stage = VK_PIPELINE_STAGE_2_NONE;
    VkAccessFlags2 dst_access = 0;
    _vklite_texture_access_scope(
        _vklite_texture_access_from_layout(object), &src_stage, &src_access);
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

    if (command->u.write_buffer.data_raw != NULL)
    {
        dvz_buffer_upload(object->buffer, offset, size, command->u.write_buffer.data_raw);
    }
    else if (command->u.write_buffer.data_base64 != NULL)
    {
        uint8_t* data = NULL;
        if (!_dvz_b64_decode_exact(command->u.write_buffer.data_base64, size, &data))
            return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, command_index);
        dvz_buffer_upload(object->buffer, offset, size, data);
        dvz_free(data);
    }
    else
    {
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, command_index);
    }
    return _drp2_ok();
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
    dvz_buffer_upload(staging, 0, size, upload_src);
    dvz_free(decoded);

    DvzCommands* cmds = _vklite_owned_commands_create(state->runtime->device);
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
        command->u.write_texture.rows_per_image);
    _vklite_region_offset(
        &region, command->u.write_texture.origin_x, command->u.write_texture.origin_y,
        command->u.write_texture.origin_z);

    if (dvz_cmd_begin_result(cmds) != 0)
    {
        _vklite_owned_commands_destroy(cmds);
        dvz_buffer_destroy(staging);
        dvz_buffer_free(staging);
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }
    _vklite_transition_image_access(cmds, texture, DRP2_TEXTURE_ACCESS_TRANSFER_WRITE);
    dvz_cmd_copy_buffer_to_image(
        cmds, dvz_buffer_handle(staging), 0, dvz_image_handle(texture->images, 0),
        _vklite_texture_access_layout(DRP2_TEXTURE_ACCESS_TRANSFER_WRITE), &region);
    DvzDrp2ValidationResult result = _vklite_owned_commands_end_submit(cmds, command_index);
    if (!result.ok)
    {
        _vklite_owned_commands_destroy(cmds);
        dvz_buffer_destroy(staging);
        dvz_buffer_free(staging);
        return result;
    }

    _vklite_owned_commands_destroy(cmds);
    dvz_buffer_destroy(staging);
    dvz_buffer_free(staging);
    return _drp2_ok();
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

    DvzCommands* cmds = _vklite_owned_commands_create(state->runtime->device);
    if (cmds == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    VkBufferCopy region = {0};
    region.srcOffset = command->u.copy_buffer_to_buffer.src_offset;
    region.dstOffset = command->u.copy_buffer_to_buffer.dst_offset;
    region.size = command->u.copy_buffer_to_buffer.size;

    if (dvz_cmd_begin_result(cmds) != 0)
    {
        _vklite_owned_commands_destroy(cmds);
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }
    vkCmdCopyBuffer(
        dvz_commands_handle(cmds), dvz_buffer_handle(src->buffer), dvz_buffer_handle(dst->buffer),
        1, &region);
    DvzDrp2ValidationResult result = _vklite_owned_commands_end_submit(cmds, command_index);
    if (!result.ok)
    {
        _vklite_owned_commands_destroy(cmds);
        return result;
    }
    _vklite_owned_commands_destroy(cmds);
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

    DvzCommands* cmds = _vklite_owned_commands_create(state->runtime->device);
    if (cmds == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    DvzImageRegion region = {0};
    _vklite_region(
        &region, command->u.copy_buffer_to_texture.width,
        command->u.copy_buffer_to_texture.height, command->u.copy_buffer_to_texture.depth,
        command->u.copy_buffer_to_texture.bytes_per_row,
        command->u.copy_buffer_to_texture.rows_per_image);
    _vklite_region_offset(
        &region, command->u.copy_buffer_to_texture.dst_origin_x,
        command->u.copy_buffer_to_texture.dst_origin_y,
        command->u.copy_buffer_to_texture.dst_origin_z);

    if (dvz_cmd_begin_result(cmds) != 0)
    {
        _vklite_owned_commands_destroy(cmds);
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }
    _vklite_transition_image_access(cmds, dst, DRP2_TEXTURE_ACCESS_TRANSFER_WRITE);
    dvz_cmd_copy_buffer_to_image(
        cmds, dvz_buffer_handle(src->buffer), command->u.copy_buffer_to_texture.src_offset,
        dvz_image_handle(dst->images, 0),
        _vklite_texture_access_layout(DRP2_TEXTURE_ACCESS_TRANSFER_WRITE), &region);
    DvzDrp2ValidationResult result = _vklite_owned_commands_end_submit(cmds, command_index);
    if (!result.ok)
    {
        _vklite_owned_commands_destroy(cmds);
        return result;
    }

    _vklite_owned_commands_destroy(cmds);
    return _drp2_ok();
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

    DvzCommands* cmds = _vklite_owned_commands_create(state->runtime->device);
    if (cmds == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    DvzImageRegion region = {0};
    _vklite_region(
        &region, command->u.copy_texture_to_buffer.width,
        command->u.copy_texture_to_buffer.height, 1,
        command->u.copy_texture_to_buffer.bytes_per_row,
        command->u.copy_texture_to_buffer.rows_per_image);

    if (dvz_cmd_begin_result(cmds) != 0)
    {
        _vklite_owned_commands_destroy(cmds);
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }
    _vklite_transition_image_access(cmds, src, DRP2_TEXTURE_ACCESS_TRANSFER_READ);
    dvz_cmd_copy_image_to_buffer(
        cmds, dvz_image_handle(src->images, 0),
        _vklite_texture_access_layout(DRP2_TEXTURE_ACCESS_TRANSFER_READ), &region,
        dvz_buffer_handle(dst->buffer), command->u.copy_texture_to_buffer.dst_offset);
    DvzDrp2ValidationResult result = _vklite_owned_commands_end_submit(cmds, command_index);
    if (!result.ok)
    {
        _vklite_owned_commands_destroy(cmds);
        return result;
    }

    _vklite_owned_commands_destroy(cmds);
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

    DvzCommands* cmds = _vklite_owned_commands_create(state->runtime->device);
    if (cmds == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    DvzImageCopy* copy = dvz_image_copy_create();
    if (copy == NULL)
    {
        _vklite_owned_commands_destroy(cmds);
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

    if (dvz_cmd_begin_result(cmds) != 0)
    {
        dvz_image_copy_free(copy);
        _vklite_owned_commands_destroy(cmds);
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }
    _vklite_transition_image_access(cmds, src, DRP2_TEXTURE_ACCESS_TRANSFER_READ);
    _vklite_transition_image_access(cmds, dst, DRP2_TEXTURE_ACCESS_TRANSFER_WRITE);
    dvz_cmd_copy_image(cmds, copy);
    DvzDrp2ValidationResult result = _vklite_owned_commands_end_submit(cmds, command_index);
    if (!result.ok)
    {
        dvz_image_copy_free(copy);
        _vklite_owned_commands_destroy(cmds);
        return result;
    }

    dvz_image_copy_free(copy);
    _vklite_owned_commands_destroy(cmds);
    return _drp2_ok();
}



#endif
