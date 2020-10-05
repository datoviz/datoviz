#ifndef VKY_UTILS_HEADER
#define VKY_UTILS_HEADER

#include <assert.h>
#include <math.h>
#include <stdint.h>

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
#include <cglm/cglm.h>
END_INCL_NO_WARN

#include "constants.h"
#include "log.h"
#include "types.h"



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



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct VkyClock VkyClock;
struct VkyClock
{
    struct timeval start, current;
    double checkpoint_time;
    uint64_t checkpoint_value;
};



/*************************************************************************************************/
/*  I/O                                                                                          */
/*************************************************************************************************/

VKY_EXPORT int
write_png(const char* filename, uint32_t width, uint32_t height, const uint8_t* image);

VKY_EXPORT int
write_ppm(const char* filename, uint32_t width, uint32_t height, const uint8_t* image);

VKY_EXPORT char* read_file(const char* filename, uint32_t* size);

VKY_EXPORT char* read_npy(const char* filename, uint32_t* size);

VKY_EXPORT uint8_t* read_ppm(const char* filename, int*, int*);



/*************************************************************************************************/
/*  Misc utils                                                                                   */
/*************************************************************************************************/

VKY_EXPORT void vky_start_timer(void);

VKY_EXPORT double vky_get_timer(void);

VKY_EXPORT uint64_t vky_get_fps(uint64_t frame_count);



/*************************************************************************************************/
/*  Data normalization and coordinate transforms                                                 */
/*************************************************************************************************/

VKY_EXPORT void vky_earth_to_pixels(uint32_t point_count, dvec2* points);

VKY_EXPORT dvec2s vky_min_max(size_t size, double* points);



/*************************************************************************************************/
/*  Random                                                                                       */
/*************************************************************************************************/

VKY_EXPORT uint8_t rand_byte(void);

VKY_EXPORT dvec4s rand_color(void);

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

void printvec2(vec2 p);

void printvec3(vec3 p);

void printvec4(vec4 p);

void printmat4(mat4 m);



#endif
