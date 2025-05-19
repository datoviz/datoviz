/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/* Segment                                                                                       */
/*************************************************************************************************/

#ifndef DVZ_HEADER_SEGMENT
#define DVZ_HEADER_SEGMENT



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "../graphics.h"
#include "../viewport.h"
#include "../visual.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzSegmentVertex DvzSegmentVertex;
typedef struct DvzSegmentParams DvzSegmentParams;

// Forward declarations.
typedef struct DvzBatch DvzBatch;
typedef struct DvzVisual DvzVisual;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzSegmentVertex
{
    vec3 P0;         /* start position */
    vec3 P1;         /* end position */
    vec4 shift;      /* shift of start (xy) and end (zw) positions, in pixels */
    DvzColor color;  /* color */
    float linewidth; /* line width, in pixels */
};



struct DvzSegmentParams
{
    DvzCapType cap0; /* type of the ends of the segment */
    DvzCapType cap1; /* type of the ends of the segment */
};



#endif
