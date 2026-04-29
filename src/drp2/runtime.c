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

#include "_alloc.h"
#include "_assertions.h"
#include "_stream.h"

#if DVZ_DRP2_HAS_VKLITE
#include "datoviz/vklite/buffers.h"
#include "datoviz/vklite/images.h"
#endif



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#ifndef DVZ_DRP2_HAS_VKLITE
#define DVZ_DRP2_HAS_VKLITE 0
#endif



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_DRP2_RUNTIME_INITIAL_OBJECT_CAPACITY 64
#define DVZ_DRP2_RGBA8_BYTES_PER_TEXEL 4



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DRP2_OBJECT_NONE,
    DRP2_OBJECT_BUFFER,
    DRP2_OBJECT_TEXTURE,
    DRP2_OBJECT_SHADER_VERTEX,
    DRP2_OBJECT_SHADER_FRAGMENT,
    DRP2_OBJECT_SHADER_COMPUTE,
    DRP2_OBJECT_RENDER_PIPELINE,
    DRP2_OBJECT_COMPUTE_PIPELINE,
    DRP2_OBJECT_SAMPLER,
    DRP2_OBJECT_BIND_GROUP_LAYOUT,
    DRP2_OBJECT_BIND_GROUP,
    DRP2_OBJECT_ENCODER,
    DRP2_OBJECT_RENDER_PASS,
    DRP2_OBJECT_COMPUTE_PASS,
    DRP2_OBJECT_COMMAND_BUFFER,
} Drp2ObjectKind;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct Drp2Object Drp2Object;
typedef struct Drp2RuntimeState Drp2RuntimeState;
#if DVZ_DRP2_HAS_VKLITE
typedef struct Drp2VkliteObject Drp2VkliteObject;
typedef struct Drp2VkliteState Drp2VkliteState;
#endif

struct DvzDrp2Runtime
{
    DvzDevice* device;
    DvzVma* allocator;
    bool semantic_only;
#if DVZ_DRP2_HAS_VKLITE
    Drp2VkliteState* vklite_state;
#endif
};


struct Drp2Object
{
    uint64_t id;
    Drp2ObjectKind kind;
    uint64_t size;
    uint32_t usage;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t vertex_buffer_slots;
    uint64_t vertex_shader_module_id;
    uint64_t fragment_shader_module_id;
    uint64_t compute_shader_module_id;
    uint64_t bind_group_layout_id;
    uint64_t texture_id;
    uint64_t sampler_id;
    uint64_t buffer0_id;
    uint64_t buffer1_id;
    uint64_t buffer_size;
    bool destroyed;
    bool referenced_by_work;
    bool open;
    bool submitted;
    uint64_t encoder_id;
    uint64_t pipeline_id;
    uint32_t bound_vertex_mask;
    bool index_buffer_bound;
    uint32_t bound_bind_group_mask;
    bool storage_buffers;
};


struct Drp2RuntimeState
{
    bool hello_seen;
    bool reply_seen;
    bool failed;
    uint32_t capacity;
    uint32_t count;
    Drp2Object* objects;
};

#if DVZ_DRP2_HAS_VKLITE
struct Drp2VkliteObject
{
    uint64_t id;
    Drp2ObjectKind kind;
    DvzBuffer* buffer;
    DvzImages* images;
    bool destroyed;
};


struct Drp2VkliteState
{
    DvzDrp2Runtime* runtime;
    uint32_t capacity;
    uint32_t count;
    Drp2VkliteObject* objects;
};
#endif



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

#if DVZ_DRP2_HAS_VKLITE
bool _dvz_drp2_runtime_vklite_download_buffer(
    DvzDrp2Runtime* runtime, uint64_t buffer_id, uint64_t offset, uint64_t size, void* data);
#endif



static DvzDrp2ValidationResult _result(
    bool ok, DvzDrp2ValidationCode code, uint32_t command_index)
{
    DvzDrp2ValidationResult result = {0};
    result.ok = ok;
    result.code = code;
    result.command_index = command_index;
    return result;
}



static DvzDrp2ValidationResult _ok(void)
{
    return _result(true, DVZ_DRP2_VALIDATION_OK, UINT32_MAX);
}



static DvzDrp2ValidationResult _fail(DvzDrp2ValidationCode code, uint32_t command_index)
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



static uint64_t _texture_layout_size(
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

    state->capacity *= 2;
    state->objects =
        (Drp2Object*)dvz_realloc(state->objects, state->capacity * sizeof(Drp2Object));
    return state->objects != NULL;
}



