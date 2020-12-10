#ifndef VKY_COMMON_HEADER
#define VKY_COMMON_HEADER

#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>
#include <math.h>
#include <stdint.h>



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
VKY_EXPORT const unsigned char* vkl_binary_shader_load(const char* name, unsigned long* size);



/*************************************************************************************************/
/*  Random                                                                                       */
/*************************************************************************************************/

VKY_EXPORT uint8_t rand_byte(void);

VKY_EXPORT float rand_float(void);

VKY_EXPORT float randn(void);

#define RAND_POS_2D                                                                               \
    {                                                                                             \
        .25f * randn(), .25f * randn(), 0.0f                                                      \
    }

#define RAND_POS_3D                                                                               \
    {                                                                                             \
        .25f * randn(), .25f * randn(), .25f * randn()                                            \
    }

#define RAND_MARKER_SIZE (5.0f + 20.0f * rand_float())



/*************************************************************************************************/
/*  Debug                                                                                        */
/*************************************************************************************************/



#ifdef __cplusplus
}
#endif

#endif
