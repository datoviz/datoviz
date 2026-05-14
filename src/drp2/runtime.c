/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  DRP2 runtime semantic validation                                                            */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#if DVZ_DRP2_HAS_VKLITE
#include <volk.h>
#endif

#include "_alloc.h"
#include "_assertions.h"
#include "_base64.h"
#include "_log.h"
#include "_overflow.h"
#include "_runtime.h"
#include "_stream.h"
#include "datoviz/stream/frame_stream.h"
#include "datoviz/vklite/descriptors.h"
#include "datoviz/vk/gpu_ctx.h"

#if DVZ_DRP2_HAS_VKLITE
#include "datoviz/vk/device.h"
#include "datoviz/vklite/buffers.h"
#include "datoviz/vklite/compute.h"
#include "datoviz/vklite/commands.h"
#include "datoviz/vklite/descriptors.h"
#include "datoviz/vklite/graphics.h"
#include "datoviz/vklite/images.h"
#include "datoviz/vklite/rendering.h"
#include "datoviz/vklite/sampler.h"
#include "datoviz/vklite/shader.h"
#include "datoviz/vklite/slots.h"
#include "datoviz/vklite/sync.h"
#endif



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#ifndef DVZ_DRP2_HAS_VKLITE
#define DVZ_DRP2_HAS_VKLITE 0
#endif



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

#if DVZ_DRP2_HAS_VKLITE
bool _dvz_drp2_runtime_vklite_download_buffer(
    DvzDrp2Runtime* runtime, uint64_t buffer_id, uint64_t offset, uint64_t size, void* data);
#endif


/**
 * Return the smaller of two 32-bit unsigned integers.
 *
 * @param a the first value
 * @param b the second value
 * @return the smaller value
 */
static uint32_t _min_u32(uint32_t a, uint32_t b)
{
    return a < b ? a : b;
}


/**
 * Return whether a borrowed stream frame can be exposed as a render target.
 *
 * @param texture_id the DRP2 texture id assigned to the frame
 * @param frame the borrowed stream frame
 * @return whether the frame has the required target handles and extent
 */
