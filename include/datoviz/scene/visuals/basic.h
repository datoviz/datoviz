/*
* Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
* Licensed under the MIT license. See LICENSE file in the project root for details.
* SPDX-License-Identifier: MIT
*/

/*************************************************************************************************/
/* Basic                                                                                         */
/*************************************************************************************************/

#ifndef DVZ_HEADER_BASIC
#define DVZ_HEADER_BASIC



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "../viewport.h"
#include "../visual.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

// typedef struct DvzBasic DvzBasic;
typedef struct DvzBasicVertex DvzBasicVertex;
typedef struct DvzBasicParams DvzBasicParams;

// Forward declarations.
typedef struct DvzBatch DvzBatch;
typedef struct DvzVisual DvzVisual;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzBasicVertex
{
    vec3 pos;    /* position */
    cvec4 color; /* color */
    float group; /* group */
};



struct DvzBasicParams
{
    float size; /* point size */
};



#endif
