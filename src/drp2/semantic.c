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

#include <volk.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_overflow.h"
#include "_runtime.h"
#include "_stream.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

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



bool _drp2_range_overflows(uint64_t offset, uint64_t size, uint64_t total)
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
    uint32_t format, uint32_t width, uint32_t height, uint32_t depth, uint32_t bytes_per_row,
    uint32_t rows_per_image)
{
    if (width == 0 || height == 0 || depth == 0)
        return true;
    uint32_t bytes_per_texel = 0;
    if (!_drp2_texture_format_bytes_per_texel(format, &bytes_per_texel) || bytes_per_texel == 0)
        return true;
    uint64_t min_row = 0;
    if (_dvz_mul_u64_overflows(width, bytes_per_texel, &min_row))
        return true;
    if (bytes_per_row < min_row)
        return true;
    if (bytes_per_row % bytes_per_texel != 0)
        return true;
    if (rows_per_image < height)
        return true;
    uint64_t size = 0;
    if (_dvz_mul_u64_overflows(rows_per_image, bytes_per_row, &size))
        return true;
    if (_dvz_mul_u64_overflows(depth, size, &size))
        return true;
    return false;
}



/**
 * Return the effective texture format used by DRP2 when a format is omitted.
 *
 * @param format texture format from a command or object
 * @return backend-native texture format enum value
 */
static uint32_t _effective_color_format(uint32_t format)
{
    return format != 0 ? format : VK_FORMAT_R8G8B8A8_UNORM;
}



/**
 * Return the effective depth attachment format used by DRP2 transient depth.
 *
 * @return backend-native depth texture format enum value
 */
static uint32_t _effective_depth_format(void)
{
    return VK_FORMAT_D32_SFLOAT;
}



static bool _binding_type_is_buffer(DvzDrp2BindingType type)
{
    return type == DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER ||
           type == DVZ_DRP2_BINDING_TYPE_STORAGE_BUFFER;
}



static bool _binding_type_is_texture(DvzDrp2BindingType type)
{
    return type == DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE ||
           type == DVZ_DRP2_BINDING_TYPE_STORAGE_TEXTURE;
}



/**
 * Validate live bind-group entries that would keep referencing a recreated buffer.
 *
 * @param state semantic runtime state
 * @param buffer_id recreated buffer id
 * @param size recreated buffer size
 * @param usage recreated buffer usage flags
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
static DvzDrp2ValidationResult _validate_buffer_recreate_bind_groups(
    Drp2RuntimeState* state, uint64_t buffer_id, uint64_t size, uint32_t usage,
    uint32_t command_index)
{
    ANN(state);
    for (uint32_t i = 0; i < state->count; i++)
    {
        const Drp2Object* bind_group = &state->objects[i];
        if (bind_group->destroyed || bind_group->kind != DRP2_OBJECT_BIND_GROUP)
            continue;
        for (uint32_t j = 0; j < bind_group->bind_group_entry_count; j++)
        {
            const DvzDrp2BindGroupEntry* entry = &bind_group->bind_group_entries[j];
            if (entry->resource_id != buffer_id || !_binding_type_is_buffer(entry->binding_type))
                continue;
            if (entry->binding_type == DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER &&
                (usage & DVZ_DRP2_BUFFER_USAGE_UNIFORM) == 0)
                return _drp2_fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
            if (entry->binding_type == DVZ_DRP2_BINDING_TYPE_STORAGE_BUFFER &&
                (usage & DVZ_DRP2_BUFFER_USAGE_STORAGE) == 0)
                return _drp2_fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
            if (_drp2_range_overflows(entry->offset, entry->size, size))
                return _drp2_fail(DVZ_DRP2_VALIDATION_OUT_OF_RANGE, command_index);
        }
    }
    return _drp2_ok();
}



static const DvzDrp2BindGroupLayoutEntry* _layout_entry_for_binding(
    const Drp2Object* layout, uint32_t binding)
{
    ANN(layout);
    for (uint32_t i = 0; i < layout->layout_entry_count; i++)
    {
        if (layout->layout_entries[i].binding == binding)
            return &layout->layout_entries[i];
    }
    return NULL;
}



static bool _entry_bindings_unique(
    const DvzDrp2BindGroupLayoutEntry* entries, uint32_t count)
{
    ANN(entries);
    for (uint32_t i = 0; i < count; i++)
    {
        for (uint32_t j = i + 1; j < count; j++)
        {
            if (entries[i].binding == entries[j].binding)
                return false;
        }
    }
    return true;
}



static bool _bind_group_entry_bindings_unique(
    const DvzDrp2BindGroupEntry* entries, uint32_t count)
{
    ANN(entries);
    for (uint32_t i = 0; i < count; i++)
    {
        for (uint32_t j = i + 1; j < count; j++)
        {
            if (entries[i].binding == entries[j].binding)
                return false;
        }
    }
    return true;
}



static uint32_t _dynamic_binding_count(const Drp2Object* layout)
{
    ANN(layout);
    uint32_t count = 0;
    for (uint32_t i = 0; i < layout->layout_entry_count; i++)
    {
        if (layout->layout_entries[i].has_dynamic_offset)
            count++;
    }
    return count;
}



/**
 * Return whether the cull mode is a supported Vulkan cull-mode flag combination.
 *
 * @param cull_mode VkCullModeFlags value
 * @return whether the value is supported
 */
