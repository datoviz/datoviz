#ifndef VKY_COMMON_HEADER
#define VKY_COMMON_HEADER

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

#if MSVC
#ifdef VKY_SHARED
#define VKY_EXPORT __declspec(dllexport)
#else
#define VKY_EXPORT __declspec(dllimport)
#endif
#define VKY_INLINE __forceinline
#else
#define VKY_EXPORT __attribute__((visibility("default")))
#define VKY_INLINE static inline __attribute((always_inline))
#endif


#ifndef __STDC_VERSION__
#define __STDC_VERSION__ 0
#endif

#if GCC

#define BEGIN_INCL_NO_WARN                                                                        \
    _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wsign-conversion\"")        \
        _Pragma("GCC diagnostic ignored \"-Wundef\"")                                             \
            _Pragma("GCC diagnostic ignored \"-Wcast-qual\"")                                     \
                _Pragma("GCC diagnostic ignored \"-Wredundant-decls\"")                           \
                    _Pragma("GCC diagnostic ignored \"-Wcast-qual\"")                             \
                        _Pragma("GCC diagnostic ignored \"-Wunused\"")                            \
                            _Pragma("GCC diagnostic ignored \"-Wunused-parameter\"")              \
                                _Pragma("GCC diagnostic ignored \"-Wstrict-overflow\"")           \
                                    _Pragma("GCC diagnostic ignored \"-Wswitch-default\"")        \
                                        _Pragma("GCC diagnostic ignored \"-Wmissing-braces\"")

#define END_INCL_NO_WARN _Pragma("GCC diagnostic pop")

#elif CLANG

#define BEGIN_INCL_NO_WARN                                                                        \
    _Pragma("clang diagnostic push") _Pragma("clang diagnostic ignored \"-Wsign-conversion\"")    \
        _Pragma("clang diagnostic ignored \"-Wcast-qual\"")                                       \
            _Pragma("clang diagnostic ignored \"-Wredundant-decls\"")                             \
                _Pragma("clang diagnostic ignored \"-Wcast-qual\"")                               \
                    _Pragma("clang diagnostic ignored \"-Wstrict-overflow\"")                     \
                        _Pragma("clang diagnostic ignored \"-Wswitch-default\"")                  \
                            _Pragma("clang diagnostic ignored \"-Wcast-align\"")                  \
                                _Pragma("clang diagnostic ignored \"-Wundef\"")                   \
                                    _Pragma("clang diagnostic ignored \"-Wmissing-braces\"")

#define END_INCL_NO_WARN _Pragma("clang diagnostic pop")

#else

#define BEGIN_INCL_NO_WARN
#define END_INCL_NO_WARN

#endif

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

#define ENGINE_NAME         "Visky"
#define APPLICATION_NAME    "Visky prototype"
#define APPLICATION_VERSION VK_MAKE_VERSION(1, 0, 0)

#define VKL_MAX_FRAMES_IN_FLIGHT    2
#define VKL_CONTAINER_DEFAULT_COUNT 64


/*************************************************************************************************/
/*  Math                                                                                         */
/*************************************************************************************************/

#ifndef M_PI
#define M_PI 3.141592653589793
#endif
#define M_2PI 6.283185307179586

#define VKY_NEVER -1000000



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define CLIP(x, a, b) fmax(fmin((x), (b)), (a))

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define POS(a)    ((a) >= 0 ? (a) : 0)

#define DBG(x)  printf("%d\n", (int)(x))
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
    VKL_AXES_COORD_X,
    VKL_AXES_COORD_Y,
} VklAxisCoord;



/*************************************************************************************************/
/*  Misc structures                                                                              */
/*************************************************************************************************/

typedef struct VklMVP VklMVP;
struct VklMVP
{
    mat4 model;
    mat4 view;
    mat4 proj;
    float time;
};



/*************************************************************************************************/
/*  Objects                                                                                      */
/*************************************************************************************************/

