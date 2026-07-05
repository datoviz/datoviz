/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/* Box                                                                                           */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "datoviz/common/macros.h"
#include "dim.h"
#include "types.h"



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



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Box merge flags.
typedef enum
{
    DVZ_BOX_MERGE_DEFAULT = 0, // take extrema of input boxes
    DVZ_BOX_MERGE_CENTER = 1,  // merged is centered around 0 and encompasses all input boxes
    DVZ_BOX_MERGE_CORNER = 2,  // merged has (0,0,0) in its lower left corner
} DvzBoxMergeStrategy;



// Box flags.
typedef enum
{
    DVZ_BOX_EXTENT_DEFAULT = 0,               // no fixed aspect ratio
    DVZ_BOX_EXTENT_FIXED_ASPECT_EXPAND = 1,   // expand the box to match the aspect ratio
    DVZ_BOX_EXTENT_FIXED_ASPECT_CONTRACT = 2, // contract the box to match the aspect ratio
} DvzBoxExtentStrategy;



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
DVZ_EXPORT DvzBox
dvz_box(double xmin, double xmax, double ymin, double ymax, double zmin, double zmax);



/**
 * Return the aspect ratio of a box.
 *
 * @param box the box
 * @returns the aspect ratio width/height
 */
DVZ_EXPORT double dvz_box_aspect(DvzBox box);



/**
 * Return the box center.
 *
 * @param box the box
 * @param[out] the box's center
 */
DVZ_EXPORT void dvz_box_center(DvzBox box, dvec3 center);



/**
 * Return the extent of a box, in the same coordinate system, depending on the target viewport
 * aspect ratio. `DVZ_BOX_EXTENT_DEFAULT` returns the input box unchanged. `EXPAND` preserves the
 * center and grows the smaller axis so the returned box contains the input. `CONTRACT` preserves
 * the center and shrinks the larger axis so the returned box is contained by the input.
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
DVZ_EXPORT DvzBox
dvz_box_merge(uint32_t box_count, const DvzBox* boxes, DvzBoxMergeStrategy strategy);



/**
 * Normalize 1D input positions into a target box.
 *
 * @param source the source box, in data coordinates
 * @param target the target box, typically in normalized coordinates
 * @param dim which dimension
 * @param count the number of positions to normalize
 * @param pos the positions to normalize (double precision)
 * @param[out] out normalized positions to compute, as single-precision 3D positions
 */
DVZ_EXPORT
void dvz_box_normalize_1D(
    DvzBox source, DvzBox target, DvzDim dim, uint32_t count, const double* pos, vec3* out);



/**
 * Normalize 2D input positions into a target box.
 *
 * @param source the source box, in data coordinates
 * @param target the target box, typically in normalized coordinates
 * @param count the number of positions to normalize
 * @param pos the positions to normalize (double precision)
 * @param[out] out normalized positions to compute, as single-precision 3D positions
 */
DVZ_EXPORT void
dvz_box_normalize_2D(DvzBox source, DvzBox target, uint32_t count, dvec2* pos, vec3* out);



/**
 * Normalize 2D input positions into a target box, using dvec2* as output format.
 *
 * @param source the source box, in data coordinates
 * @param target the target box, typically in normalized coordinates
 * @param count the number of positions to normalize
 * @param pos the positions to normalize (double precision)
 * @param[out] out normalized positions to compute, as double-precision 2D positions
 */
DVZ_EXPORT void dvz_box_normalize_polygon(
    DvzBox source, DvzBox target, uint32_t count, dvec2* pos, dvec2* out);



/**
 * Normalize 3D input positions into a target box.
 *
 * @param source the source box, in data coordinates
 * @param target the target box, typically in normalized coordinates
 * @param count the number of positions to normalize
 * @param pos the positions to normalize (double precision)
 * @param[out] out normalized positions to compute, as single-precision 3D positions
 */
DVZ_EXPORT void
dvz_box_normalize_3D(DvzBox source, DvzBox target, uint32_t count, dvec3* pos, vec3* out);



/**
 * Perform an inverse transformation of a position from a target box to a source box.
 *
 * @param source the source box, in data coordinates
 * @param target the target box, typically in normalized coordinates
 * @param pos the position in the target box
 * @param[out] out position transformed back into the source box
 */
DVZ_EXPORT void dvz_box_inverse(DvzBox source, DvzBox target, const vec3 pos, dvec3* out);



EXTERN_C_OFF
