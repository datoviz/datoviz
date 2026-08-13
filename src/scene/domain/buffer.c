/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene buffers                                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_overflow.h"
#include "_scene.h"
#include "core/scene_notify_internal.h"
#include "core/_scene_resource_key.h"
#include "buffer_internal.h"
#include "field_internal.h"
#include "frame_plan/emit.h"
#include "visuals/bindings_internal.h"
#include "_visual_internal.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

#define DVZ_SCENE_BUFFER_DESC_KNOWN_FLAGS 0u



static bool _scene_buffer_desc_validate(const DvzSceneBufferDesc* desc)
{
    if (desc == NULL)
        return false;
    if (!DVZ_STRUCT_VALID(desc, DvzSceneBufferDesc, DVZ_SCENE_BUFFER_DESC_KNOWN_FLAGS))
    {
        log_error("invalid DvzSceneBufferDesc ABI prologue");
        return false;
    }
    return true;
}



DvzSceneBufferDesc dvz_scene_buffer_desc(void)
{
    DvzSceneBufferDesc desc = {DVZ_STRUCT_INIT_FIELDS(DvzSceneBufferDesc)};
    return desc;
}



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

uint32_t _scene_buffer_index(const DvzScene* scene, const DvzSceneBuffer* buffer)
{
    if (scene == NULL || buffer == NULL)
        return UINT32_MAX;
    for (uint32_t i = 0; i < scene->buffer_count; i++)
    {
        if (&scene->buffers[i] == buffer && buffer->scene == scene)
            return i;
    }
    return UINT32_MAX;
}


/**
 * Reset one scene-buffer slot to its empty state.
 *
 * @param buffer the buffer slot
 */
void _scene_buffer_reset(DvzSceneBuffer* buffer)
{
    if (buffer == NULL)
        return;
    if (buffer->data != NULL)
    {
        dvz_free(buffer->data);
        buffer->data = NULL;
    }
    dvz_memset(buffer, sizeof(DvzSceneBuffer), 0, sizeof(DvzSceneBuffer));
}



/**
 * Allocate and copy a complete scene-buffer payload without mutating the buffer.
 *
 * @param buffer the target buffer
 * @param data the packed payload
 * @param byte_size the logical payload size
 * @param out_data output prepared allocation
 * @param out_capacity output allocation capacity
 * @return DVZ_OK on success, DVZ_ERROR on error
 */
DvzResult _scene_buffer_prepare_data(
    const DvzSceneBuffer* buffer, const void* data, uint64_t byte_size, void** out_data,
    uint64_t* out_capacity)
{
    ANN(buffer);
    ANN(out_data);
    ANN(out_capacity);
    if (byte_size == 0 || data == NULL || byte_size % buffer->desc.stride != 0)
        return DVZ_ERROR;

    uint64_t capacity = buffer->capacity > byte_size ? buffer->capacity : byte_size;
    void* prepared = dvz_malloc(capacity);
    if (prepared == NULL)
        return DVZ_ERROR;
    dvz_memcpy(prepared, capacity, data, byte_size);
    *out_data = prepared;
    *out_capacity = capacity;
    return DVZ_OK;
}



/**
 * Commit a payload prepared by _scene_buffer_prepare_data().
 *
 * @param buffer the target buffer
 * @param data the prepared allocation
 * @param byte_size the logical payload size
 * @param capacity the allocation capacity
 */
void _scene_buffer_commit_data(
    DvzSceneBuffer* buffer, void* data, uint64_t byte_size, uint64_t capacity)
{
    ANN(buffer);
    ANN(data);
    ASSERT(byte_size > 0 && capacity >= byte_size);
    bool extent_changed = buffer->desc.byte_size != byte_size;
    dvz_free(buffer->data);
    buffer->data = data;
    buffer->capacity = capacity;
    buffer->desc.byte_size = byte_size;
    buffer->content_revision =
        buffer->content_revision == UINT64_MAX ? 1 : buffer->content_revision + 1;
    if (extent_changed)
        buffer->extent_revision =
            buffer->extent_revision == UINT64_MAX ? 1 : buffer->extent_revision + 1;
    buffer->dirty = true;
    _scene_notify_buffer_changed(buffer);
}


/**
 * Allocate one free scene-buffer slot from a scene.
 *
 * @param scene the scene
 * @return the zero-initialized slot, or NULL when full
 */