static Drp2Object* _find_any_object(Drp2RuntimeState* state, uint64_t id)
{
    ANN(state);
    for (uint32_t i = 0; i < state->count; i++)
    {
        if (state->objects[i].id == id)
            return &state->objects[i];
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
    for (uint32_t i = 0; i < state->count; i++)
    {
        if (state->objects[i].id == id)
            return &state->objects[i];
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
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (object->referenced_by_work || object->open || object->submitted)
        return _fail(DVZ_DRP2_VALIDATION_USAGE, command_index);

    if (kind == DRP2_OBJECT_SHADER_VERTEX || kind == DRP2_OBJECT_SHADER_FRAGMENT ||
        kind == DRP2_OBJECT_SHADER_COMPUTE)
    {
        for (uint32_t i = 0; i < state->count; i++)
        {
            Drp2Object* other = &state->objects[i];
            if (!other->destroyed && _pipeline_uses_shader(other, id))
                return _fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
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
                return _fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
        }
    }

    object->destroyed = true;
    return _ok();
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
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    return _ok();
}



static DvzDrp2ValidationResult _validate_create_buffer(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    uint64_t id = command->u.create_buffer.id;
    uint64_t size = command->u.create_buffer.size;
    if (id == 0 || size == 0)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, command_index);
    if (_find_any_object(state, id) != NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2Object* object = _add_object(state, id, DRP2_OBJECT_BUFFER);
    if (object == NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    object->size = size;
    object->usage = command->u.create_buffer.usage;
    return _ok();
}



static DvzDrp2ValidationResult _validate_create_texture(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    uint64_t id = command->u.create_texture.id;
    if (id == 0 || command->u.create_texture.width == 0 || command->u.create_texture.height == 0)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, command_index);
    if (_find_any_object(state, id) != NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (_add_object(state, id, DRP2_OBJECT_TEXTURE) == NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    Drp2Object* object = _find_object(state, id);
    ANN(object);
    object->width = command->u.create_texture.width;
    object->height = command->u.create_texture.height;
    object->depth = 1;
    object->usage = command->u.create_texture.usage;
    return _ok();
}



static DvzDrp2ValidationResult _validate_create_shader_module(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    uint64_t id = command->u.create_shader_module.id;
    if (id == 0 || command->u.create_shader_module.stage[0] == '\0')
        return _fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, command_index);
    if (_find_any_object(state, id) != NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

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
        return _fail(DVZ_DRP2_VALIDATION_USAGE, command_index);

    if (_add_object(state, id, kind) == NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    return _ok();
}



static DvzDrp2ValidationResult _validate_create_render_pipeline(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    uint64_t id = command->u.create_render_pipeline.id;
    if (id == 0)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, command_index);
    if (_find_any_object(state, id) != NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (!_has_object_kind(
            state, command->u.create_render_pipeline.vertex_shader_module_id,
            DRP2_OBJECT_SHADER_VERTEX))
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (!_has_object_kind(
            state, command->u.create_render_pipeline.fragment_shader_module_id,
            DRP2_OBJECT_SHADER_FRAGMENT))
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (command->u.create_render_pipeline.bind_group_layout_id != 0 &&
        !_has_object_kind(
            state, command->u.create_render_pipeline.bind_group_layout_id,
            DRP2_OBJECT_BIND_GROUP_LAYOUT))
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2Object* object = _add_object(state, id, DRP2_OBJECT_RENDER_PIPELINE);
    if (object == NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    object->vertex_buffer_slots = command->u.create_render_pipeline.vertex_buffer_slots;
    object->vertex_shader_module_id = command->u.create_render_pipeline.vertex_shader_module_id;
    object->fragment_shader_module_id = command->u.create_render_pipeline.fragment_shader_module_id;
    object->bind_group_layout_id = command->u.create_render_pipeline.bind_group_layout_id;
    return _ok();
}



static DvzDrp2ValidationResult _validate_create_compute_pipeline(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    uint64_t id = command->u.create_compute_pipeline.id;
    if (id == 0)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, command_index);
    if (_find_any_object(state, id) != NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (!_has_object_kind(
            state, command->u.create_compute_pipeline.compute_shader_module_id,
            DRP2_OBJECT_SHADER_COMPUTE))
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (command->u.create_compute_pipeline.bind_group_layout_id != 0 &&
        !_has_object_kind(
            state, command->u.create_compute_pipeline.bind_group_layout_id,
            DRP2_OBJECT_BIND_GROUP_LAYOUT))
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2Object* object = _add_object(state, id, DRP2_OBJECT_COMPUTE_PIPELINE);
    if (object == NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    object->compute_shader_module_id = command->u.create_compute_pipeline.compute_shader_module_id;
    object->bind_group_layout_id = command->u.create_compute_pipeline.bind_group_layout_id;
    return _ok();
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
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (object->kind != DRP2_OBJECT_SHADER_VERTEX && object->kind != DRP2_OBJECT_SHADER_FRAGMENT &&
        object->kind != DRP2_OBJECT_SHADER_COMPUTE)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
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
        return _fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, command_index);
    if (_find_any_object(state, id) != NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (_add_object(state, id, DRP2_OBJECT_SAMPLER) == NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    return _ok();
}



static DvzDrp2ValidationResult _validate_create_bind_group_layout(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    uint64_t id = command->u.create_bind_group_layout.id;
    if (id == 0)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, command_index);
    if (_find_any_object(state, id) != NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (_add_object(state, id, DRP2_OBJECT_BIND_GROUP_LAYOUT) == NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    Drp2Object* object = _find_object(state, id);
    ANN(object);
    object->storage_buffers = command->u.create_bind_group_layout.storage_buffers;
    return _ok();
}



static DvzDrp2ValidationResult _validate_create_bind_group(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    uint64_t id = command->u.create_bind_group.id;
    if (id == 0)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, command_index);
    if (_find_any_object(state, id) != NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (!_has_object_kind(
            state, command->u.create_bind_group.bind_group_layout_id,
            DRP2_OBJECT_BIND_GROUP_LAYOUT))
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2Object* layout = _find_object(state, command->u.create_bind_group.bind_group_layout_id);
    ANN(layout);
    if (layout->storage_buffers)
    {
        Drp2Object* buffer0 = _find_object(state, command->u.create_bind_group.buffer0_id);
        Drp2Object* buffer1 = _find_object(state, command->u.create_bind_group.buffer1_id);
        if (buffer0 == NULL || buffer0->kind != DRP2_OBJECT_BUFFER || buffer1 == NULL ||
            buffer1->kind != DRP2_OBJECT_BUFFER)
            return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
        if ((buffer0->usage & DVZ_DRP2_BUFFER_USAGE_STORAGE) == 0 ||
            (buffer1->usage & DVZ_DRP2_BUFFER_USAGE_STORAGE) == 0)
            return _fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
        if (_range_overflows(0, command->u.create_bind_group.buffer_size, buffer0->size) ||
            _range_overflows(0, command->u.create_bind_group.buffer_size, buffer1->size))
            return _fail(DVZ_DRP2_VALIDATION_OUT_OF_RANGE, command_index);
    }
    else
    {
        Drp2Object* texture = _find_object(state, command->u.create_bind_group.texture_id);
        if (texture == NULL || texture->kind != DRP2_OBJECT_TEXTURE)
            return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
        if ((texture->usage & DVZ_DRP2_TEXTURE_USAGE_TEXTURE_BINDING) == 0)
            return _fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
        if (!_has_object_kind(state, command->u.create_bind_group.sampler_id, DRP2_OBJECT_SAMPLER))
            return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }

    Drp2Object* object = _add_object(state, id, DRP2_OBJECT_BIND_GROUP);
    if (object == NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    object->bind_group_layout_id = command->u.create_bind_group.bind_group_layout_id;
    object->texture_id = command->u.create_bind_group.texture_id;
    object->sampler_id = command->u.create_bind_group.sampler_id;
    object->buffer0_id = command->u.create_bind_group.buffer0_id;
    object->buffer1_id = command->u.create_bind_group.buffer1_id;
    object->buffer_size = command->u.create_bind_group.buffer_size;
    object->storage_buffers = layout->storage_buffers;
    return _ok();
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
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if ((object->usage & DVZ_DRP2_BUFFER_USAGE_COPY_DST) == 0)
        return _fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
    if (_range_overflows(command->u.write_buffer.offset, command->u.write_buffer.size, object->size))
        return _fail(DVZ_DRP2_VALIDATION_OUT_OF_RANGE, command_index);
    return _ok();
}



static DvzDrp2ValidationResult _validate_write_texture(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    Drp2Object* texture = _find_object(state, command->u.write_texture.texture_id);
    if (texture == NULL || texture->kind != DRP2_OBJECT_TEXTURE)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if ((texture->usage & DVZ_DRP2_TEXTURE_USAGE_COPY_DST) == 0)
        return _fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
    if (command->u.write_texture.mip_level != 0)
        return _fail(DVZ_DRP2_VALIDATION_OUT_OF_RANGE, command_index);
    if (_texture_box_overflows(
            texture, command->u.write_texture.origin_x, command->u.write_texture.origin_y,
            command->u.write_texture.origin_z, command->u.write_texture.width,
            command->u.write_texture.height, command->u.write_texture.depth))
        return _fail(DVZ_DRP2_VALIDATION_OUT_OF_RANGE, command_index);
    if (_texture_layout_invalid(
            command->u.write_texture.width, command->u.write_texture.height,
            command->u.write_texture.depth, command->u.write_texture.bytes_per_row,
            command->u.write_texture.rows_per_image))
        return _fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
    return _ok();
}



static DvzDrp2ValidationResult _validate_begin_encoder(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    uint64_t id = command->u.begin_command_encoder.id;
    if (id == 0)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, command_index);
    if (_find_any_object(state, id) != NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2Object* object = _add_object(state, id, DRP2_OBJECT_ENCODER);
    if (object == NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    object->open = true;
    return _ok();
}



static DvzDrp2ValidationResult _validate_begin_render_pass(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    if (_open_pass(state) != NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (_find_any_object(state, command->u.begin_render_pass.id) != NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2Object* encoder = _find_object(state, command->u.begin_render_pass.encoder_id);
    if (encoder == NULL || encoder->kind != DRP2_OBJECT_ENCODER || !encoder->open)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (!_has_object_kind(state, command->u.begin_render_pass.texture_id, DRP2_OBJECT_TEXTURE))
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    _mark_referenced(state, command->u.begin_render_pass.texture_id);

    Drp2Object* pass = _add_object(state, command->u.begin_render_pass.id, DRP2_OBJECT_RENDER_PASS);
    if (pass == NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    pass->open = true;
    pass->encoder_id = command->u.begin_render_pass.encoder_id;
    return _ok();
}



static DvzDrp2ValidationResult _validate_begin_compute_pass(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    if (_open_pass(state) != NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (_find_any_object(state, command->u.begin_compute_pass.id) != NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2Object* encoder = _find_object(state, command->u.begin_compute_pass.encoder_id);
    if (encoder == NULL || encoder->kind != DRP2_OBJECT_ENCODER || !encoder->open)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2Object* pass =
        _add_object(state, command->u.begin_compute_pass.id, DRP2_OBJECT_COMPUTE_PASS);
    if (pass == NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    pass->open = true;
    pass->encoder_id = command->u.begin_compute_pass.encoder_id;
    return _ok();
}



static DvzDrp2ValidationResult _validate_set_pipeline(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    Drp2Object* pass = _find_object(state, command->u.set_pipeline.pass_id);
    if (pass == NULL || !pass->open)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2ObjectKind pipeline_kind = DRP2_OBJECT_NONE;
    if (pass->kind == DRP2_OBJECT_RENDER_PASS)
        pipeline_kind = DRP2_OBJECT_RENDER_PIPELINE;
    else if (pass->kind == DRP2_OBJECT_COMPUTE_PASS)
        pipeline_kind = DRP2_OBJECT_COMPUTE_PIPELINE;
    else
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    if (!_has_object_kind(state, command->u.set_pipeline.pipeline_id, pipeline_kind))
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    pass->pipeline_id = command->u.set_pipeline.pipeline_id;
    pass->bound_vertex_mask = 0;
    pass->index_buffer_bound = false;
    pass->bound_bind_group_mask = 0;
    _mark_referenced(state, command->u.set_pipeline.pipeline_id);
    return _ok();
}



static DvzDrp2ValidationResult _validate_set_bind_group(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    Drp2Object* pass = _find_object(state, command->u.set_bind_group.pass_id);
    if (pass == NULL || !pass->open)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2Object* pipeline = _find_object(state, pass->pipeline_id);
    if (pipeline == NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (pass->kind == DRP2_OBJECT_RENDER_PASS && pipeline->kind != DRP2_OBJECT_RENDER_PIPELINE)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (pass->kind == DRP2_OBJECT_COMPUTE_PASS && pipeline->kind != DRP2_OBJECT_COMPUTE_PIPELINE)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (pass->kind != DRP2_OBJECT_RENDER_PASS && pass->kind != DRP2_OBJECT_COMPUTE_PASS)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (command->u.set_bind_group.slot != 0 || pipeline->bind_group_layout_id == 0)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2Object* bind_group = _find_object(state, command->u.set_bind_group.bind_group_id);
    if (bind_group == NULL || bind_group->kind != DRP2_OBJECT_BIND_GROUP)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (bind_group->bind_group_layout_id != pipeline->bind_group_layout_id)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    pass->bound_bind_group_mask |= 1u;
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
    return _ok();
}



static DvzDrp2ValidationResult _validate_set_vertex_buffer(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    Drp2Object* pass = _find_object(state, command->u.set_vertex_buffer.pass_id);
    if (pass == NULL || pass->kind != DRP2_OBJECT_RENDER_PASS || !pass->open)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2Object* pipeline = _find_object(state, pass->pipeline_id);
    if (pipeline == NULL || pipeline->kind != DRP2_OBJECT_RENDER_PIPELINE)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (command->u.set_vertex_buffer.slot >= pipeline->vertex_buffer_slots ||
        command->u.set_vertex_buffer.slot >= 32)
        return _fail(DVZ_DRP2_VALIDATION_OUT_OF_RANGE, command_index);

    Drp2Object* buffer = _find_object(state, command->u.set_vertex_buffer.buffer_id);
    if (buffer == NULL || buffer->kind != DRP2_OBJECT_BUFFER)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if ((buffer->usage & DVZ_DRP2_BUFFER_USAGE_VERTEX) == 0)
        return _fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
    if (command->u.set_vertex_buffer.offset > buffer->size)
        return _fail(DVZ_DRP2_VALIDATION_OUT_OF_RANGE, command_index);

    pass->bound_vertex_mask |= (uint32_t)(1u << command->u.set_vertex_buffer.slot);
    _mark_referenced(state, command->u.set_vertex_buffer.buffer_id);
    return _ok();
}



static DvzDrp2ValidationResult _validate_set_index_buffer(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    Drp2Object* pass = _find_object(state, command->u.set_index_buffer.pass_id);
    if (pass == NULL || pass->kind != DRP2_OBJECT_RENDER_PASS || !pass->open)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2Object* buffer = _find_object(state, command->u.set_index_buffer.buffer_id);
    if (buffer == NULL || buffer->kind != DRP2_OBJECT_BUFFER)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if ((buffer->usage & DVZ_DRP2_BUFFER_USAGE_INDEX) == 0)
        return _fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
    if (command->u.set_index_buffer.offset > buffer->size)
        return _fail(DVZ_DRP2_VALIDATION_OUT_OF_RANGE, command_index);
    if (strcmp(command->u.set_index_buffer.index_format, "uint16") != 0 &&
        strcmp(command->u.set_index_buffer.index_format, "uint32") != 0)
        return _fail(DVZ_DRP2_VALIDATION_USAGE, command_index);

    pass->index_buffer_bound = true;
    _mark_referenced(state, command->u.set_index_buffer.buffer_id);
    return _ok();
}



static DvzDrp2ValidationResult _validate_render_draw_state(
    Drp2RuntimeState* state, Drp2Object* pass, uint32_t command_index)
{
    ANN(state);
    ANN(pass);

    Drp2Object* pipeline = _find_object(state, pass->pipeline_id);
    if (pipeline == NULL || pipeline->kind != DRP2_OBJECT_RENDER_PIPELINE)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    uint32_t required_mask = 0;
    if (pipeline->vertex_buffer_slots >= 32)
        required_mask = UINT32_MAX;
    else if (pipeline->vertex_buffer_slots > 0)
        required_mask = (uint32_t)((1u << pipeline->vertex_buffer_slots) - 1u);
    if ((pass->bound_vertex_mask & required_mask) != required_mask)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (pipeline->bind_group_layout_id != 0 && (pass->bound_bind_group_mask & 1u) == 0)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    return _ok();
}



static DvzDrp2ValidationResult _validate_draw(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    Drp2Object* pass = _find_object(state, command->u.draw.pass_id);
    if (pass == NULL || pass->kind != DRP2_OBJECT_RENDER_PASS || !pass->open)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    return _validate_render_draw_state(state, pass, command_index);
}



static DvzDrp2ValidationResult _validate_draw_indexed(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    Drp2Object* pass = _find_object(state, command->u.draw_indexed.pass_id);
    if (pass == NULL || pass->kind != DRP2_OBJECT_RENDER_PASS || !pass->open)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    DvzDrp2ValidationResult result = _validate_render_draw_state(state, pass, command_index);
    if (!result.ok)
        return result;
    if (!pass->index_buffer_bound)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    return _ok();
}



static DvzDrp2ValidationResult _validate_end_render_pass(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    Drp2Object* pass = _find_object(state, command->u.end_render_pass.pass_id);
    if (pass == NULL || pass->kind != DRP2_OBJECT_RENDER_PASS || !pass->open)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    pass->open = false;
    return _ok();
}



static DvzDrp2ValidationResult _validate_dispatch_workgroups(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    Drp2Object* pass = _find_object(state, command->u.dispatch.pass_id);
    if (pass == NULL || pass->kind != DRP2_OBJECT_COMPUTE_PASS || !pass->open)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2Object* pipeline = _find_object(state, pass->pipeline_id);
    if (pipeline == NULL || pipeline->kind != DRP2_OBJECT_COMPUTE_PIPELINE)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (command->u.dispatch.x == 0 || command->u.dispatch.y == 0 || command->u.dispatch.z == 0)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, command_index);
    if (pipeline->bind_group_layout_id != 0 && (pass->bound_bind_group_mask & 1u) == 0)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    _mark_referenced(state, pass->pipeline_id);
    return _ok();
}



static DvzDrp2ValidationResult _validate_end_compute_pass(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    Drp2Object* pass = _find_object(state, command->u.end_compute_pass.pass_id);
    if (pass == NULL || pass->kind != DRP2_OBJECT_COMPUTE_PASS || !pass->open)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    pass->open = false;
    return _ok();
}



static DvzDrp2ValidationResult _validate_copy_buffer_to_buffer(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    if (_open_pass(state) != NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2Object* encoder = _find_object(state, command->u.copy_buffer_to_buffer.encoder_id);
    if (encoder == NULL || encoder->kind != DRP2_OBJECT_ENCODER || !encoder->open)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2Object* src = _find_object(state, command->u.copy_buffer_to_buffer.src_buffer_id);
    Drp2Object* dst = _find_object(state, command->u.copy_buffer_to_buffer.dst_buffer_id);
    if (src == NULL || src->kind != DRP2_OBJECT_BUFFER || dst == NULL ||
        dst->kind != DRP2_OBJECT_BUFFER)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if ((src->usage & DVZ_DRP2_BUFFER_USAGE_COPY_SRC) == 0 ||
        (dst->usage & DVZ_DRP2_BUFFER_USAGE_COPY_DST) == 0)
        return _fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
    if (_range_overflows(
            command->u.copy_buffer_to_buffer.src_offset, command->u.copy_buffer_to_buffer.size,
            src->size))
        return _fail(DVZ_DRP2_VALIDATION_OUT_OF_RANGE, command_index);
    if (_range_overflows(
            command->u.copy_buffer_to_buffer.dst_offset, command->u.copy_buffer_to_buffer.size,
            dst->size))
        return _fail(DVZ_DRP2_VALIDATION_OUT_OF_RANGE, command_index);

    _mark_referenced(state, command->u.copy_buffer_to_buffer.src_buffer_id);
    _mark_referenced(state, command->u.copy_buffer_to_buffer.dst_buffer_id);
    return _ok();
}



static DvzDrp2ValidationResult _validate_copy_buffer_to_texture(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    if (_open_pass(state) != NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2Object* encoder = _find_object(state, command->u.copy_buffer_to_texture.encoder_id);
    if (encoder == NULL || encoder->kind != DRP2_OBJECT_ENCODER || !encoder->open)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2Object* buffer = _find_object(state, command->u.copy_buffer_to_texture.src_buffer_id);
    if (buffer == NULL || buffer->kind != DRP2_OBJECT_BUFFER)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if ((buffer->usage & DVZ_DRP2_BUFFER_USAGE_COPY_SRC) == 0)
        return _fail(DVZ_DRP2_VALIDATION_USAGE, command_index);

    Drp2Object* texture = _find_object(state, command->u.copy_buffer_to_texture.dst_texture_id);
    if (texture == NULL || texture->kind != DRP2_OBJECT_TEXTURE)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if ((texture->usage & DVZ_DRP2_TEXTURE_USAGE_COPY_DST) == 0)
        return _fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
    if (command->u.copy_buffer_to_texture.dst_mip_level != 0)
        return _fail(DVZ_DRP2_VALIDATION_OUT_OF_RANGE, command_index);
    if (_texture_box_overflows(
            texture, command->u.copy_buffer_to_texture.dst_origin_x,
            command->u.copy_buffer_to_texture.dst_origin_y,
            command->u.copy_buffer_to_texture.dst_origin_z, command->u.copy_buffer_to_texture.width,
            command->u.copy_buffer_to_texture.height, command->u.copy_buffer_to_texture.depth))
        return _fail(DVZ_DRP2_VALIDATION_OUT_OF_RANGE, command_index);
    if (_texture_layout_invalid(
            command->u.copy_buffer_to_texture.width, command->u.copy_buffer_to_texture.height,
            command->u.copy_buffer_to_texture.depth, command->u.copy_buffer_to_texture.bytes_per_row,
            command->u.copy_buffer_to_texture.rows_per_image))
        return _fail(DVZ_DRP2_VALIDATION_USAGE, command_index);

    uint64_t size = _texture_layout_size(
        command->u.copy_buffer_to_texture.depth, command->u.copy_buffer_to_texture.bytes_per_row,
        command->u.copy_buffer_to_texture.rows_per_image);
    if (_range_overflows(command->u.copy_buffer_to_texture.src_offset, size, buffer->size))
        return _fail(DVZ_DRP2_VALIDATION_OUT_OF_RANGE, command_index);
    _mark_referenced(state, command->u.copy_buffer_to_texture.src_buffer_id);
    _mark_referenced(state, command->u.copy_buffer_to_texture.dst_texture_id);
    return _ok();
}



static DvzDrp2ValidationResult _validate_copy_texture_to_buffer(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    if (_open_pass(state) != NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2Object* encoder = _find_object(state, command->u.copy_texture_to_buffer.encoder_id);
    if (encoder == NULL || encoder->kind != DRP2_OBJECT_ENCODER || !encoder->open)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    Drp2Object* texture = _find_object(state, command->u.copy_texture_to_buffer.src_texture_id);
    if (texture == NULL || texture->kind != DRP2_OBJECT_TEXTURE)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if ((texture->usage & DVZ_DRP2_TEXTURE_USAGE_COPY_SRC) == 0)
        return _fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
    if (_texture_box_overflows(
            texture, 0, 0, 0, command->u.copy_texture_to_buffer.width,
            command->u.copy_texture_to_buffer.height, 1))
        return _fail(DVZ_DRP2_VALIDATION_OUT_OF_RANGE, command_index);
    if (_texture_layout_invalid(
            command->u.copy_texture_to_buffer.width, command->u.copy_texture_to_buffer.height, 1,
            command->u.copy_texture_to_buffer.bytes_per_row,
            command->u.copy_texture_to_buffer.rows_per_image))
        return _fail(DVZ_DRP2_VALIDATION_USAGE, command_index);

    Drp2Object* buffer = _find_object(state, command->u.copy_texture_to_buffer.dst_buffer_id);
    if (buffer == NULL || buffer->kind != DRP2_OBJECT_BUFFER)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if ((buffer->usage & DVZ_DRP2_BUFFER_USAGE_COPY_DST) == 0)
        return _fail(DVZ_DRP2_VALIDATION_USAGE, command_index);

    uint64_t required = _texture_layout_size(
        1, command->u.copy_texture_to_buffer.bytes_per_row,
        command->u.copy_texture_to_buffer.rows_per_image);
    if (_range_overflows(command->u.copy_texture_to_buffer.dst_offset, required, buffer->size))
        return _fail(DVZ_DRP2_VALIDATION_OUT_OF_RANGE, command_index);
    _mark_referenced(state, command->u.copy_texture_to_buffer.src_texture_id);
    _mark_referenced(state, command->u.copy_texture_to_buffer.dst_buffer_id);
    return _ok();
}



static DvzDrp2ValidationResult _validate_copy_texture_to_texture(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    if (_open_pass(state) != NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2Object* encoder = _find_object(state, command->u.copy_texture_to_texture.encoder_id);
    if (encoder == NULL || encoder->kind != DRP2_OBJECT_ENCODER || !encoder->open)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2Object* src = _find_object(state, command->u.copy_texture_to_texture.src_texture_id);
    Drp2Object* dst = _find_object(state, command->u.copy_texture_to_texture.dst_texture_id);
    if (src == NULL || src->kind != DRP2_OBJECT_TEXTURE || dst == NULL ||
        dst->kind != DRP2_OBJECT_TEXTURE)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if ((src->usage & DVZ_DRP2_TEXTURE_USAGE_COPY_SRC) == 0 ||
        (dst->usage & DVZ_DRP2_TEXTURE_USAGE_COPY_DST) == 0)
        return _fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
    if (command->u.copy_texture_to_texture.src_mip_level != 0 ||
        command->u.copy_texture_to_texture.dst_mip_level != 0)
        return _fail(DVZ_DRP2_VALIDATION_OUT_OF_RANGE, command_index);
    if (_texture_box_overflows(
            src, command->u.copy_texture_to_texture.src_origin_x,
            command->u.copy_texture_to_texture.src_origin_y,
            command->u.copy_texture_to_texture.src_origin_z,
            command->u.copy_texture_to_texture.width, command->u.copy_texture_to_texture.height,
            command->u.copy_texture_to_texture.depth))
        return _fail(DVZ_DRP2_VALIDATION_OUT_OF_RANGE, command_index);
    if (_texture_box_overflows(
            dst, command->u.copy_texture_to_texture.dst_origin_x,
            command->u.copy_texture_to_texture.dst_origin_y,
            command->u.copy_texture_to_texture.dst_origin_z,
            command->u.copy_texture_to_texture.width, command->u.copy_texture_to_texture.height,
            command->u.copy_texture_to_texture.depth))
        return _fail(DVZ_DRP2_VALIDATION_OUT_OF_RANGE, command_index);
    _mark_referenced(state, command->u.copy_texture_to_texture.src_texture_id);
    _mark_referenced(state, command->u.copy_texture_to_texture.dst_texture_id);
    return _ok();
}



static DvzDrp2ValidationResult _validate_finish_encoder(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    if (_open_pass(state) != NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (_find_any_object(state, command->u.finish_command_encoder.command_buffer_id) != NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2Object* encoder = _find_object(state, command->u.finish_command_encoder.encoder_id);
    if (encoder == NULL || encoder->kind != DRP2_OBJECT_ENCODER || !encoder->open)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    encoder->open = false;

    if (_add_object(
            state, command->u.finish_command_encoder.command_buffer_id,
            DRP2_OBJECT_COMMAND_BUFFER) == NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    return _ok();
}



static DvzDrp2ValidationResult _validate_queue_submit(
    Drp2RuntimeState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    Drp2Object* command_buffer = _find_object(state, command->u.queue_submit.command_buffer_id);
    if (command_buffer == NULL || command_buffer->kind != DRP2_OBJECT_COMMAND_BUFFER)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (command_buffer->submitted)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    command_buffer->submitted = true;

    if (!command->u.queue_submit.has_readback)
        return _ok();

    Drp2Object* buffer = _find_object(state, command->u.queue_submit.buffer_id);
    if (buffer == NULL || buffer->kind != DRP2_OBJECT_BUFFER)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if ((buffer->usage & DVZ_DRP2_BUFFER_USAGE_MAP_READ) == 0)
        return _fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
    if (_range_overflows(command->u.queue_submit.offset, command->u.queue_submit.size, buffer->size))
        return _fail(DVZ_DRP2_VALIDATION_OUT_OF_RANGE, command_index);
    return _ok();
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
            return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
        state->hello_seen = true;
        return _ok();

    case DVZ_DRP2_COMMAND_RENDERER_HELLO_REPLY:
        if (!state->hello_seen || state->reply_seen)
            return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
        state->reply_seen = true;
        return _ok();

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
        return _fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
    }
}


#if DVZ_DRP2_HAS_VKLITE
static int _base64_value(char c)
{
    if (c >= 'A' && c <= 'Z')
        return c - 'A';
    if (c >= 'a' && c <= 'z')
        return c - 'a' + 26;
    if (c >= '0' && c <= '9')
        return c - '0' + 52;
    if (c == '+')
        return 62;
    if (c == '/')
        return 63;
    return -1;
}


static bool _decode_base64_exact(const char* src, uint64_t expected_size, uint8_t** out)
{
    ANN(src);
    ANN(out);
    *out = NULL;
    if (expected_size == 0)
        return false;

    uint8_t* decoded = (uint8_t*)dvz_calloc(expected_size, sizeof(uint8_t));
    if (decoded == NULL)
        return false;

    uint32_t quad[4] = {0};
    uint32_t quad_count = 0;
    uint64_t written = 0;
    bool padded = false;
    for (uint32_t i = 0; src[i] != '\0'; i++)
    {
        char c = src[i];
        if (c == ' ' || c == '\n' || c == '\r' || c == '\t')
            continue;
        if (padded && c != '=')
        {
            dvz_free(decoded);
            return false;
        }

        if (c == '=')
        {
            quad[quad_count++] = 64;
            padded = true;
        }
        else
        {
            int value = _base64_value(c);
            if (value < 0)
            {
                dvz_free(decoded);
                return false;
            }
            quad[quad_count++] = (uint32_t)value;
        }

        if (quad_count != 4)
            continue;

        if (written < expected_size)
            decoded[written++] = (uint8_t)((quad[0] << 2) | (quad[1] >> 4));
        if (quad[2] != 64 && written < expected_size)
            decoded[written++] = (uint8_t)(((quad[1] & 0x0f) << 4) | (quad[2] >> 2));
        if (quad[3] != 64 && written < expected_size)
            decoded[written++] = (uint8_t)(((quad[2] & 0x03) << 6) | quad[3]);

        quad_count = 0;
    }

    if (quad_count != 0 || written != expected_size)
    {
        dvz_free(decoded);
        return false;
    }

    *out = decoded;
    return true;
}


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


static bool _vklite_ensure_capacity(Drp2VkliteState* state)
{
    ANN(state);
    if (state->objects == NULL || state->capacity == 0)
    {
        state->capacity = DVZ_DRP2_RUNTIME_INITIAL_OBJECT_CAPACITY;
        state->objects = (Drp2VkliteObject*)dvz_calloc(
            state->capacity, sizeof(Drp2VkliteObject));
        return state->objects != NULL;
    }
    if (state->count < state->capacity)
        return true;

    state->capacity *= 2;
    state->objects = (Drp2VkliteObject*)dvz_realloc(
        state->objects, state->capacity * sizeof(Drp2VkliteObject));
    return state->objects != NULL;
}


static Drp2VkliteObject* _vklite_find(Drp2VkliteState* state, uint64_t id)
{
    ANN(state);
    for (uint32_t i = 0; i < state->count; i++)
    {
        if (state->objects[i].id == id && !state->objects[i].destroyed)
            return &state->objects[i];
    }
    return NULL;
}


static Drp2VkliteObject* _vklite_add(
    Drp2VkliteState* state, uint64_t id, Drp2ObjectKind kind)
{
    ANN(state);
    if (!_vklite_ensure_capacity(state))
        return NULL;

    Drp2VkliteObject* object = &state->objects[state->count++];
    dvz_memset(object, sizeof(Drp2VkliteObject), 0, sizeof(Drp2VkliteObject));
    object->id = id;
    object->kind = kind;
    return object;
}


static void _vklite_destroy_object(Drp2VkliteObject* object)
{
    if (object == NULL || object->destroyed)
        return;
    if (object->buffer != NULL)
    {
        dvz_buffer_destroy(object->buffer);
        dvz_buffer_free(object->buffer);
        object->buffer = NULL;
    }
    if (object->images != NULL)
    {
        dvz_images_destroy(object->images);
        dvz_images_free(object->images);
        object->images = NULL;
    }
    object->destroyed = true;
}


static void _vklite_state_cleanup(Drp2VkliteState* state)
{
    if (state == NULL)
        return;
    for (uint32_t i = 0; i < state->count; i++)
    {
        _vklite_destroy_object(&state->objects[i]);
    }
    dvz_free(state->objects);
    state->objects = NULL;
    state->capacity = 0;
    state->count = 0;
    state->runtime = NULL;
}


static DvzDrp2ValidationResult _vklite_create_buffer(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);
    Drp2VkliteObject* object =
        _vklite_add(state, command->u.create_buffer.id, DRP2_OBJECT_BUFFER);
    if (object == NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    DvzBuffer* buffer = dvz_buffer_create_wrapper();
    if (buffer == NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    object->buffer = buffer;

    dvz_buffer(state->runtime->device, state->runtime->allocator, buffer);
    dvz_buffer_size(buffer, command->u.create_buffer.size);
    dvz_buffer_usage(buffer, _vklite_buffer_usage(command->u.create_buffer.usage));
    dvz_buffer_flags(buffer, _vklite_buffer_alloc_flags(command->u.create_buffer.usage));
    if (dvz_buffer_create(buffer) != 0)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    return _ok();
}


static DvzDrp2ValidationResult _vklite_create_texture(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);
    Drp2VkliteObject* object =
        _vklite_add(state, command->u.create_texture.id, DRP2_OBJECT_TEXTURE);
    if (object == NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    DvzImages* images = dvz_images_create_wrapper();
    if (images == NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    object->images = images;

    dvz_images(state->runtime->device, state->runtime->allocator, VK_IMAGE_TYPE_2D, 1, images);
    dvz_images_format(images, VK_FORMAT_R8G8B8A8_UNORM);
    dvz_images_size(images, command->u.create_texture.width, command->u.create_texture.height, 1);
    dvz_images_mip(images, 1);
    dvz_images_layers(images, 1);
    dvz_images_samples(images, VK_SAMPLE_COUNT_1_BIT);
    dvz_images_usage(images, _vklite_texture_usage(command->u.create_texture.usage));
    if (dvz_images_create(images) != 0)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    return _ok();
}


static DvzDrp2ValidationResult _vklite_write_buffer(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);
    Drp2VkliteObject* object = _vklite_find(state, command->u.write_buffer.buffer_id);
    if (object == NULL || object->buffer == NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    uint8_t* data = NULL;
    if (!_decode_base64_exact(
            command->u.write_buffer.data_base64, command->u.write_buffer.size, &data))
        return _fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, command_index);

    dvz_buffer_upload(
        object->buffer, command->u.write_buffer.offset, command->u.write_buffer.size, data);
    dvz_free(data);
    return _ok();
}


static DvzDrp2ValidationResult _vklite_destroy_backend_object(
    Drp2VkliteState* state, uint64_t id, Drp2ObjectKind kind, uint32_t command_index)
{
    ANN(state);
    Drp2VkliteObject* object = _vklite_find(state, id);
    if (object == NULL || object->kind != kind)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    _vklite_destroy_object(object);
    return _ok();
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
    else
    {
        _vklite_state_cleanup(runtime->vklite_state);
    }

    Drp2VkliteState* state = runtime->vklite_state;
    state->runtime = runtime;
    DvzDrp2ValidationResult result = _ok();

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
        case DVZ_DRP2_COMMAND_WRITE_BUFFER:
            result = _vklite_write_buffer(state, command, i);
            break;
        default:
            result = _ok();
            break;
        }

        if (!result.ok)
            break;
    }

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
 * Destroy a DRP2 runtime.
 *
 * @param runtime the runtime
 */
void dvz_drp2_runtime_destroy(DvzDrp2Runtime* runtime)
{
    if (runtime == NULL)
        return;
#if DVZ_DRP2_HAS_VKLITE
    _vklite_state_cleanup(runtime->vklite_state);
    dvz_free(runtime->vklite_state);
#endif
    dvz_free(runtime);
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
        return _fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, 0);

    Drp2RuntimeState state = {0};
    DvzDrp2ValidationResult result = _ok();

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
        return _fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, 0);

    DvzDrp2ValidationResult result = dvz_drp2_validate_stream(stream);
    if (!result.ok)
        return result;

    if (runtime->semantic_only)
        return result;

#if DVZ_DRP2_HAS_VKLITE
    return _vklite_execute(runtime, stream);
#else
    return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, 0);
#endif
}
