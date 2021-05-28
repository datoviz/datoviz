/*************************************************************************************************/
/*  Common utils                                                                                 */
/*************************************************************************************************/

#ifndef DVZ_COMMON_HEADER
#define DVZ_COMMON_HEADER

// Atomic macro, for both C++ and C
#ifndef __cplusplus
#include <stdatomic.h>
#define atomic(t, x) _Atomic t x
#else
#include <atomic>
#define atomic(t, x) std::atomic<t> x
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>


/*************************************************************************************************/
/*  Export macros                                                                                */
/*************************************************************************************************/

#include "macros.h"

// Time utils.
#if MSVC
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdint.h> // portable: uint64_t   MSVC: __int64

// MSVC defines this in winsock2.h!?
typedef struct timeval
{
    long tv_sec;
    long tv_usec;
} timeval;

int gettimeofday(struct timeval* tp, struct timezone* tzp)
{
    // Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing
    // zero's This magic number is the number of 100 nanosecond intervals since January 1, 1601
    // (UTC) until 00:00:00 January 1, 1970
    static const uint64_t EPOCH = ((uint64_t)116444736000000000ULL);

    SYSTEMTIME system_time;
    FILETIME file_time;
    uint64_t time;

    GetSystemTime(&system_time);
    SystemTimeToFileTime(&system_time, &file_time);
    time = ((uint64_t)file_time.dwLowDateTime);
    time += ((uint64_t)file_time.dwHighDateTime) << 32;

    tp->tv_sec = (long)((time - EPOCH) / 10000000L);
    tp->tv_usec = (long)(system_time.wMilliseconds * 1000);
    return 0;
}

#else
#include <sys/time.h>
#endif

// Used for Sleep()
#if OS_WIN32
#include <Windows.h>
#endif


BEGIN_INCL_NO_WARN
#define CGLM_DEFINE_PRINTS
#include <cglm/cglm.h>
END_INCL_NO_WARN

// #define STB_IMAGE_IMPLEMENTATION

#include "log.h"
#include "types.h"



/*************************************************************************************************/
/*  Built-in fixed constants                                                                     */
/*************************************************************************************************/

#define ENGINE_NAME         "Datoviz"
#define APPLICATION_NAME    "Datoviz canvas"
#define APPLICATION_VERSION VK_MAKE_VERSION(1, 0, 0)

#define DVZ_MAX_FRAMES_IN_FLIGHT    2
#define DVZ_CONTAINER_DEFAULT_COUNT 64


/*************************************************************************************************/
/*  Math                                                                                         */
/*************************************************************************************************/

#ifndef M_PI
#define M_PI 3.141592653589793
#endif
#define M_2PI 6.283185307179586

#define DVZ_NEVER -1000000



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/


#define MIN(a, b)        (((a) < (b)) ? (a) : (b))
#define MAX(a, b)        (((a) > (b)) ? (a) : (b))
#define CLIP(x, a, b)    MAX(MIN((x), (b)), (a))
#define POS(a)           ((a) >= 0 ? (a) : 0)
#define ARRAY_COUNT(arr) sizeof((arr)) / sizeof((arr)[0])

#define DBG(x)  printf("%" PRIu64 "\n", (x));
#define DBGS(x) printf("%" PRId64 "\n", (x));
#define DBGF(x) printf("%.8f\n", (double)(x))
#define PRT(x)  printf("%s\n", (x))

#define PLUS_INF  (+1e30)
#define MINUS_INF (-1e30)

#define ASSERT(x) assert((x))

#define FREE(x)                                                                                   \
    if ((x) != NULL)                                                                              \
    {                                                                                             \
        free((x));                                                                                \
        (x) = NULL;                                                                               \
    }

#define ALIGNED_FREE(x)                                                                           \
    if (x.aligned)                                                                                \
        aligned_free(x.pointer);                                                                  \
    else                                                                                          \
        FREE(x.pointer)

