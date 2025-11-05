/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Object                                                                                       */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "datoviz/common/macros.h"
#include "datoviz/common/obj.h"
#include "datoviz/math/arithm.h"



/*************************************************************************************************/
/*  Object functions                                                                             */
/*************************************************************************************************/

void dvz_obj_init(DvzObject* obj) { obj->status = DVZ_OBJECT_STATUS_INIT; }



void dvz_obj_created(DvzObject* obj) { obj->status = DVZ_OBJECT_STATUS_CREATED; }



void dvz_obj_destroyed(DvzObject* obj) { obj->status = DVZ_OBJECT_STATUS_DESTROYED; }



bool dvz_obj_is_created(DvzObject* obj)
{
    return obj != NULL && obj->status >= DVZ_OBJECT_STATUS_CREATED &&
           obj->status != DVZ_OBJECT_STATUS_INVALID;
}



/*************************************************************************************************/
/*  Container                                                                                    */
/*************************************************************************************************/

DvzContainer dvz_container(uint32_t count, size_t item_size, DvzObjectType type)
{
    ASSERT(count > 0);
    ASSERT(item_size > 0);
    // log_trace("create container");
    INIT(DvzContainer, container);
    container.count = 0;
    container.item_size = item_size;
    container.type = type;
    container.capacity = (uint32_t)dvz_next_pow2(count);
    ASSERT(container.capacity > 0);
    container.items = (void**)dvz_calloc(container.capacity, sizeof(void*));
    ANN(container.items);
    // NOTE: we shouldn't rely on calloc() initializing pointer values to NULL as it is not
    // guaranteed that NULL is represented by 0 bits.
    // https://stackoverflow.com/a/22624643/1595060
    for (uint32_t i = 0; i < container.capacity; i++)
    {
        container.items[i] = NULL;
    }
    return container;
}



void dvz_container_delete_if_destroyed(DvzContainer* container, uint32_t idx)
{
    ANN(container);
    ASSERT(container->capacity > 0);
    ANN(container->items);
    ASSERT(idx < container->capacity);
    if (container->items[idx] == NULL)
        return;
    DvzObject* object = (DvzObject*)container->items[idx];
    if (object->status == DVZ_OBJECT_STATUS_DESTROYED)
    {
        // log_trace("delete container item #%d", idx);
        dvz_free(container->items[idx]);
        container->items[idx] = NULL;
        container->count--;
        ASSERT(container->count < UINT32_MAX);
    }
}



void* dvz_container_alloc(DvzContainer* container)
{
    ANN(container);
    ASSERT(container->capacity > 0);
    ANN(container->items);
    uint32_t available_slot = UINT32_MAX;

    // Free the memory of destroyed objects and find the first available slot.
    for (uint32_t i = 0; i < container->capacity; i++)
    {
        dvz_container_delete_if_destroyed(container, i);
        // Find first slot with empty pointer, to use for new allocation.
        if (container->items[i] == NULL && available_slot == UINT32_MAX)
            available_slot = i;
    }

    // If no slot, need to reallocate container.
    if (available_slot == UINT32_MAX)
    {
        log_trace("reallocate container up to %d items", 2 * container->capacity);
        void** _new = (void**)dvz_realloc(
            container->items, (size_t)(2 * container->capacity * container->item_size));
        ANN(_new);
        container->items = _new;

        ANN(container->items);
        // Initialize newly-allocated pointers to NULL.
        for (uint32_t i = container->capacity; i < 2 * container->capacity; i++)
        {
            container->items[i] = NULL;
        }
        ASSERT(container->items[container->capacity] == NULL);
        ASSERT(container->items[2 * container->capacity - 1] == NULL);
        // Return the first empty slot of the newly-allocated container.
        available_slot = container->capacity;
        // Update the container capacity.
        container->capacity *= 2;
    }
    ASSERT(available_slot < UINT32_MAX);
    ASSERT(container->items[available_slot] == NULL);

    // Memory allocation on the heap and store the pointer in the container.
    // log_trace("container allocates new item #%d", available_slot);
    container->items[available_slot] = dvz_calloc(1, container->item_size);
    container->count++;
    ANN(container->items[available_slot]);

    // Initialize the DvzObject field.
    DvzObject* obj = (DvzObject*)container->items[available_slot];
    obj->status = DVZ_OBJECT_STATUS_ALLOC;
    obj->type = container->type;

    return container->items[available_slot];
}



