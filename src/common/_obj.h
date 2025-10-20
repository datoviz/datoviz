/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Object                                                                                       */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "_enums.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzObject DvzObject;
typedef struct DvzContainer DvzContainer;
typedef struct DvzContainerIterator DvzContainerIterator;



/*************************************************************************************************/
/*  Macros                                                                                      */
/*************************************************************************************************/

#define DVZ_CONTAINER_DEFAULT_COUNT 64

#define CONTAINER_DESTROY_ITEMS(t, c, f)                                                          \
    {                                                                                             \
        DvzContainerIterator _iter = dvz_container_iterator(&c);                                  \
        t* o = NULL;                                                                              \
        while (_iter.item != NULL)                                                                \
        {                                                                                         \
            o = (t*)_iter.item;                                                                   \
            f(o);                                                                                 \
            dvz_container_iter(&_iter);                                                           \
        }                                                                                         \
    }



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzObject
{
    DvzObjectType type;
    DvzObjectStatus status;
    int request;

    uint32_t group_id; // group identifier
    uint64_t id;       // unique identifier among the objects of the same type and group
};



struct DvzContainer
{
    uint32_t count;
    uint32_t capacity;
    DvzObjectType type;
    void** items;
    size_t item_size;
};



struct DvzContainerIterator
{
    DvzContainer* container;
    uint32_t idx;
    void* item;
};



/*************************************************************************************************/
/*  Object functions                                                                             */
/*************************************************************************************************/

EXTERN_C_ON

/**
 * Initialize an object.
 *
 * Memory for the object has been allocated and its fields properly initialized.
 *
 * @param obj the object
 */
void dvz_obj_init(DvzObject* obj);



/**
 * Mark an object as successfully created on the GPU.
 *
 * @param obj the object
 */
void dvz_obj_created(DvzObject* obj);



/**
 * Mark an object as destroyed.
 *
 * @param obj the object
 */
void dvz_obj_destroyed(DvzObject* obj);



/**
 * Whether an object has been successfully created.
 *
 * @param obj the object
 * @returns a boolean indicated whether the object has been successfully created
 */
bool dvz_obj_is_created(DvzObject* obj);


/*************************************************************************************************/
/*  Container                                                                                    */
/*************************************************************************************************/

/**
 * Create a container that will contain an arbitrary number of objects of the same type.
 *
 * @param count initial number of objects in the container
 * @param item_size size of each object, in bytes
 * @param type object type
 */
DvzContainer dvz_container(uint32_t count, size_t item_size, DvzObjectType type);



/**
 * Free a given object in the constainer if it was previously destroyed.
 *
 * @param container the container
 * @param idx the index of the object within the container
 */
void dvz_container_delete_if_destroyed(DvzContainer* container, uint32_t idx);



/**
 * Get a pointer to a new object in the container.
 *
 * If the container is full, it will be automatically resized.
 *
 * @param container the container
 * @returns a pointer to an allocated object
 */
void* dvz_container_alloc(DvzContainer* container);



/**
 * Return the object at a given index.
 *
 * @param container the container
 * @param idx the index of the object within the container
 * @param returns a pointer to the object at the specified index
 */
void* dvz_container_get(DvzContainer* container, uint32_t idx);



/**
 * Continue an already-started loop iteration on a container.
 *
 * @param container the container
 * @returns a pointer to the next object in the container, or NULL at the end
 */
void dvz_container_iter(DvzContainerIterator* iterator);



/**
 * Start a loop iteration over all valid objects within the container.
 *
 * @param container the container
 * @returns a pointer to the first object
 */
DvzContainerIterator dvz_container_iterator(DvzContainer* container);



/**
 * Return the n-th created object.
 *
 * @param container the container
 * @param idx the index of the object within the container
 * @param returns a pointer to the created object at the specified index
 */
void* dvz_container_get_created(DvzContainer* container, uint32_t idx);



/**
 * Destroy a container.
 *
 * Free all remaining objects, as well as the container itself.
 *
 * !!! warning
 *     All objects in the container must have been destroyed beforehand, since the generic
 *     container does not know how to properly destroy objects that were created with Vulkan.
 *
 * @param container the container
 * @param idx the index of the object within the container
 */
void dvz_container_destroy(DvzContainer* container);



EXTERN_C_OFF