#define REALLOC(x, s)                                                                             \
    {                                                                                             \
        void* _new = realloc((x), (s));                                                           \
        if (_new == NULL)                                                                         \
            log_error("error reallocating %s to %d bytes", #x, (s));                              \
        else                                                                                      \
            x = _new;                                                                             \
    }



/*************************************************************************************************/
/*  Common enums                                                                                 */
/*************************************************************************************************/

// Axis coord.
typedef enum
{
    DVZ_AXES_COORD_X,
    DVZ_AXES_COORD_Y,
} DvzAxisCoord;



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
    DVZ_OBJECT_TYPE_TEXTURE,
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
    DVZ_OBJECT_TYPE_ARRAY,
    DVZ_OBJECT_TYPE_VISUAL,
    DVZ_OBJECT_TYPE_PROP,
    DVZ_OBJECT_TYPE_SOURCE,
    DVZ_OBJECT_TYPE_SCENE,
    DVZ_OBJECT_TYPE_GRID,
    DVZ_OBJECT_TYPE_PANEL,
    DVZ_OBJECT_TYPE_CONTROLLER,
    DVZ_OBJECT_TYPE_AXES_2D,
    DVZ_OBJECT_TYPE_AXES_3D,
    DVZ_OBJECT_TYPE_GUI,
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

typedef struct DvzMVP DvzMVP;
typedef struct DvzObject DvzObject;
typedef struct DvzContainer DvzContainer;
typedef struct DvzContainerIterator DvzContainerIterator;
typedef struct DvzThread DvzThread;

typedef void* (*DvzThreadCallback)(void*);



/*************************************************************************************************/
/*  Structures                                                                                   */
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



struct DvzThread
{
    DvzObject obj;
    pthread_t thread;
    pthread_mutex_t lock;
    atomic(int, lock_idx); // used to allow nested callbacks and avoid deadlocks: only 1 lock
};



struct DvzMVP
{
    mat4 model;
    mat4 view;
    mat4 proj;
    float time;
};



/*************************************************************************************************/
/*  Object functions                                                                             */
/*************************************************************************************************/

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

// Smallest power of 2 larger or equal than a positive integer.
static uint64_t dvz_next_pow2(uint64_t x)
{
    uint64_t p = 1;
    while (p < x)
        p *= 2;
    return p;
}

/**
 * Create a container that will contain an arbitrary number of objects of the same type.
 *
 * @param count initial number of objects in the container
 * @param item_size size of each object, in bytes
 * @param type object type
 */
static DvzContainer dvz_container(uint32_t count, size_t item_size, DvzObjectType type)
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
static void dvz_container_delete_if_destroyed(DvzContainer* container, uint32_t idx)
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
static void* dvz_container_alloc(DvzContainer* container)
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
static void* dvz_container_get(DvzContainer* container, uint32_t idx)
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
static void dvz_container_iter(DvzContainerIterator* iterator)
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
static DvzContainerIterator dvz_container_iterator(DvzContainer* container)
{
    ASSERT(container != NULL);
    DvzContainerIterator iterator = {0};
    iterator.container = container;
    dvz_container_iter(&iterator);
    return iterator;
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
static void dvz_container_destroy(DvzContainer* container)
{
    ASSERT(container != NULL);
    if (container->items == NULL)
        return;
    ASSERT(container->items != NULL);
    // log_trace("container destroy");
    // Check all elements have been destroyed, and free them if necessary.
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



/*************************************************************************************************/
/*  I/O                                                                                          */
/*************************************************************************************************/

/**
 * Save an image to a PNG file
 *
 * @param filename path to the PNG file to create
 * @param width width of the image
 * @param height height of the image
 * @param image pointer to an array of 32-bit RGBA values
 */
DVZ_EXPORT int
dvz_write_png(const char* filename, uint32_t width, uint32_t height, const uint8_t* image);

/**
 * Save an image to a PPM file (short ASCII header and flat binary RGBA values).
 *
 * @param filename path to the PPM file to create
 * @param width width of the image
 * @param height height of the image
 * @param image pointer to an array of 32-bit RGBA values
 */
DVZ_EXPORT int
dvz_write_ppm(const char* filename, uint32_t width, uint32_t height, const uint8_t* image);

/**
 * Read a binary file.
 *
 * @param filename path of the file to open
 * @param[out] size of the file
 * @returns pointer to a byte buffer with the file contents
 */
DVZ_EXPORT uint32_t* dvz_read_file(const char* filename, size_t* size);

/**
 * Read a NumPy NPY file.
 *
 * @param filename path of the file to open
 * @param[out] size of the file
 * @returns pointer to a buffer containing the array elements
 */
DVZ_EXPORT char* dvz_read_npy(const char* filename, size_t* size);

/**
 * Read a PPM image file.
 *
 * @param filename path of the file to open
 * @param[out] width width of the image
 * @param[out] height of the image
 * @returns pointer to a buffer with the loaded RGBA pixel colors
 */
DVZ_EXPORT uint8_t* dvz_read_ppm(const char* filename, int* width, int* height);

// Defined in cmake-generated file build/_shaders.c
DVZ_EXPORT unsigned char* dvz_resource_shader(const char* name, unsigned long* size);

// Defined in cmake-generated file build/_colortex.c
DVZ_EXPORT unsigned char* dvz_resource_texture(const char* name, unsigned long* size);

// Defined in cmake-generated file build/_fonts.c
DVZ_EXPORT unsigned char* dvz_resource_font(const char* name, unsigned long* size);



/*************************************************************************************************/
/*  Thread                                                                                       */
/*************************************************************************************************/

/**
 * Create a thread.
 *
 * Callback function signature: `void*(void*)`
 *
 * @param callback the function that will run in a background thread
 * @param user_data a pointer to arbitrary user data
 * @returns thread object
 */
DVZ_EXPORT DvzThread dvz_thread(DvzThreadCallback callback, void* user_data);

/**
 * Acquire a mutex lock associated to the thread.
 *
 * @param thread the thread
 */
DVZ_EXPORT void dvz_thread_lock(DvzThread* thread);

/**
 * Release a mutex lock associated to the thread.
 *
 * @param thread the thread
 */
DVZ_EXPORT void dvz_thread_unlock(DvzThread* thread);

/**
 * Destroy a thread after the thread function has finished running.
 *
 * @param thread the thread
 */
DVZ_EXPORT void dvz_thread_join(DvzThread* thread);



/*************************************************************************************************/
/*  Misc                                                                                         */
/*************************************************************************************************/

/**
 * Wait a given number of milliseconds.
 *
 * @param milliseconds sleep duration
 */
static inline void dvz_sleep(int milliseconds)
{
#ifdef WIN32
    Sleep((uint32_t)milliseconds);
#else
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
#endif
}

void dvz_triangulate_polygon(
    uint32_t point_count, const dvec3* polygon, uint32_t* index_count, uint32_t** out_indices);



/*************************************************************************************************/
/*  Random                                                                                       */
/*************************************************************************************************/

/**
 * Return a random integer number between 0 and 255.
 *
 * @returns random number
 */
DVZ_EXPORT uint8_t dvz_rand_byte(void);

/**
 * Return a random floating-point number between 0 and 1.
 *
 * @returns random number
 */
DVZ_EXPORT float dvz_rand_float(void);

/**
 * Return a random normal floating-point number.
 *
 * @returns random number
 */
DVZ_EXPORT float dvz_rand_normal(void);



#ifdef __cplusplus
}
#endif

#endif
