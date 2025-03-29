/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/* Box                                                                                           */
/*************************************************************************************************/

#ifndef DVZ_HEADER_BOX
#define DVZ_HEADER_BOX



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz_enums.h"
#include "datoviz_math.h"
#include "datoviz_types.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzBox DvzBox;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzBox
{
    double xmin, xmax, ymin, ymax, zmin, zmax;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Create a box.
 *
 * @param xmin minimum x value
 * @param xmax maximum x value
 * @param ymin minimum y value
 * @param ymax maximum y value
 * @param zmin minimum z value
 * @param zmax maximum z value
 * @returns the box
 */
DvzBox dvz_box(double xmin, double xmax, double ymin, double ymax, double zmin, double zmax);



/**
 * Return the aspect ratio of a box.
 *
 * @param box the box
 * @returns the aspect ratio width/height
 */
double dvz_box_aspect(DvzBox box);



/**
 * Return the box center.
 *
 * @param box the box
 * @param[out] the box's center
 */
void dvz_box_center(DvzBox box, dvec3 center);



/**
 * Return the extent of a box, in the same coordinate system, depending on the aspect ratio.
 * This will return the same box if the aspect ratio is unconstrained.
 *
 * @param box the original box
 * @param width the viewport width
 * @param height the viewport height
 * @param strategy indicates how the extent box should be computed
 * @returns the extent box
 */
DVZ_EXPORT
DvzBox dvz_box_extent(DvzBox box, float width, float height, DvzBoxExtentStrategy strategy);



/**
 * Merge a number of boxes into a single box.
 *
 * @param box_count the number of boxes to merge
 * @param boxes the boxes to merge
 * @param strategy the merge strategy
 * @returns the merged box
 */
DvzBox dvz_box_merge(uint32_t box_count, DvzBox* boxes, DvzBoxMergeStrategy strategy);



/**
 * Normalize 1D input positions into a target box.
 *
 * @param source the source box, in data coordinates
 * @param target the target box, typically in normalized coordinates
 * @param dim which dimension
 * @param count the number of positions to normalize
 * @param pos the positions to normalize (double precision)
 * @param[out] out pointer to an array with the normalized positions to compute (single precision)
 */
DVZ_EXPORT
void dvz_box_normalize_1D(
    DvzBox source, DvzBox target, DvzDim dim, uint32_t count, double* pos, vec3* out);



/**
 * Normalize 2D input positions into a target box.
 *
 * @param source the source box, in data coordinates
 * @param target the target box, typically in normalized coordinates
 * @param count the number of positions to normalize
 * @param pos the positions to normalize (double precision)
 * @param[out] out pointer to an array with the normalized positions to compute (single precision)
 */
void dvz_box_normalize2D(DvzBox source, DvzBox target, uint32_t count, dvec2* pos, vec3* out);



/**
 * Normalize 3D input positions into a target box.
 *
 * @param source the source box, in data coordinates
 * @param target the target box, typically in normalized coordinates
 * @param count the number of positions to normalize
 * @param pos the positions to normalize (double precision)
 * @param[out] out pointer to an array with the normalized positions to compute (single precision)
 */
void dvz_box_normalize_3D(DvzBox source, DvzBox target, uint32_t count, dvec3* pos, vec3* out);



/**
 * Perform an inverse transformation of a position from a target box to a source box.
 */
void dvz_box_inverse(DvzBox source, DvzBox target, vec3 pos, dvec3* out);



/**
 * Display information about a box.
 */
void dvz_box_print(DvzBox box);



EXTERN_C_OFF

#endif
