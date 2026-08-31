/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  DRP2 vklite runtime objects                                                                  */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_overflow.h"
#include "_runtime.h"
#include "datoviz/vk/device.h"



#if DVZ_DRP2_HAS_VKLITE
/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Ensure deferred-destruction storage can append one more retired backend object.
 *
 * @param state vklite runtime state
 * @return whether append capacity is available
 */
static bool _vklite_deferred_ensure_capacity(Drp2VkliteState* state)
{
    ANN(state);
    if (state->deferred == NULL || state->deferred_capacity == 0)
    {
        state->deferred_capacity = 8;
        state->deferred =
            (Drp2DeferredDestroy*)dvz_calloc(state->deferred_capacity, sizeof(Drp2DeferredDestroy));
        return state->deferred != NULL;
    }
    if (state->deferred_count < state->deferred_capacity)
        return true;
    if (state->deferred_capacity > UINT32_MAX / 2)
        return false;

    uint32_t capacity = 2 * state->deferred_capacity;
    Drp2DeferredDestroy* deferred = (Drp2DeferredDestroy*)dvz_realloc(
        state->deferred, (uint64_t)capacity * sizeof(Drp2DeferredDestroy));
    if (deferred == NULL)
        return false;

    dvz_memset(
        deferred + state->deferred_capacity,
        (uint64_t)(capacity - state->deferred_capacity) * sizeof(Drp2DeferredDestroy), 0,
        (uint64_t)(capacity - state->deferred_capacity) * sizeof(Drp2DeferredDestroy));
    state->deferred = deferred;
    state->deferred_capacity = capacity;
    return true;
}



/**
 * Reserve storage for a batch of deferred backend-object retirements.
 *
 * @param state vklite runtime state
 * @param additional_count number of additional retirements to reserve
 * @return whether the requested capacity is available
 */
bool _vklite_deferred_reserve(Drp2VkliteState* state, uint32_t additional_count)
{
    ANN(state);
    if (additional_count > UINT32_MAX - state->deferred_count)
        return false;
    uint32_t required = state->deferred_count + additional_count;
    if (required <= state->deferred_capacity)
        return true;

    uint32_t capacity = state->deferred_capacity > 0 ? state->deferred_capacity : 8;
    while (capacity < required)
    {
        if (capacity > UINT32_MAX / 2)
            return false;
        capacity *= 2;
    }

    Drp2DeferredDestroy* deferred = (Drp2DeferredDestroy*)dvz_realloc(
        state->deferred, (uint64_t)capacity * sizeof(Drp2DeferredDestroy));
    if (deferred == NULL)
        return false;
    dvz_memset(
        deferred + state->deferred_capacity,
        (uint64_t)(capacity - state->deferred_capacity) * sizeof(Drp2DeferredDestroy), 0,
        (uint64_t)(capacity - state->deferred_capacity) * sizeof(Drp2DeferredDestroy));
    state->deferred = deferred;
    state->deferred_capacity = capacity;
    return true;
}


/**
 * Remove destroyed objects from the end of a vklite object table.
 *
 * @param state vklite runtime state
 */
static void _vklite_trim_destroyed_tail(Drp2VkliteState* state)
{
    ANN(state);
    while (state->count > 0 && state->objects[state->count - 1].destroyed)
    {
        state->count--;
        dvz_memset(
            &state->objects[state->count], sizeof(Drp2VkliteObject), 0,
            sizeof(Drp2VkliteObject));
    }
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Ensure vklite object storage can append one more object.
 *
 * @param state vklite runtime state
 * @return whether append capacity is available
 */
bool _vklite_ensure_capacity(Drp2VkliteState* state)
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
    if (_dvz_mul_u64_overflows(capacity, sizeof(Drp2VkliteObject), &bytes))
        return false;

    Drp2VkliteObject* objects = (Drp2VkliteObject*)dvz_realloc(state->objects, bytes);
    if (objects == NULL)
        return false;

    state->capacity = capacity;
    state->objects = objects;
    return true;
}



/**
 * Find a live vklite object by DRP2 id.
 *
 * @param state vklite runtime state
 * @param id DRP2 object id
 * @return matching live object, or NULL when not found
 */
Drp2VkliteObject* _vklite_find(Drp2VkliteState* state, uint64_t id)
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



/**
 * Add or reuse a vklite object slot.
 *
 * @param state vklite runtime state
 * @param id DRP2 object id
 * @param kind object kind to assign
 * @return initialized object slot, or NULL on allocation failure
 */
Drp2VkliteObject* _vklite_add(Drp2VkliteState* state, uint64_t id, Drp2ObjectKind kind)
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



/**
 * Destroy resources owned by one vklite object slot.
 *
 * @param object vklite object slot to destroy
 */
