/*************************************************************************************************/
/*  Object utilities                                                                             */
/*************************************************************************************************/

#ifndef DVZ_HEADER_OBJ
#define DVZ_HEADER_OBJ



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "_log.h"
#include "_macros.h"
#include "_math.h"



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Object types.
typedef enum
{
    DVZ_OBJECT_TYPE_UNDEFINED,
    DVZ_OBJECT_TYPE_APP,
    DVZ_OBJECT_TYPE_GPU,
    DVZ_OBJECT_TYPE_WINDOW,
    DVZ_OBJECT_TYPE_SWAPCHAIN,
    DVZ_OBJECT_TYPE_CANVAS,
    DVZ_OBJECT_TYPE_COMMANDS,
    DVZ_OBJECT_TYPE_BUFFER,
    DVZ_OBJECT_TYPE_DAT,
    DVZ_OBJECT_TYPE_TEX,
    DVZ_OBJECT_TYPE_IMAGES,
    DVZ_OBJECT_TYPE_SAMPLER,
    DVZ_OBJECT_TYPE_BINDINGS,
    DVZ_OBJECT_TYPE_COMPUTE,
    DVZ_OBJECT_TYPE_GRAPHICS,
    DVZ_OBJECT_TYPE_BARRIER,
    DVZ_OBJECT_TYPE_FENCES,
    DVZ_OBJECT_TYPE_SEMAPHORES,
    DVZ_OBJECT_TYPE_RENDERPASS,
    DVZ_OBJECT_TYPE_FRAMEBUFFER,
    DVZ_OBJECT_TYPE_SUBMIT,
    DVZ_OBJECT_TYPE_SCREENCAST,
    DVZ_OBJECT_TYPE_TIMER,
    DVZ_OBJECT_TYPE_ARRAY,
    DVZ_OBJECT_TYPE_CUSTOM,
} DvzObjectType;



// Object status.
// NOTE: the order is important, status >= CREATED means the object has been created
typedef enum
{
    DVZ_OBJECT_STATUS_NONE,          //
    DVZ_OBJECT_STATUS_ALLOC,         // after allocation
    DVZ_OBJECT_STATUS_DESTROYED,     // after destruction
    DVZ_OBJECT_STATUS_INIT,          // after struct initialization but before Vulkan creation
    DVZ_OBJECT_STATUS_CREATED,       // after proper creation on the GPU
    DVZ_OBJECT_STATUS_NEED_RECREATE, // need to be recreated
    DVZ_OBJECT_STATUS_NEED_UPDATE,   // need to be updated
    DVZ_OBJECT_STATUS_NEED_DESTROY,  // need to be destroyed
    DVZ_OBJECT_STATUS_INACTIVE,      // inactive
    DVZ_OBJECT_STATUS_INVALID,       // invalid
} DvzObjectStatus;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzObject DvzObject;
typedef struct DvzContainer DvzContainer;
typedef struct DvzContainerIterator DvzContainerIterator;



/*************************************************************************************************/
/*  Macrops                                                                                      */
/*************************************************************************************************/

