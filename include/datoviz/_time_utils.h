/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Time utilities                                                                               */
/*************************************************************************************************/

// NOTE: this file should NOT be called _time.h, or this will prevent compilation on macOS
// probably due to a naming conflict of a system file.

#ifndef DVZ_HEADER_TIME
#define DVZ_HEADER_TIME



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <time.h>

#include "_macros.h"



/*************************************************************************************************/
/*  Time includes                                                                                */
/*************************************************************************************************/

// Time utils.
#if CC_MSVC
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdint.h> // portable: uint64_t   MSVC: __int64

// MSVC defines this in winsock2.h!?
typedef struct timeval
{
    long tv_sec;
    long tv_usec;
} timeval;

static int gettimeofday(struct timeval* tp, struct timezone* tzp)
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
#if OS_WINDOWS
#include <Windows.h>
#endif



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzClock DvzClock;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzClock
{
    struct timeval start, current;
    double tick;
};



/*************************************************************************************************/
/*  Sleep                                                                                        */
/*************************************************************************************************/

/**
 * Wait a given number of milliseconds.
 *
 * @param milliseconds sleep duration
 */
static inline void dvz_sleep(int milliseconds)
{
#if OS_WINDOWS
    Sleep((uint32_t)milliseconds);
#else
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
#endif
}



/*************************************************************************************************/
/*  Clock                                                                                        */
/*************************************************************************************************/

static inline void dvz_clock_reset(DvzClock* clock)
{
    ANN(clock);
    gettimeofday(&clock->start, NULL);
}



/**
 * Instantiate a new clock.
 */
static inline DvzClock dvz_clock(void)
{
    INIT(DvzClock, clock)
    dvz_clock_reset(&clock);
    return clock;
}



/**
 * Return the time elapsed since the creation of the clock.
 *
 * @param clock the clock
 */
static inline double dvz_clock_get(DvzClock* clock)
{
    ANN(clock);
    gettimeofday(&clock->current, NULL);
    double elapsed = (clock->current.tv_sec - clock->start.tv_sec) +
                     (clock->current.tv_usec - clock->start.tv_usec) / 1000000.0;
    return elapsed;
}



/**
 * Store the current clock time.
 *
 * @param clock the clock
 */
static inline void dvz_clock_tick(DvzClock* clock)
{
    ANN(clock);
    clock->tick = dvz_clock_get(clock);
}



/**
 * Return the time elapsed since the last tick.
 *
 * @param clock the clock
 * @returns the time elapsed since the last tick.
 */
static inline double dvz_clock_interval(DvzClock* clock)
{
    ANN(clock);
    return dvz_clock_get(clock) - clock->tick;
}



#endif
