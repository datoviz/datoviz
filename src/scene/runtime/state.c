/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene FramePlan runtime emitter state                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "frame_plan/emit.h"
#include "_overflow.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Ensure the converter resource map can store at least the requested number of entries.
 *
 * @param state the converter state
 * @param min_capacity the minimum required capacity
 * @return whether the resource map has enough capacity
 */
static bool _state_ensure_capacity(ConverterState* state, uint32_t min_capacity)
{
    ANN(state);
    if (state->capacity >= min_capacity)
        return true;

    uint32_t capacity = state->capacity != 0 ? state->capacity : DRP2_MAX_FIXTURE_RESOURCES;
    while (capacity < min_capacity)
    {
        if (capacity > UINT32_MAX / 2)
            return false;
        capacity *= 2;
    }

    uint64_t bytes = 0;
    if (_dvz_mul_u64_overflows(capacity, sizeof(ResourceId), &bytes))
        return false;

    uint64_t old_bytes = 0;
    if (_dvz_mul_u64_overflows(state->capacity, sizeof(ResourceId), &old_bytes))
        return false;

    ResourceId* resources = (ResourceId*)dvz_realloc(state->resources, bytes);
    if (resources == NULL)
        return false;

    uint64_t added_bytes = bytes - old_bytes;
    dvz_memset(
        (uint8_t*)resources + old_bytes, (size_t)added_bytes, 0, (size_t)added_bytes);
    state->resources = resources;
    state->capacity = capacity;
    return true;
}



static void _state_copy_key(char* dst, size_t dst_size, const char* src)
{
    ANN(dst);
    ANN(src);
    if (dst_size == 0)
        return;
    size_t len = strlen(src);
    if (len >= dst_size)
        len = dst_size - 1;
    memcpy(dst, src, len);
    dst[len] = '\0';
}



/**
 * Deep-copy one converter state.
 *
 * @param src the source converter state
 * @param dst the destination converter state
 * @return whether the state was copied successfully
 */