static bool _frame_target_valid(uint64_t texture_id, const DvzStreamFrame* frame)
{
    if (texture_id == 0 || frame == NULL)
        return false;
    if (frame->image == VK_NULL_HANDLE || frame->image_view == VK_NULL_HANDLE ||
        frame->command_buffer == VK_NULL_HANDLE)
        return false;
    if (!frame->image_borrowed || !frame->image_view_borrowed || !frame->command_buffer_borrowed)
        return false;
    if (!frame->command_buffer_recording)
        return false;
    if (frame->color_format == VK_FORMAT_UNDEFINED)
        return false;
    if (frame->image_layout != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
        return false;
    if ((frame->usage & DVZ_STREAM_FRAME_USAGE_RENDER_TARGET) == 0)
        return false;
    return frame->extent.width != 0 && frame->extent.height != 0;
}



static DvzDrp2ValidationResult _result(
    bool ok, DvzDrp2ValidationCode code, uint32_t command_index)
{
    DvzDrp2ValidationResult result = {0};
    result.ok = ok;
    result.code = code;
    result.command_index = command_index;
    return result;
}



/**
 * Return a successful DRP2 validation result.
 *
 * @return successful validation result
 */
DvzDrp2ValidationResult _drp2_ok(void)
{
    return _result(true, DVZ_DRP2_VALIDATION_OK, UINT32_MAX);
}



/**
 * Return a failed DRP2 validation result.
 *
 * @param code validation failure code
 * @param command_index command index used for validation reporting
 * @return failed validation result
 */
DvzDrp2ValidationResult _drp2_fail(DvzDrp2ValidationCode code, uint32_t command_index)
{
    return _result(false, code, command_index);
}



static bool _range_overflows(uint64_t offset, uint64_t size, uint64_t total)
{
    if (offset > total)
        return true;
    if (size > total - offset)
        return true;
    return false;
}



static bool _texture_box_overflows(
    const Drp2Object* texture, uint32_t origin_x, uint32_t origin_y, uint32_t origin_z,
    uint32_t width, uint32_t height, uint32_t depth)
{
    ANN(texture);
    if (width == 0 || height == 0 || depth == 0)
        return true;
    if (origin_x > texture->width || width > texture->width - origin_x)
        return true;
    if (origin_y > texture->height || height > texture->height - origin_y)
        return true;
    if (origin_z > texture->depth || depth > texture->depth - origin_z)
        return true;
    return false;
}



static bool _texture_layout_invalid(
    uint32_t width, uint32_t height, uint32_t depth, uint32_t bytes_per_row,
    uint32_t rows_per_image)
{
    if (width == 0 || height == 0 || depth == 0)
        return true;
    uint64_t min_row = (uint64_t)width * DVZ_DRP2_RGBA8_BYTES_PER_TEXEL;
    if (bytes_per_row < min_row)
        return true;
    if (rows_per_image < height)
        return true;
    return false;
}



/**
 * Return the byte size implied by a texture transfer layout.
 *
 * @param depth texture region depth
 * @param bytes_per_row byte pitch for one row
 * @param rows_per_image row pitch for one image slice
 * @return required byte size
 */
uint64_t _drp2_texture_layout_size(
    uint32_t depth, uint32_t bytes_per_row, uint32_t rows_per_image)
{
    return (uint64_t)depth * rows_per_image * bytes_per_row;
}



static bool _ensure_capacity(Drp2RuntimeState* state)
{
    ANN(state);
    if (state->objects == NULL || state->capacity == 0)
    {
        state->capacity = DVZ_DRP2_RUNTIME_INITIAL_OBJECT_CAPACITY;
        state->objects = (Drp2Object*)dvz_calloc(state->capacity, sizeof(Drp2Object));
        return state->objects != NULL;
    }

    if (state->count < state->capacity)
        return true;

    if (state->capacity > UINT32_MAX / 2)
        return false;
    uint32_t capacity = state->capacity * 2;
    uint64_t bytes = 0;
    if (_dvz_mul_u64_overflows(capacity, sizeof(Drp2Object), &bytes))
        return false;

    Drp2Object* objects = (Drp2Object*)dvz_realloc(state->objects, bytes);
    if (objects == NULL)
        return false;

    state->capacity = capacity;
    state->objects = objects;
    return true;
}



static Drp2Object* _find_any_object(Drp2RuntimeState* state, uint64_t id)
{
    ANN(state);
    for (uint32_t i = state->count; i > 0; i--)
    {
        Drp2Object* object = &state->objects[i - 1];
        if (object->id == id)
            return object;
    }
    return NULL;
}



static Drp2Object* _find_object(Drp2RuntimeState* state, uint64_t id)
{
    Drp2Object* object = _find_any_object(state, id);
    if (object == NULL || object->destroyed)
        return NULL;
    return object;
}



static const Drp2Object* _find_any_object_const(const Drp2RuntimeState* state, uint64_t id)
{
    ANN(state);
    for (uint32_t i = state->count; i > 0; i--)
    {
        const Drp2Object* object = &state->objects[i - 1];
        if (object->id == id)
            return object;
    }
    return NULL;
}



static const Drp2Object* _find_object_const(const Drp2RuntimeState* state, uint64_t id)
{
    const Drp2Object* object = _find_any_object_const(state, id);
    if (object == NULL || object->destroyed)
        return NULL;
    return object;
}



static Drp2Object* _add_object(Drp2RuntimeState* state, uint64_t id, Drp2ObjectKind kind)
{
    ANN(state);
    if (!_ensure_capacity(state))
        return NULL;

    Drp2Object* object = &state->objects[state->count++];
    dvz_memset(object, sizeof(Drp2Object), 0, sizeof(Drp2Object));
    object->id = id;
    object->kind = kind;
    return object;
}



static bool _has_object_kind(const Drp2RuntimeState* state, uint64_t id, Drp2ObjectKind kind)
{
    const Drp2Object* object = _find_object_const(state, id);
    return object != NULL && object->kind == kind;
}



static void _mark_referenced(Drp2RuntimeState* state, uint64_t id)
{
    Drp2Object* object = _find_object(state, id);
    if (object != NULL)
        object->referenced_by_work = true;
}



/**
 * Remove destroyed objects from the end of the semantic object table.
 *
 * @param state the runtime semantic state
 */
static void _trim_destroyed_tail(Drp2RuntimeState* state)
{
    ANN(state);
    while (state->count > 0 && state->objects[state->count - 1].destroyed)
    {
        state->count--;
        dvz_memset(
            &state->objects[state->count], sizeof(Drp2Object), 0, sizeof(Drp2Object));
    }
}



/**
 * Retire transient encoder, pass, and command-buffer objects after queue submission.
 *
 * @param state the runtime semantic state
 * @param command_buffer the submitted command-buffer object
 */
static void _retire_submitted_work(Drp2RuntimeState* state, Drp2Object* command_buffer)
{
    ANN(state);
    ANN(command_buffer);
    uint64_t encoder_id = command_buffer->encoder_id;

    command_buffer->submitted = true;
    command_buffer->destroyed = true;
    if (encoder_id == 0)
    {
        _trim_destroyed_tail(state);
        return;
    }

    for (uint32_t i = state->count; i > 0; i--)
    {
        Drp2Object* object = &state->objects[i - 1];
        if (object->destroyed)
            continue;
        if (object->id == encoder_id ||
            ((object->kind == DRP2_OBJECT_RENDER_PASS ||
              object->kind == DRP2_OBJECT_COMPUTE_PASS) &&
             object->encoder_id == encoder_id))
        {
            object->open = false;
            object->destroyed = true;
        }
    }
    _trim_destroyed_tail(state);
}



static bool _pipeline_uses_shader(const Drp2Object* object, uint64_t shader_module_id)
{
    ANN(object);
    if (object->kind == DRP2_OBJECT_RENDER_PIPELINE)
    {
        return object->vertex_shader_module_id == shader_module_id ||
               object->fragment_shader_module_id == shader_module_id;
    }
    if (object->kind == DRP2_OBJECT_COMPUTE_PIPELINE)
        return object->compute_shader_module_id == shader_module_id;
    return false;
}



static DvzDrp2ValidationResult _validate_destroy_object(
    Drp2RuntimeState* state, uint64_t id, Drp2ObjectKind kind, uint32_t command_index)
{
    ANN(state);
    Drp2Object* object = _find_any_object(state, id);
    if (object == NULL || object->destroyed || object->kind != kind)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (object->referenced_by_work || object->open || object->submitted)
        return _drp2_fail(DVZ_DRP2_VALIDATION_USAGE, command_index);

    if (kind == DRP2_OBJECT_SHADER_VERTEX || kind == DRP2_OBJECT_SHADER_FRAGMENT ||
        kind == DRP2_OBJECT_SHADER_COMPUTE)
    {
        for (uint32_t i = 0; i < state->count; i++)
        {
            Drp2Object* other = &state->objects[i];
            if (!other->destroyed && _pipeline_uses_shader(other, id))
                return _drp2_fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
        }
    }
    else if (kind == DRP2_OBJECT_BIND_GROUP_LAYOUT)
    {
        for (uint32_t i = 0; i < state->count; i++)
        {
            Drp2Object* other = &state->objects[i];
            if (other->destroyed)
                continue;
            if ((other->kind == DRP2_OBJECT_BIND_GROUP ||
                 other->kind == DRP2_OBJECT_RENDER_PIPELINE ||
                 other->kind == DRP2_OBJECT_COMPUTE_PIPELINE) &&
                other->bind_group_layout_id == id)
                return _drp2_fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
        }
    }

    object->destroyed = true;
    return _drp2_ok();
}



static Drp2Object* _open_pass(Drp2RuntimeState* state)
{
    ANN(state);
    for (uint32_t i = 0; i < state->count; i++)
    {
        if ((state->objects[i].kind == DRP2_OBJECT_RENDER_PASS ||
             state->objects[i].kind == DRP2_OBJECT_COMPUTE_PASS) &&
            state->objects[i].open)
            return &state->objects[i];
    }
    return NULL;
}



static DvzDrp2ValidationResult _validate_ready(
    const Drp2RuntimeState* state, uint32_t command_index)
{
    ANN(state);
    if (!state->hello_seen || !state->reply_seen || state->failed)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    return _drp2_ok();
}



static DvzDrp2ValidationResult _validate_create_buffer(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    uint64_t id = command->u.create_buffer.id;
    uint64_t size = command->u.create_buffer.size;
    if (id == 0 || size == 0)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, command_index);
    if (_find_any_object(state, id) != NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2Object* object = _add_object(state, id, DRP2_OBJECT_BUFFER);
    if (object == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    object->size = size;
    object->usage = command->u.create_buffer.usage;
    return _drp2_ok();
}



static DvzDrp2ValidationResult _validate_create_texture(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    uint64_t id = command->u.create_texture.id;
    if (id == 0 || command->u.create_texture.width == 0 || command->u.create_texture.height == 0)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, command_index);
    if (_find_any_object(state, id) != NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    Drp2Object* object = _add_object(state, id, DRP2_OBJECT_TEXTURE);
    if (object == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    object->width  = command->u.create_texture.width;
    object->height = command->u.create_texture.height;
    object->depth  = command->u.create_texture.depth > 1 ? command->u.create_texture.depth : 1;
    object->usage  = command->u.create_texture.usage;
    return _drp2_ok();
}



static DvzDrp2ValidationResult _validate_create_shader_module(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    uint64_t id = command->u.create_shader_module.id;
    if (id == 0 || command->u.create_shader_module.stage[0] == '\0')
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, command_index);
    if (_find_any_object(state, id) != NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2ObjectKind kind = DRP2_OBJECT_NONE;
    if (strcmp(command->u.create_shader_module.stage, "vertex") == 0 ||
        strcmp(command->u.create_shader_module.stage, "VERTEX") == 0)
        kind = DRP2_OBJECT_SHADER_VERTEX;
    else if (strcmp(command->u.create_shader_module.stage, "fragment") == 0 ||
             strcmp(command->u.create_shader_module.stage, "FRAGMENT") == 0)
        kind = DRP2_OBJECT_SHADER_FRAGMENT;
    else if (strcmp(command->u.create_shader_module.stage, "compute") == 0 ||
             strcmp(command->u.create_shader_module.stage, "COMPUTE") == 0)
        kind = DRP2_OBJECT_SHADER_COMPUTE;
    else
        return _drp2_fail(DVZ_DRP2_VALIDATION_USAGE, command_index);

    if (_add_object(state, id, kind) == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    return _drp2_ok();
}



static DvzDrp2ValidationResult _validate_create_render_pipeline(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    uint64_t id = command->u.create_render_pipeline.id;
    if (id == 0)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, command_index);
    if (_find_any_object(state, id) != NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (!_has_object_kind(
            state, command->u.create_render_pipeline.vertex_shader_module_id,
            DRP2_OBJECT_SHADER_VERTEX))
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (!_has_object_kind(
            state, command->u.create_render_pipeline.fragment_shader_module_id,
            DRP2_OBJECT_SHADER_FRAGMENT))
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (command->u.create_render_pipeline.bind_group_layout_id != 0 &&
        !_has_object_kind(
            state, command->u.create_render_pipeline.bind_group_layout_id,
            DRP2_OBJECT_BIND_GROUP_LAYOUT))
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (command->u.create_render_pipeline.bind_group_layout_id2 != 0 &&
        !_has_object_kind(
            state, command->u.create_render_pipeline.bind_group_layout_id2,
            DRP2_OBJECT_BIND_GROUP_LAYOUT))
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2Object* object = _add_object(state, id, DRP2_OBJECT_RENDER_PIPELINE);
    if (object == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    object->vertex_buffer_slots = command->u.create_render_pipeline.vertex_buffer_slots;
    object->vertex_shader_module_id = command->u.create_render_pipeline.vertex_shader_module_id;
    object->fragment_shader_module_id = command->u.create_render_pipeline.fragment_shader_module_id;
    object->bind_group_layout_id  = command->u.create_render_pipeline.bind_group_layout_id;
    object->bind_group_layout_id2 = command->u.create_render_pipeline.bind_group_layout_id2;
    object->has_depth_attachment = command->u.create_render_pipeline.has_depth_attachment;
    object->depth_write_enabled = command->u.create_render_pipeline.depth_write_enabled;
    object->depth_compare_op = command->u.create_render_pipeline.depth_compare_op;
    return _drp2_ok();
}



static DvzDrp2ValidationResult _validate_create_compute_pipeline(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    uint64_t id = command->u.create_compute_pipeline.id;
    if (id == 0)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, command_index);
    if (_find_any_object(state, id) != NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (!_has_object_kind(
            state, command->u.create_compute_pipeline.compute_shader_module_id,
            DRP2_OBJECT_SHADER_COMPUTE))
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (command->u.create_compute_pipeline.bind_group_layout_id != 0 &&
        !_has_object_kind(
            state, command->u.create_compute_pipeline.bind_group_layout_id,
            DRP2_OBJECT_BIND_GROUP_LAYOUT))
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2Object* object = _add_object(state, id, DRP2_OBJECT_COMPUTE_PIPELINE);
    if (object == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    object->compute_shader_module_id = command->u.create_compute_pipeline.compute_shader_module_id;
    object->bind_group_layout_id = command->u.create_compute_pipeline.bind_group_layout_id;
    return _drp2_ok();
}



static DvzDrp2ValidationResult _validate_destroy_buffer(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(command);
    return _validate_destroy_object(
        state, command->u.destroy_buffer.buffer_id, DRP2_OBJECT_BUFFER, command_index);
}



static DvzDrp2ValidationResult _validate_destroy_texture(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(command);
    return _validate_destroy_object(
        state, command->u.destroy_texture.texture_id, DRP2_OBJECT_TEXTURE, command_index);
}



static DvzDrp2ValidationResult _validate_destroy_shader_module(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    uint64_t id = command->u.destroy_shader_module.shader_module_id;
    Drp2Object* object = _find_any_object(state, id);
    if (object == NULL || object->destroyed)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (object->kind != DRP2_OBJECT_SHADER_VERTEX && object->kind != DRP2_OBJECT_SHADER_FRAGMENT &&
        object->kind != DRP2_OBJECT_SHADER_COMPUTE)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    return _validate_destroy_object(state, id, object->kind, command_index);
}



static DvzDrp2ValidationResult _validate_destroy_render_pipeline(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(command);
    return _validate_destroy_object(
        state, command->u.destroy_render_pipeline.render_pipeline_id, DRP2_OBJECT_RENDER_PIPELINE,
        command_index);
}



static DvzDrp2ValidationResult _validate_destroy_compute_pipeline(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(command);
    return _validate_destroy_object(
        state, command->u.destroy_compute_pipeline.compute_pipeline_id,
        DRP2_OBJECT_COMPUTE_PIPELINE, command_index);
}



static DvzDrp2ValidationResult _validate_create_sampler(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    uint64_t id = command->u.create_sampler.id;
    if (id == 0)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, command_index);
    if (_find_any_object(state, id) != NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (_add_object(state, id, DRP2_OBJECT_SAMPLER) == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    return _drp2_ok();
}



static DvzDrp2ValidationResult _validate_create_bind_group_layout(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    uint64_t id = command->u.create_bind_group_layout.id;
    if (id == 0)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, command_index);
    if (_find_any_object(state, id) != NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    Drp2Object* object = _add_object(state, id, DRP2_OBJECT_BIND_GROUP_LAYOUT);
    if (object == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    object->storage_buffers = command->u.create_bind_group_layout.storage_buffers;
    object->uniform_buffer  = command->u.create_bind_group_layout.uniform_buffer;
    return _drp2_ok();
}



static DvzDrp2ValidationResult _validate_create_bind_group(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    uint64_t id = command->u.create_bind_group.id;
    if (id == 0)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, command_index);
    if (_find_any_object(state, id) != NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (!_has_object_kind(
            state, command->u.create_bind_group.bind_group_layout_id,
            DRP2_OBJECT_BIND_GROUP_LAYOUT))
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2Object* layout = _find_object(state, command->u.create_bind_group.bind_group_layout_id);
    ANN(layout);
    bool storage_buffers = layout->storage_buffers;
    bool uniform_buffer  = layout->uniform_buffer;
    if (uniform_buffer)
    {
        Drp2Object* buffer0 = _find_object(state, command->u.create_bind_group.buffer0_id);
        if (buffer0 == NULL || buffer0->kind != DRP2_OBJECT_BUFFER)
            return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
        if ((buffer0->usage & DVZ_DRP2_BUFFER_USAGE_UNIFORM) == 0)
            return _drp2_fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
        uint64_t offset = command->u.create_bind_group.buffer0_offset;
        uint64_t size   = command->u.create_bind_group.buffer_size;
        if (_range_overflows(offset, size, buffer0->size))
            return _drp2_fail(DVZ_DRP2_VALIDATION_OUT_OF_RANGE, command_index);
    }
    else if (storage_buffers)
    {
        Drp2Object* buffer0 = _find_object(state, command->u.create_bind_group.buffer0_id);
        Drp2Object* buffer1 = _find_object(state, command->u.create_bind_group.buffer1_id);
        if (buffer0 == NULL || buffer0->kind != DRP2_OBJECT_BUFFER || buffer1 == NULL ||
            buffer1->kind != DRP2_OBJECT_BUFFER)
            return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
        if ((buffer0->usage & DVZ_DRP2_BUFFER_USAGE_STORAGE) == 0 ||
            (buffer1->usage & DVZ_DRP2_BUFFER_USAGE_STORAGE) == 0)
            return _drp2_fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
        if (_range_overflows(0, command->u.create_bind_group.buffer_size, buffer0->size) ||
            _range_overflows(0, command->u.create_bind_group.buffer_size, buffer1->size))
            return _drp2_fail(DVZ_DRP2_VALIDATION_OUT_OF_RANGE, command_index);
    }
    else
    {
        Drp2Object* texture = _find_object(state, command->u.create_bind_group.texture_id);
        if (texture == NULL || texture->kind != DRP2_OBJECT_TEXTURE)
            return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
        if ((texture->usage & DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING) == 0)
            return _drp2_fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
        if (!_has_object_kind(state, command->u.create_bind_group.sampler_id, DRP2_OBJECT_SAMPLER))
            return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }

    Drp2Object* object = _add_object(state, id, DRP2_OBJECT_BIND_GROUP);
    if (object == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    object->bind_group_layout_id = command->u.create_bind_group.bind_group_layout_id;
    object->texture_id = command->u.create_bind_group.texture_id;
    object->sampler_id = command->u.create_bind_group.sampler_id;
    object->buffer0_id = command->u.create_bind_group.buffer0_id;
    object->buffer1_id = command->u.create_bind_group.buffer1_id;
    object->buffer_size = command->u.create_bind_group.buffer_size;
    object->storage_buffers = storage_buffers;
    object->uniform_buffer  = uniform_buffer;
    return _drp2_ok();
}



static DvzDrp2ValidationResult _validate_destroy_bind_group_layout(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(command);
    return _validate_destroy_object(
        state, command->u.destroy_bind_group_layout.bind_group_layout_id,
        DRP2_OBJECT_BIND_GROUP_LAYOUT, command_index);
}



static DvzDrp2ValidationResult _validate_destroy_bind_group(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(command);
    return _validate_destroy_object(
        state, command->u.destroy_bind_group.bind_group_id, DRP2_OBJECT_BIND_GROUP,
        command_index);
}



static DvzDrp2ValidationResult _validate_write_buffer(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    Drp2Object* object = _find_object(state, command->u.write_buffer.buffer_id);
    if (object == NULL || object->kind != DRP2_OBJECT_BUFFER)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if ((object->usage & (DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_WRITE)) == 0)
        return _drp2_fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
    if (_range_overflows(command->u.write_buffer.offset, command->u.write_buffer.size, object->size))
        return _drp2_fail(DVZ_DRP2_VALIDATION_OUT_OF_RANGE, command_index);
    return _drp2_ok();
}



static DvzDrp2ValidationResult _validate_write_texture(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    Drp2Object* texture = _find_object(state, command->u.write_texture.texture_id);
    if (texture == NULL || texture->kind != DRP2_OBJECT_TEXTURE)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if ((texture->usage & DVZ_DRP2_TEXTURE_USAGE_COPY_DST) == 0)
        return _drp2_fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
    if (command->u.write_texture.mip_level != 0)
        return _drp2_fail(DVZ_DRP2_VALIDATION_OUT_OF_RANGE, command_index);
    if (_texture_box_overflows(
            texture, command->u.write_texture.origin_x, command->u.write_texture.origin_y,
            command->u.write_texture.origin_z, command->u.write_texture.width,
            command->u.write_texture.height, command->u.write_texture.depth))
        return _drp2_fail(DVZ_DRP2_VALIDATION_OUT_OF_RANGE, command_index);
    if (_texture_layout_invalid(
            command->u.write_texture.width, command->u.write_texture.height,
            command->u.write_texture.depth, command->u.write_texture.bytes_per_row,
            command->u.write_texture.rows_per_image))
        return _drp2_fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
    return _drp2_ok();
}



static DvzDrp2ValidationResult _validate_begin_encoder(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    uint64_t id = command->u.begin_command_encoder.id;
    if (id == 0)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, command_index);
    if (_find_any_object(state, id) != NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2Object* object = _add_object(state, id, DRP2_OBJECT_ENCODER);
    if (object == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    object->open = true;
    return _drp2_ok();
}



static DvzDrp2ValidationResult _validate_begin_render_pass(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    if (_open_pass(state) != NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (_find_any_object(state, command->u.begin_render_pass.id) != NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    const Drp2Object* encoder = _find_object(state, command->u.begin_render_pass.encoder_id);
    if (encoder == NULL || encoder->kind != DRP2_OBJECT_ENCODER || !encoder->open)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (!_has_object_kind(state, command->u.begin_render_pass.texture_id, DRP2_OBJECT_TEXTURE))
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2Object* pass = _add_object(state, command->u.begin_render_pass.id, DRP2_OBJECT_RENDER_PASS);
    if (pass == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    encoder = _find_object(state, command->u.begin_render_pass.encoder_id);
    if (encoder == NULL || encoder->kind != DRP2_OBJECT_ENCODER || !encoder->open ||
        !_has_object_kind(state, command->u.begin_render_pass.texture_id, DRP2_OBJECT_TEXTURE))
    {
        pass->destroyed = true;
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }

    _mark_referenced(state, command->u.begin_render_pass.texture_id);
    pass->open = true;
    pass->encoder_id = command->u.begin_render_pass.encoder_id;
    pass->has_depth_attachment = command->u.begin_render_pass.has_depth_attachment;
    pass->viewport_x = command->u.begin_render_pass.viewport[0];
    pass->viewport_y = command->u.begin_render_pass.viewport[1];
    pass->viewport_width = command->u.begin_render_pass.viewport[2];
    pass->viewport_height = command->u.begin_render_pass.viewport[3];
    pass->scissor_x = command->u.begin_render_pass.viewport[0];
    pass->scissor_y = command->u.begin_render_pass.viewport[1];
    pass->scissor_width = command->u.begin_render_pass.viewport[2];
    pass->scissor_height = command->u.begin_render_pass.viewport[3];
    pass->pipeline_id = encoder->render_pipeline_id;
    pass->bound_vertex_mask = encoder->render_bound_vertex_mask;
    pass->index_buffer_bound = encoder->render_index_buffer_bound;
    pass->bound_bind_group_mask = encoder->render_bound_bind_group_mask;
    return _drp2_ok();
}



static DvzDrp2ValidationResult _validate_begin_compute_pass(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    if (_open_pass(state) != NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (_find_any_object(state, command->u.begin_compute_pass.id) != NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    const Drp2Object* encoder = _find_object(state, command->u.begin_compute_pass.encoder_id);
    if (encoder == NULL || encoder->kind != DRP2_OBJECT_ENCODER || !encoder->open)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2Object* pass =
        _add_object(state, command->u.begin_compute_pass.id, DRP2_OBJECT_COMPUTE_PASS);
    if (pass == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    encoder = _find_object(state, command->u.begin_compute_pass.encoder_id);
    if (encoder == NULL || encoder->kind != DRP2_OBJECT_ENCODER || !encoder->open)
    {
        pass->destroyed = true;
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }
    pass->open = true;
    pass->encoder_id = command->u.begin_compute_pass.encoder_id;
    return _drp2_ok();
}



static DvzDrp2ValidationResult _validate_set_viewport(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    Drp2Object* pass = _find_object(state, command->u.set_viewport.pass_id);
    if (pass == NULL || pass->kind != DRP2_OBJECT_RENDER_PASS || !pass->open)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    pass->viewport_x = command->u.set_viewport.viewport[0];
    pass->viewport_y = command->u.set_viewport.viewport[1];
    pass->viewport_width = command->u.set_viewport.viewport[2];
    pass->viewport_height = command->u.set_viewport.viewport[3];
    return _drp2_ok();
}



static DvzDrp2ValidationResult _validate_set_scissor(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    Drp2Object* pass = _find_object(state, command->u.set_scissor.pass_id);
    if (pass == NULL || pass->kind != DRP2_OBJECT_RENDER_PASS || !pass->open)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    pass->scissor_x = command->u.set_scissor.scissor[0];
    pass->scissor_y = command->u.set_scissor.scissor[1];
    pass->scissor_width = command->u.set_scissor.scissor[2];
    pass->scissor_height = command->u.set_scissor.scissor[3];
    return _drp2_ok();
}



static DvzDrp2ValidationResult _validate_set_pipeline(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    Drp2Object* pass = _find_object(state, command->u.set_pipeline.pass_id);
    if (pass == NULL || !pass->open)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2ObjectKind pipeline_kind = DRP2_OBJECT_NONE;
    if (pass->kind == DRP2_OBJECT_RENDER_PASS)
        pipeline_kind = DRP2_OBJECT_RENDER_PIPELINE;
    else if (pass->kind == DRP2_OBJECT_COMPUTE_PASS)
        pipeline_kind = DRP2_OBJECT_COMPUTE_PIPELINE;
    else
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    if (!_has_object_kind(state, command->u.set_pipeline.pipeline_id, pipeline_kind))
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    pass->pipeline_id = command->u.set_pipeline.pipeline_id;
    pass->bound_vertex_mask = 0;
    pass->index_buffer_bound = false;
    pass->bound_bind_group_mask = 0;
    _mark_referenced(state, command->u.set_pipeline.pipeline_id);
    return _drp2_ok();
}



static DvzDrp2ValidationResult _validate_set_bind_group(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    Drp2Object* pass = _find_object(state, command->u.set_bind_group.pass_id);
    if (pass == NULL || !pass->open)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2Object* pipeline = _find_object(state, pass->pipeline_id);
    if (pipeline == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (pass->kind == DRP2_OBJECT_RENDER_PASS && pipeline->kind != DRP2_OBJECT_RENDER_PIPELINE)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (pass->kind == DRP2_OBJECT_COMPUTE_PASS && pipeline->kind != DRP2_OBJECT_COMPUTE_PIPELINE)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (pass->kind != DRP2_OBJECT_RENDER_PASS && pass->kind != DRP2_OBJECT_COMPUTE_PASS)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    uint32_t slot = command->u.set_bind_group.slot;
    if (slot == 0 && pipeline->bind_group_layout_id == 0)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (slot == 1 && pipeline->bind_group_layout_id2 == 0)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (slot > 1)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    uint64_t expected_layout_id =
        (slot == 0) ? pipeline->bind_group_layout_id : pipeline->bind_group_layout_id2;
    Drp2Object* bind_group = _find_object(state, command->u.set_bind_group.bind_group_id);
    if (bind_group == NULL || bind_group->kind != DRP2_OBJECT_BIND_GROUP)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (bind_group->bind_group_layout_id != expected_layout_id)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    pass->bound_bind_group_mask |= (1u << slot);
    _mark_referenced(state, command->u.set_bind_group.bind_group_id);
    if (bind_group->storage_buffers)
    {
        _mark_referenced(state, bind_group->buffer0_id);
        _mark_referenced(state, bind_group->buffer1_id);
    }
    else
    {
        _mark_referenced(state, bind_group->texture_id);
        _mark_referenced(state, bind_group->sampler_id);
    }
    return _drp2_ok();
}



static DvzDrp2ValidationResult _validate_set_vertex_buffer(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    Drp2Object* pass = _find_object(state, command->u.set_vertex_buffer.pass_id);
    if (pass == NULL || pass->kind != DRP2_OBJECT_RENDER_PASS || !pass->open)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2Object* pipeline = _find_object(state, pass->pipeline_id);
    if (pipeline == NULL || pipeline->kind != DRP2_OBJECT_RENDER_PIPELINE)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (command->u.set_vertex_buffer.slot >= pipeline->vertex_buffer_slots ||
        command->u.set_vertex_buffer.slot >= 32)
        return _drp2_fail(DVZ_DRP2_VALIDATION_OUT_OF_RANGE, command_index);

    Drp2Object* buffer = _find_object(state, command->u.set_vertex_buffer.buffer_id);
    if (buffer == NULL || buffer->kind != DRP2_OBJECT_BUFFER)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if ((buffer->usage & DVZ_DRP2_BUFFER_USAGE_VERTEX) == 0)
        return _drp2_fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
    if (command->u.set_vertex_buffer.offset > buffer->size)
        return _drp2_fail(DVZ_DRP2_VALIDATION_OUT_OF_RANGE, command_index);

    pass->bound_vertex_mask |= (uint32_t)(1u << command->u.set_vertex_buffer.slot);
    _mark_referenced(state, command->u.set_vertex_buffer.buffer_id);
    return _drp2_ok();
}



static DvzDrp2ValidationResult _validate_set_index_buffer(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    Drp2Object* pass = _find_object(state, command->u.set_index_buffer.pass_id);
    if (pass == NULL || pass->kind != DRP2_OBJECT_RENDER_PASS || !pass->open)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2Object* buffer = _find_object(state, command->u.set_index_buffer.buffer_id);
    if (buffer == NULL || buffer->kind != DRP2_OBJECT_BUFFER)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if ((buffer->usage & DVZ_DRP2_BUFFER_USAGE_INDEX) == 0)
        return _drp2_fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
    if (command->u.set_index_buffer.offset > buffer->size)
        return _drp2_fail(DVZ_DRP2_VALIDATION_OUT_OF_RANGE, command_index);
    if (strcmp(command->u.set_index_buffer.index_format, "uint16") != 0 &&
        strcmp(command->u.set_index_buffer.index_format, "uint32") != 0)
        return _drp2_fail(DVZ_DRP2_VALIDATION_USAGE, command_index);

    pass->index_buffer_bound = true;
    _mark_referenced(state, command->u.set_index_buffer.buffer_id);
    return _drp2_ok();
}



static DvzDrp2ValidationResult _validate_render_draw_state(
    Drp2RuntimeState* state, Drp2Object* pass, uint32_t command_index)
{
    ANN(state);
    ANN(pass);

    Drp2Object* pipeline = _find_object(state, pass->pipeline_id);
    if (pipeline == NULL || pipeline->kind != DRP2_OBJECT_RENDER_PIPELINE)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    uint32_t required_mask = 0;
    if (pipeline->vertex_buffer_slots >= 32)
        required_mask = UINT32_MAX;
    else if (pipeline->vertex_buffer_slots > 0)
        required_mask = (uint32_t)((1u << pipeline->vertex_buffer_slots) - 1u);
    if ((pass->bound_vertex_mask & required_mask) != required_mask)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (pipeline->bind_group_layout_id != 0 && (pass->bound_bind_group_mask & 1u) == 0)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (pipeline->bind_group_layout_id2 != 0 && (pass->bound_bind_group_mask & 2u) == 0)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    return _drp2_ok();
}



static DvzDrp2ValidationResult _validate_draw(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    Drp2Object* pass = _find_object(state, command->u.draw.pass_id);
    if (pass == NULL || pass->kind != DRP2_OBJECT_RENDER_PASS || !pass->open)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    return _validate_render_draw_state(state, pass, command_index);
}



static DvzDrp2ValidationResult _validate_draw_indexed(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    Drp2Object* pass = _find_object(state, command->u.draw_indexed.pass_id);
    if (pass == NULL || pass->kind != DRP2_OBJECT_RENDER_PASS || !pass->open)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    DvzDrp2ValidationResult result = _validate_render_draw_state(state, pass, command_index);
    if (!result.ok)
        return result;
    if (!pass->index_buffer_bound)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    return _drp2_ok();
}



static DvzDrp2ValidationResult _validate_end_render_pass(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    Drp2Object* pass = _find_object(state, command->u.end_render_pass.pass_id);
    if (pass == NULL || pass->kind != DRP2_OBJECT_RENDER_PASS || !pass->open)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    Drp2Object* encoder = _find_object(state, pass->encoder_id);
    if (encoder != NULL && encoder->kind == DRP2_OBJECT_ENCODER && encoder->open)
    {
        encoder->render_pipeline_id = pass->pipeline_id;
        encoder->render_bound_vertex_mask = pass->bound_vertex_mask;
        encoder->render_index_buffer_bound = pass->index_buffer_bound;
        encoder->render_bound_bind_group_mask = pass->bound_bind_group_mask;
    }
    pass->open = false;
    return _drp2_ok();
}



static DvzDrp2ValidationResult _validate_dispatch_workgroups(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    Drp2Object* pass = _find_object(state, command->u.dispatch.pass_id);
    if (pass == NULL || pass->kind != DRP2_OBJECT_COMPUTE_PASS || !pass->open)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2Object* pipeline = _find_object(state, pass->pipeline_id);
    if (pipeline == NULL || pipeline->kind != DRP2_OBJECT_COMPUTE_PIPELINE)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (command->u.dispatch.x == 0 || command->u.dispatch.y == 0 || command->u.dispatch.z == 0)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, command_index);
    if (pipeline->bind_group_layout_id != 0 && (pass->bound_bind_group_mask & 1u) == 0)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    _mark_referenced(state, pass->pipeline_id);
    return _drp2_ok();
}



static DvzDrp2ValidationResult _validate_end_compute_pass(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    Drp2Object* pass = _find_object(state, command->u.end_compute_pass.pass_id);
    if (pass == NULL || pass->kind != DRP2_OBJECT_COMPUTE_PASS || !pass->open)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    pass->open = false;
    return _drp2_ok();
}



static DvzDrp2ValidationResult _validate_copy_buffer_to_buffer(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    if (_open_pass(state) != NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2Object* encoder = _find_object(state, command->u.copy_buffer_to_buffer.encoder_id);
    if (encoder == NULL || encoder->kind != DRP2_OBJECT_ENCODER || !encoder->open)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2Object* src = _find_object(state, command->u.copy_buffer_to_buffer.src_buffer_id);
    Drp2Object* dst = _find_object(state, command->u.copy_buffer_to_buffer.dst_buffer_id);
    if (src == NULL || src->kind != DRP2_OBJECT_BUFFER || dst == NULL ||
        dst->kind != DRP2_OBJECT_BUFFER)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if ((src->usage & DVZ_DRP2_BUFFER_USAGE_COPY_SRC) == 0 ||
        (dst->usage & DVZ_DRP2_BUFFER_USAGE_COPY_DST) == 0)
        return _drp2_fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
    if (_range_overflows(
            command->u.copy_buffer_to_buffer.src_offset, command->u.copy_buffer_to_buffer.size,
            src->size))
        return _drp2_fail(DVZ_DRP2_VALIDATION_OUT_OF_RANGE, command_index);
    if (_range_overflows(
            command->u.copy_buffer_to_buffer.dst_offset, command->u.copy_buffer_to_buffer.size,
            dst->size))
        return _drp2_fail(DVZ_DRP2_VALIDATION_OUT_OF_RANGE, command_index);

    _mark_referenced(state, command->u.copy_buffer_to_buffer.src_buffer_id);
    _mark_referenced(state, command->u.copy_buffer_to_buffer.dst_buffer_id);
    return _drp2_ok();
}



static DvzDrp2ValidationResult _validate_copy_buffer_to_texture(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    if (_open_pass(state) != NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2Object* encoder = _find_object(state, command->u.copy_buffer_to_texture.encoder_id);
    if (encoder == NULL || encoder->kind != DRP2_OBJECT_ENCODER || !encoder->open)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2Object* buffer = _find_object(state, command->u.copy_buffer_to_texture.src_buffer_id);
    if (buffer == NULL || buffer->kind != DRP2_OBJECT_BUFFER)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if ((buffer->usage & DVZ_DRP2_BUFFER_USAGE_COPY_SRC) == 0)
        return _drp2_fail(DVZ_DRP2_VALIDATION_USAGE, command_index);

    Drp2Object* texture = _find_object(state, command->u.copy_buffer_to_texture.dst_texture_id);
    if (texture == NULL || texture->kind != DRP2_OBJECT_TEXTURE)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if ((texture->usage & DVZ_DRP2_TEXTURE_USAGE_COPY_DST) == 0)
        return _drp2_fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
    if (command->u.copy_buffer_to_texture.dst_mip_level != 0)
        return _drp2_fail(DVZ_DRP2_VALIDATION_OUT_OF_RANGE, command_index);
    if (_texture_box_overflows(
            texture, command->u.copy_buffer_to_texture.dst_origin_x,
            command->u.copy_buffer_to_texture.dst_origin_y,
            command->u.copy_buffer_to_texture.dst_origin_z, command->u.copy_buffer_to_texture.width,
            command->u.copy_buffer_to_texture.height, command->u.copy_buffer_to_texture.depth))
        return _drp2_fail(DVZ_DRP2_VALIDATION_OUT_OF_RANGE, command_index);
    if (_texture_layout_invalid(
            command->u.copy_buffer_to_texture.width, command->u.copy_buffer_to_texture.height,
            command->u.copy_buffer_to_texture.depth, command->u.copy_buffer_to_texture.bytes_per_row,
            command->u.copy_buffer_to_texture.rows_per_image))
        return _drp2_fail(DVZ_DRP2_VALIDATION_USAGE, command_index);

    uint64_t size = _drp2_texture_layout_size(
        command->u.copy_buffer_to_texture.depth, command->u.copy_buffer_to_texture.bytes_per_row,
        command->u.copy_buffer_to_texture.rows_per_image);
    if (_range_overflows(command->u.copy_buffer_to_texture.src_offset, size, buffer->size))
        return _drp2_fail(DVZ_DRP2_VALIDATION_OUT_OF_RANGE, command_index);
    _mark_referenced(state, command->u.copy_buffer_to_texture.src_buffer_id);
    _mark_referenced(state, command->u.copy_buffer_to_texture.dst_texture_id);
    return _drp2_ok();
}



static DvzDrp2ValidationResult _validate_copy_texture_to_buffer(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    if (_open_pass(state) != NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2Object* encoder = _find_object(state, command->u.copy_texture_to_buffer.encoder_id);
    if (encoder == NULL || encoder->kind != DRP2_OBJECT_ENCODER || !encoder->open)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    Drp2Object* texture = _find_object(state, command->u.copy_texture_to_buffer.src_texture_id);
    if (texture == NULL || texture->kind != DRP2_OBJECT_TEXTURE)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if ((texture->usage & DVZ_DRP2_TEXTURE_USAGE_COPY_SRC) == 0)
        return _drp2_fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
    if (_texture_box_overflows(
            texture, 0, 0, 0, command->u.copy_texture_to_buffer.width,
            command->u.copy_texture_to_buffer.height, 1))
        return _drp2_fail(DVZ_DRP2_VALIDATION_OUT_OF_RANGE, command_index);
    if (_texture_layout_invalid(
            command->u.copy_texture_to_buffer.width, command->u.copy_texture_to_buffer.height, 1,
            command->u.copy_texture_to_buffer.bytes_per_row,
            command->u.copy_texture_to_buffer.rows_per_image))
        return _drp2_fail(DVZ_DRP2_VALIDATION_USAGE, command_index);

    Drp2Object* buffer = _find_object(state, command->u.copy_texture_to_buffer.dst_buffer_id);
    if (buffer == NULL || buffer->kind != DRP2_OBJECT_BUFFER)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if ((buffer->usage & DVZ_DRP2_BUFFER_USAGE_COPY_DST) == 0)
        return _drp2_fail(DVZ_DRP2_VALIDATION_USAGE, command_index);

    uint64_t required = _drp2_texture_layout_size(
        1, command->u.copy_texture_to_buffer.bytes_per_row,
        command->u.copy_texture_to_buffer.rows_per_image);
    if (_range_overflows(command->u.copy_texture_to_buffer.dst_offset, required, buffer->size))
        return _drp2_fail(DVZ_DRP2_VALIDATION_OUT_OF_RANGE, command_index);
    _mark_referenced(state, command->u.copy_texture_to_buffer.src_texture_id);
    _mark_referenced(state, command->u.copy_texture_to_buffer.dst_buffer_id);
    return _drp2_ok();
}



static DvzDrp2ValidationResult _validate_copy_texture_to_texture(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    if (_open_pass(state) != NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2Object* encoder = _find_object(state, command->u.copy_texture_to_texture.encoder_id);
    if (encoder == NULL || encoder->kind != DRP2_OBJECT_ENCODER || !encoder->open)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2Object* src = _find_object(state, command->u.copy_texture_to_texture.src_texture_id);
    Drp2Object* dst = _find_object(state, command->u.copy_texture_to_texture.dst_texture_id);
    if (src == NULL || src->kind != DRP2_OBJECT_TEXTURE || dst == NULL ||
        dst->kind != DRP2_OBJECT_TEXTURE)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if ((src->usage & DVZ_DRP2_TEXTURE_USAGE_COPY_SRC) == 0 ||
        (dst->usage & DVZ_DRP2_TEXTURE_USAGE_COPY_DST) == 0)
        return _drp2_fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
    if (command->u.copy_texture_to_texture.src_mip_level != 0 ||
        command->u.copy_texture_to_texture.dst_mip_level != 0)
        return _drp2_fail(DVZ_DRP2_VALIDATION_OUT_OF_RANGE, command_index);
    if (_texture_box_overflows(
            src, command->u.copy_texture_to_texture.src_origin_x,
            command->u.copy_texture_to_texture.src_origin_y,
            command->u.copy_texture_to_texture.src_origin_z,
            command->u.copy_texture_to_texture.width, command->u.copy_texture_to_texture.height,
            command->u.copy_texture_to_texture.depth))
        return _drp2_fail(DVZ_DRP2_VALIDATION_OUT_OF_RANGE, command_index);
    if (_texture_box_overflows(
            dst, command->u.copy_texture_to_texture.dst_origin_x,
            command->u.copy_texture_to_texture.dst_origin_y,
            command->u.copy_texture_to_texture.dst_origin_z,
            command->u.copy_texture_to_texture.width, command->u.copy_texture_to_texture.height,
            command->u.copy_texture_to_texture.depth))
        return _drp2_fail(DVZ_DRP2_VALIDATION_OUT_OF_RANGE, command_index);
    _mark_referenced(state, command->u.copy_texture_to_texture.src_texture_id);
    _mark_referenced(state, command->u.copy_texture_to_texture.dst_texture_id);
    return _drp2_ok();
}



static DvzDrp2ValidationResult _validate_finish_encoder(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    if (_open_pass(state) != NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (_find_any_object(state, command->u.finish_command_encoder.command_buffer_id) != NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    const Drp2Object* encoder = _find_object(state, command->u.finish_command_encoder.encoder_id);
    if (encoder == NULL || encoder->kind != DRP2_OBJECT_ENCODER || !encoder->open)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2Object* command_buffer = _add_object(
        state, command->u.finish_command_encoder.command_buffer_id, DRP2_OBJECT_COMMAND_BUFFER);
    if (command_buffer == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    command_buffer->encoder_id = command->u.finish_command_encoder.encoder_id;

    Drp2Object* mutable_encoder =
        _find_object(state, command->u.finish_command_encoder.encoder_id);
    if (mutable_encoder == NULL || mutable_encoder->kind != DRP2_OBJECT_ENCODER ||
        !mutable_encoder->open)
    {
        command_buffer->destroyed = true;
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }
    mutable_encoder->open = false;
    return _drp2_ok();
}



static DvzDrp2ValidationResult _validate_queue_submit(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    Drp2Object* command_buffer = _find_object(state, command->u.queue_submit.command_buffer_id);
    if (command_buffer == NULL || command_buffer->kind != DRP2_OBJECT_COMMAND_BUFFER)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (command_buffer->submitted)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    if (!command->u.queue_submit.has_readback)
    {
        _retire_submitted_work(state, command_buffer);
        return _drp2_ok();
    }

    Drp2Object* buffer = _find_object(state, command->u.queue_submit.buffer_id);
    if (buffer == NULL || buffer->kind != DRP2_OBJECT_BUFFER)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if ((buffer->usage & DVZ_DRP2_BUFFER_USAGE_MAP_READ) == 0)
        return _drp2_fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
    if (_range_overflows(command->u.queue_submit.offset, command->u.queue_submit.size, buffer->size))
        return _drp2_fail(DVZ_DRP2_VALIDATION_OUT_OF_RANGE, command_index);
    _retire_submitted_work(state, command_buffer);
    return _drp2_ok();
}



static DvzDrp2ValidationResult _validate_command(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    DvzDrp2ValidationResult ready = {0};
    switch (command->type)
    {
    case DVZ_DRP2_COMMAND_HELLO_RENDERER:
        if (state->hello_seen || state->reply_seen)
            return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
        state->hello_seen = true;
        return _drp2_ok();

    case DVZ_DRP2_COMMAND_RENDERER_HELLO_REPLY:
        if (!state->hello_seen || state->reply_seen)
            return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
        state->reply_seen = true;
        return _drp2_ok();

    default:
        ready = _validate_ready(state, command_index);
        if (!ready.ok)
            return ready;
        break;
    }

    switch (command->type)
    {
    case DVZ_DRP2_COMMAND_CREATE_BUFFER:
        return _validate_create_buffer(state, command, command_index);
    case DVZ_DRP2_COMMAND_DESTROY_BUFFER:
        return _validate_destroy_buffer(state, command, command_index);
    case DVZ_DRP2_COMMAND_CREATE_TEXTURE:
        return _validate_create_texture(state, command, command_index);
    case DVZ_DRP2_COMMAND_DESTROY_TEXTURE:
        return _validate_destroy_texture(state, command, command_index);
    case DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE:
        return _validate_create_shader_module(state, command, command_index);
    case DVZ_DRP2_COMMAND_DESTROY_SHADER_MODULE:
        return _validate_destroy_shader_module(state, command, command_index);
    case DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE:
        return _validate_create_render_pipeline(state, command, command_index);
    case DVZ_DRP2_COMMAND_DESTROY_RENDER_PIPELINE:
        return _validate_destroy_render_pipeline(state, command, command_index);
    case DVZ_DRP2_COMMAND_CREATE_COMPUTE_PIPELINE:
        return _validate_create_compute_pipeline(state, command, command_index);
    case DVZ_DRP2_COMMAND_DESTROY_COMPUTE_PIPELINE:
        return _validate_destroy_compute_pipeline(state, command, command_index);
    case DVZ_DRP2_COMMAND_CREATE_SAMPLER:
        return _validate_create_sampler(state, command, command_index);
    case DVZ_DRP2_COMMAND_CREATE_BIND_GROUP_LAYOUT:
        return _validate_create_bind_group_layout(state, command, command_index);
    case DVZ_DRP2_COMMAND_CREATE_BIND_GROUP:
        return _validate_create_bind_group(state, command, command_index);
    case DVZ_DRP2_COMMAND_DESTROY_BIND_GROUP_LAYOUT:
        return _validate_destroy_bind_group_layout(state, command, command_index);
    case DVZ_DRP2_COMMAND_DESTROY_BIND_GROUP:
        return _validate_destroy_bind_group(state, command, command_index);
    case DVZ_DRP2_COMMAND_WRITE_BUFFER:
        return _validate_write_buffer(state, command, command_index);
    case DVZ_DRP2_COMMAND_WRITE_TEXTURE:
        return _validate_write_texture(state, command, command_index);
    case DVZ_DRP2_COMMAND_BEGIN_COMMAND_ENCODER:
        return _validate_begin_encoder(state, command, command_index);
    case DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS:
        return _validate_begin_render_pass(state, command, command_index);
    case DVZ_DRP2_COMMAND_BEGIN_COMPUTE_PASS:
        return _validate_begin_compute_pass(state, command, command_index);
    case DVZ_DRP2_COMMAND_SET_VIEWPORT:
        return _validate_set_viewport(state, command, command_index);
    case DVZ_DRP2_COMMAND_SET_SCISSOR:
        return _validate_set_scissor(state, command, command_index);
    case DVZ_DRP2_COMMAND_SET_PIPELINE:
        return _validate_set_pipeline(state, command, command_index);
    case DVZ_DRP2_COMMAND_SET_BIND_GROUP:
        return _validate_set_bind_group(state, command, command_index);
    case DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER:
        return _validate_set_vertex_buffer(state, command, command_index);
    case DVZ_DRP2_COMMAND_SET_INDEX_BUFFER:
        return _validate_set_index_buffer(state, command, command_index);
    case DVZ_DRP2_COMMAND_DRAW:
        return _validate_draw(state, command, command_index);
    case DVZ_DRP2_COMMAND_DRAW_INDEXED:
        return _validate_draw_indexed(state, command, command_index);
    case DVZ_DRP2_COMMAND_END_RENDER_PASS:
        return _validate_end_render_pass(state, command, command_index);
    case DVZ_DRP2_COMMAND_DISPATCH_WORKGROUPS:
        return _validate_dispatch_workgroups(state, command, command_index);
    case DVZ_DRP2_COMMAND_END_COMPUTE_PASS:
        return _validate_end_compute_pass(state, command, command_index);
    case DVZ_DRP2_COMMAND_COPY_BUFFER_TO_BUFFER:
        return _validate_copy_buffer_to_buffer(state, command, command_index);
    case DVZ_DRP2_COMMAND_COPY_BUFFER_TO_TEXTURE:
        return _validate_copy_buffer_to_texture(state, command, command_index);
    case DVZ_DRP2_COMMAND_COPY_TEXTURE_TO_BUFFER:
        return _validate_copy_texture_to_buffer(state, command, command_index);
    case DVZ_DRP2_COMMAND_COPY_TEXTURE_TO_TEXTURE:
        return _validate_copy_texture_to_texture(state, command, command_index);
    case DVZ_DRP2_COMMAND_FINISH_COMMAND_ENCODER:
        return _validate_finish_encoder(state, command, command_index);
    case DVZ_DRP2_COMMAND_QUEUE_SUBMIT:
        return _validate_queue_submit(state, command, command_index);
    default:
        return _drp2_fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
    }
}


/**
 * Release runtime semantic validation state.
 *
 * @param state the runtime state
 */
static void _runtime_state_cleanup(Drp2RuntimeState* state)
{
    if (state == NULL)
        return;
    dvz_free(state->objects);
    state->objects = NULL;
    state->capacity = 0;
    state->count = 0;
    state->hello_seen = false;
    state->reply_seen = false;
    state->failed = false;
}



/**
 * Clone runtime semantic validation state.
 *
 * @param dst the destination state
 * @param src the source state, or NULL for an empty state
 * @return whether the clone succeeded
 */
static bool _runtime_state_clone(Drp2RuntimeState* dst, const Drp2RuntimeState* src)
{
    ANN(dst);
    dvz_memset(dst, sizeof(Drp2RuntimeState), 0, sizeof(Drp2RuntimeState));
    if (src == NULL)
        return true;

    *dst = *src;
    dst->objects = NULL;
    if (src->capacity == 0)
        return true;

    uint64_t bytes = 0;
    if (_dvz_mul_u64_overflows(src->capacity, sizeof(Drp2Object), &bytes))
        return false;
    dst->objects = (Drp2Object*)dvz_calloc(src->capacity, sizeof(Drp2Object));
    if (dst->objects == NULL)
        return false;
    if (src->count > 0)
        dvz_memcpy(dst->objects, bytes, src->objects, (uint64_t)src->count * sizeof(Drp2Object));
    return true;
}



/**
 * Ensure a runtime has a semantic state object ready for commit.
 *
 * @param runtime the runtime
 * @return whether the state object is available
 */
static bool _runtime_state_ensure(DvzDrp2Runtime* runtime)
{
    ANN(runtime);
    if (runtime->semantic_state != NULL)
        return true;
    runtime->semantic_state = (Drp2RuntimeState*)dvz_calloc(1, sizeof(Drp2RuntimeState));
    return runtime->semantic_state != NULL;
}



/**
 * Replace the runtime semantic state with a validated next state.
 *
 * @param runtime the runtime
 * @param next_state the validated next state
 * @return whether the commit succeeded
 */
static bool _runtime_state_commit(DvzDrp2Runtime* runtime, Drp2RuntimeState* next_state)
{
    ANN(runtime);
    ANN(next_state);
    if (!_runtime_state_ensure(runtime))
        return false;

    _runtime_state_cleanup(runtime->semantic_state);
    *runtime->semantic_state = *next_state;
    next_state->objects = NULL;
    next_state->capacity = 0;
    next_state->count = 0;
    next_state->hello_seen = false;
    next_state->reply_seen = false;
    next_state->failed = false;
    return true;
}



/**
 * Validate a command stream against a runtime-persistent semantic state.
 *
 * @param runtime the DRP2 runtime
 * @param stream the command stream
 * @param next_state the validated next state
 * @return the validation result
 */
static DvzDrp2ValidationResult
_runtime_validate_stream(
    const DvzDrp2Runtime* runtime, const DvzDrp2CommandStream* stream,
    Drp2RuntimeState* next_state)
{
    ANN(runtime);
    ANN(stream);
    ANN(next_state);

    if (!_runtime_state_clone(next_state, runtime->semantic_state))
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, 0);

    DvzDrp2ValidationResult result = _drp2_ok();
    for (uint32_t i = 0; i < stream->count; i++)
    {
        result = _validate_command(next_state, &stream->commands[i], i);
        if (!result.ok)
            break;
    }
    return result;
}



#if DVZ_DRP2_HAS_VKLITE
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
    VkImageType img_type = depth > 1 ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D;
    dvz_images(state->runtime->device, state->runtime->allocator, img_type, 1, images);
    dvz_images_format(images, VK_FORMAT_R8G8B8A8_UNORM);
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
    object->width = command->u.create_texture.width;
    object->height = command->u.create_texture.height;
    return _drp2_ok();
}


/**
 * Attach a borrowed frame image as a vklite texture object.
 *
 * @param runtime the DRP2 runtime
 * @param texture_id the DRP2 texture id
 * @param frame the borrowed stream frame
 * @return true when the frame target was attached
 */
static bool _vklite_attach_frame_target(
    DvzDrp2Runtime* runtime, uint64_t texture_id, const DvzStreamFrame* frame)
{
    ANN(runtime);
    if (!_frame_target_valid(texture_id, frame))
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
    if (object->depth_views != NULL)
    {
        dvz_image_views_destroy(object->depth_views);
        dvz_image_views_free(object->depth_views);
        object->depth_views = NULL;
    }
    if (object->depth_images != NULL)
    {
        dvz_images_destroy(object->depth_images);
        dvz_images_free(object->depth_images);
        object->depth_images = NULL;
    }
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
    _vklite_destroy_object(object);
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
    _vklite_destroy_object(object);
    return _drp2_ok();
}


static DvzDrp2ValidationResult
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



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return a DRP2 runtime configuration for a vklite-backed runtime.
 *
 * @param device the borrowed Vulkan device wrapper
 * @param allocator the borrowed Vulkan allocator wrapper
 * @return the runtime configuration
 */
DvzDrp2RuntimeConfig dvz_drp2_runtime_vklite_config(DvzDevice* device, DvzVma* allocator)
{
    DvzDrp2RuntimeConfig cfg = {0};
    cfg.device = device;
    cfg.allocator = allocator;
    return cfg;
}



/**
 * Create a DRP2 runtime using the vklite backend boundary.
 *
 * @param cfg the runtime configuration
 * @return the runtime, or NULL on invalid configuration
 */
DvzDrp2Runtime* dvz_drp2_runtime_vklite(const DvzDrp2RuntimeConfig* cfg)
{
    if (cfg == NULL)
        return NULL;
#if !DVZ_DRP2_HAS_VKLITE
    if (!cfg->semantic_only)
        return NULL;
#endif
    if (!cfg->semantic_only && (cfg->device == NULL || cfg->allocator == NULL))
        return NULL;

    DvzDrp2Runtime* runtime = (DvzDrp2Runtime*)dvz_calloc(1, sizeof(DvzDrp2Runtime));
    ANN(runtime);
    runtime->device = cfg->device;
    runtime->allocator = cfg->allocator;
    runtime->semantic_only = cfg->semantic_only;
    return runtime;
}



/**
 * Return the borrowed configuration that was used to create a DRP2 runtime.
 *
 * @param runtime the runtime
 * @return the runtime configuration, or zero-initialized fields when runtime is NULL
 */
DvzDrp2RuntimeConfig dvz_drp2_runtime_config(const DvzDrp2Runtime* runtime)
{
    DvzDrp2RuntimeConfig cfg = {0};
    if (runtime == NULL)
        return cfg;
    cfg.device = runtime->device;
    cfg.allocator = runtime->allocator;
    cfg.semantic_only = runtime->semantic_only;
    return cfg;
}



/**
 * Destroy a DRP2 runtime.
 *
 * @param runtime the runtime
 */
void dvz_drp2_runtime_destroy(DvzDrp2Runtime* runtime)
{
    if (runtime == NULL)
        return;
    _runtime_state_cleanup(runtime->semantic_state);
    dvz_free(runtime->semantic_state);
#if DVZ_DRP2_HAS_VKLITE
    _vklite_state_cleanup(runtime->vklite_state);
    dvz_free(runtime->vklite_state);
#endif
    dvz_free(runtime);
}



/**
 * Reset a DRP2 runtime to its empty reusable state.
 *
 * @param runtime the runtime
 */
void dvz_drp2_runtime_reset(DvzDrp2Runtime* runtime)
{
    if (runtime == NULL)
        return;

    if (runtime->semantic_state != NULL)
    {
        _runtime_state_cleanup(runtime->semantic_state);
        dvz_free(runtime->semantic_state);
        runtime->semantic_state = NULL;
    }

#if DVZ_DRP2_HAS_VKLITE
    if (runtime->vklite_state != NULL)
    {
        _vklite_state_cleanup(runtime->vklite_state);
        dvz_free(runtime->vklite_state);
        runtime->vklite_state = NULL;
    }
#endif
}


#if DVZ_DRP2_HAS_VKLITE
/**
 * Download bytes from a live vklite buffer owned by a DRP2 runtime.
 *
 * @param runtime the DRP2 runtime
 * @param buffer_id the DRP2 buffer id
 * @param offset the byte offset
 * @param size the byte count to download
 * @param data the output buffer
 * @return true when the buffer exists and the download was requested
 */
bool _dvz_drp2_runtime_vklite_download_buffer(
    DvzDrp2Runtime* runtime, uint64_t buffer_id, uint64_t offset, uint64_t size, void* data)
{
    if (runtime == NULL || runtime->vklite_state == NULL || data == NULL || size == 0)
        return false;

    Drp2VkliteObject* object = _vklite_find(runtime->vklite_state, buffer_id);
    if (object == NULL || object->buffer == NULL)
        return false;
    if (runtime->semantic_state == NULL)
        return false;

    Drp2Object* semantic = _find_any_object(runtime->semantic_state, buffer_id);
    if (semantic == NULL || semantic->kind != DRP2_OBJECT_BUFFER)
        return false;
    if (_range_overflows(offset, size, semantic->size))
    {
        log_error(
            "runtime buffer download [%" PRIu64 ", %" PRIu64 ") exceeds buffer %" PRIu64
            " size %" PRIu64,
            offset, offset + size, buffer_id, semantic->size);
        return false;
    }

    dvz_buffer_download(object->buffer, offset, size, data);
    return true;
}
#endif



/**
 * Validate a DRP2 command stream against the backend-agnostic semantic rules.
 *
 * @param stream the command stream
 * @return the validation result
 */
DvzDrp2ValidationResult dvz_drp2_validate_stream(const DvzDrp2CommandStream* stream)
{
    if (stream == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, 0);

    Drp2RuntimeState state = {0};
    DvzDrp2ValidationResult result = _drp2_ok();

    for (uint32_t i = 0; i < stream->count; i++)
    {
        result = _validate_command(&state, &stream->commands[i], i);
        if (!result.ok)
            break;
    }

    dvz_free(state.objects);
    return result;
}



/**
 * Execute a command stream through a DRP2 runtime.
 *
 * @param runtime the runtime
 * @param stream the command stream
 * @return the validation result after semantic validation and backend execution
 */
DvzDrp2ValidationResult
dvz_drp2_runtime_execute(DvzDrp2Runtime* runtime, const DvzDrp2CommandStream* stream)
{
    if (runtime == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, 0);
    if (stream == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, 0);

    Drp2RuntimeState next_state = {0};
    DvzDrp2ValidationResult result = _runtime_validate_stream(runtime, stream, &next_state);
    if (!result.ok)
    {
        _runtime_state_cleanup(&next_state);
        return result;
    }

    if (!_runtime_state_ensure(runtime))
    {
        _runtime_state_cleanup(&next_state);
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, 0);
    }

    if (runtime->semantic_only)
    {
        if (!_runtime_state_commit(runtime, &next_state))
        {
            _runtime_state_cleanup(&next_state);
            return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, 0);
        }
        return result;
    }

#if DVZ_DRP2_HAS_VKLITE
    DvzDrp2ValidationResult backend_result = _vklite_execute(runtime, stream);
    if (!backend_result.ok)
    {
        _runtime_state_cleanup(&next_state);
        return backend_result;
    }
    if (!_runtime_state_commit(runtime, &next_state))
    {
        _runtime_state_cleanup(&next_state);
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, 0);
    }
    return backend_result;
#else
    _runtime_state_cleanup(&next_state);
    return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, 0);
#endif
}


/**
 * Attach a borrowed stream frame as a runtime render target.
 *
 * @param runtime the runtime
 * @param texture_id the DRP2 texture id to expose for render passes
 * @param frame the borrowed stream frame whose command buffer is currently recording
 * @return whether the frame target was attached
 */
bool dvz_drp2_runtime_attach_frame_target(
    DvzDrp2Runtime* runtime, uint64_t texture_id, const DvzStreamFrame* frame)
{
    if (runtime == NULL || !_frame_target_valid(texture_id, frame))
        return false;

    if (runtime->semantic_state == NULL)
    {
        runtime->semantic_state = (Drp2RuntimeState*)dvz_calloc(1, sizeof(Drp2RuntimeState));
        if (runtime->semantic_state == NULL)
            return false;
    }

    Drp2Object* object = _find_any_object(runtime->semantic_state, texture_id);
    if (object != NULL && object->kind != DRP2_OBJECT_TEXTURE)
        return false;
    if (object == NULL && !_ensure_capacity(runtime->semantic_state))
        return false;

    if (!runtime->semantic_only)
    {
#if DVZ_DRP2_HAS_VKLITE
        if (!_vklite_attach_frame_target(runtime, texture_id, frame))
            return false;
#else
        return false;
#endif
    }

    object = _find_any_object(runtime->semantic_state, texture_id);
    if (object == NULL)
        object = _add_object(runtime->semantic_state, texture_id, DRP2_OBJECT_TEXTURE);
    if (object == NULL || object->kind != DRP2_OBJECT_TEXTURE)
        return false;

    object->destroyed = false;
    object->width = frame->extent.width;
    object->height = frame->extent.height;
    object->depth = 1;
    object->usage = DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT | DVZ_DRP2_TEXTURE_USAGE_COPY_SRC;
    return true;
}


/**
 * Record a copy from a runtime-owned texture into a borrowed stream frame.
 *
 * @param runtime the runtime
 * @param texture_id the DRP2 texture id to copy from
 * @param frame the borrowed stream frame whose command buffer is currently recording
 * @return whether the copy commands were recorded
 */
bool dvz_drp2_runtime_copy_texture_to_frame(
    DvzDrp2Runtime* runtime, uint64_t texture_id, const DvzStreamFrame* frame)
{
#if DVZ_DRP2_HAS_VKLITE
    if (runtime == NULL || runtime->vklite_state == NULL || frame == NULL)
        return false;
    if (!_frame_target_valid(texture_id, frame))
        return false;
    if ((frame->usage & DVZ_STREAM_FRAME_USAGE_COPY_DST) == 0)
        return false;

    Drp2VkliteObject* source = _vklite_find(runtime->vklite_state, texture_id);
    if (source == NULL || source->images == NULL)
        return false;

    uint32_t width = _min_u32(source->width, frame->extent.width);
    uint32_t height = _min_u32(source->height, frame->extent.height);
    if (width == 0 || height == 0)
        return false;

    DvzCommands* cmds =
        _vklite_borrowed_frame_commands_create(runtime->device, frame->command_buffer);
    if (cmds == NULL)
        return false;

    _vklite_transition_image(
        cmds, source, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        VK_ACCESS_2_TRANSFER_READ_BIT);

    DvzBarriers barriers = {0};
    dvz_barriers(&barriers);
    DvzBarrierImage* dst = dvz_barriers_image(&barriers, frame->image);
    ANN(dst);
    dvz_barrier_image_stage(
        dst, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT);
    dvz_barrier_image_access(
        dst, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT);
    dvz_barrier_image_layout(
        dst, frame->image_layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    dvz_cmd_barriers(cmds, &barriers);

    DvzImageCopy* copy = dvz_image_copy_create();
    if (copy == NULL)
    {
        _vklite_borrowed_frame_commands_free(cmds);
        return false;
    }
    dvz_cmd_copy_source(
        copy, dvz_image_handle(source->images, 0), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 0, 0, 0,
        width, height, 1);
    dvz_cmd_copy_destination(
        copy, frame->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, 0, 0);
    dvz_cmd_copy_image(cmds, copy);
    dvz_image_copy_free(copy);

    dvz_barriers(&barriers);
    dst = dvz_barriers_image(&barriers, frame->image);
    ANN(dst);
    dvz_barrier_image_stage(
        dst, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
    dvz_barrier_image_access(
        dst, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);
    dvz_barrier_image_layout(
        dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, frame->image_layout);
    dvz_cmd_barriers(cmds, &barriers);

    _vklite_borrowed_frame_commands_free(cmds);
    return true;
#else
    (void)runtime;
    (void)texture_id;
    (void)frame;
    return false;
#endif
}



/*************************************************************************************************/
/*  Public GLSL compilation utility                                                              */
/*************************************************************************************************/

uint32_t* dvz_compile_glsl(const char* stage, const char* glsl, uint64_t* out_size)
{
    ANN(stage);
    ANN(glsl);
    ANN(out_size);
    *out_size = 0;
    uint32_t* spv = NULL;
    uint64_t spv_size = 0;
    if (!_vklite_compile_glsl(stage, glsl, &spv, &spv_size))
        return NULL;
    *out_size = spv_size;
    return spv;
}


bool dvz_drp2_runtime_download_buffer(
    DvzDrp2Runtime* runtime, uint64_t buffer_id, uint64_t offset, uint64_t size, void* dst)
{
#if DVZ_DRP2_HAS_VKLITE
    return _dvz_drp2_runtime_vklite_download_buffer(runtime, buffer_id, offset, size, dst);
#else
    (void)runtime;
    (void)buffer_id;
    (void)offset;
    (void)size;
    (void)dst;
    return false;
#endif
}
