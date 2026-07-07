/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  DRP2 vklite runtime passes                                                                   */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <volk.h>

#include "_assertions.h"
#include "_runtime.h"
#include "_stream.h"
#include "datoviz/vklite/sync.h"



#if DVZ_DRP2_HAS_VKLITE
/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Clamp a normalized float to the closed [0, 1] interval.
 *
 * @param value the input value
 * @return the clamped value
 */
static float _clamp_unit(float value)
{
    if (value < 0.0f)
        return 0.0f;
    if (value > 1.0f)
        return 1.0f;
    return value;
}


/**
 * Convert a normalized [0, 1] viewport to an in-bounds pixel render area.
 *
 * @param target_width render-target width in pixels
 * @param target_height render-target height in pixels
 * @param viewport normalized x/y/width/height
 * @param out_x output left pixel coordinate
 * @param out_y output top pixel coordinate
 * @param out_width output width in pixels
 * @param out_height output height in pixels
 */
static void _render_area_from_viewport(
    uint32_t target_width, uint32_t target_height, const float viewport[4], uint32_t* out_x,
    uint32_t* out_y, uint32_t* out_width, uint32_t* out_height)
{
    ANN(viewport);
    ANN(out_x);
    ANN(out_y);
    ANN(out_width);
    ANN(out_height);

    float x0 = _clamp_unit(viewport[0]);
    float y0 = _clamp_unit(viewport[1]);
    float x1 = _clamp_unit(viewport[0] + viewport[2]);
    float y1 = _clamp_unit(viewport[1] + viewport[3]);
    if (x1 < x0)
        x1 = x0;
    if (y1 < y0)
        y1 = y0;

    uint32_t px0 = (uint32_t)(x0 * (float)target_width);
    uint32_t py0 = (uint32_t)(y0 * (float)target_height);
    uint32_t px1 = (uint32_t)(x1 * (float)target_width);
    uint32_t py1 = (uint32_t)(y1 * (float)target_height);

    if (px0 > target_width)
        px0 = target_width;
    if (py0 > target_height)
        py0 = target_height;
    if (px1 > target_width)
        px1 = target_width;
    if (py1 > target_height)
        py1 = target_height;

    if (px1 <= px0 && target_width > px0)
        px1 = px0 + 1;
    if (py1 <= py0 && target_height > py0)
        py1 = py0 + 1;

    *out_x = px0;
    *out_y = py0;
    *out_width = px1 > px0 ? px1 - px0 : 0;
    *out_height = py1 > py0 ? py1 - py0 : 0;
}



/**
 * Clamp a framebuffer-coordinate rectangle to an in-bounds pixel render area.
 *
 * @param target_width render-target width in pixels
 * @param target_height render-target height in pixels
 * @param rect framebuffer x/y/width/height
 * @param out_x output left pixel coordinate
 * @param out_y output top pixel coordinate
 * @param out_width output width in pixels
 * @param out_height output height in pixels
 */
static void _render_area_from_framebuffer_rect(
    uint32_t target_width, uint32_t target_height, const float rect[4], uint32_t* out_x,
    uint32_t* out_y, uint32_t* out_width, uint32_t* out_height)
{
    ANN(rect);
    ANN(out_x);
    ANN(out_y);
    ANN(out_width);
    ANN(out_height);

    float x0 = rect[0] > 0.0f ? rect[0] : 0.0f;
    float y0 = rect[1] > 0.0f ? rect[1] : 0.0f;
    float x1 = rect[0] + rect[2];
    float y1 = rect[1] + rect[3];
    if (x1 < x0)
        x1 = x0;
    if (y1 < y0)
        y1 = y0;
    if (x1 > (float)target_width)
        x1 = (float)target_width;
    if (y1 > (float)target_height)
        y1 = (float)target_height;

    uint32_t px0 = (uint32_t)(x0 + 0.5f);
    uint32_t py0 = (uint32_t)(y0 + 0.5f);
    uint32_t px1 = (uint32_t)(x1 + 0.5f);
    uint32_t py1 = (uint32_t)(y1 + 0.5f);
    if (px0 > target_width)
        px0 = target_width;
    if (py0 > target_height)
        py0 = target_height;
    if (px1 > target_width)
        px1 = target_width;
    if (py1 > target_height)
        py1 = target_height;
    if (px1 <= px0 && target_width > px0)
        px1 = px0 + 1;
    if (py1 <= py0 && target_height > py0)
        py1 = py0 + 1;

    *out_x = px0;
    *out_y = py0;
    *out_width = px1 > px0 ? px1 - px0 : 0;
    *out_height = py1 > py0 ? py1 - py0 : 0;
}


static void _render_area_from_pixel_rect(
    uint32_t target_width, uint32_t target_height, const uint32_t rect[4], uint32_t* out_x,
    uint32_t* out_y, uint32_t* out_width, uint32_t* out_height)
{
    ANN(rect);
    ANN(out_x);
    ANN(out_y);
    ANN(out_width);
    ANN(out_height);

    uint32_t x = rect[0];
    uint32_t y = rect[1];
    uint32_t width = rect[2];
    uint32_t height = rect[3];
    if (width == 0 || height == 0)
    {
        x = 0;
        y = 0;
        width = target_width;
        height = target_height;
    }
    if (x > target_width)
        x = target_width;
    if (y > target_height)
        y = target_height;
    if (width > target_width - x)
        width = target_width - x;
    if (height > target_height - y)
        height = target_height - y;

    *out_x = x;
    *out_y = y;
    *out_width = width;
    *out_height = height;
}



static void _framebuffer_rect_or_render_area(
    const float rect[4], uint32_t render_x, uint32_t render_y, uint32_t render_width,
    uint32_t render_height, float out[4])
{
    ANN(rect);
    ANN(out);
    out[0] = rect[0];
    out[1] = rect[1];
    out[2] = rect[2];
    out[3] = rect[3];
    if (out[2] <= 0.0f || out[3] <= 0.0f)
    {
        out[0] = (float)render_x;
        out[1] = (float)render_y;
        out[2] = (float)render_width;
        out[3] = (float)render_height;
    }
}


/**
 * Convert DRP2 depth attachment access to vklite texture access.
 *
 * @param access DRP2 attachment access intent.
 * @return vklite texture access.
 */
static Drp2TextureAccess _depth_texture_access(DvzDrp2AttachmentAccess access)
{
    switch (access)
    {
    case DVZ_DRP2_ATTACHMENT_ACCESS_READ:
        return DRP2_TEXTURE_ACCESS_DEPTH_ATTACHMENT_READ;
    case DVZ_DRP2_ATTACHMENT_ACCESS_WRITE:
        return DRP2_TEXTURE_ACCESS_DEPTH_ATTACHMENT_WRITE;
    case DVZ_DRP2_ATTACHMENT_ACCESS_READ_WRITE:
    default:
        return DRP2_TEXTURE_ACCESS_DEPTH_ATTACHMENT;
    }
}



