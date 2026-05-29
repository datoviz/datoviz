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
#include "buffer_internal.h"
#include "visuals/bindings_internal.h"
#include "_visual_internal.h"



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
    ANN(desc);
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
    if (scene->buffer_count >= DVZ_SCENE_MAX_BUFFERS)
    {
        log_error("maximum scene buffer count reached");
        return NULL;
    }
    DvzSceneBuffer* buffer = _scene_alloc_buffer_slot(scene);
    if (buffer == NULL)
    {
        log_error("maximum scene buffer count reached");
        return NULL;
    }
    buffer->desc = *desc;
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
        for (uint32_t i = 0; i < scene->visual_count; i++)
        {
            DvzVisual* visual = &scene->visuals[i];
            if (visual->buffer == buffer)
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
    }
    _scene_buffer_reset(buffer);
}



/**
 * Replace the full payload of a scene-owned buffer resource.
 *
 * @param buffer the buffer
 * @param data the packed payload
 * @param byte_size the payload size
 * @return true on success, false on error
 */
bool dvz_scene_buffer_set_data(DvzSceneBuffer* buffer, const void* data, uint64_t byte_size)
{
    ANN(buffer);
    ANN(data);
    if (!_scene_visual_mutation_allowed(buffer->scene, "replace scene buffer data"))
        return false;
    if (byte_size == 0)
    {
        log_error("scene buffer payload size must be non-zero");
        return false;
    }
    if (byte_size % buffer->desc.stride != 0)
    {
        log_error(
            "scene buffer payload size %" PRIu64 " is not aligned to stride %u", byte_size,
            buffer->desc.stride);
        return false;
    }
    if (buffer->data != NULL && buffer->desc.byte_size != byte_size)
    {
        dvz_free(buffer->data);
        buffer->data = NULL;
    }
    if (buffer->data == NULL)
    {
        buffer->data = dvz_malloc(byte_size);
        if (buffer->data == NULL)
        {
            log_error("scene buffer allocation failed for %" PRIu64 " bytes", byte_size);
            return false;
        }
    }
    dvz_memcpy(buffer->data, byte_size, data, byte_size);
    buffer->desc.byte_size = byte_size;
    buffer->dirty = true;
    _scene_notify_buffer_changed(buffer);
    return true;
}



/**
 * Return the immutable buffer descriptor.
 *
 * @param buffer the buffer
 * @return the descriptor, or NULL on error
 */
const DvzSceneBufferDesc* dvz_scene_buffer_desc(const DvzSceneBuffer* buffer)
{
    return buffer != NULL ? &buffer->desc : NULL;
}



/**
 * Bind a scene-owned buffer to a named visual slot.
 *
 * @param visual the visual
 * @param slot_name the slot name
 * @param buffer the buffer, or NULL to clear the binding
 * @return true on success, false on error
 */
bool dvz_visual_set_buffer(DvzVisual* visual, const char* slot_name, DvzSceneBuffer* buffer)
{
    ANN(visual);
    ANN(slot_name);
    if (buffer != NULL && buffer->scene != visual->scene)
    {
        log_error("cannot bind a buffer from a different scene");
        return false;
    }
    if (visual->type != DVZ_VISUAL_TYPE_PRIMITIVE && visual->type != DVZ_VISUAL_TYPE_MESH)
    {
        log_error("dvz_visual_set_buffer is only supported for primitive and mesh visuals in the first slice");
        return false;
    }
    if (strcmp(slot_name, "index") != 0)
    {
        log_error("unsupported indexed buffer slot '%s' (expected 'index')", slot_name);
        return false;
    }
    if (buffer != NULL)
    {
        if ((buffer->desc.usage & DVZ_SCENE_BUFFER_USAGE_INDEX) == 0)
        {
            log_error("indexed draw slot requires a buffer with INDEX usage");
            return false;
        }
        if (buffer->desc.stride != sizeof(uint16_t) && buffer->desc.stride != sizeof(uint32_t))
        {
            log_error("indexed draw buffers require stride 2 or 4 bytes");
            return false;
        }
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "bind scene buffer"))
        return false;
    _scene_release_visual_buffer(visual);
    if (buffer != NULL)
        _visual_binding_assign(visual, DVZ_VISUAL_BINDING_BUFFER, slot_name, buffer, false);
    else
        _visual_binding_clear(visual, DVZ_VISUAL_BINDING_BUFFER);
    _scene_notify_visual_changed(visual);
    return true;
}



/**
 * Replace one visual's index buffer with copied 32-bit index data.
 *
 * @param visual the primitive or mesh visual
 * @param indices the index array
 * @param index_count the number of indices
 * @return 0 on success, -1 on error
 */
int dvz_visual_set_index_data(
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

    DvzSceneBuffer* buffer = dvz_scene_buffer(
        visual->scene,
        &(DvzSceneBufferDesc){
            .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
            .stride = sizeof(DvzIndex),
        });
    if (buffer == NULL)
        return -1;
    if (!dvz_scene_buffer_set_data(buffer, indices, byte_size))
    {
        dvz_scene_buffer_destroy(buffer);
        return -1;
    }
    if (!dvz_visual_set_buffer(visual, "index", buffer))
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
    DvzSceneBuffer* buffer = visual->buffer;
    const DvzVisualBinding* binding = _visual_binding_const(visual, DVZ_VISUAL_BINDING_BUFFER);
    bool owned = binding != NULL ? binding->owned : false;
    _visual_binding_clear(visual, DVZ_VISUAL_BINDING_BUFFER);
    if (owned && buffer != NULL)
        dvz_scene_buffer_destroy(buffer);
}
