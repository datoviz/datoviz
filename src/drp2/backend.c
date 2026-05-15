/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  DRP2 vklite runtime backend                                                                  */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include <volk.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_runtime.h"
#include "_stream.h"



#if DVZ_DRP2_HAS_VKLITE
/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static VkBufferUsageFlags _vklite_buffer_usage(uint32_t usage)
{
    VkBufferUsageFlags out = 0;
    if ((usage & DVZ_DRP2_BUFFER_USAGE_COPY_SRC) != 0)
        out |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    if ((usage & DVZ_DRP2_BUFFER_USAGE_COPY_DST) != 0)
        out |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    if ((usage & DVZ_DRP2_BUFFER_USAGE_VERTEX) != 0)
        out |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    if ((usage & DVZ_DRP2_BUFFER_USAGE_INDEX) != 0)
        out |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    if ((usage & DVZ_DRP2_BUFFER_USAGE_UNIFORM) != 0)
        out |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    if ((usage & DVZ_DRP2_BUFFER_USAGE_STORAGE) != 0)
        out |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    return out != 0 ? out : VK_BUFFER_USAGE_TRANSFER_DST_BIT;
}


static DvzAllocationFlags _vklite_buffer_alloc_flags(uint32_t usage)
{
    DvzAllocationFlags flags = DVZ_ALLOC_FLAGS_NONE;
    if ((usage & DVZ_DRP2_BUFFER_USAGE_MAP_READ) != 0)
    {
        flags |= DVZ_ALLOC_HOST_ACCESS_RANDOM;
    }
    else if ((usage & (DVZ_DRP2_BUFFER_USAGE_MAP_WRITE | DVZ_DRP2_BUFFER_USAGE_COPY_DST)) != 0)
    {
        flags |= DVZ_ALLOC_HOST_ACCESS_SEQUENTIAL_WRITE;
    }
    return flags;
}


static VkImageUsageFlags _vklite_texture_usage(uint32_t usage)
{
    VkImageUsageFlags out = 0;
    if ((usage & DVZ_DRP2_TEXTURE_USAGE_COPY_SRC) != 0)
        out |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    if ((usage & DVZ_DRP2_TEXTURE_USAGE_COPY_DST) != 0)
        out |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if ((usage & DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING) != 0)
        out |= VK_IMAGE_USAGE_SAMPLED_BIT;
    if ((usage & DVZ_DRP2_TEXTURE_USAGE_STORAGE_BINDING) != 0)
        out |= VK_IMAGE_USAGE_STORAGE_BIT;
    if ((usage & DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT) != 0)
        out |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    return out != 0 ? out : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
}


/**
 * Destroy a partially-created vklite object and return a validation failure.
 *
 * @param object vklite object slot to clean up
 * @param code validation failure code
 * @param command_index command index used for validation reporting
 * @return DRP2 validation failure result
 */
DvzDrp2ValidationResult _vklite_fail_destroy_object(
    Drp2VkliteObject* object, DvzDrp2ValidationCode code, uint32_t command_index)
{
    _vklite_destroy_object(object);
    return _drp2_fail(code, command_index);
}