// Object types.
typedef enum
{
    VKL_OBJECT_TYPE_UNDEFINED,
    VKL_OBJECT_TYPE_APP,
    VKL_OBJECT_TYPE_GPU,
    VKL_OBJECT_TYPE_WINDOW,
    VKL_OBJECT_TYPE_SWAPCHAIN,
    VKL_OBJECT_TYPE_CANVAS,
    VKL_OBJECT_TYPE_COMMANDS,
    VKL_OBJECT_TYPE_BUFFER,
    VKL_OBJECT_TYPE_TEXTURE,
    VKL_OBJECT_TYPE_IMAGES,
    VKL_OBJECT_TYPE_SAMPLER,
    VKL_OBJECT_TYPE_BINDINGS,
    VKL_OBJECT_TYPE_COMPUTE,
    VKL_OBJECT_TYPE_GRAPHICS,
    VKL_OBJECT_TYPE_BARRIER,
    VKL_OBJECT_TYPE_FENCES,
    VKL_OBJECT_TYPE_SEMAPHORES,
    VKL_OBJECT_TYPE_RENDERPASS,
    VKL_OBJECT_TYPE_FRAMEBUFFER,
    VKL_OBJECT_TYPE_SUBMIT,
    VKL_OBJECT_TYPE_SCREENCAST,
    VKL_OBJECT_TYPE_ARRAY,
    VKL_OBJECT_TYPE_VISUAL,
    VKL_OBJECT_TYPE_PROP,
    VKL_OBJECT_TYPE_SOURCE,
    VKL_OBJECT_TYPE_SCENE,
    VKL_OBJECT_TYPE_GRID,
    VKL_OBJECT_TYPE_PANEL,
    VKL_OBJECT_TYPE_CONTROLLER,
    VKL_OBJECT_TYPE_AXES_2D,
    VKL_OBJECT_TYPE_AXES_3D,
    VKL_OBJECT_TYPE_CUSTOM,
} VklObjectType;


// Object status.
// NOTE: the order is important, status >= CREATED means the object has been created
typedef enum
{
    VKL_OBJECT_STATUS_NONE,          //
    VKL_OBJECT_STATUS_ALLOC,         // after allocation
    VKL_OBJECT_STATUS_DESTROYED,     // after destruction
    VKL_OBJECT_STATUS_INIT,          // after struct initialization but before Vulkan creation
    VKL_OBJECT_STATUS_CREATED,       // after proper creation on the GPU
    VKL_OBJECT_STATUS_NEED_RECREATE, // need to be recreated
    VKL_OBJECT_STATUS_NEED_UPDATE,   // need to be updated
    VKL_OBJECT_STATUS_NEED_DESTROY,  // need to be destroyed
    VKL_OBJECT_STATUS_INACTIVE,      // inactive
    VKL_OBJECT_STATUS_INVALID,       // invalid
} VklObjectStatus;

typedef struct VklObject VklObject;
typedef struct VklContainer VklContainer;


/*************************************************************************************************/
/*  Object structure                                                                             */
/*************************************************************************************************/

struct VklObject
{
    VklObjectType type;
    VklObjectStatus status;
    int request;

    uint32_t group_id; // group identifier
    uint32_t id;       // unique identifier among the objects of the same type and group
};



struct VklContainer
{
    uint32_t count;
    uint32_t capacity;
    VklObjectType type;
    void** items;
    size_t item_size;
    uint32_t _loop_idx;
};



/*************************************************************************************************/
/*  Object functions                                                                             */
/*************************************************************************************************/

static inline void obj_init(VklObject* obj) { obj->status = VKL_OBJECT_STATUS_INIT; }

static inline void obj_created(VklObject* obj) { obj->status = VKL_OBJECT_STATUS_CREATED; }

static inline void obj_destroyed(VklObject* obj) { obj->status = VKL_OBJECT_STATUS_DESTROYED; }

static inline bool is_obj_created(VklObject* obj)
{
    return obj != NULL && obj->status >= VKL_OBJECT_STATUS_CREATED &&
           obj->status != VKL_OBJECT_STATUS_INVALID;
}



/*************************************************************************************************/
/*  Container                                                                                    */
/*************************************************************************************************/

static uint64_t next_pow2(uint64_t x)
{
    uint64_t p = 1;
    while (p < x)
        p *= 2;
    return p;
}

