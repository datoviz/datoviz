/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/* Glyph                                                                                         */
/*************************************************************************************************/

#ifndef DVZ_HEADER_GLYPH
#define DVZ_HEADER_GLYPH



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "../graphics.h"
#include "../viewport.h"
#include "../visual.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzGlyphVertex DvzGlyphVertex;
typedef struct DvzGlyphParams DvzGlyphParams;

// Forward declarations.
typedef struct DvzBatch DvzBatch;
typedef struct DvzAtlas DvzAtlas;
typedef struct DvzVisual DvzVisual;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzGlyphVertex
{
    vec3 pos;         /* 0: position */
    vec3 axis;        /* 1: axis */
    vec2 size;        /* 2: size */
    vec2 anchor;      /* 3: anchor */
    vec2 shift;       /* 4: shift */
    vec2 uv;          /* 5: texture coordinates */
    float angle;      /* 6: angle */
    DvzColor color;   /* 7: color */
    float group_size; /* 8: group_size */
};



struct DvzGlyphParams
{
    vec2 size;    /* glyph size in pixels */
    vec4 bgcolor; /* background color for glyph antialiasing*/
};



#endif