static DvzSceneBuffer* _scene_alloc_buffer_slot(DvzScene* scene)
{
    ANN(scene);
    for (uint32_t i = 0; i < DVZ_SCENE_MAX_BUFFERS; i++)
    {
        DvzSceneBuffer* buffer = &scene->buffers[i];
        if (buffer->scene != NULL)
            continue;
        dvz_memset(buffer, sizeof(DvzSceneBuffer), 0, sizeof(DvzSceneBuffer));
        buffer->scene = scene;
        if (i + 1 > scene->buffer_count)
            scene->buffer_count = i + 1;
        return buffer;
    }
    return NULL;
}


/**
 * Create a scene-owned buffer resource.
 *
 * @param scene the scene
 * @param desc the buffer descriptor
 * @return the buffer, or NULL on error
 */
DvzSceneBuffer* dvz_scene_buffer(DvzScene* scene, const DvzSceneBufferDesc* desc)
{
    ANN(scene);
    if (!_scene_buffer_desc_validate(desc))
        return NULL;
    if (desc->usage == 0)
    {
        log_error("scene buffer usage must be non-zero");
        return NULL;
    }
    if (desc->stride == 0)
    {
        log_error("scene buffer stride must be non-zero");
        return NULL;
    }
    DvzSceneBuffer* buffer = _scene_alloc_buffer_slot(scene);
    if (buffer == NULL)
    {
        log_error("maximum scene buffer count reached");
        return NULL;
    }
    buffer->id = _scene_next_id(scene);
    buffer->desc = *desc;
    buffer->capacity = desc->byte_size;
    buffer->lifecycle_revision = 1;
    return buffer;
}



/**
 * Destroy a scene-owned buffer resource.
 *
 * @param buffer the buffer
 */
void dvz_scene_buffer_destroy(DvzSceneBuffer* buffer)
{
    if (buffer == NULL)
        return;
    if (!_scene_visual_mutation_allowed(buffer->scene, "destroy scene buffer"))
        return;
    DvzScene* scene = buffer->scene;
    if (scene != NULL)
    {
        for (uint32_t i = 0; i < scene->field_count; i++)
        {
            DvzSampledField* field = &scene->fields[i];
            if (field->scene != scene || field->buffer != buffer)
                continue;
            field->buffer = NULL;
            for (uint32_t vi = 0; vi < scene->visual_count; vi++)
            {
                DvzVisual* visual = &scene->visuals[vi];
                if (visual->scene == scene && _visual_family_state(visual)->field == field)
                    _scene_notify_visual_changed(visual);
            }
            _scene_release_field_bindings(field);
        }
        for (uint32_t i = 0; i < scene->visual_count; i++)
        {
            DvzVisual* visual = &scene->visuals[i];
            DvzVisualFamilyState* state = _visual_family_state(visual);
            if (state != NULL && state->buffer == buffer)
                _visual_binding_clear(visual, DVZ_VISUAL_BINDING_BUFFER);
            for (uint32_t ai = 0; ai < visual->attr_count; ai++)
            {
                if (visual->attrs[ai].buffer == buffer)
                {
                    visual->attrs[ai].buffer = NULL;
                    visual->attrs[ai].buffer_byte_offset = 0;
                    if (visual->attrs[ai].data == NULL)
                        visual->attrs[ai].item_count = 0;
                }
            }
        }
        for (uint32_t i = 0; i < scene->compute_count; i++)
        {
            DvzSceneCompute* compute = &scene->computes[i];
            if (compute->scene != scene)
                continue;
            for (uint32_t bi = 0; bi < compute->binding_count; bi++)
            {
                if (compute->bindings[bi].active && compute->bindings[bi].buffer == buffer)
                {
                    compute->bindings[bi].active = false;
                    compute->bindings[bi].buffer = NULL;
                }
            }
        }
    }
    if (scene != NULL)
    {
        char key[DVZ_SCENE_LABEL_SIZE];
        bool realized = _scene_resource_key_buffer(buffer->id, key, sizeof(key)) &&
                        scene->emitter != NULL &&
                        _resource_lookup_id(&scene->emitter->resources, key) != 0;
        if (realized)
        {
            ASSERT(scene->buffer_retirement_count < DVZ_SCENE_MAX_BUFFER_RETIREMENTS);
            DvzSceneBufferRetirement* retirement =
                &scene->buffer_retirements[scene->buffer_retirement_count++];
            retirement->id = buffer->id;
            retirement->lifecycle_revision = buffer->lifecycle_revision + 1;
        }
    }
    _scene_buffer_reset(buffer);
}



