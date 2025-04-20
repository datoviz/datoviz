/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/* Axes                                                                                          */
/*************************************************************************************************/

#ifndef DVZ_HEADER_AXES
#define DVZ_HEADER_AXES



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

typedef struct DvzAxes DvzAxes;

// Forward declarations.
typedef struct DvzVisual DvzVisual;
typedef struct DvzPanzoom DvzPanzoom;
typedef struct DvzPanel DvzPanel;
typedef struct DvzRef DvzRef;
typedef struct DvzTicks DvzTicks;
typedef struct DvzAxis DvzAxis;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzAxes
{
    DvzAtlasFont af;
    DvzPanel* panel;
    DvzVisual* grid;
    DvzAxis* axis[DVZ_DIM_COUNT];
};



/*************************************************************************************************/
/*  Axes                                                                                         */
/*************************************************************************************************/

/**
 * Create an axis.
 *
 * @param placeholder placeholder
 */
DVZ_EXPORT DvzAxes*
dvz_axes_2D(DvzPanel* panel, double xmin, double xmax, double ymin, double ymax, int flags);



/**
 * Create an axis.
 *
 * @param placeholder placeholder
 */
DVZ_EXPORT DvzAxis* dvz_axes_axis(DvzAxes* axes, DvzDim dim);



/**
 * Create an axis.
 *
 * @param placeholder placeholder
 */
DVZ_EXPORT void dvz_axes_update(DvzAxes* axes);



/**
 * Create an axis.
 *
 * @param placeholder placeholder
 */
DVZ_EXPORT void dvz_axes_destroy(DvzAxes* axes);



#endif
