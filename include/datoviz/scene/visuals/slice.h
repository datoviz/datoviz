/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/* Slice                                                                                         */
/*************************************************************************************************/

#ifndef DVZ_HEADER_SLICE
#define DVZ_HEADER_SLICE



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "../viewport.h"
#include "../visual.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzSliceVertex DvzSliceVertex;
typedef struct DvzSliceParams DvzSliceParams;

// Forward declarations.
typedef struct DvzBatch DvzBatch;
typedef struct DvzVisual DvzVisual;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzSliceVertex
{
    vec3 pos; /* position */
    vec3 uvw; /* texture coordinates */
};

struct DvzSliceParams
{
    float alpha;
};


#endif
