/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/* Monoglyph                                                                                     */
/*************************************************************************************************/

#ifndef DVZ_HEADER_MONOGLYPH
#define DVZ_HEADER_MONOGLYPH



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "../viewport.h"
#include "../visual.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

// typedef struct DvzMonoglyph DvzMonoglyph;
typedef struct DvzMonoglyphVertex DvzMonoglyphVertex;
typedef struct DvzMonoglyphParams DvzMonoglyphParams;

// Forward declarations.
typedef struct DvzBatch DvzBatch;
typedef struct DvzVisual DvzVisual;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzMonoglyphVertex
{
    vec3 pos;       /* position */
    vec3 bytes_012; /* bytes_012 */
    vec3 bytes_345; /* bytes_345 */
    ivec2 offset;   /* offset */
    DvzColor color; /* color */
};



struct DvzMonoglyphParams
{
    vec2 anchor; /* glyph anchor */
    float size;  /* glyph relative size */
};



#endif
