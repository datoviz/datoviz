/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/* Axes                                                                                          */
/*************************************************************************************************/

#ifndef DVZ_HEADER_AXIS
#define DVZ_HEADER_AXIS



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_log.h"
#include "datoviz_enums.h"
#include "datoviz_math.h"
#include "datoviz_types.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzAxis DvzAxis;
typedef struct DvzAxes DvzAxes;

// Forward declarations.
typedef struct DvzVisual DvzVisual;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzAxis
{
    DvzVisual* glyph;
    DvzVisual* segment;
    DvzDim dim;
    int flags;
};



struct DvzAxes
{
    DvzAxis axis_xyz[3];
};



#endif