/**
 * Convert DRP2 color attachment access to vklite texture access.
 *
 * @param access DRP2 attachment access intent
 * @return vklite texture access
 */
static Drp2TextureAccess _color_texture_access(DvzDrp2AttachmentAccess access)
{
    switch (access)
    {
    case DVZ_DRP2_ATTACHMENT_ACCESS_READ:
        return DRP2_TEXTURE_ACCESS_COLOR_ATTACHMENT_READ;
    case DVZ_DRP2_ATTACHMENT_ACCESS_WRITE:
        return DRP2_TEXTURE_ACCESS_COLOR_ATTACHMENT_WRITE;
    case DVZ_DRP2_ATTACHMENT_ACCESS_READ_WRITE:
    default:
        return DRP2_TEXTURE_ACCESS_COLOR_ATTACHMENT;
    }
}



/**
 * Return attachment access expanded for a load operation that reads existing contents.
 *
 * @param access declared attachment access
 * @param load_op declared load operation
 * @return effective access used for synchronization
 */
static DvzDrp2AttachmentAccess _attachment_effective_access(
    DvzDrp2AttachmentAccess access, DvzDrp2AttachmentLoadOp load_op)
{
    if (access == DVZ_DRP2_ATTACHMENT_ACCESS_WRITE &&
        load_op == DVZ_DRP2_ATTACHMENT_LOAD_LOAD)
    {
        return DVZ_DRP2_ATTACHMENT_ACCESS_READ_WRITE;
    }
    return access;
}



/**
 * Return whether a Vulkan format carries a depth aspect.
 *
 * @param format backend-native texture format enum value
 * @return whether the format is a depth format
 */
static bool _vklite_pass_format_has_depth(uint32_t format)
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
 * Find the layout entry that declares one bind-group binding's access.
 *
 * @param layout vklite bind-group layout object
 * @param binding binding index
 * @return layout entry, or NULL when not found
 */
static const DvzDrp2BindGroupLayoutEntry*
_bind_group_layout_entry(const Drp2VkliteObject* layout, uint32_t binding)
{
    ANN(layout);
    for (uint32_t i = 0; i < layout->layout_entry_count; i++)
    {
        if (layout->layout_entries[i].binding == binding)
            return &layout->layout_entries[i];
    }
    return NULL;
}



/**
 * Convert a texture binding declaration to vklite texture access.
 *
 * @param binding_type DRP2 binding type
 * @param access declared binding access
 * @param format backend-native texture format enum value
 * @return vklite texture access, or none when the binding does not imply image access
 */
static Drp2TextureAccess _binding_texture_access(
    DvzDrp2BindingType binding_type, DvzDrp2BindingAccess access, uint32_t format)
{
    if (binding_type == DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE)
    {
        return _vklite_pass_format_has_depth(format) ? DRP2_TEXTURE_ACCESS_DEPTH_ATTACHMENT_READ :
                                                       DRP2_TEXTURE_ACCESS_SAMPLED_READ;
    }
    if (binding_type == DVZ_DRP2_BINDING_TYPE_STORAGE_TEXTURE)
    {
        return access == DVZ_DRP2_BINDING_ACCESS_READ ?
                   DRP2_TEXTURE_ACCESS_STORAGE_TEXTURE_READ :
                   DRP2_TEXTURE_ACCESS_STORAGE_TEXTURE_READ_WRITE;
    }
    return DRP2_TEXTURE_ACCESS_NONE;
}



/**
 * Transition the image used by a DRP2 texture binding before descriptor use.
 *
 * @param state vklite runtime state
 * @param cmds command buffer wrapper
 * @param bind_group bind-group object carrying resource bindings
 * @param skip_count number of texture ids to skip because they are active attachments
 * @param skip_texture_ids texture ids to leave in their attachment layouts
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
static DvzDrp2ValidationResult _transition_bind_group_textures(
    Drp2VkliteState* state, DvzCommands* cmds, const Drp2VkliteObject* bind_group,
    uint32_t skip_count, const uint64_t* skip_texture_ids, uint32_t command_index)
{
    ANN(state);
    ANN(cmds);
    ANN(bind_group);

    Drp2VkliteObject* layout = _vklite_find(state, bind_group->bind_group_layout_id);
    if (layout == NULL || layout->kind != DRP2_OBJECT_BIND_GROUP_LAYOUT)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    for (uint32_t i = 0; i < bind_group->bind_group_entry_count; i++)
    {
        const DvzDrp2BindGroupEntry* entry = &bind_group->bind_group_entries[i];
        if (entry->binding_type != DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE &&
            entry->binding_type != DVZ_DRP2_BINDING_TYPE_STORAGE_TEXTURE)
            continue;
        bool skip = false;
        for (uint32_t j = 0; j < skip_count; j++)
        {
            if (skip_texture_ids != NULL && skip_texture_ids[j] == entry->resource_id)
            {
                skip = true;
                break;
            }
        }
        if (skip)
            continue;

        const DvzDrp2BindGroupLayoutEntry* layout_entry =
            _bind_group_layout_entry(layout, entry->binding);
        Drp2VkliteObject* texture = _vklite_find(state, entry->resource_id);
        if (texture == NULL || texture->images == NULL)
            return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
        DvzDrp2BindingType binding_type =
            layout_entry != NULL ? layout_entry->binding_type : entry->binding_type;
        DvzDrp2BindingAccess access =
            layout_entry != NULL ? layout_entry->access : DVZ_DRP2_BINDING_ACCESS_READ;
        Drp2TextureAccess texture_access =
            _binding_texture_access(binding_type, access, texture->format);
        if (texture_access == DRP2_TEXTURE_ACCESS_NONE)
            continue;
        _vklite_transition_image_access(cmds, texture, texture_access);
    }
    return _drp2_ok();
}



/**
 * Transition an owned transient depth image according to a declared attachment access.
 *
 * @param cmds command buffer wrapper
 * @param owner object that owns the depth image wrapper
 * @param access requested vklite texture access
 */
