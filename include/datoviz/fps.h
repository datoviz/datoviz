/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  FPS                                                                                          */
/*************************************************************************************************/

#ifndef DVZ_HEADER_FPS
#define DVZ_HEADER_FPS



/*************************************************************************************************/
/*  Includes                                                                                    */
/*************************************************************************************************/

#include "_time.h"
#include "datoviz_math.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_FPS_MAX_COUNT 1000
#define DVZ_FPS_BINS      50
#define DVZ_FPS_HEIGHT    50.0f
#define DVZ_FPS_CUTOFF    2.0



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzFps DvzFps;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzFps
{
    DvzClock clock;
    uint64_t counter; // total number of frames
    uint32_t count;   // number of items in 'values'
    dvec2 min_max;    // min and max of values
    // double max;
    double* values; // time to render the last 'count' frames
    float* hist;    // histogram of 'values', with DVZ_FPS_BINS bins
};



EXTERN_C_ON

/*************************************************************************************************/
/*  FPS functions                                                                                */
/*************************************************************************************************/

DvzFps dvz_fps(void);



void dvz_fps_tick(DvzFps* fps);



void dvz_fps_destroy(DvzFps* fps);



/**
 * Display a FPS histogram.
 */
void dvz_fps_histogram(DvzFps* fps);



EXTERN_C_OFF

#endif