/**
 * Replace the full payload of a scene-owned buffer resource.
 *
 * @param buffer the buffer
 * @param data the packed payload
 * @param byte_size the payload size
 * @return DVZ_OK on success, DVZ_ERROR on error
 */
DvzResult dvz_scene_buffer_set_data(DvzSceneBuffer* buffer, const void* data, uint64_t byte_size)
{
    ANN(buffer);
    if (buffer == NULL || buffer->scene == NULL)
        return DVZ_ERROR;
    if (!_scene_visual_mutation_allowed(buffer->scene, "replace scene buffer data"))
        return DVZ_ERROR;
    if (byte_size > 0 && data == NULL)
    {
        log_error("scene buffer payload data must be non-NULL for a non-zero size");
        return DVZ_ERROR;
    }
    if (byte_size % buffer->desc.stride != 0)
    {
        log_error(
            "scene buffer payload size %" PRIu64 " is not aligned to stride %u", byte_size,
            buffer->desc.stride);
        return DVZ_ERROR;
    }
    if (byte_size > 0 && (buffer->data == NULL || byte_size > buffer->capacity))
    {
        uint64_t capacity = buffer->capacity > byte_size ? buffer->capacity : byte_size;
        void* grown = dvz_realloc(buffer->data, capacity);
        if (grown == NULL)
        {
            log_error("scene buffer allocation failed for %" PRIu64 " bytes", capacity);
            return DVZ_ERROR;
        }
        buffer->data = grown;
        buffer->capacity = capacity;
    }
    if (byte_size > 0)
        dvz_memcpy(buffer->data, buffer->capacity, data, byte_size);
    bool extent_changed = buffer->desc.byte_size != byte_size;
    buffer->desc.byte_size = byte_size;
    buffer->content_revision = buffer->content_revision == UINT64_MAX ? 1 : buffer->content_revision + 1;
    if (extent_changed)
        buffer->extent_revision = buffer->extent_revision == UINT64_MAX ? 1 : buffer->extent_revision + 1;
    buffer->dirty = true;
    _scene_notify_buffer_changed(buffer);
    return DVZ_OK;
}



/**
 * Copy immutable buffer descriptor information.
 *
 * @param buffer the buffer
 * @param out output buffer descriptor
 * @return whether the descriptor was copied
 */
bool dvz_scene_buffer_info(const DvzSceneBuffer* buffer, DvzSceneBufferDesc* out)
{
    if (buffer == NULL || out == NULL)
        return false;
    *out = buffer->desc;
    return true;
}



/**
 * Return the retained scene resource key for a scene buffer.
 *
 * @param buffer the scene buffer
 * @param out output string buffer
 * @param out_size output string capacity
 * @return whether the resource key was written
 */
bool dvz_scene_buffer_resource_key(const DvzSceneBuffer* buffer, char* out, size_t out_size)
{
    ANN(out);
    if (buffer == NULL || buffer->scene == NULL || out_size == 0)
        return false;

    if (_scene_buffer_index(buffer->scene, buffer) == UINT32_MAX)
        return false;
    return _scene_resource_key_buffer(buffer->id, out, out_size);
}



/**
 * Bind a scene-owned buffer to a named visual slot.
 *
 * @param visual the visual
 * @param slot_name the slot name
 * @param buffer the buffer, or NULL to clear the binding
 * @return DVZ_OK on success, DVZ_ERROR on error
 */