static bool _raster_cull_mode_valid(uint32_t cull_mode)
{
    return cull_mode == VK_CULL_MODE_NONE || cull_mode == VK_CULL_MODE_FRONT_BIT ||
           cull_mode == VK_CULL_MODE_BACK_BIT ||
           cull_mode == (VK_CULL_MODE_FRONT_BIT | VK_CULL_MODE_BACK_BIT);
}



/**
 * Return whether the front-face value is a supported Vulkan front-face enum.
 *
 * @param front_face VkFrontFace value
 * @return whether the value is supported
 */
static bool _raster_front_face_valid(uint32_t front_face)
{
    return front_face == VK_FRONT_FACE_COUNTER_CLOCKWISE ||
           front_face == VK_FRONT_FACE_CLOCKWISE;
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


/**
 * Return the byte size of one texel for a DRP2 texture format.
 *
 * @param format texture format, using VkFormat values; zero means default RGBA8
 * @param out_bytes output byte size
 * @return whether the format is supported by DRP2 texture layout validation
 */
bool _drp2_texture_format_bytes_per_texel(uint32_t format, uint32_t* out_bytes)
{
    ANN(out_bytes);
    VkFormat vk_format = format != 0 ? (VkFormat)format : VK_FORMAT_R8G8B8A8_UNORM;
    switch (vk_format)
    {
    case VK_FORMAT_R8_UNORM:
    case VK_FORMAT_R8_SNORM:
    case VK_FORMAT_R8_UINT:
    case VK_FORMAT_R8_SINT:
        *out_bytes = 1;
        return true;
    case VK_FORMAT_R16_UNORM:
    case VK_FORMAT_R16_SNORM:
    case VK_FORMAT_R16_UINT:
    case VK_FORMAT_R16_SINT:
    case VK_FORMAT_R16_SFLOAT:
        *out_bytes = 2;
        return true;
    case VK_FORMAT_R32_UINT:
    case VK_FORMAT_R32_SINT:
    case VK_FORMAT_R32_SFLOAT:
    case VK_FORMAT_D32_SFLOAT:
        *out_bytes = 4;
        return true;
    case VK_FORMAT_R8G8B8A8_UNORM:
    case VK_FORMAT_R8G8B8A8_UINT:
    case VK_FORMAT_R8G8B8A8_SINT:
    case VK_FORMAT_B8G8R8A8_UNORM:
        *out_bytes = 4;
        return true;
    case VK_FORMAT_R16G16B16A16_UNORM:
    case VK_FORMAT_R16G16B16A16_UINT:
    case VK_FORMAT_R16G16B16A16_SINT:
    case VK_FORMAT_R16G16B16A16_SFLOAT:
        *out_bytes = 8;
        return true;
    case VK_FORMAT_R32G32B32A32_UINT:
    case VK_FORMAT_R32G32B32A32_SINT:
    case VK_FORMAT_R32G32B32A32_SFLOAT:
        *out_bytes = 16;
        return true;
    default:
        *out_bytes = 0;
        return false;
    }
}



bool _drp2_runtime_state_ensure_capacity(Drp2RuntimeState* state)
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



Drp2Object* _drp2_find_any_object(Drp2RuntimeState* state, uint64_t id)
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
    Drp2Object* object = _drp2_find_any_object(state, id);
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



Drp2Object* _drp2_add_object(Drp2RuntimeState* state, uint64_t id, Drp2ObjectKind kind)
{
    ANN(state);
    if (!_drp2_runtime_state_ensure_capacity(state))
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
    Drp2Object* object = _drp2_find_any_object(state, id);
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
            if (other->kind == DRP2_OBJECT_RENDER_PIPELINE ||
                other->kind == DRP2_OBJECT_COMPUTE_PIPELINE)
            {
                for (uint32_t j = 0; j < other->bind_group_layout_count; j++)
                {
                    if (other->bind_group_layout_ids[j] == id)
                        return _drp2_fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
                }
            }
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

    Drp2Object* object = _drp2_find_any_object(state, id);
    if (object != NULL)
    {
        if (object->destroyed || object->kind != DRP2_OBJECT_BUFFER ||
            !object->referenced_by_work)
            return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
        DvzDrp2ValidationResult result = _validate_buffer_recreate_bind_groups(
            state, id, size, command->u.create_buffer.usage, command_index);
        if (!result.ok)
            return result;
    }
    else
    {
        object = _drp2_add_object(state, id, DRP2_OBJECT_BUFFER);
        if (object == NULL)
            return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }
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
    uint32_t bytes_per_texel = 0;
    if (!_drp2_texture_format_bytes_per_texel(command->u.create_texture.format, &bytes_per_texel))
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, command_index);
    Drp2Object* object = _drp2_find_any_object(state, id);
    if (object != NULL)
    {
        if (object->destroyed || object->kind != DRP2_OBJECT_TEXTURE ||
            !object->referenced_by_work)
            return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }
    else
    {
        object = _drp2_add_object(state, id, DRP2_OBJECT_TEXTURE);
        if (object == NULL)
            return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }
    object->width  = command->u.create_texture.width;
    object->height = command->u.create_texture.height;
    object->depth  = command->u.create_texture.depth > 1 ? command->u.create_texture.depth : 1;
    object->format = command->u.create_texture.format;
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
    if (_drp2_find_any_object(state, id) != NULL)
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

    if (_drp2_add_object(state, id, kind) == NULL)
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
    if (_drp2_find_any_object(state, id) != NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (!_has_object_kind(
            state, command->u.create_render_pipeline.vertex_shader_module_id,
            DRP2_OBJECT_SHADER_VERTEX))
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (!_has_object_kind(
            state, command->u.create_render_pipeline.fragment_shader_module_id,
            DRP2_OBJECT_SHADER_FRAGMENT))
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (command->u.create_render_pipeline.bind_group_layout_count > DVZ_DRP2_MAX_BIND_GROUPS)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (command->u.create_render_pipeline.color_target_count > DVZ_DRP2_MAX_COLOR_ATTACHMENTS)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (command->u.create_render_pipeline.has_raster_state &&
        (!_raster_cull_mode_valid(command->u.create_render_pipeline.cull_mode) ||
         !_raster_front_face_valid(command->u.create_render_pipeline.front_face)))
        return _drp2_fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
    for (uint32_t i = 0; i < command->u.create_render_pipeline.bind_group_layout_count; i++)
    {
        if (!_has_object_kind(
                state, command->u.create_render_pipeline.bind_group_layout_ids[i],
                DRP2_OBJECT_BIND_GROUP_LAYOUT))
            return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }

    Drp2Object* object = _drp2_add_object(state, id, DRP2_OBJECT_RENDER_PIPELINE);
    if (object == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    object->vertex_buffer_slots = command->u.create_render_pipeline.vertex_buffer_slots;
    object->vertex_shader_module_id = command->u.create_render_pipeline.vertex_shader_module_id;
    object->fragment_shader_module_id = command->u.create_render_pipeline.fragment_shader_module_id;
    object->bind_group_layout_count = command->u.create_render_pipeline.bind_group_layout_count;
    if (object->bind_group_layout_count > 0)
    {
        dvz_memcpy(
            object->bind_group_layout_ids, sizeof(object->bind_group_layout_ids),
            command->u.create_render_pipeline.bind_group_layout_ids,
            object->bind_group_layout_count * sizeof(uint64_t));
    }
    object->has_depth_attachment = command->u.create_render_pipeline.has_depth_attachment;
    uint32_t color_target_count = command->u.create_render_pipeline.color_target_count;
    if (color_target_count == 0)
        color_target_count = 1;
    object->color_attachment_count = color_target_count;
    for (uint32_t i = 0; i < color_target_count; i++)
    {
        object->color_attachment_formats[i] = _effective_color_format(
            command->u.create_render_pipeline.color_targets[i].format);
    }
    object->depth_attachment_format =
        object->has_depth_attachment ? _effective_depth_format() : 0;
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
    if (_drp2_find_any_object(state, id) != NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (!_has_object_kind(
            state, command->u.create_compute_pipeline.compute_shader_module_id,
            DRP2_OBJECT_SHADER_COMPUTE))
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (command->u.create_compute_pipeline.bind_group_layout_count > DVZ_DRP2_MAX_BIND_GROUPS)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    for (uint32_t i = 0; i < command->u.create_compute_pipeline.bind_group_layout_count; i++)
    {
        if (!_has_object_kind(
                state, command->u.create_compute_pipeline.bind_group_layout_ids[i],
                DRP2_OBJECT_BIND_GROUP_LAYOUT))
            return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }

    Drp2Object* object = _drp2_add_object(state, id, DRP2_OBJECT_COMPUTE_PIPELINE);
    if (object == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    object->compute_shader_module_id = command->u.create_compute_pipeline.compute_shader_module_id;
    object->bind_group_layout_count = command->u.create_compute_pipeline.bind_group_layout_count;
    if (object->bind_group_layout_count > 0)
    {
        dvz_memcpy(
            object->bind_group_layout_ids, sizeof(object->bind_group_layout_ids),
            command->u.create_compute_pipeline.bind_group_layout_ids,
            object->bind_group_layout_count * sizeof(uint64_t));
    }
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
    Drp2Object* object = _drp2_find_any_object(state, id);
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
    Drp2Object* object = _drp2_find_any_object(state, id);
    if (object != NULL)
    {
        if (object->destroyed || object->kind != DRP2_OBJECT_SAMPLER ||
            !object->referenced_by_work)
            return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }
    else if (_drp2_add_object(state, id, DRP2_OBJECT_SAMPLER) == NULL)
    {
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }
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
    if (command->u.create_bind_group_layout.entry_count == 0 ||
        command->u.create_bind_group_layout.entry_count > DVZ_DRP2_MAX_BINDINGS)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, command_index);
    if (!_entry_bindings_unique(
            command->u.create_bind_group_layout.entries,
            command->u.create_bind_group_layout.entry_count))
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, command_index);
    if (_drp2_find_any_object(state, id) != NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    Drp2Object* object = _drp2_add_object(state, id, DRP2_OBJECT_BIND_GROUP_LAYOUT);
    if (object == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    object->layout_entry_count = command->u.create_bind_group_layout.entry_count;
    dvz_memcpy(
        object->layout_entries, sizeof(object->layout_entries),
        command->u.create_bind_group_layout.entries,
        object->layout_entry_count * sizeof(DvzDrp2BindGroupLayoutEntry));
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
    if (command->u.create_bind_group.entry_count == 0 ||
        command->u.create_bind_group.entry_count > DVZ_DRP2_MAX_BINDINGS)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, command_index);
    if (!_bind_group_entry_bindings_unique(
            command->u.create_bind_group.entries, command->u.create_bind_group.entry_count))
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, command_index);
    if (!_has_object_kind(
            state, command->u.create_bind_group.bind_group_layout_id,
            DRP2_OBJECT_BIND_GROUP_LAYOUT))
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2Object* layout = _find_object(state, command->u.create_bind_group.bind_group_layout_id);
    ANN(layout);
    if (command->u.create_bind_group.entry_count != layout->layout_entry_count)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, command_index);

    for (uint32_t i = 0; i < command->u.create_bind_group.entry_count; i++)
    {
        const DvzDrp2BindGroupEntry* entry = &command->u.create_bind_group.entries[i];
        const DvzDrp2BindGroupLayoutEntry* layout_entry =
            _layout_entry_for_binding(layout, entry->binding);
        if (layout_entry == NULL || layout_entry->binding_type != entry->binding_type)
            return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, command_index);

        if (_binding_type_is_buffer(entry->binding_type))
        {
            if (entry->resource_kind != DVZ_DRP2_BINDING_RESOURCE_BUFFER)
                return _drp2_fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
            Drp2Object* buffer = _find_object(state, entry->resource_id);
            if (buffer == NULL || buffer->kind != DRP2_OBJECT_BUFFER)
                return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
            if (entry->binding_type == DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER &&
                (buffer->usage & DVZ_DRP2_BUFFER_USAGE_UNIFORM) == 0)
                return _drp2_fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
            if (entry->binding_type == DVZ_DRP2_BINDING_TYPE_STORAGE_BUFFER &&
                (buffer->usage & DVZ_DRP2_BUFFER_USAGE_STORAGE) == 0)
                return _drp2_fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
            if (layout_entry->has_dynamic_offset && entry->size == 0)
                return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, command_index);
            if (_drp2_range_overflows(entry->offset, entry->size, buffer->size))
                return _drp2_fail(DVZ_DRP2_VALIDATION_OUT_OF_RANGE, command_index);
        }
        else if (_binding_type_is_texture(entry->binding_type))
        {
            if (entry->resource_kind != DVZ_DRP2_BINDING_RESOURCE_TEXTURE &&
                entry->resource_kind != DVZ_DRP2_BINDING_RESOURCE_TEXTURE_VIEW)
                return _drp2_fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
            Drp2Object* texture = _find_object(state, entry->resource_id);
            if (texture == NULL || texture->kind != DRP2_OBJECT_TEXTURE)
                return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
            if (entry->binding_type == DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE &&
                (texture->usage & DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING) == 0)
                return _drp2_fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
            if (entry->binding_type == DVZ_DRP2_BINDING_TYPE_STORAGE_TEXTURE &&
                (texture->usage & DVZ_DRP2_TEXTURE_USAGE_STORAGE_BINDING) == 0)
                return _drp2_fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
        }
        else if (entry->binding_type == DVZ_DRP2_BINDING_TYPE_SAMPLER)
        {
            if (entry->resource_kind != DVZ_DRP2_BINDING_RESOURCE_SAMPLER)
                return _drp2_fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
            if (!_has_object_kind(state, entry->resource_id, DRP2_OBJECT_SAMPLER))
                return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
        }
        else
            return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, command_index);
    }

    Drp2Object* object = _drp2_find_any_object(state, id);
    if (object != NULL)
    {
        if (object->destroyed || object->kind != DRP2_OBJECT_BIND_GROUP ||
            !object->referenced_by_work)
            return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }
    else
    {
        object = _drp2_add_object(state, id, DRP2_OBJECT_BIND_GROUP);
        if (object == NULL)
            return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }
    object->bind_group_layout_id = command->u.create_bind_group.bind_group_layout_id;
    object->bind_group_entry_count = command->u.create_bind_group.entry_count;
    dvz_memcpy(
        object->bind_group_entries, sizeof(object->bind_group_entries),
        command->u.create_bind_group.entries,
        object->bind_group_entry_count * sizeof(DvzDrp2BindGroupEntry));
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
    if (_drp2_range_overflows(command->u.write_buffer.offset, command->u.write_buffer.size, object->size))
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
            texture->format, command->u.write_texture.width, command->u.write_texture.height,
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
    if (_drp2_find_any_object(state, id) != NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2Object* object = _drp2_add_object(state, id, DRP2_OBJECT_ENCODER);
    if (object == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    object->open = true;
    return _drp2_ok();
}



static DvzDrp2ValidationResult _validate_render_pipeline_attachments(
    const Drp2Object* pass, const Drp2Object* pipeline, uint32_t command_index);



static DvzDrp2ValidationResult _validate_begin_render_pass(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    if (_open_pass(state) != NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (_drp2_find_any_object(state, command->u.begin_render_pass.id) != NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    const Drp2Object* encoder = _find_object(state, command->u.begin_render_pass.encoder_id);
    if (encoder == NULL || encoder->kind != DRP2_OBJECT_ENCODER || !encoder->open)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    uint32_t color_count = command->u.begin_render_pass.color_attachment_count;
    if (color_count == 0)
        color_count = 1;
    if (color_count > DVZ_DRP2_MAX_COLOR_ATTACHMENTS)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    const Drp2Object* first_color = NULL;
    for (uint32_t i = 0; i < color_count; i++)
    {
        const DvzDrp2ColorAttachment* attachment =
            command->u.begin_render_pass.color_attachment_count > 0
                ? &command->u.begin_render_pass.color_attachments[i]
                : NULL;
        if (attachment != NULL &&
            (attachment->load_op > DVZ_DRP2_ATTACHMENT_LOAD_DONT_CARE ||
             attachment->store_op > DVZ_DRP2_ATTACHMENT_STORE_DONT_CARE ||
             attachment->access > DVZ_DRP2_ATTACHMENT_ACCESS_READ_WRITE))
            return _drp2_fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
        uint64_t texture_id = command->u.begin_render_pass.color_attachment_count > 0
                                  ? command->u.begin_render_pass.color_attachments[i].texture_id
                                  : command->u.begin_render_pass.texture_id;
        const Drp2Object* texture = _find_object(state, texture_id);
        if (texture == NULL || texture->kind != DRP2_OBJECT_TEXTURE)
            return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
        if (i == 0)
            first_color = texture;
    }
    if (command->u.begin_render_pass.has_depth_attachment &&
        (command->u.begin_render_pass.depth_load_op > DVZ_DRP2_ATTACHMENT_LOAD_DONT_CARE ||
         command->u.begin_render_pass.depth_store_op > DVZ_DRP2_ATTACHMENT_STORE_DONT_CARE ||
         command->u.begin_render_pass.depth_access > DVZ_DRP2_ATTACHMENT_ACCESS_READ_WRITE))
        return _drp2_fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
    if (command->u.begin_render_pass.has_depth_attachment &&
        command->u.begin_render_pass.depth_texture_id != 0)
    {
        const Drp2Object* depth =
            _find_object(state, command->u.begin_render_pass.depth_texture_id);
        if (depth == NULL || depth->kind != DRP2_OBJECT_TEXTURE)
            return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
        if ((depth->usage & DVZ_DRP2_TEXTURE_USAGE_RENDER_ATTACHMENT) == 0)
            return _drp2_fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
        if (first_color == NULL || depth->width != first_color->width ||
            depth->height != first_color->height)
            return _drp2_fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
    }

    Drp2Object* pass = _drp2_add_object(state, command->u.begin_render_pass.id, DRP2_OBJECT_RENDER_PASS);
    if (pass == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    encoder = _find_object(state, command->u.begin_render_pass.encoder_id);
    if (encoder == NULL || encoder->kind != DRP2_OBJECT_ENCODER || !encoder->open)
    {
        pass->destroyed = true;
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }
    for (uint32_t i = 0; i < color_count; i++)
    {
        uint64_t texture_id = command->u.begin_render_pass.color_attachment_count > 0
                                  ? command->u.begin_render_pass.color_attachments[i].texture_id
                                  : command->u.begin_render_pass.texture_id;
        if (!_has_object_kind(state, texture_id, DRP2_OBJECT_TEXTURE))
        {
            pass->destroyed = true;
            return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
        }
    }

    for (uint32_t i = 0; i < color_count; i++)
    {
        uint64_t texture_id = command->u.begin_render_pass.color_attachment_count > 0
                                  ? command->u.begin_render_pass.color_attachments[i].texture_id
                                  : command->u.begin_render_pass.texture_id;
        _mark_referenced(state, texture_id);
    }
    if (command->u.begin_render_pass.has_depth_attachment &&
        command->u.begin_render_pass.depth_texture_id != 0)
        _mark_referenced(state, command->u.begin_render_pass.depth_texture_id);
    pass->open = true;
    pass->encoder_id = command->u.begin_render_pass.encoder_id;
    pass->has_depth_attachment = command->u.begin_render_pass.has_depth_attachment;
    pass->color_attachment_count = color_count;
    for (uint32_t i = 0; i < color_count; i++)
    {
        uint64_t texture_id = command->u.begin_render_pass.color_attachment_count > 0
                                  ? command->u.begin_render_pass.color_attachments[i].texture_id
                                  : command->u.begin_render_pass.texture_id;
        const Drp2Object* texture = _find_object(state, texture_id);
        if (texture == NULL || texture->kind != DRP2_OBJECT_TEXTURE)
        {
            pass->destroyed = true;
            return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
        }
        pass->color_attachment_formats[i] = _effective_color_format(texture->format);
    }
    if (command->u.begin_render_pass.has_depth_attachment)
    {
        if (command->u.begin_render_pass.depth_texture_id != 0)
        {
            const Drp2Object* depth =
                _find_object(state, command->u.begin_render_pass.depth_texture_id);
            if (depth == NULL || depth->kind != DRP2_OBJECT_TEXTURE)
            {
                pass->destroyed = true;
                return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
            }
            pass->depth_attachment_format = depth->format;
        }
        else
        {
            pass->depth_attachment_format = _effective_depth_format();
        }
    }
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
    if (_drp2_find_any_object(state, command->u.begin_compute_pass.id) != NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    const Drp2Object* encoder = _find_object(state, command->u.begin_compute_pass.encoder_id);
    if (encoder == NULL || encoder->kind != DRP2_OBJECT_ENCODER || !encoder->open)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2Object* pass =
        _drp2_add_object(state, command->u.begin_compute_pass.id, DRP2_OBJECT_COMPUTE_PASS);
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



/**
 * Validate that a render pipeline is compatible with the active render pass attachments.
 *
 * @param pass the active render pass object
 * @param pipeline the render pipeline object
 * @param command_index command index used for validation reporting
 * @return validation result
 */
static DvzDrp2ValidationResult _validate_render_pipeline_attachments(
    const Drp2Object* pass, const Drp2Object* pipeline, uint32_t command_index)
{
    ANN(pass);
    ANN(pipeline);

    if (pipeline->color_attachment_count != pass->color_attachment_count)
        return _drp2_fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
    for (uint32_t i = 0; i < pipeline->color_attachment_count; i++)
    {
        if (pipeline->color_attachment_formats[i] != pass->color_attachment_formats[i])
            return _drp2_fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
    }
    if (pipeline->has_depth_attachment != pass->has_depth_attachment)
        return _drp2_fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
    if (pipeline->has_depth_attachment &&
        pipeline->depth_attachment_format != pass->depth_attachment_format)
        return _drp2_fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
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

    Drp2Object* pipeline = _find_object(state, command->u.set_pipeline.pipeline_id);
    if (pipeline == NULL || pipeline->kind != pipeline_kind)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (pass->kind == DRP2_OBJECT_RENDER_PASS)
    {
        DvzDrp2ValidationResult result =
            _validate_render_pipeline_attachments(pass, pipeline, command_index);
        if (!result.ok)
            return result;
    }

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
    if (slot >= pipeline->bind_group_layout_count)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    uint64_t expected_layout_id = pipeline->bind_group_layout_ids[slot];
    Drp2Object* bind_group = _find_object(state, command->u.set_bind_group.bind_group_id);
    if (bind_group == NULL || bind_group->kind != DRP2_OBJECT_BIND_GROUP)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (bind_group->bind_group_layout_id != expected_layout_id)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    Drp2Object* layout = _find_object(state, expected_layout_id);
    if (layout == NULL || layout->kind != DRP2_OBJECT_BIND_GROUP_LAYOUT)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (command->u.set_bind_group.dynamic_offset_count != _dynamic_binding_count(layout))
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, command_index);

    uint32_t dynamic_index = 0;
    for (uint32_t i = 0; i < layout->layout_entry_count; i++)
    {
        const DvzDrp2BindGroupLayoutEntry* layout_entry = &layout->layout_entries[i];
        if (!layout_entry->has_dynamic_offset)
            continue;
        const DvzDrp2BindGroupEntry* entry = NULL;
        for (uint32_t j = 0; j < bind_group->bind_group_entry_count; j++)
        {
            if (bind_group->bind_group_entries[j].binding == layout_entry->binding)
            {
                entry = &bind_group->bind_group_entries[j];
                break;
            }
        }
        if (entry == NULL || !_binding_type_is_buffer(entry->binding_type))
            return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
        Drp2Object* buffer = _find_object(state, entry->resource_id);
        if (buffer == NULL || buffer->kind != DRP2_OBJECT_BUFFER)
            return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
        uint64_t offset = entry->offset + command->u.set_bind_group.dynamic_offsets[dynamic_index++];
        if (offset < entry->offset || _drp2_range_overflows(offset, entry->size, buffer->size))
            return _drp2_fail(DVZ_DRP2_VALIDATION_OUT_OF_RANGE, command_index);
    }

    pass->bound_bind_group_mask |= (1u << slot);
    _mark_referenced(state, command->u.set_bind_group.bind_group_id);
    _mark_referenced(state, bind_group->bind_group_layout_id);
    for (uint32_t i = 0; i < bind_group->bind_group_entry_count; i++)
        _mark_referenced(state, bind_group->bind_group_entries[i].resource_id);
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

    DvzDrp2ValidationResult attachment_result =
        _validate_render_pipeline_attachments(pass, pipeline, command_index);
    if (!attachment_result.ok)
        return attachment_result;

    uint32_t required_mask = 0;
    if (pipeline->vertex_buffer_slots >= 32)
        required_mask = UINT32_MAX;
    else if (pipeline->vertex_buffer_slots > 0)
        required_mask = (uint32_t)((1u << pipeline->vertex_buffer_slots) - 1u);
    if ((pass->bound_vertex_mask & required_mask) != required_mask)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    for (uint32_t i = 0; i < pipeline->bind_group_layout_count; i++)
    {
        if ((pass->bound_bind_group_mask & (1u << i)) == 0)
            return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }
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
    for (uint32_t i = 0; i < pipeline->bind_group_layout_count; i++)
    {
        if ((pass->bound_bind_group_mask & (1u << i)) == 0)
            return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }
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
    if (_drp2_range_overflows(
            command->u.copy_buffer_to_buffer.src_offset, command->u.copy_buffer_to_buffer.size,
            src->size))
        return _drp2_fail(DVZ_DRP2_VALIDATION_OUT_OF_RANGE, command_index);
    if (_drp2_range_overflows(
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
            texture->format, command->u.copy_buffer_to_texture.width,
            command->u.copy_buffer_to_texture.height, command->u.copy_buffer_to_texture.depth,
            command->u.copy_buffer_to_texture.bytes_per_row,
            command->u.copy_buffer_to_texture.rows_per_image))
        return _drp2_fail(DVZ_DRP2_VALIDATION_USAGE, command_index);

    uint64_t size = _drp2_texture_layout_size(
        command->u.copy_buffer_to_texture.depth, command->u.copy_buffer_to_texture.bytes_per_row,
        command->u.copy_buffer_to_texture.rows_per_image);
    if (_drp2_range_overflows(command->u.copy_buffer_to_texture.src_offset, size, buffer->size))
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
            texture->format, command->u.copy_texture_to_buffer.width,
            command->u.copy_texture_to_buffer.height, 1,
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
    if (_drp2_range_overflows(command->u.copy_texture_to_buffer.dst_offset, required, buffer->size))
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
    if (_drp2_find_any_object(state, command->u.finish_command_encoder.command_buffer_id) != NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    const Drp2Object* encoder = _find_object(state, command->u.finish_command_encoder.encoder_id);
    if (encoder == NULL || encoder->kind != DRP2_OBJECT_ENCODER || !encoder->open)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2Object* command_buffer = _drp2_add_object(
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
    if (_drp2_range_overflows(command->u.queue_submit.offset, command->u.queue_submit.size, buffer->size))
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
void _drp2_runtime_state_cleanup(Drp2RuntimeState* state)
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
bool _drp2_runtime_state_ensure(DvzDrp2Runtime* runtime)
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
bool _drp2_runtime_state_commit(DvzDrp2Runtime* runtime, Drp2RuntimeState* next_state)
{
    ANN(runtime);
    ANN(next_state);
    if (!_drp2_runtime_state_ensure(runtime))
        return false;

    _drp2_runtime_state_cleanup(runtime->semantic_state);
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
DvzDrp2ValidationResult
_drp2_runtime_validate_stream(
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