void* dvz_container_get(DvzContainer* container, uint32_t idx)
{
    ANN(container);
    ANN(container->items);
    ASSERT(idx < container->capacity);
    return container->items[idx];
}



void dvz_container_iter(DvzContainerIterator* iterator)
{
    ANN(iterator);
    DvzContainer* container = iterator->container;
    ANN(container);

    // IMPORTANT: make sure the item is reset to null if we return early in this function, so that
    // the infinite while loop stops.
    iterator->item = NULL;

    if (container->items == NULL || container->capacity == 0 || container->count == 0)
        return;
    if (iterator->idx >= container->capacity)
        return;
    ASSERT(iterator->idx <= container->capacity - 1);
    for (uint32_t i = iterator->idx; i < container->capacity; i++)
    {
        dvz_container_delete_if_destroyed(container, i);
        if (container->items[i] != NULL)
        {
            iterator->idx = i + 1;
            // log_info("item %d: %d", i, container->items[i]);
            iterator->item = container->items[i];
            return;
        }
    }
    // End the outer loop, reset the internal idx.
    iterator->idx = 0;
    iterator->item = NULL;
}



DvzContainerIterator dvz_container_iterator(DvzContainer* container)
{
    ANN(container);

    INIT(DvzContainerIterator, iterator);
    iterator.container = container;
    dvz_container_iter(&iterator);
    return iterator;
}



void* dvz_container_get_created(DvzContainer* container, uint32_t idx)
{
    ANN(container);
    ANN(container->items);
    DvzContainerIterator iter = dvz_container_iterator(container);
    DvzObject* obj = NULL;
    uint32_t n = 0;
    while (iter.item != NULL)
    {
        // WARNING: we assume that every item in the container is a struct for which the first item
        // is a DvzObject.
        obj = (DvzObject*)iter.item;
        ANN(obj);
        if (dvz_obj_is_created(obj))
        {
            if (n == idx)
                return obj;
            n++;
        }
        dvz_container_iter(&iter);
    }
    return NULL;
}



void dvz_container_destroy(DvzContainer* container)
{
    ANN(container);
    if (container->items == NULL)
        return;
    ANN(container->items);
    // log_trace("container destroy");
    // Check all elements have been destroyed, and free them if necessary.
    uint32_t count = container->count;
    DvzObject* item = NULL;
    for (uint32_t i = 0; i < container->capacity; i++)
    {
        if (container->items[i] != NULL)
        {
            // log_trace("deleting container item #%d", i);
            // When destroying the container, ensure that all objects have been destroyed first.
            // NOTE: only works if every item has a DvzObject as first struct field.
            item = (DvzObject*)container->items[i];
            dvz_container_delete_if_destroyed(container, i);
            // Also deallocate objects allocated/initialized, but not created/destroyed.
            if (container->items[i] != NULL)
            {
                ASSERT(item->status <= DVZ_OBJECT_STATUS_INIT);
                ASSERT(item->status != DVZ_OBJECT_STATUS_DESTROYED);
                dvz_free(container->items[i]);
                container->items[i] = NULL;
                container->count--;
                ASSERT(container->count < UINT32_MAX);
            }
            ASSERT(container->items[i] == NULL);
        }
    }
    ASSERT(container->count == 0);
    // log_trace("free container items");
    dvz_free(container->items);
    log_trace("container destroy (%d elements)", count);
    container->capacity = 0;
}
