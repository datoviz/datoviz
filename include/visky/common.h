#ifndef VKY_COMMON_HEADER
#define VKY_COMMON_HEADER

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


BEGIN_INCL_NO_WARN
#define CGLM_DEFINE_PRINTS
#include <cglm/cglm.h>
END_INCL_NO_WARN

// #define STB_IMAGE_IMPLEMENTATION

#include "log.h"



/*************************************************************************************************/
/*  Built-in fixed constants                                                                     */
/*************************************************************************************************/

#define ENGINE_NAME         "Visky"
#define APPLICATION_NAME    "Visky prototype"
#define APPLICATION_VERSION VK_MAKE_VERSION(1, 0, 0)

#define VKY_MAX_FRAMES_IN_FLIGHT 2



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

#define REALLOC(x, s)                                                                             \
    {                                                                                             \
        void* _new = realloc(x, s);                                                               \
        if (_new == NULL)                                                                         \
            log_error("error reallocating %s to %d bytes", #x, s);                                \
        else                                                                                      \
            x = _new;                                                                             \
    }



/*************************************************************************************************/
/*  Object macros                                                                                */
/*************************************************************************************************/

#define INSTANCES_INIT(s, o, p, c, n, t)                                                          \
    log_trace("init %d object(s) %s", n, #s);                                                     \
    o->p = calloc(n, sizeof(s));                                                                  \
    for (uint32_t i = 0; i < n; i++)                                                              \
    {                                                                                             \
        o->p[i].obj.type = t;                                                                     \
    }                                                                                             \
    o->c = n;


#define INSTANCE_NEW(s, o, instances, n)                                                          \
    s* o = NULL;                                                                                  \
    for (uint32_t i = 0; i < n; i++)                                                              \
        if (instances[i].obj.status < VKL_OBJECT_STATUS_INIT)                                     \
        {                                                                                         \
            o = &instances[i];                                                                    \
            o->obj.status = VKL_OBJECT_STATUS_INIT;                                               \
            o->obj.id = i;                                                                        \
            log_trace("new instance %s idx #%d", #s, i);                                          \
            break;                                                                                \
        }                                                                                         \
    if (o == NULL)                                                                                \
    {                                                                                             \
        log_error("maximum number of %s instances reached", #s);                                  \
        exit(1);                                                                                  \
    }


#define INSTANCES_DESTROY(o)                                                                      \
    log_trace("destroy objects %s", #o);                                                          \
    FREE(o);                                                                                      \
    o = NULL;



/*************************************************************************************************/
/*  8-bit integers                                                                               */
/*************************************************************************************************/

typedef uint8_t cvec2[2];
typedef uint8_t cvec3[3];
typedef uint8_t cvec4[4]; // used for color index



/*************************************************************************************************/
/*  16-bit integers                                                                              */
/*************************************************************************************************/

/* Signed */
typedef int16_t svec2[2];
typedef int16_t svec4[4]; // used for glyph as vec4 of uint16

/* Unsigned */
typedef uint16_t usvec2[2];
typedef uint16_t usvec4[4]; // used for glyph as vec4 of uint16



/*************************************************************************************************/
/*  32-bit integers                                                                              */
/*************************************************************************************************/

/* Signed */
typedef int32_t ivec2[2];
typedef int32_t ivec4[4];

/* Unsigned */
typedef uint32_t uvec2[2];
typedef uint32_t uvec3[3];
typedef uint32_t uvec4[4];

/* Index */
typedef uint32_t VklIndex;



/*************************************************************************************************/
/*  Single-precision floating-point numbers                                                      */
/*************************************************************************************************/

typedef vec2 fvec2;
typedef vec4 fvec4;



/*************************************************************************************************/
/*  Double-precision floating-point numbers                                                      */
/*************************************************************************************************/

/* Array types */
typedef double dvec2[2];
typedef double dvec4[4];



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
    VKL_OBJECT_STATUS_NONE,          // after allocation
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



/*************************************************************************************************/
/*  Object structure                                                                             */
/*************************************************************************************************/

struct VklObject
{
    VklObjectType type;
    VklObjectStatus status;

    uint32_t group_id; // group identifier
    uint32_t id;       // unique identifier among the objects of the same type and group
};



/*************************************************************************************************/
/*  Object functions                                                                             */
/*************************************************************************************************/

static inline void obj_init(VklObject* obj) { obj->status = VKL_OBJECT_STATUS_INIT; }

static inline void obj_created(VklObject* obj) { obj->status = VKL_OBJECT_STATUS_CREATED; }

static inline void obj_destroyed(VklObject* obj) { obj->status = VKL_OBJECT_STATUS_DESTROYED; }

static inline bool is_obj_created(VklObject* obj)
{
    return obj != NULL && obj->status >= VKL_OBJECT_STATUS_CREATED && obj->status != VKL_OBJECT_STATUS_INVALID;
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
    Sleep(milliseconds);
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
