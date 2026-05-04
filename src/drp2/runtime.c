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
#include "shaderc/shaderc.h"
#include <volk.h>
#include "_dynload.h"
#endif

#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "_stream.h"
#include "datoviz/stream/frame_stream.h"

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
    Drp2RuntimeState* semantic_state;
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
    DvzImageViews* views;
    DvzShader* shader;
    DvzGraphics* graphics;
    DvzCompute* compute;
    DvzSlots* slots;
    DvzDescriptors* descriptors;
    DvzSampler* sampler;
    DvzCommands* commands;
    DvzRendering* rendering;
    VkCommandBuffer command_buffer;
    VkImageView image_view;
    VkImageLayout image_layout;
    uint64_t texture_id;
    uint64_t sampler_id;
    uint32_t width;
    uint32_t height;
    bool borrowed_slots;
    bool borrowed_commands;
    bool borrowed_frame_target;
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
static DvzCommands* _vklite_owned_commands_create(DvzDevice* device);
static void _vklite_owned_commands_destroy(DvzCommands* cmds);
static DvzCommands*
_vklite_borrowed_frame_commands_create(DvzDevice* device, VkCommandBuffer command_buffer);
static void _vklite_borrowed_frame_commands_free(DvzCommands* cmds);
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



/**
 * Multiply two unsigned 64-bit integers with overflow detection.
 *
 * @param a the first operand
 * @param b the second operand
 * @param out the product output
 * @return whether the multiplication would overflow
 */