#define DVZ_CONTAINER_DEFAULT_COUNT 64



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzObject
{
    DvzObjectType type;
    DvzObjectStatus status;
    int request;

    uint32_t group_id; // group identifier
    uint32_t id;       // unique identifier among the objects of the same type and group
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
static inline void dvz_obj_init(DvzObject* obj) { obj->status = DVZ_OBJECT_STATUS_INIT; }



/**
 * Mark an object as successfully created on the GPU.
 *
 * @param obj the object
 */
static inline void dvz_obj_created(DvzObject* obj) { obj->status = DVZ_OBJECT_STATUS_CREATED; }



/**
 * Mark an object as destroyed.
 *
 * @param obj the object
 */
static inline void dvz_obj_destroyed(DvzObject* obj) { obj->status = DVZ_OBJECT_STATUS_DESTROYED; }



/**
 * Whether an object has been successfully created.
 *
 * @param obj the object
 * @returns a boolean indicated whether the object has been successfully created
 */
static inline bool dvz_obj_is_created(DvzObject* obj)
{
    return obj != NULL && obj->status >= DVZ_OBJECT_STATUS_CREATED &&
           obj->status != DVZ_OBJECT_STATUS_INVALID;
}



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
static inline DvzContainer dvz_container(uint32_t count, size_t item_size, DvzObjectType type)
{
    ASSERT(count > 0);
    ASSERT(item_size > 0);
    // log_trace("create container");
    DvzContainer container = {0};
    container.count = 0;
    container.item_size = item_size;
    container.type = type;
    container.capacity = dvz_next_pow2(count);
    ASSERT(container.capacity > 0);
    container.items = (void**)calloc(container.capacity, sizeof(void*));
    // NOTE: we shouldn't rely on calloc() initializing pointer values to NULL as it is not
    // guaranteed that NULL is represented by 0 bits.
    // https://stackoverflow.com/a/22624643/1595060
    for (uint32_t i = 0; i < container.capacity; i++)
    {
        container.items[i] = NULL;
    }
    return container;
}



/**
 * Free a given object in the constainer if it was previously destroyed.
 *
 * @param container the container
 * @param idx the index of the object within the container
 */
static inline void dvz_container_delete_if_destroyed(DvzContainer* container, uint32_t idx)
{
    ASSERT(container != NULL);
    ASSERT(container->capacity > 0);
    ASSERT(container->items != NULL);
    ASSERT(idx < container->capacity);
    if (container->items[idx] == NULL)
        return;
    DvzObject* object = (DvzObject*)container->items[idx];
    if (object->status == DVZ_OBJECT_STATUS_DESTROYED)
    {
        // log_trace("delete container item #%d", idx);
        FREE(container->items[idx]);
        container->items[idx] = NULL;
        container->count--;
        ASSERT(container->count < UINT32_MAX);
    }
}



/**
 * Get a pointer to a new object in the container.
 *
 * If the container is full, it will be automatically resized.
 *
 * @param container the container
 * @returns a pointer to an allocated object
 */
static inline void* dvz_container_alloc(DvzContainer* container)
{
    ASSERT(container != NULL);
    ASSERT(container->capacity > 0);
    ASSERT(container->items != NULL);
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
        void** _new =
            (void**)realloc(container->items, 2 * container->capacity * container->item_size);
        ASSERT(_new != NULL);
        container->items = _new;

        ASSERT(container->items != NULL);
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
    container->items[available_slot] = calloc(1, container->item_size);
    container->count++;
    ASSERT(container->items[available_slot] != NULL);

    // Initialize the DvzObject field.
    DvzObject* obj = (DvzObject*)container->items[available_slot];
    obj->status = DVZ_OBJECT_STATUS_ALLOC;
    obj->type = container->type;

    return container->items[available_slot];
}



/**
 * Return the object at a given index.
 *
 * @param container the container
 * @param idx the index of the object within the container
 * @param returns a pointer to the object at the specified index
 */
static inline void* dvz_container_get(DvzContainer* container, uint32_t idx)
{
    ASSERT(container != NULL);
    ASSERT(container->items != NULL);
    ASSERT(idx < container->capacity);
    return container->items[idx];
}



/**
 * Continue an already-started loop iteration on a container.
 *
 * @param container the container
 * @returns a pointer to the next object in the container, or NULL at the end
 */
static inline void dvz_container_iter(DvzContainerIterator* iterator)
{
    ASSERT(iterator != NULL);
    DvzContainer* container = iterator->container;
    ASSERT(container != NULL);

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



/**
 * Start a loop iteration over all valid objects within the container.
 *
 * @param container the container
 * @returns a pointer to the first object
 */
static inline DvzContainerIterator dvz_container_iterator(DvzContainer* container)
{
    ASSERT(container != NULL);
    DvzContainerIterator iterator = {0};
    iterator.container = container;
    dvz_container_iter(&iterator);
    return iterator;
}



/**
 * Return the n-th created object.
 *
 * @param container the container
 * @param idx the index of the object within the container
 * @param returns a pointer to the created object at the specified index
 */
static inline void* dvz_container_get_created(DvzContainer* container, uint32_t idx)
{
    ASSERT(container != NULL);
    ASSERT(container->items != NULL);
    DvzContainerIterator iter = dvz_container_iterator(container);
    DvzObject* obj = NULL;
    uint32_t n = 0;
    while (iter.item != NULL)
    {
        // WARNING: we assume that every item in the container is a struct for which the first item
        // is a DvzObject.
        obj = (DvzObject*)iter.item;
        ASSERT(obj != NULL);
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
static inline void dvz_container_destroy(DvzContainer* container)
{
    ASSERT(container != NULL);
    if (container->items == NULL)
        return;
    ASSERT(container->items != NULL);
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
                FREE(container->items[i]);
                container->items[i] = NULL;
                container->count--;
                ASSERT(container->count < UINT32_MAX);
            }
            ASSERT(container->items[i] == NULL);
        }
    }
    ASSERT(container->count == 0);
    // log_trace("free container items");
    FREE(container->items);
    log_trace("container destroy (%d elements)", count);
    container->capacity = 0;
}



#define CONTAINER_DESTROY_ITEMS(t, c, f)                                                          \
    {                                                                                             \
        DvzContainerIterator _iter = dvz_container_iterator(&c);                                  \
        t* o = NULL;                                                                              \
        while (_iter.item != NULL)                                                                \
        {                                                                                         \
            o = _iter.item;                                                                       \
            f(o);                                                                                 \
            dvz_container_iter(&_iter);                                                           \
        }                                                                                         \
    }



EXTERN_C_OFF

#endif