static DvzDrp2ValidationResult _vklite_create_buffer(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);
    Drp2VkliteObject* object =
        _vklite_add(state, command->u.create_buffer.id, DRP2_OBJECT_BUFFER);
    if (object == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    DvzBuffer* buffer = dvz_buffer_create_wrapper();
    if (buffer == NULL)
        return _vklite_fail_destroy_object(
            object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    object->buffer = buffer;

    dvz_buffer(state->runtime->device, state->runtime->allocator, buffer);
    dvz_buffer_size(buffer, command->u.create_buffer.size);
    dvz_buffer_usage(buffer, _vklite_buffer_usage(command->u.create_buffer.usage));
    dvz_buffer_flags(buffer, _vklite_buffer_alloc_flags(command->u.create_buffer.usage));
    if (dvz_buffer_create(buffer) != 0)
        return _vklite_fail_destroy_object(
            object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    return _drp2_ok();
}


static DvzDrp2ValidationResult _vklite_create_texture(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);
    Drp2VkliteObject* object =
        _vklite_add(state, command->u.create_texture.id, DRP2_OBJECT_TEXTURE);
    if (object == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    DvzImages* images = dvz_images_create_wrapper();
    if (images == NULL)
        return _vklite_fail_destroy_object(
            object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    object->images = images;

    uint32_t depth = command->u.create_texture.depth > 1 ? command->u.create_texture.depth : 1;
    VkFormat format = command->u.create_texture.format != 0 ?
                          (VkFormat)command->u.create_texture.format :
                          VK_FORMAT_R8G8B8A8_UNORM;
    VkImageType img_type = depth > 1 ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D;
    dvz_images(state->runtime->device, state->runtime->allocator, img_type, 1, images);
    dvz_images_format(images, format);
    dvz_images_size(
        images, command->u.create_texture.width, command->u.create_texture.height, depth);
    dvz_images_mip(images, 1);
    dvz_images_layers(images, 1);
    dvz_images_samples(images, VK_SAMPLE_COUNT_1_BIT);
    dvz_images_usage(images, _vklite_texture_usage(command->u.create_texture.usage));
    if (dvz_images_create(images) != 0)
        return _vklite_fail_destroy_object(
            object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    if ((command->u.create_texture.usage &
         (DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING |
          DVZ_DRP2_TEXTURE_USAGE_STORAGE_BINDING |
          DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT)) != 0)
    {
        DvzImageViews* views = dvz_image_views_create_wrapper();
        if (views == NULL)
            return _vklite_fail_destroy_object(
                object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
        object->views = views;
        dvz_image_views(images, views);
        dvz_image_views_create(views);
        if (dvz_image_views_handle(views, 0) == VK_NULL_HANDLE)
            return _vklite_fail_destroy_object(
                object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }

    object->image_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    object->usage = command->u.create_texture.usage;
    object->format = (uint32_t)format;
    object->width = command->u.create_texture.width;
    object->height = command->u.create_texture.height;
    return _drp2_ok();
}



/**
 * Retire a borrowed frame target's previous depth attachment after its command buffer completes.
 *
 * @param state vklite runtime state
 * @param object borrowed frame target object that owns the depth attachment pointers
 * @return whether the previous depth attachment was retired
 */
bool _vklite_retire_frame_target_depth(
    Drp2VkliteState* state, Drp2VkliteObject* object)
{
    ANN(state);
    ANN(object);
    if (object->depth_images == NULL && object->depth_views == NULL)
        return true;

    Drp2VkliteObject retired = {0};
    retired.kind = DRP2_OBJECT_TEXTURE;
    retired.depth_images = object->depth_images;
    retired.depth_views = object->depth_views;
    object->depth_images = NULL;
    object->depth_views = NULL;

    VkCommandBuffer previous_command_buffer = object->command_buffer;
    if (previous_command_buffer != VK_NULL_HANDLE)
    {
        if (_vklite_defer_destroy_object(state, &retired, previous_command_buffer))
            return true;
        object->depth_images = retired.depth_images;
        object->depth_views = retired.depth_views;
        return false;
    }

    _vklite_destroy_object(&retired);
    return true;
}



/**
 * Attach a borrowed frame image as a vklite texture object.
 *
 * @param runtime the DRP2 runtime
 * @param texture_id the DRP2 texture id
 * @param frame the borrowed stream frame
 * @return true when the frame target was attached
 */
bool _vklite_attach_frame_target(
    DvzDrp2Runtime* runtime, uint64_t texture_id, const DvzStreamFrame* frame)
{
    ANN(runtime);
    if (!_drp2_frame_target_valid(texture_id, frame))
        return false;

    if (runtime->vklite_state == NULL)
    {
        runtime->vklite_state = (Drp2VkliteState*)dvz_calloc(1, sizeof(Drp2VkliteState));
        if (runtime->vklite_state == NULL)
            return false;
        runtime->vklite_state->runtime = runtime;
    }
    _vklite_flush_deferred_for_command_buffer(runtime->vklite_state, frame->command_buffer);
    runtime->vklite_state->active_borrowed_command_buffer = frame->command_buffer;

    DvzImages* images = dvz_images_create_wrapper();
    if (images == NULL)
        return false;
    dvz_images_wrap(runtime->device, runtime->allocator, VK_IMAGE_TYPE_2D, frame->image, images);
    dvz_images_format(images, frame->color_format);
    dvz_images_size(images, frame->extent.width, frame->extent.height, 1);

    Drp2VkliteObject* object = _vklite_find(runtime->vklite_state, texture_id);
    if (object == NULL)
        object = _vklite_add(runtime->vklite_state, texture_id, DRP2_OBJECT_TEXTURE);
    if (object == NULL || object->kind != DRP2_OBJECT_TEXTURE)
    {
        dvz_images_free(images);
        return false;
    }

    if (!_vklite_retire_frame_target_depth(runtime->vklite_state, object))
    {
        dvz_images_free(images);
        return false;
    }

    if (object->views != NULL)
    {
        dvz_image_views_destroy(object->views);
        dvz_image_views_free(object->views);
        object->views = NULL;
    }
    object->image_view = VK_NULL_HANDLE;
    if (object->images != NULL)
    {
        dvz_images_destroy(object->images);
        dvz_images_free(object->images);
        object->images = NULL;
    }

    object->images = images;
    object->image_layout = frame->image_layout;
    object->command_buffer = frame->command_buffer;
    object->image_view = frame->image_view;
    object->width = frame->extent.width;
    object->height = frame->extent.height;
    object->borrowed_frame_target = true;
    object->destroyed = false;
    return true;
}


/**
 * Create an owned command-buffer wrapper for immediate DRP2 runtime work.
 *
 * @param device the borrowed Vulkan device wrapper
 * @return owned command-buffer wrapper, or NULL on failure
 */
DvzCommands* _vklite_owned_commands_create(DvzDevice* device)
{
    ANN(device);

    DvzQueue* queue = dvz_device_queue(device, DVZ_QUEUE_MAIN);
    if (queue == NULL)
        return NULL;

    DvzCommands* cmds = dvz_commands_create_wrapper();
    if (cmds == NULL)
        return NULL;

    dvz_commands(device, queue, 1, cmds);
    if (dvz_commands_count(cmds) == 0 || dvz_commands_handle(cmds) == VK_NULL_HANDLE)
    {
        dvz_commands_free(cmds);
        return NULL;
    }
    return cmds;
}


/**
 * Destroy an owned DRP2 command-buffer wrapper.
 *
 * @param cmds owned command-buffer wrapper to destroy and free
 */
void _vklite_owned_commands_destroy(DvzCommands* cmds)
{
    if (cmds == NULL)
        return;
    dvz_commands_destroy(cmds);
    dvz_commands_free(cmds);
}


/**
 * Wrap a borrowed frame command buffer that is already recording.
 *
 * DRP2 may record commands into the returned wrapper, but must not begin, end,
 * reset, submit, or destroy the borrowed command buffer.
 *
 * @param device the borrowed Vulkan device wrapper
 * @param command_buffer borrowed recording command buffer
 * @return borrowed command-buffer wrapper, or NULL on failure
 */
DvzCommands*
_vklite_borrowed_frame_commands_create(DvzDevice* device, VkCommandBuffer command_buffer)
{
    ANN(device);
    if (command_buffer == VK_NULL_HANDLE)
        return NULL;

    DvzCommands* cmds = dvz_commands_create_wrapper();
    if (cmds == NULL)
        return NULL;

    dvz_commands_wrap_borrowed_recording(device, command_buffer, cmds);
    return cmds;
}


/**
 * Free a borrowed frame command-buffer wrapper without touching the Vulkan command buffer.
 *
 * @param cmds borrowed command-buffer wrapper to free
 */
void _vklite_borrowed_frame_commands_free(DvzCommands* cmds)
{
    dvz_commands_free(cmds);
}


/**
 * End and submit an owned vklite command buffer, surfacing Vulkan failures to DRP2.
 *
 * @param cmds owned command-buffer wrapper
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
DvzDrp2ValidationResult
_vklite_owned_commands_end_submit(DvzCommands* cmds, uint32_t command_index)
{
    ANN(cmds);
    if (dvz_cmd_end_result(cmds) != 0)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (dvz_cmd_submit_result(cmds) != 0)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    return _drp2_ok();
}


static DvzDrp2ValidationResult _vklite_destroy_backend_object(
    Drp2VkliteState* state, uint64_t id, Drp2ObjectKind kind, uint32_t command_index)
{
    ANN(state);
    Drp2VkliteObject* object = _vklite_find(state, id);
    if (object == NULL || object->kind != kind)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (state->active_borrowed_command_buffer != VK_NULL_HANDLE &&
        _vklite_defer_destroy_object(state, object, state->active_borrowed_command_buffer))
        return _drp2_ok();
    _vklite_destroy_object_slot(state, object);
    return _drp2_ok();
}


/**
 * Destroy a vklite shader module object by DRP2 object id.
 *
 * @param state vklite runtime state
 * @param id DRP2 shader module object id
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
static DvzDrp2ValidationResult _vklite_destroy_shader_module(
    Drp2VkliteState* state, uint64_t id, uint32_t command_index)
{
    ANN(state);
    Drp2VkliteObject* object = _vklite_find(state, id);
    if (object == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (object->kind != DRP2_OBJECT_SHADER_VERTEX &&
        object->kind != DRP2_OBJECT_SHADER_FRAGMENT &&
        object->kind != DRP2_OBJECT_SHADER_COMPUTE)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (state->active_borrowed_command_buffer != VK_NULL_HANDLE &&
        _vklite_defer_destroy_object(state, object, state->active_borrowed_command_buffer))
        return _drp2_ok();
    _vklite_destroy_object_slot(state, object);
    return _drp2_ok();
}


DvzDrp2ValidationResult
_vklite_execute(DvzDrp2Runtime* runtime, const DvzDrp2CommandStream* stream)
{
    ANN(runtime);
    ANN(stream);
    if (runtime->vklite_state == NULL)
    {
        runtime->vklite_state = (Drp2VkliteState*)dvz_calloc(1, sizeof(Drp2VkliteState));
        ANN(runtime->vklite_state);
    }

    Drp2VkliteState* state = runtime->vklite_state;
    state->runtime = runtime;
    DvzDrp2ValidationResult result = _drp2_ok();

    for (uint32_t i = 0; i < stream->count; i++)
    {
        const DvzDrp2Command* command = &stream->commands[i];
        switch (command->type)
        {
        case DVZ_DRP2_COMMAND_CREATE_BUFFER:
            result = _vklite_create_buffer(state, command, i);
            break;
        case DVZ_DRP2_COMMAND_DESTROY_BUFFER:
            result = _vklite_destroy_backend_object(
                state, command->u.destroy_buffer.buffer_id, DRP2_OBJECT_BUFFER, i);
            break;
        case DVZ_DRP2_COMMAND_CREATE_TEXTURE:
            result = _vklite_create_texture(state, command, i);
            break;
        case DVZ_DRP2_COMMAND_DESTROY_TEXTURE:
            result = _vklite_destroy_backend_object(
                state, command->u.destroy_texture.texture_id, DRP2_OBJECT_TEXTURE, i);
            break;
        case DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE:
            result = _vklite_create_shader_module(state, command, i);
            break;
        case DVZ_DRP2_COMMAND_DESTROY_SHADER_MODULE:
            result = _vklite_destroy_shader_module(
                state, command->u.destroy_shader_module.shader_module_id, i);
            break;
        case DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE:
            result = _vklite_create_render_pipeline(state, command, i);
            break;
        case DVZ_DRP2_COMMAND_CREATE_COMPUTE_PIPELINE:
            result = _vklite_create_compute_pipeline(state, command, i);
            break;
        case DVZ_DRP2_COMMAND_DESTROY_RENDER_PIPELINE:
            result = _vklite_destroy_backend_object(
                state, command->u.destroy_render_pipeline.render_pipeline_id,
                DRP2_OBJECT_RENDER_PIPELINE, i);
            break;
        case DVZ_DRP2_COMMAND_DESTROY_COMPUTE_PIPELINE:
            result = _vklite_destroy_backend_object(
                state, command->u.destroy_compute_pipeline.compute_pipeline_id,
                DRP2_OBJECT_COMPUTE_PIPELINE, i);
            break;
        case DVZ_DRP2_COMMAND_CREATE_SAMPLER:
            result = _vklite_create_sampler(state, command, i);
            break;
        case DVZ_DRP2_COMMAND_CREATE_BIND_GROUP_LAYOUT:
            result = _vklite_create_bind_group_layout(state, command, i);
            break;
        case DVZ_DRP2_COMMAND_DESTROY_BIND_GROUP_LAYOUT:
            result = _vklite_destroy_backend_object(
                state, command->u.destroy_bind_group_layout.bind_group_layout_id,
                DRP2_OBJECT_BIND_GROUP_LAYOUT, i);
            break;
        case DVZ_DRP2_COMMAND_CREATE_BIND_GROUP:
            result = _vklite_create_bind_group(state, command, i);
            break;
        case DVZ_DRP2_COMMAND_DESTROY_BIND_GROUP:
            result = _vklite_destroy_backend_object(
                state, command->u.destroy_bind_group.bind_group_id, DRP2_OBJECT_BIND_GROUP, i);
            break;
        case DVZ_DRP2_COMMAND_WRITE_BUFFER:
            result = _vklite_write_buffer(state, command, i);
            break;
        case DVZ_DRP2_COMMAND_WRITE_TEXTURE:
            result = _vklite_write_texture(state, command, i);
            break;
        case DVZ_DRP2_COMMAND_COPY_BUFFER_TO_BUFFER:
            result = _vklite_copy_buffer_to_buffer(state, command, i);
            break;
        case DVZ_DRP2_COMMAND_COPY_BUFFER_TO_TEXTURE:
            result = _vklite_copy_buffer_to_texture(state, command, i);
            break;
        case DVZ_DRP2_COMMAND_COPY_TEXTURE_TO_BUFFER:
            result = _vklite_copy_texture_to_buffer(state, command, i);
            break;
        case DVZ_DRP2_COMMAND_COPY_TEXTURE_TO_TEXTURE:
            result = _vklite_copy_texture_to_texture(state, command, i);
            break;
        case DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS:
            result = _vklite_begin_render_pass(state, command, i);
            break;
        case DVZ_DRP2_COMMAND_BEGIN_COMPUTE_PASS:
            result = _vklite_begin_compute_pass(state, command, i);
            break;
        case DVZ_DRP2_COMMAND_SET_VIEWPORT:
            result = _vklite_set_viewport(state, command, i);
            break;
        case DVZ_DRP2_COMMAND_SET_SCISSOR:
            result = _vklite_set_scissor(state, command, i);
            break;
        case DVZ_DRP2_COMMAND_SET_PIPELINE:
            result = _vklite_set_pipeline(state, command, i);
            break;
        case DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER:
            result = _vklite_set_vertex_buffer(state, command, i);
            break;
        case DVZ_DRP2_COMMAND_SET_INDEX_BUFFER:
            result = _vklite_set_index_buffer(state, command, i);
            break;
        case DVZ_DRP2_COMMAND_SET_BIND_GROUP:
            result = _vklite_set_bind_group(state, command, i);
            break;
        case DVZ_DRP2_COMMAND_DRAW:
            result = _vklite_draw(state, command, i);
            break;
        case DVZ_DRP2_COMMAND_DRAW_INDEXED:
            result = _vklite_draw_indexed(state, command, i);
            break;
        case DVZ_DRP2_COMMAND_DISPATCH_WORKGROUPS:
            result = _vklite_dispatch_workgroups(state, command, i);
            break;
        case DVZ_DRP2_COMMAND_END_RENDER_PASS:
            result = _vklite_end_render_pass(state, command->u.end_render_pass.pass_id, i);
            break;
        case DVZ_DRP2_COMMAND_END_COMPUTE_PASS:
            result = _vklite_end_compute_pass(state, command->u.end_compute_pass.pass_id, i);
            break;
        default:
            result = _drp2_ok();
            break;
        }

        if (!result.ok)
            break;
    }

    state->active_borrowed_command_buffer = VK_NULL_HANDLE;
    return result;
}
#endif