static void _transition_owned_depth_image_access(
    DvzCommands* cmds, Drp2VkliteObject* owner, Drp2TextureAccess access)
{
    ANN(cmds);
    ANN(owner);
    ANN(owner->depth_images);

    VkImageLayout layout = _vklite_texture_access_layout(access);
    Drp2TextureAccess previous_access =
        owner->depth_texture_access != DRP2_TEXTURE_ACCESS_NONE
            ? owner->depth_texture_access
            : DRP2_TEXTURE_ACCESS_NONE;
    if (owner->depth_image_layout == layout && previous_access == access)
        return;

    VkPipelineStageFlags2 src_stage = VK_PIPELINE_STAGE_2_NONE;
    VkAccessFlags2 src_access = 0;
    VkPipelineStageFlags2 dst_stage = VK_PIPELINE_STAGE_2_NONE;
    VkAccessFlags2 dst_access = 0;
    _vklite_texture_access_scope(previous_access, &src_stage, &src_access);
    _vklite_texture_access_scope(access, &dst_stage, &dst_access);

    DvzBarriers barriers = {0};
    dvz_barriers(&barriers);
    DvzBarrierImage* bimg =
        dvz_barriers_image(&barriers, dvz_image_handle(owner->depth_images, 0));
    ANN(bimg);
    dvz_barrier_image_stage(bimg, src_stage, dst_stage);
    dvz_barrier_image_access(bimg, src_access, dst_access);
    dvz_barrier_image_layout(bimg, owner->depth_image_layout, layout);
    dvz_barrier_image_aspect(bimg, VK_IMAGE_ASPECT_DEPTH_BIT);
    dvz_cmd_barriers(cmds, &barriers);
    owner->depth_image_layout = layout;
    owner->depth_texture_access = access;
}



/**
 * Find the active borrowed frame target depth image for same-size intermediate passes.
 *
 * @param state vklite runtime state
 * @param width required target width
 * @param height required target height
 * @param sample_count required target sample count
 * @return borrowed frame target with live depth resources, or NULL
 */
static Drp2VkliteObject* _active_borrowed_depth_target(
    Drp2VkliteState* state, uint32_t width, uint32_t height, uint32_t sample_count)
{
    ANN(state);
    if (state->active_borrowed_command_buffer == VK_NULL_HANDLE)
        return NULL;

    for (uint32_t i = 0; i < state->count; i++)
    {
        Drp2VkliteObject* object = &state->objects[i];
        if (object->destroyed || !object->borrowed_frame_target)
            continue;
        if (object->width != width || object->height != height)
            continue;
        if (object->sample_count != sample_count)
            continue;
        if (object->depth_images == NULL || object->depth_views == NULL)
            continue;
        return object;
    }
    return NULL;
}



static VkAttachmentLoadOp _vklite_attachment_load_op(DvzDrp2AttachmentLoadOp op)
{
    switch (op)
    {
    case DVZ_DRP2_ATTACHMENT_LOAD_CLEAR:
        return VK_ATTACHMENT_LOAD_OP_CLEAR;
    case DVZ_DRP2_ATTACHMENT_LOAD_LOAD:
        return VK_ATTACHMENT_LOAD_OP_LOAD;
    case DVZ_DRP2_ATTACHMENT_LOAD_DONT_CARE:
        return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    default:
        return VK_ATTACHMENT_LOAD_OP_LOAD;
    }
}



static VkAttachmentStoreOp _vklite_attachment_store_op(DvzDrp2AttachmentStoreOp op)
{
    switch (op)
    {
    case DVZ_DRP2_ATTACHMENT_STORE_STORE:
        return VK_ATTACHMENT_STORE_OP_STORE;
    case DVZ_DRP2_ATTACHMENT_STORE_DONT_CARE:
        return VK_ATTACHMENT_STORE_OP_DONT_CARE;
    default:
        return VK_ATTACHMENT_STORE_OP_STORE;
    }
}