static bool _mul_u64_overflows(uint64_t a, uint64_t b, uint64_t* out)
{
    ANN(out);
    if (a != 0 && b > UINT64_MAX / a)
        return true;
    *out = a * b;
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

    if (state->capacity > UINT32_MAX / 2)
        return false;
    uint32_t capacity = state->capacity * 2;
    uint64_t bytes = 0;
    if (_mul_u64_overflows(capacity, sizeof(Drp2Object), &bytes))
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
    bool storage_buffers = layout->storage_buffers;
    if (storage_buffers)
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
    object->storage_buffers = storage_buffers;
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

    const Drp2Object* encoder = _find_object(state, command->u.begin_render_pass.encoder_id);
    if (encoder == NULL || encoder->kind != DRP2_OBJECT_ENCODER || !encoder->open)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (!_has_object_kind(state, command->u.begin_render_pass.texture_id, DRP2_OBJECT_TEXTURE))
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2Object* pass = _add_object(state, command->u.begin_render_pass.id, DRP2_OBJECT_RENDER_PASS);
    if (pass == NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    encoder = _find_object(state, command->u.begin_render_pass.encoder_id);
    if (encoder == NULL || encoder->kind != DRP2_OBJECT_ENCODER || !encoder->open ||
        !_has_object_kind(state, command->u.begin_render_pass.texture_id, DRP2_OBJECT_TEXTURE))
    {
        pass->destroyed = true;
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }

    _mark_referenced(state, command->u.begin_render_pass.texture_id);
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

    const Drp2Object* encoder = _find_object(state, command->u.begin_compute_pass.encoder_id);
    if (encoder == NULL || encoder->kind != DRP2_OBJECT_ENCODER || !encoder->open)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2Object* pass =
        _add_object(state, command->u.begin_compute_pass.id, DRP2_OBJECT_COMPUTE_PASS);
    if (pass == NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    encoder = _find_object(state, command->u.begin_compute_pass.encoder_id);
    if (encoder == NULL || encoder->kind != DRP2_OBJECT_ENCODER || !encoder->open)
    {
        pass->destroyed = true;
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }
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

    const Drp2Object* encoder = _find_object(state, command->u.finish_command_encoder.encoder_id);
    if (encoder == NULL || encoder->kind != DRP2_OBJECT_ENCODER || !encoder->open)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2Object* command_buffer = _add_object(
        state, command->u.finish_command_encoder.command_buffer_id, DRP2_OBJECT_COMMAND_BUFFER);
    if (command_buffer == NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    command_buffer->encoder_id = command->u.finish_command_encoder.encoder_id;

    Drp2Object* mutable_encoder =
        _find_object(state, command->u.finish_command_encoder.encoder_id);
    if (mutable_encoder == NULL || mutable_encoder->kind != DRP2_OBJECT_ENCODER ||
        !mutable_encoder->open)
    {
        command_buffer->destroyed = true;
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }
    mutable_encoder->open = false;
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

    if (!command->u.queue_submit.has_readback)
    {
        _retire_submitted_work(state, command_buffer);
        return _ok();
    }

    Drp2Object* buffer = _find_object(state, command->u.queue_submit.buffer_id);
    if (buffer == NULL || buffer->kind != DRP2_OBJECT_BUFFER)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if ((buffer->usage & DVZ_DRP2_BUFFER_USAGE_MAP_READ) == 0)
        return _fail(DVZ_DRP2_VALIDATION_USAGE, command_index);
    if (_range_overflows(command->u.queue_submit.offset, command->u.queue_submit.size, buffer->size))
        return _fail(DVZ_DRP2_VALIDATION_OUT_OF_RANGE, command_index);
    _retire_submitted_work(state, command_buffer);
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
 * Validate a command stream against a runtime-persistent semantic state.
 *
 * @param runtime the DRP2 runtime
 * @param stream the command stream
 * @return the validation result
 */
static DvzDrp2ValidationResult
_runtime_validate_stream(DvzDrp2Runtime* runtime, const DvzDrp2CommandStream* stream)
{
    ANN(runtime);
    ANN(stream);
    if (runtime->semantic_state == NULL)
    {
        runtime->semantic_state = (Drp2RuntimeState*)dvz_calloc(1, sizeof(Drp2RuntimeState));
        ANN(runtime->semantic_state);
    }

    DvzDrp2ValidationResult result = _ok();
    for (uint32_t i = 0; i < stream->count; i++)
    {
        result = _validate_command(runtime->semantic_state, &stream->commands[i], i);
        if (!result.ok)
            break;
    }
    return result;
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

    if (state->capacity > UINT32_MAX / 2)
        return false;
    uint32_t capacity = state->capacity * 2;
    uint64_t bytes = 0;
    if (_mul_u64_overflows(capacity, sizeof(Drp2VkliteObject), &bytes))
        return false;

    Drp2VkliteObject* objects = (Drp2VkliteObject*)dvz_realloc(state->objects, bytes);
    if (objects == NULL)
        return false;

    state->capacity = capacity;
    state->objects = objects;
    return true;
}


static Drp2VkliteObject* _vklite_find(Drp2VkliteState* state, uint64_t id)
{
    ANN(state);
    for (uint32_t i = state->count; i > 0; i--)
    {
        Drp2VkliteObject* object = &state->objects[i - 1];
        if (object->id == id && !object->destroyed)
            return object;
    }
    return NULL;
}


static Drp2VkliteObject* _vklite_add(
    Drp2VkliteState* state, uint64_t id, Drp2ObjectKind kind)
{
    ANN(state);
    for (uint32_t i = 0; i < state->count; i++)
    {
        if (state->objects[i].destroyed)
        {
            Drp2VkliteObject* object = &state->objects[i];
            dvz_memset(object, sizeof(Drp2VkliteObject), 0, sizeof(Drp2VkliteObject));
            object->id = id;
            object->kind = kind;
            return object;
        }
    }

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
    if (object->commands != NULL)
    {
        if (!object->borrowed_commands)
            _vklite_owned_commands_destroy(object->commands);
        else
            _vklite_borrowed_frame_commands_free(object->commands);
        object->commands = NULL;
    }
    if (object->rendering != NULL)
    {
        dvz_rendering_free(object->rendering);
        object->rendering = NULL;
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
    if (object->graphics != NULL)
    {
        dvz_graphics_destroy(object->graphics);
        dvz_graphics_free(object->graphics);
        object->graphics = NULL;
    }
    if (object->compute != NULL)
    {
        dvz_compute_destroy(object->compute);
        dvz_compute_free(object->compute);
        object->compute = NULL;
    }
    if (object->slots != NULL && !object->borrowed_slots)
    {
        dvz_slots_destroy(object->slots);
        dvz_slots_free(object->slots);
        object->slots = NULL;
    }
    if (object->descriptors != NULL)
    {
        dvz_descriptors_free(object->descriptors);
        object->descriptors = NULL;
    }
    if (object->sampler != NULL)
    {
        dvz_sampler_destroy(object->sampler);
        dvz_sampler_free(object->sampler);
        object->sampler = NULL;
    }
    if (object->shader != NULL)
    {
        dvz_shader_destroy(object->shader);
        dvz_shader_free(object->shader);
        object->shader = NULL;
    }
    object->destroyed = true;
}



/**
 * Destroy a partially-created vklite object and return a validation failure.
 *
 * @param object vklite object slot to clean up
 * @param code validation failure code
 * @param command_index command index used for validation reporting
 * @return DRP2 validation failure result
 */
static DvzDrp2ValidationResult _vklite_fail_destroy_object(
    Drp2VkliteObject* object, DvzDrp2ValidationCode code, uint32_t command_index)
{
    _vklite_destroy_object(object);
    return _fail(code, command_index);
}



/**
 * Return the image view associated with a vklite texture object.
 *
 * @param object vklite texture object
 * @return Vulkan image view handle, or VK_NULL_HANDLE when unavailable
 */
static VkImageView _vklite_object_image_view(const Drp2VkliteObject* object)
{
    if (object == NULL)
        return VK_NULL_HANDLE;
    if (object->borrowed_frame_target && object->image_view != VK_NULL_HANDLE)
        return object->image_view;
    if (object->views == NULL)
        return VK_NULL_HANDLE;
    return dvz_image_views_handle(object->views, 0);
}



static void _vklite_state_cleanup(Drp2VkliteState* state)
{
    if (state == NULL)
        return;
    for (uint32_t i = state->count; i > 0; i--)
    {
        _vklite_destroy_object(&state->objects[i - 1]);
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
        return _vklite_fail_destroy_object(
            object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    object->images = images;

    dvz_images(state->runtime->device, state->runtime->allocator, VK_IMAGE_TYPE_2D, 1, images);
    dvz_images_format(images, VK_FORMAT_R8G8B8A8_UNORM);
    dvz_images_size(images, command->u.create_texture.width, command->u.create_texture.height, 1);
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
    return _ok();
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

    DvzImages* images = dvz_images_create_wrapper();
    if (images == NULL)
        return false;
    dvz_images_wrap(runtime->device, runtime->allocator, VK_IMAGE_TYPE_2D, frame->image, images);
    dvz_images_format(images, VK_FORMAT_R8G8B8A8_UNORM);
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
    object->image_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    object->command_buffer = frame->command_buffer;
    object->image_view = frame->image_view;
    object->width = frame->extent.width;
    object->height = frame->extent.height;
    object->borrowed_frame_target = true;
    object->destroyed = false;
    return true;
}


/* Lazy-loaded shaderc function-pointer table. Populated once on first GLSL compile call. */
typedef struct
{
    shaderc_compiler_t (*compiler_initialize)(void);
    void (*compiler_release)(shaderc_compiler_t);
    shaderc_compile_options_t (*compile_options_initialize)(void);
    void (*compile_options_release)(shaderc_compile_options_t);
    void (*compile_options_set_source_language)(
        shaderc_compile_options_t, shaderc_source_language);
    void (*compile_options_set_target_env)(
        shaderc_compile_options_t, shaderc_target_env, uint32_t);
    void (*compile_options_set_target_spirv)(
        shaderc_compile_options_t, shaderc_spirv_version);
    shaderc_compilation_result_t (*compile_into_spv)(
        shaderc_compiler_t, const char*, size_t, shaderc_shader_kind, const char*, const char*,
        const shaderc_compile_options_t);
    shaderc_compilation_status (*result_get_compilation_status)(shaderc_compilation_result_t);
    const char* (*result_get_error_message)(shaderc_compilation_result_t);
    const char* (*result_get_bytes)(shaderc_compilation_result_t);
    size_t (*result_get_length)(shaderc_compilation_result_t);
    void (*result_release)(shaderc_compilation_result_t);
} ShadercSyms;

static ShadercSyms g_shaderc = {0};
static bool g_shaderc_loaded = false;
static bool g_shaderc_available = false;

static bool _shaderc_load(void)
{
    if (g_shaderc_loaded)
        return g_shaderc_available;
    g_shaderc_loaded = true;

#ifndef DVZ_SHADERC_LIB_PATH
#define DVZ_SHADERC_LIB_PATH "libshaderc_shared.so.1"
#endif
    DvzDynLib lib = dvz_dynlib_open(DVZ_SHADERC_LIB_PATH);
    if (lib == NULL)
    {
        log_error("shaderc not available: could not load " DVZ_SHADERC_LIB_PATH);
        return false;
    }

    /* POSIX allows void* <-> function pointer via memcpy to avoid -Wpedantic warnings. */
#define _SC_SYM(field, name)                                                                      \
    {                                                                                             \
        void* _p = dvz_dynlib_sym(lib, name);                                                     \
        if (_p == NULL)                                                                           \
        {                                                                                         \
            log_error("shaderc: symbol '%s' not found", name);                                    \
            dvz_dynlib_close(lib);                                                                \
            return false;                                                                         \
        }                                                                                         \
        memcpy(&g_shaderc.field, &_p, sizeof(_p));                                                \
    }

    _SC_SYM(compiler_initialize, "shaderc_compiler_initialize")
    _SC_SYM(compiler_release, "shaderc_compiler_release")
    _SC_SYM(compile_options_initialize, "shaderc_compile_options_initialize")
    _SC_SYM(compile_options_release, "shaderc_compile_options_release")
    _SC_SYM(compile_options_set_source_language, "shaderc_compile_options_set_source_language")
    _SC_SYM(compile_options_set_target_env, "shaderc_compile_options_set_target_env")
    _SC_SYM(compile_options_set_target_spirv, "shaderc_compile_options_set_target_spirv")
    _SC_SYM(compile_into_spv, "shaderc_compile_into_spv")
    _SC_SYM(result_get_compilation_status, "shaderc_result_get_compilation_status")
    _SC_SYM(result_get_error_message, "shaderc_result_get_error_message")
    _SC_SYM(result_get_bytes, "shaderc_result_get_bytes")
    _SC_SYM(result_get_length, "shaderc_result_get_length")
    _SC_SYM(result_release, "shaderc_result_release")
#undef _SC_SYM

    /* Keep the library resident — we do not dlclose it. The handle is intentionally retained
       so the symbols remain valid for the process lifetime without re-opening on each call. */
    g_shaderc_available = true;
    return true;
}


/**
 * Return the shaderc GLSL shader kind for a DRP2 shader stage string.
 *
 * @param stage shader stage string
 * @return shaderc shader kind, or infer-from-source when the stage is unknown
 */
static shaderc_shader_kind _vklite_shader_kind(const char* stage)
{
    ANN(stage);
    if (strcmp(stage, "VERTEX") == 0 || strcmp(stage, "vertex") == 0)
        return shaderc_glsl_vertex_shader;
    if (strcmp(stage, "FRAGMENT") == 0 || strcmp(stage, "fragment") == 0)
        return shaderc_glsl_fragment_shader;
    if (strcmp(stage, "COMPUTE") == 0 || strcmp(stage, "compute") == 0)
        return shaderc_glsl_compute_shader;
    return shaderc_glsl_infer_from_source;
}


/**
 * Return the DRP2 runtime object kind for a shader stage string.
 *
 * @param stage shader stage string
 * @return shader object kind, or DRP2_OBJECT_NONE when the stage is unknown
 */
static Drp2ObjectKind _vklite_shader_object_kind(const char* stage)
{
    ANN(stage);
    if (strcmp(stage, "VERTEX") == 0 || strcmp(stage, "vertex") == 0)
        return DRP2_OBJECT_SHADER_VERTEX;
    if (strcmp(stage, "FRAGMENT") == 0 || strcmp(stage, "fragment") == 0)
        return DRP2_OBJECT_SHADER_FRAGMENT;
    if (strcmp(stage, "COMPUTE") == 0 || strcmp(stage, "compute") == 0)
        return DRP2_OBJECT_SHADER_COMPUTE;
    return DRP2_OBJECT_NONE;
}


/**
 * Compile GLSL source code into SPIR-V with shaderc.
 *
 * @param stage shader stage string
 * @param code GLSL source code
 * @param spv output pointer to aligned SPIR-V words, freed by the caller
 * @param spv_size output SPIR-V byte size
 * @return true when compilation succeeds, false otherwise
 */
static bool _vklite_compile_glsl(
    const char* stage, const char* code, uint32_t** spv, uint64_t* spv_size)
{
    ANN(stage);
    ANN(code);
    ANN(spv);
    ANN(spv_size);
    *spv = NULL;
    *spv_size = 0;

    if (!_shaderc_load())
        return false;

    shaderc_compiler_t compiler = g_shaderc.compiler_initialize();
    if (compiler == NULL)
        return false;
    shaderc_compile_options_t options = g_shaderc.compile_options_initialize();
    if (options == NULL)
    {
        g_shaderc.compiler_release(compiler);
        return false;
    }

    g_shaderc.compile_options_set_source_language(options, shaderc_source_language_glsl);
    g_shaderc.compile_options_set_target_env(
        options, shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
    g_shaderc.compile_options_set_target_spirv(options, shaderc_spirv_version_1_6);

    shaderc_shader_kind kind = _vklite_shader_kind(stage);
    shaderc_compilation_result_t result = g_shaderc.compile_into_spv(
        compiler, code, strlen(code), kind, "drp2_scene_fixture.glsl", "main", options);

    g_shaderc.compile_options_release(options);
    g_shaderc.compiler_release(compiler);

    if (result == NULL)
        return false;
    if (g_shaderc.result_get_compilation_status(result) != shaderc_compilation_status_success)
    {
        log_error("GLSL compilation failed: %s", g_shaderc.result_get_error_message(result));
        g_shaderc.result_release(result);
        return false;
    }

    const char* bytes = g_shaderc.result_get_bytes(result);
    uint64_t size = (uint64_t)g_shaderc.result_get_length(result);
    if (bytes == NULL || size == 0 || size % sizeof(uint32_t) != 0)
    {
        g_shaderc.result_release(result);
        return false;
    }

    uint32_t* out = (uint32_t*)dvz_calloc((size_t)(size / sizeof(uint32_t)), sizeof(uint32_t));
    if (out == NULL)
    {
        g_shaderc.result_release(result);
        return false;
    }
    dvz_memcpy(out, (size_t)size, bytes, (size_t)size);
    g_shaderc.result_release(result);

    *spv = out;
    *spv_size = size;
    return true;
}


/**
 * Create a vklite shader module object from a DRP2 CreateShaderModule command.
 *
 * @param state vklite runtime state
 * @param command DRP2 CreateShaderModule command
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
static DvzDrp2ValidationResult _vklite_create_shader_module(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);
    if (strcmp(command->u.create_shader_module.format, "glsl") != 0)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, command_index);

    Drp2ObjectKind kind = _vklite_shader_object_kind(command->u.create_shader_module.stage);
    if (kind == DRP2_OBJECT_NONE)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, command_index);

    uint32_t* spv = NULL;
    uint64_t spv_size = 0;
    if (!_vklite_compile_glsl(
            command->u.create_shader_module.stage, command->u.create_shader_module.code, &spv,
            &spv_size))
    {
        return _fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, command_index);
    }

    Drp2VkliteObject* object = _vklite_add(state, command->u.create_shader_module.id, kind);
    if (object == NULL)
    {
        dvz_free(spv);
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }

    DvzShader* shader = dvz_shader_create_wrapper();
    if (shader == NULL)
    {
        dvz_free(spv);
        return _vklite_fail_destroy_object(
            object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }
    object->shader = shader;

    int out = dvz_shader(state->runtime->device, spv_size, spv, shader);
    dvz_free(spv);
    if (out != 0)
        return _vklite_fail_destroy_object(
            object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    return _ok();
}


/**
 * Create an empty pipeline layout for vklite graphics pipelines without bind groups.
 *
 * @param state vklite runtime state
 * @param command_index command index used for validation reporting
 * @param slots output slots wrapper
 * @return DRP2 validation result
 */
static DvzDrp2ValidationResult _vklite_create_empty_slots(
    Drp2VkliteState* state, uint32_t command_index, DvzSlots** slots)
{
    ANN(state);
    ANN(slots);
    *slots = NULL;

    DvzSlots* out = dvz_slots_create_wrapper();
    if (out == NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    dvz_slots(state->runtime->device, out);
    if (dvz_slots_create(out) != 0)
    {
        dvz_slots_destroy(out);
        dvz_slots_free(out);
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }
    *slots = out;
    return _ok();
}


/**
 * Create a vklite sampler from a DRP2 CreateSampler command.
 *
 * @param state vklite runtime state
 * @param command DRP2 CreateSampler command
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
static DvzDrp2ValidationResult
_vklite_create_sampler(Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    Drp2VkliteObject* object = _vklite_add(state, command->u.create_sampler.id, DRP2_OBJECT_SAMPLER);
    if (object == NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    DvzSampler* sampler = dvz_sampler_create_wrapper();
    if (sampler == NULL)
        return _vklite_fail_destroy_object(
            object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    object->sampler = sampler;

    dvz_sampler(state->runtime->device, sampler);
    dvz_sampler_min_filter(sampler, VK_FILTER_LINEAR);
    dvz_sampler_mag_filter(sampler, VK_FILTER_LINEAR);
    dvz_sampler_address_mode(sampler, DVZ_SAMPLER_AXIS_U, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    dvz_sampler_address_mode(sampler, DVZ_SAMPLER_AXIS_V, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    dvz_sampler_address_mode(sampler, DVZ_SAMPLER_AXIS_W, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    if (dvz_sampler_create(sampler) != 0)
        return _vklite_fail_destroy_object(
            object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    return _ok();
}


/**
 * Create a vklite bind-group layout from a DRP2 CreateBindGroupLayout command.
 *
 * @param state vklite runtime state
 * @param command DRP2 CreateBindGroupLayout command
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
static DvzDrp2ValidationResult _vklite_create_bind_group_layout(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    Drp2VkliteObject* object =
        _vklite_add(state, command->u.create_bind_group_layout.id, DRP2_OBJECT_BIND_GROUP_LAYOUT);
    if (object == NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    DvzSlots* slots = dvz_slots_create_wrapper();
    if (slots == NULL)
        return _vklite_fail_destroy_object(
            object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    object->slots = slots;

    dvz_slots(state->runtime->device, slots);
    if (command->u.create_bind_group_layout.storage_buffers)
    {
        dvz_slots_binding(
            slots, 0, 0, 1, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        dvz_slots_binding(
            slots, 0, 1, 1, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    }
    else
    {
        dvz_slots_binding(
            slots, 0, 0, 1, VK_SHADER_STAGE_FRAGMENT_BIT,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    }
    if (dvz_slots_create(slots) != 0)
        return _vklite_fail_destroy_object(
            object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    return _ok();
}


/**
 * Create vklite descriptors from a DRP2 CreateBindGroup command.
 *
 * @param state vklite runtime state
 * @param command DRP2 CreateBindGroup command
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
static DvzDrp2ValidationResult _vklite_create_bind_group(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    Drp2VkliteObject* layout = _vklite_find(state, command->u.create_bind_group.bind_group_layout_id);
    if (layout == NULL || layout->kind != DRP2_OBJECT_BIND_GROUP_LAYOUT || layout->slots == NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2VkliteObject* object =
        _vklite_add(state, command->u.create_bind_group.id, DRP2_OBJECT_BIND_GROUP);
    if (object == NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    layout = _vklite_find(state, command->u.create_bind_group.bind_group_layout_id);
    if (layout == NULL || layout->kind != DRP2_OBJECT_BIND_GROUP_LAYOUT || layout->slots == NULL)
        return _vklite_fail_destroy_object(
            object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    DvzDescriptors* descriptors = dvz_descriptors_create_wrapper();
    if (descriptors == NULL)
        return _vklite_fail_destroy_object(
            object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    object->descriptors = descriptors;
    object->texture_id = command->u.create_bind_group.texture_id;
    object->sampler_id = command->u.create_bind_group.sampler_id;
    dvz_descriptors(layout->slots, descriptors);

    if (command->u.create_bind_group.buffer_size != 0)
    {
        Drp2VkliteObject* buffer0 = _vklite_find(state, command->u.create_bind_group.buffer0_id);
        Drp2VkliteObject* buffer1 = _vklite_find(state, command->u.create_bind_group.buffer1_id);
        if (buffer0 == NULL || buffer0->buffer == NULL || buffer1 == NULL ||
            buffer1->buffer == NULL)
            return _vklite_fail_destroy_object(
                object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
        dvz_descriptors_buffer(
            descriptors, 0, 0, 0, dvz_buffer_handle(buffer0->buffer), 0,
            command->u.create_bind_group.buffer_size);
        dvz_descriptors_buffer(
            descriptors, 0, 1, 0, dvz_buffer_handle(buffer1->buffer), 0,
            command->u.create_bind_group.buffer_size);
    }
    else
    {
        Drp2VkliteObject* texture = _vklite_find(state, command->u.create_bind_group.texture_id);
        Drp2VkliteObject* sampler = _vklite_find(state, command->u.create_bind_group.sampler_id);
        VkImageView texture_view = _vklite_object_image_view(texture);
        if (texture_view == VK_NULL_HANDLE || sampler == NULL || sampler->sampler == NULL)
            return _vklite_fail_destroy_object(
                object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
        dvz_descriptors_image(
            descriptors, 0, 0, 0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, texture_view,
            dvz_sampler_handle(sampler->sampler));
    }
    return _ok();
}


/**
 * Create a vklite graphics pipeline from a DRP2 CreateRenderPipeline command.
 *
 * @param state vklite runtime state
 * @param command DRP2 CreateRenderPipeline command
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
static DvzDrp2ValidationResult _vklite_create_render_pipeline(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    Drp2VkliteObject* vertex =
        _vklite_find(state, command->u.create_render_pipeline.vertex_shader_module_id);
    Drp2VkliteObject* fragment =
        _vklite_find(state, command->u.create_render_pipeline.fragment_shader_module_id);
    if (vertex == NULL || vertex->kind != DRP2_OBJECT_SHADER_VERTEX || vertex->shader == NULL ||
        fragment == NULL || fragment->kind != DRP2_OBJECT_SHADER_FRAGMENT ||
        fragment->shader == NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2VkliteObject* object =
        _vklite_add(state, command->u.create_render_pipeline.id, DRP2_OBJECT_RENDER_PIPELINE);
    if (object == NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    vertex = _vklite_find(state, command->u.create_render_pipeline.vertex_shader_module_id);
    fragment = _vklite_find(state, command->u.create_render_pipeline.fragment_shader_module_id);
    if (vertex == NULL || vertex->kind != DRP2_OBJECT_SHADER_VERTEX || vertex->shader == NULL ||
        fragment == NULL || fragment->kind != DRP2_OBJECT_SHADER_FRAGMENT ||
        fragment->shader == NULL)
        return _vklite_fail_destroy_object(
            object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    if (command->u.create_render_pipeline.bind_group_layout_id != 0)
    {
        Drp2VkliteObject* layout =
            _vklite_find(state, command->u.create_render_pipeline.bind_group_layout_id);
        if (layout == NULL || layout->kind != DRP2_OBJECT_BIND_GROUP_LAYOUT ||
            layout->slots == NULL)
            return _vklite_fail_destroy_object(
                object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
        object->slots = layout->slots;
        object->borrowed_slots = true;
    }
    else
    {
        DvzDrp2ValidationResult result =
            _vklite_create_empty_slots(state, command_index, &object->slots);
        if (!result.ok)
        {
            _vklite_destroy_object(object);
            return result;
        }
    }

    DvzGraphics* graphics = dvz_graphics_create_wrapper();
    if (graphics == NULL)
        return _vklite_fail_destroy_object(
            object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    object->graphics = graphics;

    dvz_graphics(state->runtime->device, graphics);
    dvz_graphics_layout(graphics, dvz_slots_handle(object->slots));
    dvz_graphics_shader(graphics, VK_SHADER_STAGE_VERTEX_BIT, dvz_shader_handle(vertex->shader));
    dvz_graphics_shader(
        graphics, VK_SHADER_STAGE_FRAGMENT_BIT, dvz_shader_handle(fragment->shader));
    dvz_graphics_attachment_color(graphics, 0, VK_FORMAT_R8G8B8A8_UNORM);
    dvz_graphics_color_write_mask(
        graphics, 0,
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT);
    dvz_graphics_primitive(graphics, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, DVZ_GRAPHICS_FLAGS_FIXED);
    dvz_graphics_viewport(graphics, 0, 0, 1, 1, 0, 1, DVZ_GRAPHICS_FLAGS_DYNAMIC);
    dvz_graphics_scissor(graphics, 0, 0, 1, 1, DVZ_GRAPHICS_FLAGS_DYNAMIC);

    if (dvz_graphics_create(graphics) != 0)
        return _vklite_fail_destroy_object(
            object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    return _ok();
}


/**
 * Create a vklite compute pipeline from a DRP2 CreateComputePipeline command.
 *
 * @param state vklite runtime state
 * @param command DRP2 CreateComputePipeline command
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
static DvzDrp2ValidationResult _vklite_create_compute_pipeline(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    Drp2VkliteObject* shader =
        _vklite_find(state, command->u.create_compute_pipeline.compute_shader_module_id);
    if (shader == NULL || shader->kind != DRP2_OBJECT_SHADER_COMPUTE || shader->shader == NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2VkliteObject* object =
        _vklite_add(state, command->u.create_compute_pipeline.id, DRP2_OBJECT_COMPUTE_PIPELINE);
    if (object == NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    shader = _vklite_find(state, command->u.create_compute_pipeline.compute_shader_module_id);
    if (shader == NULL || shader->kind != DRP2_OBJECT_SHADER_COMPUTE || shader->shader == NULL)
        return _vklite_fail_destroy_object(
            object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    if (command->u.create_compute_pipeline.bind_group_layout_id != 0)
    {
        Drp2VkliteObject* layout =
            _vklite_find(state, command->u.create_compute_pipeline.bind_group_layout_id);
        if (layout == NULL || layout->kind != DRP2_OBJECT_BIND_GROUP_LAYOUT ||
            layout->slots == NULL)
            return _vklite_fail_destroy_object(
                object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
        object->slots = layout->slots;
        object->borrowed_slots = true;
    }
    else
    {
        DvzDrp2ValidationResult result =
            _vklite_create_empty_slots(state, command_index, &object->slots);
        if (!result.ok)
        {
            _vklite_destroy_object(object);
            return result;
        }
    }

    DvzCompute* compute = dvz_compute_create_wrapper();
    if (compute == NULL)
        return _vklite_fail_destroy_object(
            object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    object->compute = compute;

    dvz_compute(state->runtime->device, compute);
    dvz_compute_shader(compute, dvz_shader_handle(shader->shader));
    dvz_compute_layout(compute, dvz_slots_handle(object->slots));
    if (dvz_compute_create(compute) != 0)
        return _vklite_fail_destroy_object(
            object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    return _ok();
}


/**
 * Create an owned command-buffer wrapper for immediate DRP2 runtime work.
 *
 * @param device the borrowed Vulkan device wrapper
 * @return owned command-buffer wrapper, or NULL on failure
 */
static DvzCommands* _vklite_owned_commands_create(DvzDevice* device)
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
static void _vklite_owned_commands_destroy(DvzCommands* cmds)
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
static DvzCommands*
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
static void _vklite_borrowed_frame_commands_free(DvzCommands* cmds)
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
static DvzDrp2ValidationResult
_vklite_owned_commands_end_submit(DvzCommands* cmds, uint32_t command_index)
{
    ANN(cmds);
    if (dvz_cmd_end_result(cmds) != 0)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (dvz_cmd_submit_result(cmds) != 0)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    return _ok();
}


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


static void _vklite_transition_image(
    DvzCommands* cmds, Drp2VkliteObject* object, VkImageLayout layout,
    VkPipelineStageFlags2 dst_stage, VkAccessFlags2 dst_access)
{
    ANN(cmds);
    ANN(object);
    ANN(object->images);
    if (object->image_layout == layout)
        return;

    VkPipelineStageFlags2 src_stage = VK_PIPELINE_STAGE_2_NONE;
    VkAccessFlags2 src_access = 0;
    if (object->image_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        src_stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        src_access = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    }
    else if (object->image_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
    {
        src_stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        src_access = VK_ACCESS_2_TRANSFER_READ_BIT;
    }
    else if (object->image_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
    {
        src_stage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        src_access = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    }
    else if (object->image_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        src_stage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
        src_access = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
    }

    DvzBarriers barriers = {0};
    dvz_barriers(&barriers);
    DvzBarrierImage* bimg = dvz_barriers_image(&barriers, dvz_image_handle(object->images, 0));
    ANN(bimg);
    dvz_barrier_image_stage(bimg, src_stage, dst_stage);
    dvz_barrier_image_access(bimg, src_access, dst_access);
    dvz_barrier_image_layout(bimg, object->image_layout, layout);
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


static DvzDrp2ValidationResult _vklite_write_texture(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);
    Drp2VkliteObject* texture = _vklite_find(state, command->u.write_texture.texture_id);
    if (texture == NULL || texture->images == NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    uint64_t size = _texture_layout_size(
        command->u.write_texture.depth, command->u.write_texture.bytes_per_row,
        command->u.write_texture.rows_per_image);
    uint8_t* data = NULL;
    if (!_decode_base64_exact(command->u.write_texture.data_base64, size, &data))
        return _fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, command_index);

    DvzBuffer* staging = NULL;
    if (!_vklite_create_staging_buffer(state, size, &staging, VK_BUFFER_USAGE_TRANSFER_SRC_BIT))
    {
        dvz_free(data);
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }
    dvz_buffer_upload(staging, 0, size, data);
    dvz_free(data);

    DvzCommands* cmds = _vklite_owned_commands_create(state->runtime->device);
    if (cmds == NULL)
    {
        dvz_buffer_destroy(staging);
        dvz_buffer_free(staging);
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
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
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }
    _vklite_transition_image(
        cmds, texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        VK_ACCESS_2_TRANSFER_WRITE_BIT);
    dvz_cmd_copy_buffer_to_image(
        cmds, dvz_buffer_handle(staging), 0, dvz_image_handle(texture->images, 0),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &region);
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
    return _ok();
}


static DvzDrp2ValidationResult _vklite_copy_buffer_to_buffer(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);
    Drp2VkliteObject* src = _vklite_find(state, command->u.copy_buffer_to_buffer.src_buffer_id);
    Drp2VkliteObject* dst = _vklite_find(state, command->u.copy_buffer_to_buffer.dst_buffer_id);
    if (src == NULL || src->buffer == NULL || dst == NULL || dst->buffer == NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    DvzCommands* cmds = _vklite_owned_commands_create(state->runtime->device);
    if (cmds == NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    VkBufferCopy region = {0};
    region.srcOffset = command->u.copy_buffer_to_buffer.src_offset;
    region.dstOffset = command->u.copy_buffer_to_buffer.dst_offset;
    region.size = command->u.copy_buffer_to_buffer.size;

    if (dvz_cmd_begin_result(cmds) != 0)
    {
        _vklite_owned_commands_destroy(cmds);
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
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
    return _ok();
}


static DvzDrp2ValidationResult _vklite_copy_buffer_to_texture(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);
    Drp2VkliteObject* src = _vklite_find(state, command->u.copy_buffer_to_texture.src_buffer_id);
    Drp2VkliteObject* dst = _vklite_find(state, command->u.copy_buffer_to_texture.dst_texture_id);
    if (src == NULL || src->buffer == NULL || dst == NULL || dst->images == NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    DvzCommands* cmds = _vklite_owned_commands_create(state->runtime->device);
    if (cmds == NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

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
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }
    _vklite_transition_image(
        cmds, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        VK_ACCESS_2_TRANSFER_WRITE_BIT);
    dvz_cmd_copy_buffer_to_image(
        cmds, dvz_buffer_handle(src->buffer), command->u.copy_buffer_to_texture.src_offset,
        dvz_image_handle(dst->images, 0), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &region);
    DvzDrp2ValidationResult result = _vklite_owned_commands_end_submit(cmds, command_index);
    if (!result.ok)
    {
        _vklite_owned_commands_destroy(cmds);
        return result;
    }

    _vklite_owned_commands_destroy(cmds);
    return _ok();
}


static DvzDrp2ValidationResult _vklite_copy_texture_to_buffer(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);
    Drp2VkliteObject* src = _vklite_find(state, command->u.copy_texture_to_buffer.src_texture_id);
    Drp2VkliteObject* dst = _vklite_find(state, command->u.copy_texture_to_buffer.dst_buffer_id);
    if (src == NULL || src->images == NULL || dst == NULL || dst->buffer == NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    DvzCommands* cmds = _vklite_owned_commands_create(state->runtime->device);
    if (cmds == NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    DvzImageRegion region = {0};
    _vklite_region(
        &region, command->u.copy_texture_to_buffer.width,
        command->u.copy_texture_to_buffer.height, 1,
        command->u.copy_texture_to_buffer.bytes_per_row,
        command->u.copy_texture_to_buffer.rows_per_image);

    if (dvz_cmd_begin_result(cmds) != 0)
    {
        _vklite_owned_commands_destroy(cmds);
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }
    _vklite_transition_image(
        cmds, src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        VK_ACCESS_2_TRANSFER_READ_BIT);
    dvz_cmd_copy_image_to_buffer(
        cmds, dvz_image_handle(src->images, 0), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, &region,
        dvz_buffer_handle(dst->buffer), command->u.copy_texture_to_buffer.dst_offset);
    DvzDrp2ValidationResult result = _vklite_owned_commands_end_submit(cmds, command_index);
    if (!result.ok)
    {
        _vklite_owned_commands_destroy(cmds);
        return result;
    }

    _vklite_owned_commands_destroy(cmds);
    return _ok();
}


static DvzDrp2ValidationResult _vklite_copy_texture_to_texture(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);
    Drp2VkliteObject* src = _vklite_find(state, command->u.copy_texture_to_texture.src_texture_id);
    Drp2VkliteObject* dst = _vklite_find(state, command->u.copy_texture_to_texture.dst_texture_id);
    if (src == NULL || src->images == NULL || dst == NULL || dst->images == NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    DvzCommands* cmds = _vklite_owned_commands_create(state->runtime->device);
    if (cmds == NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    DvzImageCopy* copy = dvz_image_copy_create();
    if (copy == NULL)
    {
        _vklite_owned_commands_destroy(cmds);
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }
    dvz_cmd_copy_source(
        copy, dvz_image_handle(src->images, 0), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        (int32_t)command->u.copy_texture_to_texture.src_origin_x,
        (int32_t)command->u.copy_texture_to_texture.src_origin_y,
        (int32_t)command->u.copy_texture_to_texture.src_origin_z,
        command->u.copy_texture_to_texture.width, command->u.copy_texture_to_texture.height,
        command->u.copy_texture_to_texture.depth);
    dvz_cmd_copy_destination(
        copy, dvz_image_handle(dst->images, 0), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        (int32_t)command->u.copy_texture_to_texture.dst_origin_x,
        (int32_t)command->u.copy_texture_to_texture.dst_origin_y,
        (int32_t)command->u.copy_texture_to_texture.dst_origin_z);

    if (dvz_cmd_begin_result(cmds) != 0)
    {
        dvz_image_copy_free(copy);
        _vklite_owned_commands_destroy(cmds);
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }
    _vklite_transition_image(
        cmds, src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        VK_ACCESS_2_TRANSFER_READ_BIT);
    _vklite_transition_image(
        cmds, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        VK_ACCESS_2_TRANSFER_WRITE_BIT);
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
    return _ok();
}


/**
 * Begin a vklite dynamic-rendering pass for a DRP2 BeginRenderPass command.
 *
 * @param state vklite runtime state
 * @param command DRP2 BeginRenderPass command
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
static DvzDrp2ValidationResult _vklite_begin_render_pass(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);
    Drp2VkliteObject* target = _vklite_find(state, command->u.begin_render_pass.texture_id);
    VkImageView target_view = _vklite_object_image_view(target);
    if (target == NULL || target->images == NULL || target_view == VK_NULL_HANDLE)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2VkliteObject* pass =
        _vklite_add(state, command->u.begin_render_pass.id, DRP2_OBJECT_RENDER_PASS);
    if (pass == NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    target = _vklite_find(state, command->u.begin_render_pass.texture_id);
    target_view = _vklite_object_image_view(target);
    if (target == NULL || target->images == NULL || target_view == VK_NULL_HANDLE)
        return _vklite_fail_destroy_object(
            pass, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    DvzCommands* cmds = NULL;
    if (target->borrowed_frame_target)
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

    DvzRendering* rendering = dvz_rendering_create_wrapper();
    if (rendering == NULL)
        return _vklite_fail_destroy_object(
            pass, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    pass->rendering = rendering;

    VkClearValue clear = {0};
    dvz_cmd_rendering_default(
        cmds, target_view, target->width, target->height, clear, rendering);

    if (!target->borrowed_frame_target && dvz_cmd_begin_result(cmds) != 0)
        return _vklite_fail_destroy_object(
            pass, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    for (uint32_t i = 0; i < state->count; i++)
    {
        Drp2VkliteObject* object = &state->objects[i];
        if (object->kind == DRP2_OBJECT_TEXTURE && object != target && object->views != NULL &&
            !object->borrowed_frame_target)
        {
            _vklite_transition_image(
                cmds, object, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT);
        }
    }
    if (!target->borrowed_frame_target)
    {
        _vklite_transition_image(
            cmds, target, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);
    }
    dvz_cmd_rendering_begin(cmds, rendering);
    return _ok();
}


/**
 * Begin a vklite command buffer for a DRP2 compute pass.
 *
 * @param state vklite runtime state
 * @param command DRP2 BeginComputePass command
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
static DvzDrp2ValidationResult _vklite_begin_compute_pass(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    Drp2VkliteObject* pass =
        _vklite_add(state, command->u.begin_compute_pass.id, DRP2_OBJECT_COMPUTE_PASS);
    if (pass == NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    DvzCommands* cmds = _vklite_owned_commands_create(state->runtime->device);
    if (cmds == NULL)
        return _vklite_fail_destroy_object(
            pass, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    pass->commands = cmds;

    if (dvz_cmd_begin_result(cmds) != 0)
        return _vklite_fail_destroy_object(
            pass, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    return _ok();
}


/**
 * Bind a vklite pipeline within a DRP2 render or compute pass.
 *
 * @param state vklite runtime state
 * @param command DRP2 SetPipeline command
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
static DvzDrp2ValidationResult _vklite_set_pipeline(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);
    Drp2VkliteObject* pass = _vklite_find(state, command->u.set_pipeline.pass_id);
    Drp2VkliteObject* pipeline = _vklite_find(state, command->u.set_pipeline.pipeline_id);
    if (pass == NULL || pass->commands == NULL || pipeline == NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    if (pass->kind == DRP2_OBJECT_RENDER_PASS && pipeline->kind == DRP2_OBJECT_RENDER_PIPELINE &&
        pipeline->graphics != NULL)
    {
        dvz_graphics_viewport(
            pipeline->graphics, 0, 0, (float)pass->width, (float)pass->height, 0, 1,
            DVZ_GRAPHICS_FLAGS_DYNAMIC);
        dvz_graphics_scissor(
            pipeline->graphics, 0, 0, pass->width, pass->height, DVZ_GRAPHICS_FLAGS_DYNAMIC);
        dvz_cmd_bind_graphics(pass->commands, pipeline->graphics);
        return _ok();
    }
    if (pass->kind == DRP2_OBJECT_COMPUTE_PASS && pipeline->kind == DRP2_OBJECT_COMPUTE_PIPELINE &&
        pipeline->compute != NULL)
    {
        dvz_cmd_bind_compute(pass->commands, pipeline->compute);
        return _ok();
    }
    return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
}


/**
 * Bind a vertex buffer within a vklite render pass.
 *
 * @param state vklite runtime state
 * @param command DRP2 SetVertexBuffer command
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
static DvzDrp2ValidationResult _vklite_set_vertex_buffer(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);
    Drp2VkliteObject* pass = _vklite_find(state, command->u.set_vertex_buffer.pass_id);
    Drp2VkliteObject* buffer = _vklite_find(state, command->u.set_vertex_buffer.buffer_id);
    if (pass == NULL || pass->kind != DRP2_OBJECT_RENDER_PASS || pass->commands == NULL ||
        buffer == NULL || buffer->buffer == NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    DvzSize offset = command->u.set_vertex_buffer.offset;
    dvz_cmd_bind_vertex_buffers(
        pass->commands, command->u.set_vertex_buffer.slot, 1, buffer->buffer, &offset);
    return _ok();
}


/**
 * Bind a vklite descriptor set within a DRP2 render or compute pass.
 *
 * @param state vklite runtime state
 * @param command DRP2 SetBindGroup command
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
static DvzDrp2ValidationResult _vklite_set_bind_group(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);
    Drp2VkliteObject* pass = _vklite_find(state, command->u.set_bind_group.pass_id);
    Drp2VkliteObject* bind_group = _vklite_find(state, command->u.set_bind_group.bind_group_id);
    if (pass == NULL || pass->commands == NULL || bind_group == NULL ||
        bind_group->descriptors == NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    VkPipelineBindPoint bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;
    if (pass->kind == DRP2_OBJECT_COMPUTE_PASS)
        bind_point = VK_PIPELINE_BIND_POINT_COMPUTE;
    else if (pass->kind != DRP2_OBJECT_RENDER_PASS)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    dvz_cmd_bind_descriptors(
        pass->commands, bind_point, bind_group->descriptors, command->u.set_bind_group.slot, 1, 0,
        NULL);
    return _ok();
}


/**
 * Record a direct draw within a vklite render pass.
 *
 * @param state vklite runtime state
 * @param command DRP2 Draw command
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
static DvzDrp2ValidationResult _vklite_draw(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);
    Drp2VkliteObject* pass = _vklite_find(state, command->u.draw.pass_id);
    if (pass == NULL || pass->kind != DRP2_OBJECT_RENDER_PASS || pass->commands == NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    dvz_cmd_draw(
        pass->commands, command->u.draw.first_vertex, command->u.draw.vertex_count,
        command->u.draw.first_instance, command->u.draw.instance_count);
    return _ok();
}


/**
 * Record a compute dispatch within a vklite compute pass.
 *
 * @param state vklite runtime state
 * @param command DRP2 DispatchWorkgroups command
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
static DvzDrp2ValidationResult _vklite_dispatch_workgroups(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);
    Drp2VkliteObject* pass = _vklite_find(state, command->u.dispatch.pass_id);
    if (pass == NULL || pass->kind != DRP2_OBJECT_COMPUTE_PASS || pass->commands == NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    dvz_cmd_dispatch(
        pass->commands, command->u.dispatch.x, command->u.dispatch.y, command->u.dispatch.z);
    return _ok();
}


/**
 * End and submit a vklite dynamic-rendering pass.
 *
 * @param state vklite runtime state
 * @param pass_id DRP2 render pass id
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
static DvzDrp2ValidationResult
_vklite_end_render_pass(Drp2VkliteState* state, uint64_t pass_id, uint32_t command_index)
{
    ANN(state);
    Drp2VkliteObject* pass = _vklite_find(state, pass_id);
    if (pass == NULL || pass->kind != DRP2_OBJECT_RENDER_PASS || pass->commands == NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    dvz_cmd_rendering_end(pass->commands);
    if (!pass->borrowed_commands)
    {
        DvzDrp2ValidationResult result =
            _vklite_owned_commands_end_submit(pass->commands, command_index);
        if (!result.ok)
        {
            _vklite_destroy_object(pass);
            return result;
        }
    }
    _vklite_destroy_object(pass);
    return _ok();
}


/**
 * End and submit a vklite compute pass.
 *
 * @param state vklite runtime state
 * @param pass_id DRP2 compute pass id
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
static DvzDrp2ValidationResult
_vklite_end_compute_pass(Drp2VkliteState* state, uint64_t pass_id, uint32_t command_index)
{
    ANN(state);
    Drp2VkliteObject* pass = _vklite_find(state, pass_id);
    if (pass == NULL || pass->kind != DRP2_OBJECT_COMPUTE_PASS || pass->commands == NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    DvzDrp2ValidationResult result = _vklite_owned_commands_end_submit(pass->commands, command_index);
    if (!result.ok)
    {
        _vklite_destroy_object(pass);
        return result;
    }
    _vklite_destroy_object(pass);
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
        return _fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (object->kind != DRP2_OBJECT_SHADER_VERTEX &&
        object->kind != DRP2_OBJECT_SHADER_FRAGMENT &&
        object->kind != DRP2_OBJECT_SHADER_COMPUTE)
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
        case DVZ_DRP2_COMMAND_SET_PIPELINE:
            result = _vklite_set_pipeline(state, command, i);
            break;
        case DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER:
            result = _vklite_set_vertex_buffer(state, command, i);
            break;
        case DVZ_DRP2_COMMAND_SET_BIND_GROUP:
            result = _vklite_set_bind_group(state, command, i);
            break;
        case DVZ_DRP2_COMMAND_DRAW:
            result = _vklite_draw(state, command, i);
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
    _runtime_state_cleanup(runtime->semantic_state);
    dvz_free(runtime->semantic_state);
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
    if (stream == NULL)
        return _fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, 0);

    DvzDrp2ValidationResult result = _runtime_validate_stream(runtime, stream);
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
    if (frame->command_buffer == VK_NULL_HANDLE || frame->image == VK_NULL_HANDLE)
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
        dst, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
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
        dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
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