static VklContainer vkl_container(uint32_t count, size_t item_size, VklObjectType type)
{
    ASSERT(count > 0);
    ASSERT(item_size > 0);
    // log_trace("create container");
    VklContainer container = {0};
    container.count = 0;
    container.item_size = item_size;
    container.type = type;
    container.capacity = next_pow2(count);
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

static void vkl_container_delete_if_destroyed(VklContainer* container, uint32_t idx)
{
    ASSERT(container != NULL);
    ASSERT(container->capacity > 0);
    ASSERT(container->items != NULL);
    ASSERT(idx < container->capacity);
    if (container->items[idx] == NULL)
        return;
    VklObject* object = (VklObject*)container->items[idx];
    if (object->status == VKL_OBJECT_STATUS_DESTROYED)
    {
        // log_trace("delete container item #%d", idx);
        FREE(container->items[idx]);
        container->items[idx] = NULL;
        container->count--;
        ASSERT(container->count < UINT32_MAX);
    }
}

static void* vkl_container_alloc(VklContainer* container)
{
    ASSERT(container != NULL);
    ASSERT(container->capacity > 0);
    ASSERT(container->items != NULL);
    uint32_t available_slot = UINT32_MAX;

    // Free the memory of destroyed objects and find the first available slot.
    for (uint32_t i = 0; i < container->capacity; i++)
    {
        vkl_container_delete_if_destroyed(container, i);
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

    // Initialize the VklObject field.
    VklObject* obj = (VklObject*)container->items[available_slot];
    obj->status = VKL_OBJECT_STATUS_ALLOC;
    obj->type = container->type;

    return container->items[available_slot];
}

static void* vkl_container_get(VklContainer* container, uint32_t idx)
{
    ASSERT(container != NULL);
    ASSERT(container->items != NULL);
    ASSERT(idx < container->capacity);
    return container->items[idx];
}

static void* vkl_container_iter(VklContainer* container)
{
    ASSERT(container != NULL);
    if (container->items == NULL || container->capacity == 0 || container->count == 0)
        return NULL;
    if (container->_loop_idx >= container->capacity)
        return NULL;
    ASSERT(container->_loop_idx <= container->capacity - 1);
    for (uint32_t i = container->_loop_idx; i < container->capacity; i++)
    {
        vkl_container_delete_if_destroyed(container, i);
        if (container->items[i] != NULL)
        {
            container->_loop_idx = i + 1;
            return container->items[i];
        }
    }
    // End the outer loop, reset the internal idx.
    container->_loop_idx = 0;
    return NULL;
}

static void* vkl_container_iter_init(VklContainer* container)
{
    ASSERT(container != NULL);
    container->_loop_idx = 0;
    return vkl_container_iter(container);
}

static void vkl_container_destroy(VklContainer* container)
{
    ASSERT(container != NULL);
    if (container->items == NULL)
        return;
    ASSERT(container->items != NULL);
    // log_trace("container destroy");
    // Check all elements have been destroyed, and free them if necessary.
    VklObject* item = NULL;
    for (uint32_t i = 0; i < container->capacity; i++)
    {
        if (container->items[i] != NULL)
        {
            // log_trace("deleting container item #%d", i);
            // When destroying the container, ensure that all objects have been destroyed first.
            // NOTE: only works if every item has a VklObject as first struct field.
            item = (VklObject*)container->items[i];
            vkl_container_delete_if_destroyed(container, i);
            // Also deallocate objects allocated/initialized, but not created/destroyed.
            if (container->items[i] != NULL)
            {
                ASSERT(item->status <= VKL_OBJECT_STATUS_INIT);
                ASSERT(item->status != VKL_OBJECT_STATUS_DESTROYED);
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
        t* o = vkl_container_iter_init(&c);                                                       \
        while (o != NULL)                                                                         \
        {                                                                                         \
            f(o);                                                                                 \
            o = vkl_container_iter(&c);                                                           \
        }                                                                                         \
    }



/*************************************************************************************************/
/*  I/O                                                                                          */
/*************************************************************************************************/

VKY_EXPORT int
write_png(const char* filename, uint32_t width, uint32_t height, const uint8_t* image);

VKY_EXPORT int
write_ppm(const char* filename, uint32_t width, uint32_t height, const uint8_t* image);

VKY_EXPORT char* read_file(const char* filename, size_t* size);

VKY_EXPORT char* read_npy(const char* filename, size_t* size);

VKY_EXPORT uint8_t* read_ppm(const char* filename, int*, int*);

// Defined in cmake-generated file build/_shaders.c
VKY_EXPORT const unsigned char* vkl_resource_shader(const char* name, unsigned long* size);

// Defined in cmake-generated file build/_colortex.c
VKY_EXPORT const unsigned char* vkl_resource_texture(const char* name, unsigned long* size);



/*************************************************************************************************/
/*  Misc                                                                                         */
/*************************************************************************************************/

static inline void vkl_sleep(int milliseconds)
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



/*************************************************************************************************/
/*  Random                                                                                       */
/*************************************************************************************************/

VKY_EXPORT uint8_t rand_byte(void);

VKY_EXPORT float rand_float(void);

VKY_EXPORT float randn(void);



#ifdef __cplusplus
}
#endif

#endif