/**
 * Begin a vklite dynamic-rendering pass for a DRP2 BeginRenderPass command.
 *
 * @param state vklite runtime state
 * @param stream DRP2 command stream being executed
 * @param command DRP2 BeginRenderPass command
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
DvzDrp2ValidationResult _vklite_begin_render_pass(
    Drp2VkliteState* state, const DvzDrp2CommandStream* stream,
    const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(stream);
    ANN(command);

    uint32_t color_count = command->u.begin_render_pass.color_attachment_count;
    if (color_count == 0)
        color_count = 1;
    if (color_count > DVZ_DRP2_MAX_COLOR_ATTACHMENTS)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2VkliteObject* targets[DVZ_DRP2_MAX_COLOR_ATTACHMENTS] = {0};
    Drp2VkliteObject* resolves[DVZ_DRP2_MAX_COLOR_ATTACHMENTS] = {0};
    VkImageView target_views[DVZ_DRP2_MAX_COLOR_ATTACHMENTS] = {0};
    VkImageView resolve_views[DVZ_DRP2_MAX_COLOR_ATTACHMENTS] = {0};
    for (uint32_t i = 0; i < color_count; i++)
    {
        const DvzDrp2ColorAttachment* attachment =
            command->u.begin_render_pass.color_attachment_count > 0
                ? &command->u.begin_render_pass.color_attachments[i]
                : NULL;
        uint64_t texture_id =
            attachment != NULL ? attachment->texture_id :
                                 command->u.begin_render_pass.texture_id;
        targets[i] = _vklite_find(state, texture_id);
        target_views[i] = _vklite_object_image_view(targets[i]);
        if (targets[i] == NULL || targets[i]->images == NULL ||
            target_views[i] == VK_NULL_HANDLE)
            return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
        if (attachment != NULL && attachment->resolve_texture_id != 0)
        {
            resolves[i] = _vklite_find(state, attachment->resolve_texture_id);
            resolve_views[i] = _vklite_object_image_view(resolves[i]);
            if (resolves[i] == NULL || resolves[i]->images == NULL ||
                resolve_views[i] == VK_NULL_HANDLE || targets[i]->sample_count <= 1 ||
                resolves[i]->sample_count != 1 || resolves[i]->width != targets[i]->width ||
                resolves[i]->height != targets[i]->height ||
                resolves[i]->format != targets[i]->format)
                return _drp2_fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
        }
    }
    Drp2VkliteObject* target = targets[0];
    if (target->borrowed_frame_target && color_count > 1)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    for (uint32_t i = 1; i < color_count; i++)
    {
        if (targets[i]->borrowed_frame_target || targets[i]->width != target->width ||
            targets[i]->height != target->height ||
            targets[i]->sample_count != target->sample_count)
            return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }

    Drp2VkliteObject* named_depth = NULL;
    VkImageView named_depth_view = VK_NULL_HANDLE;
    if (command->u.begin_render_pass.has_depth_attachment &&
        command->u.begin_render_pass.depth_texture_id != 0)
    {
        named_depth = _vklite_find(state, command->u.begin_render_pass.depth_texture_id);
        named_depth_view = _vklite_object_image_view(named_depth);
        if (named_depth == NULL || named_depth->images == NULL ||
            named_depth_view == VK_NULL_HANDLE ||
            (named_depth->usage & DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT) == 0 ||
            named_depth->width != target->width || named_depth->height != target->height ||
            named_depth->sample_count != target->sample_count)
            return _drp2_fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
    }

    Drp2VkliteObject* pass =
        _vklite_add(state, command->u.begin_render_pass.id, DRP2_OBJECT_RENDER_PASS);
    if (pass == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    for (uint32_t i = 0; i < color_count; i++)
    {
        const DvzDrp2ColorAttachment* attachment =
            command->u.begin_render_pass.color_attachment_count > 0
                ? &command->u.begin_render_pass.color_attachments[i]
                : NULL;
        uint64_t texture_id =
            attachment != NULL ? attachment->texture_id :
                                 command->u.begin_render_pass.texture_id;
        targets[i] = _vklite_find(state, texture_id);
        target_views[i] = _vklite_object_image_view(targets[i]);
        if (targets[i] == NULL || targets[i]->images == NULL ||
            target_views[i] == VK_NULL_HANDLE)
            return _vklite_fail_destroy_object(
                pass, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
        if (attachment != NULL && attachment->resolve_texture_id != 0)
        {
            resolves[i] = _vklite_find(state, attachment->resolve_texture_id);
            resolve_views[i] = _vklite_object_image_view(resolves[i]);
            if (resolves[i] == NULL || resolves[i]->images == NULL ||
                resolve_views[i] == VK_NULL_HANDLE)
                return _vklite_fail_destroy_object(
                    pass, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
        }
    }
    target = targets[0];

    DvzCommands* cmds = NULL;
    if (state->active_borrowed_command_buffer != VK_NULL_HANDLE)
    {
        cmds = _vklite_borrowed_frame_commands_create(
            state->runtime->device, state->active_borrowed_command_buffer);
        if (cmds == NULL)
            return _vklite_fail_destroy_object(
                pass, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
        pass->borrowed_commands = true;
    }
    else if (target->borrowed_frame_target)
    {
        cmds = _vklite_borrowed_frame_commands_create(
            state->runtime->device, target->command_buffer);
        if (cmds == NULL)
            return _vklite_fail_destroy_object(
                pass, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
        pass->borrowed_commands = true;
    }
    else
    {
        cmds = _vklite_owned_commands_create(state->runtime->device);
        if (cmds == NULL)
            return _vklite_fail_destroy_object(
                pass, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }
    pass->commands = cmds;
    pass->width = target->width;
    pass->height = target->height;
    pass->sample_count = target->sample_count;
    pass->color_target_count = color_count;
    for (uint32_t i = 0; i < color_count; i++)
        pass->color_target_ids[i] = targets[i]->id;
    uint32_t render_area_x = 0;
    uint32_t render_area_y = 0;
    uint32_t render_area_width = target->width;
    uint32_t render_area_height = target->height;
    uint32_t viewport_x = 0;
    uint32_t viewport_y = 0;
    uint32_t viewport_width = target->width;
    uint32_t viewport_height = target->height;
    uint32_t scissor_x = 0;
    uint32_t scissor_y = 0;
    uint32_t scissor_width = target->width;
    uint32_t scissor_height = target->height;
    if (command->u.begin_render_pass.has_explicit_rects)
    {
        _render_area_from_pixel_rect(
            target->width, target->height, command->u.begin_render_pass.render_area_px,
            &render_area_x, &render_area_y, &render_area_width, &render_area_height);
        float viewport_rect[4] = {0};
        float scissor_rect[4] = {0};
        _framebuffer_rect_or_render_area(
            command->u.begin_render_pass.viewport_px, render_area_x, render_area_y,
            render_area_width, render_area_height, viewport_rect);
        _framebuffer_rect_or_render_area(
            command->u.begin_render_pass.scissor_px, render_area_x, render_area_y,
            render_area_width, render_area_height, scissor_rect);
        _render_area_from_framebuffer_rect(
            target->width, target->height, viewport_rect, &viewport_x, &viewport_y,
            &viewport_width, &viewport_height);
        _render_area_from_framebuffer_rect(
            target->width, target->height, scissor_rect, &scissor_x, &scissor_y,
            &scissor_width, &scissor_height);
    }
    else
    {
        _render_area_from_viewport(
            target->width, target->height, command->u.begin_render_pass.viewport, &viewport_x,
            &viewport_y, &viewport_width, &viewport_height);
        render_area_x = viewport_x;
        render_area_y = viewport_y;
        render_area_width = viewport_width;
        render_area_height = viewport_height;
        scissor_x = viewport_x;
        scissor_y = viewport_y;
        scissor_width = viewport_width;
        scissor_height = viewport_height;
    }
    pass->viewport_x = (float)viewport_x;
    pass->viewport_y = (float)viewport_y;
    pass->viewport_width = (float)viewport_width;
    pass->viewport_height = (float)viewport_height;
    pass->scissor_x = (float)scissor_x;
    pass->scissor_y = (float)scissor_y;
    pass->scissor_width = (float)scissor_width;
    pass->scissor_height = (float)scissor_height;

    DvzRendering* rendering = dvz_rendering_create_wrapper();
    if (rendering == NULL)
        return _vklite_fail_destroy_object(
            pass, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    pass->rendering = rendering;
    Drp2VkliteObject* transient_depth_owner = NULL;

    int32_t render_x = render_area_x <= (uint32_t)INT32_MAX ? (int32_t)render_area_x : INT32_MAX;
    int32_t render_y = render_area_y <= (uint32_t)INT32_MAX ? (int32_t)render_area_y : INT32_MAX;
    dvz_rendering(rendering);
    dvz_rendering_area(rendering, render_x, render_y, render_area_width, render_area_height);
    for (uint32_t i = 0; i < color_count; i++)
    {
        const DvzDrp2ColorAttachment* attachment =
            command->u.begin_render_pass.color_attachment_count > 0
                ? &command->u.begin_render_pass.color_attachments[i]
                : NULL;
        bool clear_attachment =
            attachment != NULL ? attachment->clear : command->u.begin_render_pass.clear;
        DvzDrp2AttachmentLoadOp load_op =
            attachment != NULL ? attachment->load_op :
                                 (clear_attachment ? DVZ_DRP2_ATTACHMENT_LOAD_CLEAR :
                                                     DVZ_DRP2_ATTACHMENT_LOAD_LOAD);
        DvzDrp2AttachmentStoreOp store_op =
            attachment != NULL ? attachment->store_op : DVZ_DRP2_ATTACHMENT_STORE_STORE;
        const float* clear_color =
            attachment != NULL ? attachment->clear_color : command->u.begin_render_pass.clear_color;
        VkClearValue clear = {
            .color.float32 = {clear_color[0], clear_color[1], clear_color[2], clear_color[3]}};
        DvzAttachment* catt = dvz_rendering_color(rendering, i);
        dvz_attachment_image(catt, target_views[i], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        if (resolve_views[i] != VK_NULL_HANDLE)
        {
            VkResolveModeFlagBits resolve_mode =
                attachment != NULL && attachment->resolve_mode != 0 ?
                    (VkResolveModeFlagBits)attachment->resolve_mode :
                    VK_RESOLVE_MODE_AVERAGE_BIT;
            dvz_attachment_resolve(
                catt, resolve_mode, resolve_views[i],
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        }
        dvz_attachment_ops(
            catt, _vklite_attachment_load_op(load_op), _vklite_attachment_store_op(store_op));
        if (load_op == DVZ_DRP2_ATTACHMENT_LOAD_CLEAR)
            dvz_attachment_clear(catt, clear);
    }
    if (command->u.begin_render_pass.has_depth_attachment)
    {
        Drp2VkliteObject* borrowed_depth_owner = NULL;
        Drp2VkliteObject* depth_owner = NULL;
        bool load_existing_depth = false;
        VkImageView depth_view = named_depth_view;

        if (named_depth == NULL)
        {
            borrowed_depth_owner =
                target->borrowed_frame_target
                    ? NULL
                    : _active_borrowed_depth_target(
                          state, target->width, target->height, target->sample_count);
            depth_owner = target->borrowed_frame_target
                              ? target
                              : (borrowed_depth_owner != NULL ? borrowed_depth_owner : pass);
            load_existing_depth = borrowed_depth_owner != NULL;
            transient_depth_owner = depth_owner;

            if (load_existing_depth)
            {
                depth_view = dvz_image_views_handle(depth_owner->depth_views, 0);
            }
            else
            {
                DvzImages* depth_images = dvz_images_create_wrapper();
                DvzImageViews* depth_views = dvz_image_views_create_wrapper();
                if (depth_images == NULL || depth_views == NULL)
                {
                    if (depth_views != NULL)
                        dvz_image_views_free(depth_views);
                    if (depth_images != NULL)
                        dvz_images_free(depth_images);
                    return _vklite_fail_destroy_object(
                        pass, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
                }
                dvz_images(
                    state->runtime->device, state->runtime->allocator, VK_IMAGE_TYPE_2D, 1,
                    depth_images);
                dvz_images_format(depth_images, VK_FORMAT_D32_SFLOAT);
                dvz_images_size(depth_images, target->width, target->height, 1);
                dvz_images_mip(depth_images, 1);
                dvz_images_layers(depth_images, 1);
                dvz_images_samples(depth_images, _vklite_sample_count(target->sample_count));
                dvz_images_usage(depth_images, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
                if (dvz_images_create(depth_images) != 0)
                {
                    dvz_image_views_free(depth_views);
                    dvz_images_free(depth_images);
                    return _vklite_fail_destroy_object(
                        pass, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
                }
                dvz_image_views(depth_images, depth_views);
                dvz_image_views_aspect(depth_views, VK_IMAGE_ASPECT_DEPTH_BIT);
                dvz_image_views_create(depth_views);
                depth_view = dvz_image_views_handle(depth_views, 0);
                if (depth_view == VK_NULL_HANDLE)
                {
                    dvz_image_views_free(depth_views);
                    dvz_images_destroy(depth_images);
                    dvz_images_free(depth_images);
                    return _vklite_fail_destroy_object(
                        pass, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
                }
                if (depth_owner->depth_images != NULL || depth_owner->depth_views != NULL)
                {
                    if (!_vklite_retire_frame_target_depth(state, depth_owner))
                    {
                        dvz_image_views_destroy(depth_views);
                        dvz_image_views_free(depth_views);
                        dvz_images_destroy(depth_images);
                        dvz_images_free(depth_images);
                        return _vklite_fail_destroy_object(
                            pass, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
                    }
                }
                depth_owner->depth_images = depth_images;
                depth_owner->depth_views = depth_views;
                depth_owner->depth_image_layout = VK_IMAGE_LAYOUT_UNDEFINED;
                depth_owner->depth_texture_access = DRP2_TEXTURE_ACCESS_NONE;
            }
        }
        if (depth_view == VK_NULL_HANDLE)
            return _vklite_fail_destroy_object(
                pass, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
        DvzAttachment* datt = dvz_rendering_depth(rendering);
        DvzDrp2AttachmentLoadOp depth_load_op = command->u.begin_render_pass.depth_load_op;
        if (!command->u.begin_render_pass.depth_ops_explicit && load_existing_depth)
            depth_load_op = DVZ_DRP2_ATTACHMENT_LOAD_LOAD;
        DvzDrp2AttachmentAccess depth_attachment_access =
            _attachment_effective_access(
                command->u.begin_render_pass.depth_access, depth_load_op);
        Drp2TextureAccess depth_access = _depth_texture_access(depth_attachment_access);
        dvz_attachment_image(datt, depth_view, _vklite_texture_access_layout(depth_access));
        dvz_attachment_ops(
            datt, _vklite_attachment_load_op(depth_load_op),
            _vklite_attachment_store_op(command->u.begin_render_pass.depth_store_op));
        if (depth_load_op == DVZ_DRP2_ATTACHMENT_LOAD_CLEAR)
        {
            dvz_attachment_clear(
                datt,
                (VkClearValue){.depthStencil = {command->u.begin_render_pass.clear_depth, 0}});
        }
    }

    if (!pass->borrowed_commands && dvz_cmd_begin_result(cmds) != 0)
        return _vklite_fail_destroy_object(
            pass, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    uint64_t skip_texture_ids[(2 * DVZ_DRP2_MAX_COLOR_ATTACHMENTS) + 1] = {0};
    uint32_t skip_count = 0;
    for (uint32_t i = 0; i < color_count; i++)
        skip_texture_ids[skip_count++] = targets[i]->id;
    for (uint32_t i = 0; i < color_count; i++)
    {
        if (resolves[i] != NULL)
            skip_texture_ids[skip_count++] = resolves[i]->id;
    }
    DvzDrp2AttachmentAccess depth_transition_access = DVZ_DRP2_ATTACHMENT_ACCESS_READ_WRITE;
    if (named_depth != NULL || transient_depth_owner != NULL)
    {
        depth_transition_access = _attachment_effective_access(
            command->u.begin_render_pass.depth_access, command->u.begin_render_pass.depth_load_op);
    }
    if (named_depth != NULL && depth_transition_access != DVZ_DRP2_ATTACHMENT_ACCESS_READ)
        skip_texture_ids[skip_count++] = named_depth->id;

    for (uint32_t i = command_index + 1; i < stream->count; i++)
    {
        const DvzDrp2Command* pass_command = &stream->commands[i];
        if (pass_command->type == DVZ_DRP2_COMMAND_END_RENDER_PASS &&
            pass_command->u.end_render_pass.pass_id == command->u.begin_render_pass.id)
            break;
        if (pass_command->type != DVZ_DRP2_COMMAND_SET_BIND_GROUP ||
            pass_command->u.set_bind_group.pass_id != command->u.begin_render_pass.id)
            continue;

        Drp2VkliteObject* bind_group =
            _vklite_find(state, pass_command->u.set_bind_group.bind_group_id);
        if (bind_group == NULL || bind_group->kind != DRP2_OBJECT_BIND_GROUP)
            return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, i);
        DvzDrp2ValidationResult transition_result = _transition_bind_group_textures(
            state, cmds, bind_group, skip_count, skip_texture_ids, i);
        if (!transition_result.ok)
            return transition_result;
    }

    for (uint32_t i = 0; i < color_count; i++)
    {
        if (!targets[i]->borrowed_frame_target)
        {
            const DvzDrp2ColorAttachment* attachment =
                command->u.begin_render_pass.color_attachment_count > 0
                    ? &command->u.begin_render_pass.color_attachments[i]
                    : NULL;
            DvzDrp2AttachmentAccess access =
                attachment != NULL ? attachment->access : DVZ_DRP2_ATTACHMENT_ACCESS_WRITE;
            DvzDrp2AttachmentLoadOp load_op =
                attachment != NULL ? attachment->load_op :
                                     (command->u.begin_render_pass.clear ?
                                          DVZ_DRP2_ATTACHMENT_LOAD_CLEAR :
                                          DVZ_DRP2_ATTACHMENT_LOAD_LOAD);
            access = _attachment_effective_access(access, load_op);
            _vklite_transition_image_access(
                cmds, targets[i], _color_texture_access(access));
        }
        if (resolves[i] != NULL)
        {
            _vklite_transition_image_access(
                cmds, resolves[i], DRP2_TEXTURE_ACCESS_COLOR_ATTACHMENT_WRITE);
        }
    }
    if (named_depth != NULL)
    {
        _vklite_transition_image_access(
            cmds, named_depth, _depth_texture_access(depth_transition_access));
    }
    else if (transient_depth_owner != NULL && transient_depth_owner->depth_images != NULL)
        _transition_owned_depth_image_access(
            cmds, transient_depth_owner, _depth_texture_access(depth_transition_access));
    dvz_cmd_rendering_begin(cmds, rendering);
    return _drp2_ok();
}


/**
 * Begin a vklite command buffer for a DRP2 compute pass.
 *
 * @param state vklite runtime state
 * @param command DRP2 BeginComputePass command
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
DvzDrp2ValidationResult _vklite_begin_compute_pass(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    Drp2VkliteObject* pass =
        _vklite_add(state, command->u.begin_compute_pass.id, DRP2_OBJECT_COMPUTE_PASS);
    if (pass == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    DvzCommands* cmds = _vklite_owned_commands_create(state->runtime->device);
    if (cmds == NULL)
        return _vklite_fail_destroy_object(
            pass, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    pass->commands = cmds;

    if (dvz_cmd_begin_result(cmds) != 0)
        return _vklite_fail_destroy_object(
            pass, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    return _drp2_ok();
}


/**
 * Bind a vklite pipeline within a DRP2 render or compute pass.
 *
 * @param state vklite runtime state
 * @param command DRP2 SetPipeline command
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
DvzDrp2ValidationResult _vklite_set_pipeline(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);
    Drp2VkliteObject* pass = _vklite_find(state, command->u.set_pipeline.pass_id);
    Drp2VkliteObject* pipeline = _vklite_find(state, command->u.set_pipeline.pipeline_id);
    if (pass == NULL || pass->commands == NULL || pipeline == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    if (pass->kind == DRP2_OBJECT_RENDER_PASS && pipeline->kind == DRP2_OBJECT_RENDER_PIPELINE &&
        pipeline->graphics != NULL)
    {
        uint32_t viewport_x = 0, viewport_y = 0, viewport_width = pass->width,
                 viewport_height = pass->height;
        float viewport[4] = {
            pass->viewport_x,
            pass->viewport_y,
            pass->viewport_width,
            pass->viewport_height,
        };
        uint32_t scissor_x = 0, scissor_y = 0, scissor_width = pass->width,
                 scissor_height = pass->height;
        float scissor[4] = {
            pass->scissor_x,
            pass->scissor_y,
            pass->scissor_width,
            pass->scissor_height,
        };
        _render_area_from_framebuffer_rect(
            pass->width, pass->height, viewport, &viewport_x, &viewport_y, &viewport_width,
            &viewport_height);
        _render_area_from_framebuffer_rect(
            pass->width, pass->height, scissor, &scissor_x, &scissor_y, &scissor_width,
            &scissor_height);
        dvz_graphics_viewport(
            pipeline->graphics, (float)viewport_x, (float)viewport_y, (float)viewport_width,
            (float)viewport_height, 0, 1,
            DVZ_GRAPHICS_FLAGS_DYNAMIC);
        dvz_graphics_scissor(
            pipeline->graphics, (int32_t)scissor_x, (int32_t)scissor_y, scissor_width,
            scissor_height,
            DVZ_GRAPHICS_FLAGS_DYNAMIC);
        dvz_cmd_bind_graphics(pass->commands, pipeline->graphics);
        pass->current_pipeline_id = command->u.set_pipeline.pipeline_id;
        return _drp2_ok();
    }
    if (pass->kind == DRP2_OBJECT_COMPUTE_PASS && pipeline->kind == DRP2_OBJECT_COMPUTE_PIPELINE &&
        pipeline->compute != NULL)
    {
        dvz_cmd_bind_compute(pass->commands, pipeline->compute);
        pass->current_pipeline_id = command->u.set_pipeline.pipeline_id;
        return _drp2_ok();
    }
    return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
}


/**
 * Bind a vertex buffer within a vklite render pass.
 *
 * @param state vklite runtime state
 * @param command DRP2 SetVertexBuffer command
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
DvzDrp2ValidationResult _vklite_set_vertex_buffer(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);
    Drp2VkliteObject* pass = _vklite_find(state, command->u.set_vertex_buffer.pass_id);
    Drp2VkliteObject* buffer = _vklite_find(state, command->u.set_vertex_buffer.buffer_id);
    if (pass == NULL || pass->kind != DRP2_OBJECT_RENDER_PASS || pass->commands == NULL ||
        buffer == NULL || buffer->buffer == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    DvzSize offset = command->u.set_vertex_buffer.offset;
    dvz_cmd_bind_vertex_buffers(
        pass->commands, command->u.set_vertex_buffer.slot, 1, buffer->buffer, &offset);
    return _drp2_ok();
}


/**
 * Bind an index buffer within a vklite render pass.
 *
 * @param state vklite runtime state
 * @param command DRP2 SetIndexBuffer command
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
DvzDrp2ValidationResult _vklite_set_index_buffer(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);
    Drp2VkliteObject* pass = _vklite_find(state, command->u.set_index_buffer.pass_id);
    Drp2VkliteObject* buffer = _vklite_find(state, command->u.set_index_buffer.buffer_id);
    if (pass == NULL || pass->kind != DRP2_OBJECT_RENDER_PASS || pass->commands == NULL ||
        buffer == NULL || buffer->buffer == NULL)
    {
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }

    VkIndexType index_type = VK_INDEX_TYPE_UINT32;
    if (strcmp(command->u.set_index_buffer.index_format, "uint16") == 0)
        index_type = VK_INDEX_TYPE_UINT16;
    else if (strcmp(command->u.set_index_buffer.index_format, "uint32") != 0)
        return _drp2_fail(DVZ_DRP2_VALIDATION_USAGE, command_index);

    dvz_cmd_bind_index_buffer(
        pass->commands, buffer->buffer, command->u.set_index_buffer.offset, index_type);
    return _drp2_ok();
}


/**
 * Set a dynamic viewport within a vklite render pass.
 *
 * @param state vklite runtime state
 * @param command DRP2 SetViewport command
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
DvzDrp2ValidationResult _vklite_set_viewport(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);
    Drp2VkliteObject* pass = _vklite_find(state, command->u.set_viewport.pass_id);
    if (pass == NULL || pass->kind != DRP2_OBJECT_RENDER_PASS || pass->commands == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    pass->viewport_x = command->u.set_viewport.viewport[0];
    pass->viewport_y = command->u.set_viewport.viewport[1];
    pass->viewport_width = command->u.set_viewport.viewport[2];
    pass->viewport_height = command->u.set_viewport.viewport[3];

    uint32_t x = 0, y = 0, width = pass->width, height = pass->height;
    _render_area_from_framebuffer_rect(
        pass->width, pass->height, command->u.set_viewport.viewport, &x, &y, &width, &height);
    VkViewport viewport = {
        .x = (float)x,
        .y = (float)y,
        .width = (float)width,
        .height = (float)height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    vkCmdSetViewport(dvz_commands_handle(pass->commands), 0, 1, &viewport);
    return _drp2_ok();
}



/**
 * Set a dynamic scissor within a vklite render pass.
 *
 * @param state vklite runtime state
 * @param command DRP2 SetScissor command
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
DvzDrp2ValidationResult _vklite_set_scissor(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);
    Drp2VkliteObject* pass = _vklite_find(state, command->u.set_scissor.pass_id);
    if (pass == NULL || pass->kind != DRP2_OBJECT_RENDER_PASS || pass->commands == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    pass->scissor_x = command->u.set_scissor.scissor[0];
    pass->scissor_y = command->u.set_scissor.scissor[1];
    pass->scissor_width = command->u.set_scissor.scissor[2];
    pass->scissor_height = command->u.set_scissor.scissor[3];

    uint32_t x = 0, y = 0, width = pass->width, height = pass->height;
    _render_area_from_framebuffer_rect(
        pass->width, pass->height, command->u.set_scissor.scissor, &x, &y, &width, &height);
    VkRect2D scissor = {
        .offset = {.x = (int32_t)x, .y = (int32_t)y},
        .extent = {.width = width, .height = height},
    };
    vkCmdSetScissor(dvz_commands_handle(pass->commands), 0, 1, &scissor);
    return _drp2_ok();
}


/**
 * Bind a vklite descriptor set within a DRP2 render or compute pass.
 *
 * @param state vklite runtime state
 * @param command DRP2 SetBindGroup command
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
DvzDrp2ValidationResult _vklite_set_bind_group(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);
    Drp2VkliteObject* pass = _vklite_find(state, command->u.set_bind_group.pass_id);
    Drp2VkliteObject* bind_group = _vklite_find(state, command->u.set_bind_group.bind_group_id);
    if (pass == NULL || pass->commands == NULL || bind_group == NULL ||
        bind_group->descriptors == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    Drp2VkliteObject* pipeline = _vklite_find(state, pass->current_pipeline_id);
    if (pipeline == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    VkPipelineBindPoint bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;
    if (pass->kind == DRP2_OBJECT_COMPUTE_PASS)
        bind_point = VK_PIPELINE_BIND_POINT_COMPUTE;
    else if (pass->kind != DRP2_OBJECT_RENDER_PASS)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    if (pass->kind == DRP2_OBJECT_COMPUTE_PASS)
    {
        DvzDrp2ValidationResult transition_result =
            _transition_bind_group_textures(state, pass->commands, bind_group, 0, NULL,
                                            command_index);
        if (!transition_result.ok)
            return transition_result;
    }

    if (pipeline->combined_pipeline_layout != VK_NULL_HANDLE)
    {
        VkDescriptorSet descriptor_set = dvz_descriptors_handle(bind_group->descriptors, 0);
        uint32_t dynamic_offsets[DVZ_DRP2_MAX_BINDINGS] = {0};
        for (uint32_t i = 0; i < command->u.set_bind_group.dynamic_offset_count; i++)
            dynamic_offsets[i] = (uint32_t)command->u.set_bind_group.dynamic_offsets[i];
        vkCmdBindDescriptorSets(
            dvz_commands_handle(pass->commands), bind_point, pipeline->combined_pipeline_layout,
            command->u.set_bind_group.slot, 1, &descriptor_set,
            command->u.set_bind_group.dynamic_offset_count, dynamic_offsets);
    }
    else
    {
        uint32_t dynamic_offsets[DVZ_DRP2_MAX_BINDINGS] = {0};
        for (uint32_t i = 0; i < command->u.set_bind_group.dynamic_offset_count; i++)
            dynamic_offsets[i] = (uint32_t)command->u.set_bind_group.dynamic_offsets[i];
        dvz_cmd_bind_descriptors(
            pass->commands, bind_point, bind_group->descriptors, command->u.set_bind_group.slot, 1,
            command->u.set_bind_group.dynamic_offset_count, dynamic_offsets);
    }
    return _drp2_ok();
}


/**
 * Record a direct draw within a vklite render pass.
 *
 * @param state vklite runtime state
 * @param command DRP2 Draw command
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
DvzDrp2ValidationResult _vklite_draw(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);
    Drp2VkliteObject* pass = _vklite_find(state, command->u.draw.pass_id);
    if (pass == NULL || pass->kind != DRP2_OBJECT_RENDER_PASS || pass->commands == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    dvz_cmd_draw(
        pass->commands, command->u.draw.first_vertex, command->u.draw.vertex_count,
        command->u.draw.first_instance, command->u.draw.instance_count);
    return _drp2_ok();
}


/**
 * Record an indexed draw within a vklite render pass.
 *
 * @param state vklite runtime state
 * @param command DRP2 DrawIndexed command
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
DvzDrp2ValidationResult _vklite_draw_indexed(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);
    Drp2VkliteObject* pass = _vklite_find(state, command->u.draw_indexed.pass_id);
    if (pass == NULL || pass->kind != DRP2_OBJECT_RENDER_PASS || pass->commands == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    dvz_cmd_draw_indexed(
        pass->commands, command->u.draw_indexed.first_index, command->u.draw_indexed.base_vertex,
        command->u.draw_indexed.index_count, command->u.draw_indexed.first_instance,
        command->u.draw_indexed.instance_count);
    return _drp2_ok();
}


/**
 * Record a compute dispatch within a vklite compute pass.
 *
 * @param state vklite runtime state
 * @param command DRP2 DispatchWorkgroups command
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
DvzDrp2ValidationResult _vklite_dispatch_workgroups(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);
    Drp2VkliteObject* pass = _vklite_find(state, command->u.dispatch.pass_id);
    if (pass == NULL || pass->kind != DRP2_OBJECT_COMPUTE_PASS || pass->commands == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    dvz_cmd_dispatch(
        pass->commands, command->u.dispatch.x, command->u.dispatch.y, command->u.dispatch.z);
    return _drp2_ok();
}


/**
 * Record and submit a buffer resource barrier in an owned vklite command buffer.
 *
 * @param state vklite runtime state
 * @param command DRP2 ResourceBarrier command
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
DvzDrp2ValidationResult _vklite_resource_barrier(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);
    Drp2VkliteObject* buffer = _vklite_find(state, command->u.resource_barrier.buffer_id);
    if (buffer == NULL || buffer->kind != DRP2_OBJECT_BUFFER || buffer->buffer == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    VkPipelineStageFlags2 dst_stage = VK_PIPELINE_STAGE_2_NONE;
    VkAccessFlags2 dst_access = VK_ACCESS_2_NONE;
    if (strcmp(command->u.resource_barrier.dst_stage, "VERTEX_INPUT") == 0 &&
        strcmp(command->u.resource_barrier.dst_access, "VERTEX_READ") == 0)
    {
        dst_stage = VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
        dst_access = VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
    }
    else if (strcmp(command->u.resource_barrier.dst_stage, "COPY") == 0 &&
             strcmp(command->u.resource_barrier.dst_access, "COPY_READ") == 0)
    {
        dst_stage = VK_PIPELINE_STAGE_2_COPY_BIT;
        dst_access = VK_ACCESS_2_TRANSFER_READ_BIT;
    }
    else
    {
        return _drp2_fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
    }

    DvzSize size = command->u.resource_barrier.size;
    if (size == 0)
        size = VK_WHOLE_SIZE;

    DvzCommands* cmds = _vklite_owned_commands_create(state->runtime->device);
    if (cmds == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (dvz_cmd_begin_result(cmds) != 0)
    {
        _vklite_owned_commands_destroy(cmds);
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }

    DvzBarriers barriers = {0};
    dvz_barriers(&barriers);
    DvzBarrierBuffer* bbuf = dvz_barriers_buffer(
        &barriers, dvz_buffer_handle(buffer->buffer), command->u.resource_barrier.offset, size);
    if (bbuf == NULL)
    {
        _vklite_owned_commands_destroy(cmds);
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }
    dvz_barrier_buffer_stage(bbuf, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, dst_stage);
    dvz_barrier_buffer_access(bbuf, VK_ACCESS_2_SHADER_WRITE_BIT, dst_access);
    dvz_cmd_barriers(cmds, &barriers);

    DvzDrp2ValidationResult result = _vklite_owned_commands_end_submit(cmds, command_index);
    _vklite_owned_commands_destroy(cmds);
    return result;
}


/**
 * End and submit a vklite dynamic-rendering pass.
 *
 * @param state vklite runtime state
 * @param pass_id DRP2 render pass id
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
DvzDrp2ValidationResult
_vklite_end_render_pass(Drp2VkliteState* state, uint64_t pass_id, uint32_t command_index)
{
    ANN(state);
    Drp2VkliteObject* pass = _vklite_find(state, pass_id);
    if (pass == NULL || pass->kind != DRP2_OBJECT_RENDER_PASS || pass->commands == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    dvz_cmd_rendering_end(pass->commands);
    if (!pass->borrowed_commands)
    {
        DvzDrp2ValidationResult result =
            _vklite_owned_commands_end_submit(pass->commands, command_index);
        if (!result.ok)
        {
            _vklite_destroy_object_slot(state, pass);
            return result;
        }
    }
    else if (pass->depth_images != NULL &&
             state->active_borrowed_command_buffer != VK_NULL_HANDLE &&
             _vklite_defer_destroy_object(state, pass, state->active_borrowed_command_buffer))
    {
        return _drp2_ok();
    }
    _vklite_destroy_object_slot(state, pass);
    return _drp2_ok();
}


/**
 * End and submit a vklite compute pass.
 *
 * @param state vklite runtime state
 * @param pass_id DRP2 compute pass id
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
DvzDrp2ValidationResult
_vklite_end_compute_pass(Drp2VkliteState* state, uint64_t pass_id, uint32_t command_index)
{
    ANN(state);
    Drp2VkliteObject* pass = _vklite_find(state, pass_id);
    if (pass == NULL || pass->kind != DRP2_OBJECT_COMPUTE_PASS || pass->commands == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    DvzDrp2ValidationResult result = _vklite_owned_commands_end_submit(pass->commands, command_index);
    if (!result.ok)
    {
        _vklite_destroy_object_slot(state, pass);
        return result;
    }
    _vklite_destroy_object_slot(state, pass);
    return _drp2_ok();
}



#endif
