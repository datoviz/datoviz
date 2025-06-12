/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/* Colorbar                                                                                      */
/*************************************************************************************************/

#ifndef DVZ_HEADER_COLORBAR
#define DVZ_HEADER_COLORBAR



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz_math.h"
#include "datoviz_types.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzColorbar DvzColorbar;

// Forward declarations.
typedef struct DvzVisual DvzVisual;
typedef struct DvzAxis DvzAxis;
typedef struct DvzTexture DvzTexture;
typedef struct DvzBatch DvzBatch;
typedef struct DvzRef DvzRef;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzColorbar
{
    DvzBatch* batch;
    int flags;
    DvzAtlasFont af;
    DvzRef* ref;
    DvzColor* imgdata;
    DvzVisual* image;
    DvzTexture* texture;
    DvzAxis* axis;

    DvzColormap cmap;
    vec2 position;
    vec2 anchor;
    uvec2 size;
};



#endif