void _vklite_destroy_object(Drp2VkliteObject* object)
{
    if (object == NULL || object->destroyed)
        return;
    if (object->buffer != NULL)
    {
        if (!object->borrowed_buffer)
        {
            dvz_buffer_destroy(object->buffer);
            dvz_buffer_free(object->buffer);
        }
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
    object->depth_image = VK_NULL_HANDLE;
    object->depth_image_view = VK_NULL_HANDLE;
    object->borrowed_frame_depth = false;
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
    if (object->combined_pipeline_layout != VK_NULL_HANDLE &&
        object->combined_layout_device != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(
            object->combined_layout_device, object->combined_pipeline_layout, NULL);
        object->combined_pipeline_layout = VK_NULL_HANDLE;
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
 * Destroy resources owned by one vklite object table slot and trim trailing destroyed slots.
 *
 * @param state vklite runtime state
 * @param object vklite object slot to destroy
 */
void _vklite_destroy_object_slot(Drp2VkliteState* state, Drp2VkliteObject* object)
{
    ANN(state);
    _vklite_destroy_object(object);
    _vklite_trim_destroyed_tail(state);
}



/**
 * Return the image view associated with a vklite texture object.
 *
 * @param object vklite texture object
 * @return Vulkan image view handle, or VK_NULL_HANDLE when unavailable
 */
VkImageView _vklite_object_image_view(const Drp2VkliteObject* object)
{
    if (object == NULL)
        return VK_NULL_HANDLE;
    if (object->borrowed_frame_target && object->image_view != VK_NULL_HANDLE)
        return object->image_view;
    if (object->views == NULL)
        return VK_NULL_HANDLE;
    return dvz_image_views_handle(object->views, 0);
}



/**
 * Destroy all vklite runtime objects and deferred objects.
 *
 * @param state vklite runtime state to clean up
 */
void _vklite_state_cleanup(Drp2VkliteState* state)
{
    if (state == NULL)
        return;
    for (uint32_t i = state->deferred_count; i > 0; i--)
        _vklite_destroy_object(&state->deferred[i - 1].object);
    dvz_free(state->deferred);
    state->deferred = NULL;
    state->deferred_capacity = 0;
    state->deferred_count = 0;
    state->active_borrowed_command_buffer = VK_NULL_HANDLE;
    state->retirement_borrowed_command_buffer = VK_NULL_HANDLE;
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



/**
 * Move one backend object into deferred destruction until a borrowed frame command buffer is
 * reacquired.
 *
 * @param state vklite runtime state
 * @param object backend object to retire later
 * @param command_buffer borrowed frame command buffer that must finish first
 * @return whether the object was queued successfully
 */
bool _vklite_defer_destroy_object(
    Drp2VkliteState* state, Drp2VkliteObject* object, VkCommandBuffer command_buffer)
{
    ANN(state);
    ANN(object);
    if (command_buffer == VK_NULL_HANDLE)
        return false;
    if (!_vklite_deferred_ensure_capacity(state))
        return false;

    Drp2DeferredDestroy* deferred = &state->deferred[state->deferred_count++];
    deferred->command_buffer = command_buffer;
    deferred->object = *object;
    dvz_memset(object, sizeof(Drp2VkliteObject), 0, sizeof(Drp2VkliteObject));
    object->destroyed = true;
    _vklite_trim_destroyed_tail(state);
    return true;
}



/**
 * Destroy backend objects deferred for a borrowed frame command buffer once that command buffer has
 * been reacquired after its fence wait.
 *
 * @param state vklite runtime state
 * @param command_buffer reacquired borrowed frame command buffer
 */
void _vklite_flush_deferred_for_command_buffer(
    Drp2VkliteState* state, VkCommandBuffer command_buffer)
{
    ANN(state);
    if (command_buffer == VK_NULL_HANDLE || state->deferred_count == 0)
        return;

    bool has_match = false;
    for (uint32_t i = 0; i < state->deferred_count; i++)
    {
        if (state->deferred[i].command_buffer == command_buffer)
        {
            has_match = true;
            break;
        }
    }
    if (has_match && state->runtime != NULL && state->runtime->device != NULL)
        dvz_device_wait(state->runtime->device);

    uint32_t write_idx = 0;
    for (uint32_t i = 0; i < state->deferred_count; i++)
    {
        Drp2DeferredDestroy* deferred = &state->deferred[i];
        if (deferred->command_buffer == command_buffer)
        {
            _vklite_destroy_object(&deferred->object);
            dvz_memset(deferred, sizeof(Drp2DeferredDestroy), 0, sizeof(Drp2DeferredDestroy));
            continue;
        }
        if (write_idx != i)
            state->deferred[write_idx] = state->deferred[i];
        write_idx++;
    }
    for (uint32_t i = write_idx; i < state->deferred_count; i++)
    {
        dvz_memset(
            &state->deferred[i], sizeof(Drp2DeferredDestroy), 0,
            sizeof(Drp2DeferredDestroy));
    }
    state->deferred_count = write_idx;
}



/**
 * Destroy every deferred backend object after an explicit device completion wait.
 *
 * @param state vklite runtime state
 */
void _vklite_flush_deferred(Drp2VkliteState* state)
{
    ANN(state);
    if (state->deferred_count == 0)
        return;
    if (state->runtime != NULL && state->runtime->device != NULL)
        dvz_device_wait(state->runtime->device);
    for (uint32_t i = state->deferred_count; i > 0; i--)
    {
        _vklite_destroy_object(&state->deferred[i - 1].object);
        state->deferred[i - 1] = (Drp2DeferredDestroy){0};
    }
    state->deferred_count = 0;
}
#endif
