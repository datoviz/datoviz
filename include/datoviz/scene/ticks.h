/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/* Ticks                                                                                         */
/*************************************************************************************************/

#ifndef DVZ_HEADER_TICKS
#define DVZ_HEADER_TICKS



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_log.h"
#include "datoviz_math.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define PRECISION_MAX        9
#define DIST_MIN             50
#define MAX_GLYPHS_PER_LABEL 24
#define MAX_LABELS           256



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzTicksSpec DvzTicksSpec;
typedef struct DvzTicks DvzTicks;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DVZ_TICKS_FLAGS_HORIZONTAL = 0x0000,
    DVZ_TICKS_FLAGS_VERTICAL = 0x0001,
    DVZ_TICKS_FLAGS_LOG = 0x0010,
} DvzTicksFlags;



// Tick format type.
typedef enum
{
    DVZ_TICKS_FORMAT_UNDEFINED,
    DVZ_TICKS_FORMAT_DECIMAL,
    DVZ_TICKS_FORMAT_DECIMAL_FACTORED,
    DVZ_TICKS_FORMAT_SCIENTIFIC,
    DVZ_TICKS_FORMAT_SCIENTIFIC_FACTORED,
    DVZ_TICKS_FORMAT_COUNT,

    // NOTE: disable these formats for now, not that useful in practice
    DVZ_TICKS_FORMAT_THOUSANDS,
    DVZ_TICKS_FORMAT_THOUSANDS_FACTORED,
    DVZ_TICKS_FORMAT_MILLIONS,
    DVZ_TICKS_FORMAT_MILLIONS_FACTORED,
} DvzTicksFormat;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzTicksSpec
{
    DvzTicksFormat format;
    uint32_t precision;
    int32_t exponent;
    double offset;
};



struct DvzTicks
{
    int flags;
    double range_size; // size in pixels between the two ends of the axis
    double glyph_size; // average size of a glyph
    double dmin, dmax; // requested min and max of the range

    double lmin, lmax, lstep; // computed min and max of the ticks
    DvzTicksSpec spec;
};



/*************************************************************************************************/
/*  Util functions                                                                               */
/*************************************************************************************************/

DVZ_INLINE uint32_t get_tick_count(double lmin, double lmax, double lstep)
{
    ASSERT(lstep != 0);
    // TODO: replace by "round()"?
    return floor(1 + (lmax - lmin) / lstep);
}



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzTicks* dvz_ticks(int flags);



void dvz_ticks_size(DvzTicks* ticks, double range_size, double glyph_size);



bool dvz_ticks_compute(DvzTicks* ticks, double dmin, double dmax, uint32_t requested_count);



void dvz_ticks_range(DvzTicks* ticks, double* lmin, double* lmax, double* lstep);



DvzTicksSpec dvz_ticks_spec(DvzTicks* ticks);



bool dvz_ticks_dirty(DvzTicks* ticks, double dmin, double dmax);



void dvz_ticks_print(DvzTicks* ticks);



void dvz_ticks_clear(DvzTicks* ticks);



void dvz_ticks_destroy(DvzTicks* ticks);



/*************************************************************************************************/
/*  Tick label generation                                                                        */
/*************************************************************************************************/

void dvz_ticks_linspace(
    DvzTicksSpec* spec, uint32_t tick_count, double lmin, double lmax, double lstep, //
    char** out_labels, double* out_tick_pos);



EXTERN_C_OFF

#endif
