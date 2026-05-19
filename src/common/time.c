/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Time                                                                                         */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "_assertions.h"
#include "datoviz/common/functions.h"
#include "datoviz/common/macros.h"

#if OS_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif OS_MACOS
#include <mach/mach_time.h>
#else
#include <time.h>
#endif



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return the current monotonic timestamp in nanoseconds.
 *
 * @return monotonic timestamp in nanoseconds
 */
uint64_t dvz_time_monotonic_ns(void)
{
#if OS_WINDOWS
    LARGE_INTEGER freq;
    LARGE_INTEGER counter;
    if (!QueryPerformanceFrequency(&freq) || freq.QuadPart <= 0)
        return 0;
    if (!QueryPerformanceCounter(&counter))
        return 0;
    return (uint64_t)(
        (long double)counter.QuadPart * 1000000000.0L / (long double)freq.QuadPart);
#elif OS_MACOS
    mach_timebase_info_data_t info;
    if (mach_timebase_info(&info) != KERN_SUCCESS || info.denom == 0)
        return 0;
    uint64_t ticks = mach_absolute_time();
    return (uint64_t)((long double)ticks * (long double)info.numer / (long double)info.denom);
#else
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
        return 0;
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
#endif
}
