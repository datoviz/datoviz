/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

#ifndef DVZ_HEADER_GRID
#define DVZ_HEADER_GRID



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz_math.h"
#include "params.h"
#include "visual.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

/**
 * Grid visual parameters.
 */
typedef struct DvzGridParams
{
    vec4 color;      /* Line color. */
    float linewidth; /* Line width. */
    float scale;     /* Grid scaling. */
    float elevation; /* Grid elevation on the y axis. */
} DvzGridParams;



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Create an infinite horizontal grid visual.
 *
 * This visual renders a discreet, modern grid fixed in world space, giving the illusion of
 * an infinite ground plane. The grid is procedurally generated in a fragment shader and
 * uses ray-plane intersection to compute fragment positions.
 *
 * @param batch the batch
 * @param flags visual creation flags
 * @returns the grid object
 */
DVZ_EXPORT DvzVisual* dvz_grid(DvzBatch* batch, int flags);



/**
 * Destroy a grid visual.
 *
 * @param grid the visual to destroy
 */
DVZ_EXPORT void dvz_grid_destroy(DvzVisual* grid);



#endif // DVZ_HEADER_GRID
