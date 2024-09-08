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
    cvec4 color;     /* color */
    float linewidth; /* line width, in pixels */
    DvzCapType cap0; /* start cap enum */
    DvzCapType cap1; /* end cap enum */
};



#endif
