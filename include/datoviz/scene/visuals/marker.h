/*
* Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
* Licensed under the MIT license. See LICENSE file in the project root for details.
* SPDX-License-Identifier: MIT
*/

/*************************************************************************************************/
/* Marker                                                                                        */
/*************************************************************************************************/

#ifndef DVZ_HEADER_MARKER
#define DVZ_HEADER_MARKER



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "../viewport.h"
#include "../visual.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzMarkerVertex DvzMarkerVertex;
typedef struct DvzMarkerParams DvzMarkerParams;

// Forward declarations.
typedef struct DvzBatch DvzBatch;
typedef struct DvzVisual DvzVisual;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzMarkerVertex
{
    vec3 pos;    /* position */
    float size;  /* size */
    float angle; /* angle */
    cvec4 color; /* color */
};



struct DvzMarkerParams
{
    vec4 edge_color;
    float edge_width;
    float tex_scale;
};



#endif