static bool _state_clone(const ConverterState* src, ConverterState* dst)
{
    ANN(src);
    ANN(dst);
    *dst = *src;
    dst->resources = NULL;
    if (src->capacity == 0)
        return true;

    uint64_t bytes = 0;
    if (_dvz_mul_u64_overflows(src->capacity, sizeof(ResourceId), &bytes))
        return false;
    dst->resources = (ResourceId*)dvz_calloc(1, bytes);
    if (dst->resources == NULL)
        return false;
    dvz_memcpy(dst->resources, (size_t)bytes, src->resources, (size_t)bytes);
    return true;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Initialize converter state.
 *
 * @param state the converter state
 */
void _state_init(ConverterState* state)
{
    ANN(state);
    dvz_memset(state, sizeof(ConverterState), 0, sizeof(ConverterState));
    state->next_id = DRP2_ID_RESOURCE_BASE;
}



/**
 * Destroy converter state.
 *
 * @param state the converter state
 */
void _state_destroy(ConverterState* state)
{
    if (state == NULL)
        return;
    dvz_free(state->resources);
    dvz_memset(state, sizeof(ConverterState), 0, sizeof(ConverterState));
}



/**
 * Clone all persistent emitter state into an isolated emission candidate.
 *
 * @param emitter the persistent emitter
 * @param candidate the zero-initialized candidate emitter
 * @return whether the emitter state was cloned successfully
 */
bool _emitter_state_clone(
    const DvzFramePlanEmitter* emitter, DvzFramePlanEmitter* candidate)
{
    ANN(emitter);
    ANN(candidate);
    dvz_memset(candidate, sizeof(DvzFramePlanEmitter), 0, sizeof(DvzFramePlanEmitter));
    *candidate = *emitter;
    candidate->resources.resources = NULL;
    candidate->objects.resources = NULL;
    if (!_state_clone(&emitter->resources, &candidate->resources) ||
        !_state_clone(&emitter->objects, &candidate->objects))
    {
        _emitter_state_discard(candidate);
        return false;
    }
    return true;
}



/**
 * Atomically replace persistent emitter state with a successful candidate.
 *
 * @param emitter the persistent emitter
 * @param candidate the successful emission candidate
 */
void _emitter_state_commit(
    DvzFramePlanEmitter* emitter, DvzFramePlanEmitter* candidate)
{
    ANN(emitter);
    ANN(candidate);
    _state_destroy(&emitter->resources);
    _state_destroy(&emitter->objects);
    *emitter = *candidate;
    dvz_memset(candidate, sizeof(DvzFramePlanEmitter), 0, sizeof(DvzFramePlanEmitter));
}



/**
 * Destroy an isolated emitter candidate without changing persistent state.
 *
 * @param candidate the emission candidate
 */
void _emitter_state_discard(DvzFramePlanEmitter* candidate)
{
    if (candidate == NULL)
        return;
    _state_destroy(&candidate->resources);
    _state_destroy(&candidate->objects);
    dvz_memset(candidate, sizeof(DvzFramePlanEmitter), 0, sizeof(DvzFramePlanEmitter));
}



/**
 * Return the next runtime-mode transient id.
 *
 * @param emitter the persistent emitter
 * @return a unique transient DRP2 id
 */
uint64_t _emitter_next_transient_id(DvzFramePlanEmitter* emitter)
{
    ANN(emitter);
    return emitter->next_transient_id++;
}



/**
 * Return the volume uniform cache slot for a visual key, creating it when capacity allows.
 *
 * @param emitter the persistent emitter
 * @param key the volume cache key
 * @return the cached volume uniform slot, or NULL when the cache is full
 */
DvzSceneVolumeUniform*
_emitter_volume_slot(DvzFramePlanEmitter* emitter, const char* key)
{
    ANN(emitter);
    ANN(key);
    for (uint32_t i = 0; i < emitter->volume_count; i++)
    {
        if (strncmp(emitter->volume_ids[i], key, DVZ_SCENE_LABEL_SIZE) == 0)
            return &emitter->volume_cache[i];
    }
    if (emitter->volume_count >= DVZ_SCENE_VOLUME_CACHE_CAPACITY)
        return NULL;
    uint32_t slot = emitter->volume_count++;
    strncpy(emitter->volume_ids[slot], key, DVZ_SCENE_LABEL_SIZE - 1);
    return &emitter->volume_cache[slot];
}


/**
 * Return the labels uniform cache slot for a visual key, creating it when capacity allows.
 *
 * @param emitter the persistent emitter
 * @param key the labels cache key
 * @return the cached labels uniform slot, or NULL when the cache is full
 */
DvzSceneLabelsUniform*
_emitter_labels_slot(DvzFramePlanEmitter* emitter, const char* key)
{
    ANN(emitter);
    ANN(key);
    for (uint32_t i = 0; i < emitter->labels_count; i++)
    {
        if (strncmp(emitter->labels_ids[i], key, DVZ_SCENE_LABEL_SIZE) == 0)
            return &emitter->labels_cache[i];
    }
    if (emitter->labels_count >= DVZ_SCENE_LABELS_CACHE_CAPACITY)
        return NULL;
    uint32_t slot = emitter->labels_count++;
    strncpy(emitter->labels_ids[slot], key, DVZ_SCENE_LABEL_SIZE - 1);
    return &emitter->labels_cache[slot];
}



/**
 * Return a deterministic DRP2 id for a scene resource key.
 *
 * @param state the converter state
 * @param key the scene resource key
 * @return the DRP2 id, or 0 when the map cannot grow
 */
uint64_t _resource_id(ConverterState* state, const char* key)
{
    ANN(state);
    ANN(key);
    if (key[0] == '\0')
        return 0;
    for (uint32_t i = 0; i < state->count; i++)
    {
        if (strcmp(state->resources[i].key, key) == 0)
            return state->resources[i].id;
    }
    if (state->next_id == UINT64_MAX)
        return 0;
    if (state->count == UINT32_MAX)
        return 0;
    if (!_state_ensure_capacity(state, state->count + 1))
        return 0;

    ResourceId* resource = &state->resources[state->count++];
    dvz_memset(resource, sizeof(ResourceId), 0, sizeof(ResourceId));
    _state_copy_key(resource->key, sizeof(resource->key), key);
    resource->id = state->next_id++;
    resource->topology = UINT32_MAX;
    return resource->id;
}



/**
 * Look up an existing deterministic DRP2 id for a scene resource key.
 *
 * @param state the converter state
 * @param key the scene resource key
 * @return the DRP2 id, or 0 when the key is unknown
 */
uint64_t _resource_lookup_id(const ConverterState* state, const char* key)
{
    ANN(state);
    ANN(key);
    for (uint32_t i = 0; i < state->count; i++)
    {
        if (strcmp(state->resources[i].key, key) == 0)
            return state->resources[i].id;
    }
    return 0;
}



/**
 * Return the mutable resource entry for a scene resource key.
 *
 * @param state the converter state
 * @param key the scene resource key
 * @return the resource entry, or NULL when not found
 */
ResourceId* _resource_find(ConverterState* state, const char* key)
{
    ANN(state);
    ANN(key);
    for (uint32_t i = 0; i < state->count; i++)
    {
        if (strcmp(state->resources[i].key, key) == 0)
            return &state->resources[i];
    }
    return NULL;
}



/**
 * Return a resource entry, creating it when needed.
 *
 * @param state the converter state
 * @param key the scene resource key
 * @param is_new whether a new entry was created
 * @return the resource entry, or NULL when the map cannot grow
 */
ResourceId* _resource_entry(ConverterState* state, const char* key, bool* is_new)
{
    ANN(state);
    ANN(key);
    ANN(is_new);
    *is_new = false;
    if (key[0] == '\0')
        return NULL;

    ResourceId* resource = _resource_find(state, key);
    if (resource != NULL)
    {
        return resource;
    }
    if (state->next_id == UINT64_MAX)
        return NULL;
    if (state->count == UINT32_MAX)
        return NULL;
    if (!_state_ensure_capacity(state, state->count + 1))
        return NULL;

    resource = &state->resources[state->count++];
    dvz_memset(resource, sizeof(ResourceId), 0, sizeof(ResourceId));
    _state_copy_key(resource->key, sizeof(resource->key), key);
    resource->id = state->next_id++;
    resource->topology = UINT32_MAX;
    *is_new = true;
    return resource;
}



/**
 * Remove one persistent resource entry after committed retirement.
 *
 * @param state the converter state
 * @param key the semantic resource key
 * @return whether an entry was removed
 */
bool _resource_remove(ConverterState* state, const char* key)
{
    ANN(state);
    ANN(key);
    for (uint32_t i = 0; i < state->count; i++)
    {
        if (strcmp(state->resources[i].key, key) != 0)
            continue;
        if (i + 1 < state->count)
        {
            dvz_memmove(
                &state->resources[i], (state->count - i) * sizeof(ResourceId),
                &state->resources[i + 1], (state->count - i - 1) * sizeof(ResourceId));
        }
        state->count--;
        dvz_memset(&state->resources[state->count], sizeof(ResourceId), 0, sizeof(ResourceId));
        return true;
    }
    return false;
}



/**
 * Ensure a persisted resource has enough byte capacity.
 *
 * @param state the converter state
 * @param resource the resource entry
 * @param required_size the required byte size
 * @param needs_create whether a CreateBuffer command must be emitted
 * @return whether the resource was sized successfully
 */
bool _resource_ensure_byte_size(
    ConverterState* state, ResourceId* resource, uint64_t required_size, bool* needs_create)
{
    ANN(state);
    ANN(resource);
    ANN(needs_create);

    if (*needs_create || resource->byte_size == 0)
    {
        resource->byte_size = required_size;
        *needs_create = true;
        return true;
    }
    if (required_size <= resource->byte_size)
        return true;

    resource->byte_size = required_size;
    *needs_create = true;
    return true;
}



/**
 * Ensure a persisted resource has the requested 2D texture extent.
 *
 * @param state the converter state
 * @param resource the resource entry
 * @param width the requested texture width
 * @param height the requested texture height
 * @param needs_create whether a CreateTexture command must be emitted
 * @return whether the resource was sized successfully
 */
bool _resource_ensure_texture_2d(
    ConverterState* state, ResourceId* resource, uint32_t width, uint32_t height,
    bool* needs_create)
{
    return _resource_ensure_texture(state, resource, width, height, 1, 0, needs_create);
}



/**
 * Ensure a persisted resource has the requested texture extent and format.
 *
 * @param state the converter state
 * @param resource the resource entry
 * @param width the requested texture width
 * @param height the requested texture height
 * @param depth the requested texture depth
 * @param format the requested texture format, or zero for the default RGBA8 format
 * @param needs_create whether a CreateTexture command must be emitted
 * @return whether the resource was sized successfully
 */
bool _resource_ensure_texture(
    ConverterState* state, ResourceId* resource, uint32_t width, uint32_t height,
    uint32_t depth, uint32_t format, bool* needs_create)
{
    ANN(state);
    ANN(resource);
    ANN(needs_create);

    if (width == 0 || height == 0 || depth == 0)
        return false;

    if (*needs_create || resource->texture_width == 0 || resource->texture_height == 0 ||
        resource->texture_depth == 0)
    {
        resource->texture_width = width;
        resource->texture_height = height;
        resource->texture_depth = depth;
        resource->texture_format = format;
        *needs_create = true;
        return true;
    }
    if (width == resource->texture_width && height == resource->texture_height &&
        depth == resource->texture_depth && format == resource->texture_format)
        return true;

    resource->texture_width = width;
    resource->texture_height = height;
    resource->texture_depth = depth;
    resource->texture_format = format;
    *needs_create = true;
    return true;
}



/**
 * Look up the data tag for a resource id.
 *
 * @param state the converter state
 * @param id the DRP2 resource id
 * @return the data tag, or an empty string when the id is unknown
 */
const char* _resource_data_tag(const ConverterState* state, uint64_t id)
{
    for (uint32_t i = 0; i < state->count; i++)
        if (state->resources[i].id == id)
            return state->resources[i].data_tag;
    return "";
}



/**
 * Look up the typed role for a resource id.
 *
 * @param state the converter state
 * @param id the DRP2 resource id
 * @return the typed role, or NONE when the id is unknown or untyped
 */
DvzFramePlanResourceRole _resource_role(const ConverterState* state, uint64_t id)
{
    for (uint32_t i = 0; i < state->count; i++)
        if (state->resources[i].id == id)
            return state->resources[i].role;
    return DVZ_FRAME_PLAN_RESOURCE_ROLE_NONE;
}



/**
 * Look up the byte size for a resource id.
 *
 * @param state the converter state
 * @param id the DRP2 resource id
 * @return the byte size, or 0 when the id is unknown
 */
uint64_t _resource_byte_size(const ConverterState* state, uint64_t id)
{
    for (uint32_t i = 0; i < state->count; i++)
        if (state->resources[i].id == id)
            return state->resources[i].byte_size;
    return 0;
}



/**
 * Look up the buffer usage for a resource id.
 *
 * @param state the converter state
 * @param id the DRP2 resource id
 * @return the usage flags, or 0 when the id is unknown
 */
uint32_t _resource_usage(const ConverterState* state, uint64_t id)
{
    for (uint32_t i = 0; i < state->count; i++)
        if (state->resources[i].id == id)
            return state->resources[i].usage;
    return 0;
}



/**
 * Look up the logical item count for a resource id.
 *
 * @param state the converter state
 * @param id the DRP2 resource id
 * @return the logical item count, or 0 when the id is unknown or untracked
 */
uint64_t _resource_logical_item_count(const ConverterState* state, uint64_t id)
{
    for (uint32_t i = 0; i < state->count; i++)
        if (state->resources[i].id == id)
            return state->resources[i].logical_item_count;
    return 0;
}



/**
 * Return whether a resource has an explicit logical extent.
 *
 * @param state the converter state
 * @param id the DRP2 resource id
 * @return whether logical_item_count is authoritative, including when it is zero
 */
bool _resource_has_logical_extent(const ConverterState* state, uint64_t id)
{
    for (uint32_t i = 0; i < state->count; i++)
        if (state->resources[i].id == id)
            return state->resources[i].has_logical_extent;
    return false;
}



/**
 * Look up the item stride for a resource id.
 *
 * @param state the converter state
 * @param id the DRP2 resource id
 * @return the item stride, or 0 when the id is unknown
 */
uint32_t _resource_item_stride(const ConverterState* state, uint64_t id)
{
    for (uint32_t i = 0; i < state->count; i++)
        if (state->resources[i].id == id)
            return state->resources[i].item_stride;
    return 0;
}



/**
 * Look up the topology hint for a resource id.
 *
 * @param state the converter state
 * @param id the DRP2 resource id
 * @return the topology hint, or UINT32_MAX when unset or unknown
 */
uint32_t _resource_topology(const ConverterState* state, uint64_t id)
{
    for (uint32_t i = 0; i < state->count; i++)
        if (state->resources[i].id == id)
            return state->resources[i].topology;
    return UINT32_MAX;
}



/**
 * Return a persistent runtime object id.
 *
 * @param emitter the persistent emitter
 * @param key the object key
 * @param is_new whether a new entry was created
 * @return the object id, or 0 on failure
 */
uint64_t _obj_id(DvzFramePlanEmitter* emitter, const char* key, bool* is_new)
{
    ANN(emitter);
    ANN(key);
    ANN(is_new);
    if (key[0] == '\0')
    {
        *is_new = false;
        return 0;
    }
    uint32_t n = emitter->objects.count;
    uint64_t id = _resource_id(&emitter->objects, key);
    *is_new = (id != 0) && (emitter->objects.count > n);
    return id;
}



/**
 * Return a persistent runtime object id for a buffer with at least the requested size.
 *
 * @param emitter the persistent emitter
 * @param key the object key
 * @param byte_size the required byte size
 * @param is_new whether a CreateBuffer command must be emitted
 * @return the object id, or 0 on failure
 */
uint64_t
_obj_buffer_id(DvzFramePlanEmitter* emitter, const char* key, uint64_t byte_size, bool* is_new)
{
    ANN(emitter);
    ANN(key);
    ANN(is_new);
    if (key[0] == '\0')
    {
        *is_new = false;
        return 0;
    }

    ResourceId* resource = _resource_entry(&emitter->objects, key, is_new);
    if (resource == NULL)
        return 0;
    if (!_resource_ensure_byte_size(&emitter->objects, resource, byte_size, is_new))
        return 0;
    return resource->id;
}



/**
 * Create a persistent FramePlan-to-DRP2 emitter for runtime-mode streams.
 *
 * @return the emitter
 */
DvzFramePlanEmitter* dvz_frame_plan_emitter(void)
{
    DvzFramePlanEmitter* emitter = (DvzFramePlanEmitter*)dvz_calloc(
        1, sizeof(DvzFramePlanEmitter));
    if (emitter == NULL)
        return NULL;
    _state_init(&emitter->resources);
    _state_init(&emitter->objects);
    emitter->objects.next_id = DRP2_EMITTER_OBJECT_ID_BASE;
    emitter->next_transient_id = DRP2_RUNTIME_TRANSIENT_ID_BASE;
    emitter->max_color_sample_count = 16;
    emitter->max_depth_sample_count = 16;
    return emitter;
}



/**
 * Destroy a persistent FramePlan-to-DRP2 emitter.
 *
 * @param emitter the emitter
 */
void dvz_frame_plan_emitter_destroy(DvzFramePlanEmitter* emitter)
{
    if (emitter == NULL)
        return;
    _state_destroy(&emitter->resources);
    _state_destroy(&emitter->objects);
    dvz_free(emitter);
}



/**
 * Look up a persistent runtime object id.
 *
 * @param emitter the persistent emitter
 * @param key the object key
 * @return the object id, or 0 when the key is unknown
 */
uint64_t dvz_frame_plan_emitter_object_id(const DvzFramePlanEmitter* emitter, const char* key)
{
    ANN(emitter);
    ANN(key);
    const ConverterState* state = &emitter->objects;
    for (uint32_t i = 0; i < state->count; i++)
    {
        if (strcmp(state->resources[i].key, key) == 0)
            return state->resources[i].id;
    }
    return 0;
}
