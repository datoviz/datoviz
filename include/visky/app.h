#ifndef VKL_APP_HEADER
#define VKL_APP_HEADER

#include <assert.h>
#include <math.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// #define GLFW_INCLUDE_VULKAN
#include <vulkan/vulkan.h>

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif



/*************************************************************************************************/
/*  Type definitions */
/*************************************************************************************************/

typedef struct VklApp VklApp;
typedef struct VklClock VklClock;
typedef struct VklThread VklThread;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Backend.
typedef enum
{
    VKL_BACKEND_NONE,
    VKL_BACKEND_GLFW,
    VKL_BACKEND_OFFSCREEN,
} VklBackend;



/*************************************************************************************************/
/*  Clock                                                                                        */
/*************************************************************************************************/

struct VklClock
{
    double elapsed;  // time in seconds elapsed since calling _start_time(clock)
    double interval; // interval since the last clock update

    struct timeval start, current;
    double checkpoint_time;
    uint64_t checkpoint_value;
};



static inline void _clock_init(VklClock* clock) { gettimeofday(&clock->start, NULL); }



static inline double _clock_get(VklClock* clock)
{
    gettimeofday(&clock->current, NULL);
    double elapsed = (clock->current.tv_sec - clock->start.tv_sec) +
                     (clock->current.tv_usec - clock->start.tv_usec) / 1000000.0;
    return elapsed;
}



static inline void _clock_set(VklClock* clock)
{
    // Typically called at every frame.
    double elapsed = _clock_get(clock);
    clock->interval = elapsed - clock->elapsed;
    clock->elapsed = elapsed;
}



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct VklThread
{
    VklObject obj;
    pthread_t thread;
    pthread_mutex_t lock;
};



struct VklApp
{
    VklObject obj;
    uint32_t n_errors;

    // Backend
    VklBackend backend;

    // Global clock
    VklClock clock;
    bool is_running;

    // Vulkan objects.
    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_messenger;

    // Containers.
    VklContainer gpus;
    VklContainer windows;
    VklContainer canvases;

    // Threads.
    VklThread timer_thread;
};



#ifdef __cplusplus
}
#endif

#endif