DvzResult dvz_visual_set_buffer(DvzVisual* visual, const char* slot_name, DvzSceneBuffer* buffer)
{
    ANN(visual);
    ANN(slot_name);
    if (buffer != NULL && buffer->scene != visual->scene)
    {
        log_error("cannot bind a buffer from a different scene");
        return DVZ_ERROR;
    }
    if (visual->type != DVZ_VISUAL_TYPE_PRIMITIVE && visual->type != DVZ_VISUAL_TYPE_MESH)
    {
        log_error("dvz_visual_set_buffer is only supported for primitive and mesh visuals in the first slice");
        return DVZ_ERROR;
    }
    if (strcmp(slot_name, "index") != 0)
    {
        log_error("unsupported indexed buffer slot '%s' (expected 'index')", slot_name);
        return DVZ_ERROR;
    }
    if (buffer != NULL)
    {
        if ((buffer->desc.usage & DVZ_SCENE_BUFFER_USAGE_INDEX) == 0)
        {
            log_error("indexed draw slot requires a buffer with INDEX usage");
            return DVZ_ERROR;
        }
        if (buffer->desc.stride != sizeof(uint16_t) && buffer->desc.stride != sizeof(uint32_t))
        {
            log_error("indexed draw buffers require stride 2 or 4 bytes");
            return DVZ_ERROR;
        }
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "bind scene buffer"))
        return DVZ_ERROR;
    _scene_release_visual_buffer(visual);
    if (buffer != NULL)
        _visual_binding_assign(visual, DVZ_VISUAL_BINDING_BUFFER, slot_name, buffer, false);
    else
        _visual_binding_clear(visual, DVZ_VISUAL_BINDING_BUFFER);
    _scene_notify_visual_changed(visual);
    return DVZ_OK;
}



/**
 * Replace one visual's index buffer with copied 32-bit index data.
 *
 * @param visual the primitive or mesh visual
 * @param indices the index array
 * @param index_count the number of indices
 * @return 0 on success, -1 on error
 */
DvzResult dvz_visual_set_index_data(
    DvzVisual* visual, const DvzIndex* indices, uint32_t index_count)
{
    if (visual == NULL || indices == NULL || index_count == 0)
        return -1;
    if (visual->type != DVZ_VISUAL_TYPE_PRIMITIVE && visual->type != DVZ_VISUAL_TYPE_MESH)
        return -1;
    if (!_scene_visual_mutation_allowed(visual->scene, "replace visual index data"))
        return -1;

    uint64_t byte_size = 0;
    if (_dvz_mul_u64_overflows((uint64_t)index_count, (uint64_t)sizeof(DvzIndex), &byte_size))
    {
        log_error("visual index data byte size overflow for index_count=%u", index_count);
        return -1;
    }

    DvzVisualFamilyState* state = _visual_family_state(visual);
    const DvzVisualBinding* current =
        _visual_binding_const(visual, DVZ_VISUAL_BINDING_BUFFER);
    if (state->buffer != NULL && current != NULL && current->owned &&
        state->buffer->desc.usage == DVZ_SCENE_BUFFER_USAGE_INDEX &&
        state->buffer->desc.stride == sizeof(DvzIndex))
    {
        return dvz_scene_buffer_set_data(state->buffer, indices, byte_size) == DVZ_OK ? 0 : -1;
    }

    DvzSceneBufferDesc desc = dvz_scene_buffer_desc();
    desc.usage = DVZ_SCENE_BUFFER_USAGE_INDEX;
    desc.stride = sizeof(DvzIndex);
    DvzSceneBuffer* buffer = dvz_scene_buffer(visual->scene, &desc);
    if (buffer == NULL)
        return -1;
    if (dvz_scene_buffer_set_data(buffer, indices, byte_size) != DVZ_OK)
    {
        dvz_scene_buffer_destroy(buffer);
        return -1;
    }
    if (dvz_visual_set_buffer(visual, "index", buffer) != DVZ_OK)
    {
        dvz_scene_buffer_destroy(buffer);
        return -1;
    }

    _visual_binding_assign(visual, DVZ_VISUAL_BINDING_BUFFER, "index", buffer, true);
    return 0;
}


void _scene_release_visual_buffer(DvzVisual* visual)
{
    if (visual == NULL)
        return;
    DvzVisualFamilyState* state = _visual_family_state(visual);
    if (state == NULL)
        return;
    DvzSceneBuffer* buffer = state->buffer;
    const DvzVisualBinding* binding = _visual_binding_const(visual, DVZ_VISUAL_BINDING_BUFFER);
    bool owned = binding != NULL ? binding->owned : false;
    _visual_binding_clear(visual, DVZ_VISUAL_BINDING_BUFFER);
    if (owned && buffer != NULL)
        dvz_scene_buffer_destroy(buffer);
}
